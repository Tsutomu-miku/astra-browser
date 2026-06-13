#include "astra/ui/views/sidebar/astra_workspace_switcher_view.h"

#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kSwitcherHeight = 44;
constexpr int kSwitcherHorizontalPadding = 12;
constexpr int kSwitcherSpacing = 8;
constexpr int kChevronSize = 16;

// Astra color ID for the workspace switcher text.
// Uses the Astra sidebar item text color from the Astra color system.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kSwitcherTextColorId = kColorAstraSidebarItemText;

}  // namespace

AstraWorkspaceSwitcherView::AstraWorkspaceSwitcherView(
    AstraWorkspaceService* workspace_service)
    : workspace_service_(workspace_service) {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, kSwitcherHorizontalPadding), kSwitcherSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Workspace name label.
  workspace_name_label_ = AddChildView(std::make_unique<views::Label>());
  workspace_name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  workspace_name_label_->SetAutoColorReadabilityEnabled(false);
  workspace_name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  layout->SetFlexForView(workspace_name_label_, 1);

  // Workspace count label (windows · tabs).
  workspace_count_label_ = AddChildView(std::make_unique<views::Label>());
  workspace_count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  workspace_count_label_->SetAutoColorReadabilityEnabled(false);
  workspace_count_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  // Count label uses secondary text styling.
  workspace_count_label_->SetFontList(
      workspace_count_label_->font_list().Derive(
          -1, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));

  // Chevron button — expands/collapses the workspace list.
  chevron_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraWorkspaceSwitcherView::OnSwitcherPressed,
                          base::Unretained(this))));
  chevron_button_->SetPreferredSize(gfx::Size(kChevronSize, kChevronSize));
  chevron_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  // Initialize display state.
  UpdateFromService();
  UpdateCountLabel();
}

AstraWorkspaceSwitcherView::~AstraWorkspaceSwitcherView() = default;

void AstraWorkspaceSwitcherView::UpdateFromService() {
  if (!workspace_service_) {
    workspace_name_label_->SetText(std::u16string());
    if (workspace_count_label_) {
      workspace_count_label_->SetText(std::u16string());
    }
    return;
  }

  // Find the active workspace name.
  const auto& workspaces = workspace_service_->workspaces();
  const std::string& active_id = workspace_service_->active_workspace_id();
  std::u16string name;
  for (const auto& ws : workspaces) {
    if (ws.id == active_id) {
      // TODO(astra): Use proper UTF-8 to UTF-16 conversion via
      // base::UTF8ToUTF16 once we link against //base strings.
      name = std::u16string(ws.name.begin(), ws.name.end());
      break;
    }
  }
  workspace_name_label_->SetText(name);

  // TODO(astra): Set chevron icon from ui/views/vector_icons or a
  // Chromium resource ID. For now the button is a placeholder.
}

void AstraWorkspaceSwitcherView::SetTabCount(int tab_count) {
  tab_count_ = std::max(0, tab_count);
  UpdateCountLabel();
}

void AstraWorkspaceSwitcherView::SetWindowCount(int window_count) {
  window_count_ = std::max(1, window_count);
  UpdateCountLabel();
}

void AstraWorkspaceSwitcherView::SetIsCurrentWindowWorkspace(bool is_current) {
  is_current_window_workspace_ = is_current;
  // TODO(astra): Add visual indicator for current window's workspace.
  // Options:
  //   - A small accent color dot next to the workspace name.
  //   - Bold the workspace name.
  //   - Highlight the background.
  // For now we just update the state; visual treatment will come
  // when the Astra color system is in place.
  // Chromium owner: ColorProvider / views theme system.
}

void AstraWorkspaceSwitcherView::UpdateCountLabel() {
  if (!workspace_count_label_) {
    return;
  }

  // Build the count string: e.g. "2 windows · 8 tabs".
  // For a single window, just show the tab count.
  //
  // TODO(astra): Use proper localized string formatting via
  //   l10n_util::GetStringFUTF16() once the Chromium l10n system is wired up.
  //   Chromium owner: ui/base/l10n/l10n_util.h
  std::u16string text;
  if (window_count_ <= 1) {
    text = base::NumberToString16(tab_count_) + u" tabs";
  } else {
    text = base::NumberToString16(window_count_) + u" windows · " +
           base::NumberToString16(tab_count_) + u" tabs";
  }
  workspace_count_label_->SetText(text);
}

void AstraWorkspaceSwitcherView::OnWorkspaceClicked(
    const std::string& workspace_id) {
  // TODO(astra): Dispatch workspace activation through the command delegate
  // or directly call AstraWorkspaceService::ActivateWorkspace, then notify
  // the sidebar to refresh.
  // The sidebar should never mutate workspace state on its own — state flows
  // from the service to UI via UpdateFromService().
  if (workspace_service_) {
    workspace_service_->ActivateWorkspace(workspace_id);
  }
}

gfx::Size AstraWorkspaceSwitcherView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = views::View::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kSwitcherHeight));
  return size;
}

void AstraWorkspaceSwitcherView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  node_data->SetName("Workspace switcher");
}

void AstraWorkspaceSwitcherView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }
  workspace_name_label_->SetEnabledColor(
      color_provider->GetColor(kSwitcherTextColorId));

  // TODO(astra): Apply theme to chevron button icon once vector icons
  // are wired in.
}

void AstraWorkspaceSwitcherView::OnSwitcherPressed() {
  expanded_ = !expanded_;
  // TODO(astra): Show workspace dropdown / expand the workspace section.
  // For now this is a stub that toggles internal state.
  // The dropdown should be implemented as a views::BubbleDialogDelegate
  // or a collapsible section inside the sidebar.
}

}  // namespace astra
