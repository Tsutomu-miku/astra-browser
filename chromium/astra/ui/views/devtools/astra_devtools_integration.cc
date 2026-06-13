#include "astra/ui/views/devtools/astra_devtools_integration.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/browser/astra_workspace_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

class Profile;

namespace astra {

namespace {

// Sidebar width for panel tabs.
constexpr int kSidebarWidth = 200;

// Settings drawer width.
constexpr int kSettingsDrawerWidth = 280;

// Height of sidebar tab items.
constexpr int kSidebarTabHeight = 36;

// Spacing between sidebar tabs.
constexpr int kSidebarTabSpacing = 2;

// Top padding in the sidebar.
constexpr int kSidebarTopPadding = 8;

// Sidebar horizontal padding.
constexpr int kSidebarHorizontalPadding = 4;

// Dark theme colors.
constexpr SkColor kDarkSidebarBg = SkColorSetRGB(0x25, 0x25, 0x25);
constexpr SkColor kDarkPanelBg = SkColorSetRGB(0x20, 0x20, 0x20);
constexpr SkColor kDarkText = SK_ColorWHITE;
constexpr SkColor kDarkTextSecondary = SkColorSetRGB(0x9A, 0x9A, 0x9A);
constexpr SkColor kDarkActiveTabBg = SkColorSetRGB(0x1A, 0x5A, 0x9A);
constexpr SkColor kDarkBorder = SkColorSetRGB(0x44, 0x44, 0x44);

// Light theme colors.
constexpr SkColor kLightSidebarBg = SkColorSetRGB(0xF0, 0xF0, 0xF0);
constexpr SkColor kLightPanelBg = SkColorSetRGB(0xFA, 0xFA, 0xFA);
constexpr SkColor kLightText = SkColorSetRGB(0x20, 0x20, 0x20);
constexpr SkColor kLightTextSecondary = SkColorSetRGB(0x66, 0x66, 0x66);
constexpr SkColor kLightActiveTabBg = SkColorSetRGB(0xC8, 0xE0, 0xFC);
constexpr SkColor kLightBorder = SkColorSetRGB(0xDD, 0xDD, 0xDD);

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraDevToolsIntegration::AstraDevToolsIntegration(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
  InitializeServices();
  EnsureModel();
}

AstraDevToolsIntegration::~AstraDevToolsIntegration() {
  if (model_) {
    model_->RemoveObserver(static_cast<AstraDevToolsModelObserver*>(this));
    model_->RemoveObserver(static_cast<AstraDevToolsObserver*>(this));
  }
}

// =========================================================================
// DevTools lifecycle (deepened)
// =========================================================================

void AstraDevToolsIntegration::ShowDevTools() {
  if (!model_) {
    return;
  }
  if (model_->IsDevToolsOpen()) {
    return;
  }
  model_->SetDevToolsOpen(true);
}

void AstraDevToolsIntegration::CloseDevTools() {
  if (!model_) {
    return;
  }
  if (!model_->IsDevToolsOpen()) {
    return;
  }
  model_->SetDevToolsOpen(false);
}

bool AstraDevToolsIntegration::IsDevToolsOpen() const {
  if (!model_) {
    return false;
  }
  return model_->IsDevToolsOpen();
}

void AstraDevToolsIntegration::ToggleDevTools() {
  if (IsDevToolsOpen()) {
    CloseDevTools();
  } else {
    ShowDevTools();
  }
}

void AstraDevToolsIntegration::ReloadDevTools() {
  // TODO(astra): Implement actual DevTools reload via DevToolsWindow.
  //   In a full Chromium build, this would call
  //   DevToolsWindow::ReloadInspectedWebContents() or similar.
  //   For the overlay skeleton, this is a no-op that logs.
  VLOG(1) << "Astra DevTools: Reload requested";

  // Refresh UI as if a reload happened.
  RefreshAll();
}

// =========================================================================
// Panel management (deepened)
// =========================================================================

bool AstraDevToolsIntegration::ShowPanel(AstraDevToolsPanelType panel_type) {
  if (!model_) {
    return false;
  }

  // Open DevTools if not already open.
  if (!model_->IsDevToolsOpen()) {
    model_->SetDevToolsOpen(true);
  }

  // Show the panel via the model.
  bool result = model_->ShowAstraPanel(panel_type);

  if (result) {
    panel_open_ = true;
    NotifyPanelShown(panel_type);

    // Update UI.
    if (toolbar_) {
      toolbar_->UpdateFromModel();
    }
    UpdateActivePanelView();
  }

  return result;
}

void AstraDevToolsIntegration::ClosePanel() {
  if (!panel_open_) {
    return;
  }

  panel_open_ = false;
  NotifyPanelHidden();

  // Update active panel view (hide panel content).
  if (panel_container_) {
    panel_container_->SetVisible(false);
  }
}

bool AstraDevToolsIntegration::IsPanelOpen() const {
  return panel_open_;
}

// =========================================================================
// Dock state (deepened)
// =========================================================================

void AstraDevToolsIntegration::SetDockState(AstraDevToolsDockState state) {
  if (!model_) {
    return;
  }
  model_->SetDockState(state);
}

AstraDevToolsDockState AstraDevToolsIntegration::GetDockState() const {
  if (!model_) {
    return AstraDevToolsDockState::kDockedBottom;
  }
  return model_->GetDockState();
}

// =========================================================================
// Zoom (deepened)
// =========================================================================

double AstraDevToolsIntegration::GetZoomLevel() const {
  if (!model_) {
    return AstraDevToolsModel::kDefaultZoomLevel;
  }
  return model_->GetZoomLevel();
}

void AstraDevToolsIntegration::SetZoomLevel(double level) {
  if (!model_) {
    return;
  }
  model_->SetZoomLevel(level);
}

// =========================================================================
// Inspection & emulation (deepened)
// =========================================================================

void AstraDevToolsIntegration::InspectElement() {
  // TODO(astra): Implement actual element inspection via DevToolsWindow.
  //   In a full Chromium build, this would call
  //   DevToolsWindow::ToggleInspectElementMode() or similar.
  //   For the overlay skeleton, this toggles a local flag.
  //   Chromium owner: chrome/browser/devtools/devtools_window.h
  inspect_element_active_ = !inspect_element_active_;
  VLOG(1) << "Astra DevTools: Inspect element mode: "
          << (inspect_element_active_ ? "on" : "off");
}

void AstraDevToolsIntegration::ToggleDeviceMode() {
  // TODO(astra): Implement device mode toggle via DevToolsWindow.
  //   In a full Chromium build, this would toggle device emulation mode
  //   via DevToolsWindow::ToggleDeviceMode() or similar.
  //   Chromium owner: chrome/browser/devtools/devtools_window.h
  device_mode_active_ = !device_mode_active_;
  VLOG(1) << "Astra DevTools: Device mode: "
          << (device_mode_active_ ? "on" : "off");
}

// =========================================================================
// Services
// =========================================================================

void AstraDevToolsIntegration::InitializeServices() {
  // Look up Astra workspace service from the profile.
  // TODO(astra): Use the proper factory getter in a full Chromium build.
  //   For the overlay skeleton, this is null — UI handles it gracefully.
  if (profile_) {
    workspace_service_ = nullptr;
  }
}

void AstraDevToolsIntegration::EnsureModel() {
  if (model_) {
    return;
  }

  // TODO(astra): Get PrefService from profile_->GetPrefs() in a full
  //   Chromium build.  For the overlay skeleton, the model runs in
  //   in-memory mode (no persistence).
  //   Chromium owner: Profile::GetPrefs()
  model_ = std::make_unique<AstraDevToolsModel>(nullptr);
  model_->AddObserver(static_cast<AstraDevToolsModelObserver*>(this));
  model_->AddObserver(static_cast<AstraDevToolsObserver*>(this));
}

void AstraDevToolsIntegration::EnsureWorkspacePanel() {
  if (workspace_panel_) {
    return;
  }
  workspace_panel_ = std::make_unique<AstraDevToolsWorkspacePanel>();
  workspace_panel_->SetDelegate(this);
  workspace_panel_->SetModel(model_.get());
  workspace_panel_->SetWorkspaceService(workspace_service_);
  workspace_panel_->SetInspectedWebContents(inspected_contents_);
  workspace_panel_->Refresh();
}

// =========================================================================
// Inspected tab
// =========================================================================

void AstraDevToolsIntegration::SetInspectedWebContents(
    content::WebContents* web_contents) {
  if (inspected_contents_ == web_contents) {
    return;
  }

  inspected_contents_ = web_contents;

  if (toolbar_) {
    toolbar_->SetInspectedWebContents(web_contents);
  }
  if (workspace_panel_) {
    workspace_panel_->SetInspectedWebContents(web_contents);
  }

  RefreshAll();
}

// =========================================================================
// Container view
// =========================================================================

views::View* AstraDevToolsIntegration::container_view() {
  if (!container_view_) {
    BuildContainerView();
  }
  return container_view_.get();
}

void AstraDevToolsIntegration::BuildContainerView() {
  container_view_ = std::make_unique<views::View>();
  auto* main_layout = container_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          0));

  bool dark_theme = true;
  if (model_) {
    dark_theme = model_->GetEffectiveTheme() == AstraDevToolsTheme::kDark;
  }

  SkColor sidebar_bg = dark_theme ? kDarkSidebarBg : kLightSidebarBg;
  SkColor panel_bg = dark_theme ? kDarkPanelBg : kLightPanelBg;
  SkColor border_color = dark_theme ? kDarkBorder : kLightBorder;

  // --- Sidebar (panel tabs).
  sidebar_view_ = container_view_->AddChildView(
      std::make_unique<views::View>());
  sidebar_view_->SetPreferredSize(gfx::Size(kSidebarWidth, 0));
  sidebar_view_->SetBackground(views::CreateSolidBackground(sidebar_bg));
  sidebar_view_->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 0, 1, border_color));

  auto* sidebar_layout = sidebar_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(kSidebarTopPadding, kSidebarHorizontalPadding),
          kSidebarTabSpacing));
  sidebar_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  sidebar_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Sidebar header label.
  auto* sidebar_header = sidebar_view_->AddChildView(
      std::make_unique<views::Label>(u"Astra Panels"));
  sidebar_header->SetEnabledColor(dark_theme ? kDarkText : kLightText);
  sidebar_header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  sidebar_header->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, 8)));
  gfx::FontList font = sidebar_header->font_list();
  sidebar_header->SetFontList(font.Derive(
      0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD));

  // Scroll view for tab list.
  auto* sidebar_scroll = sidebar_view_->AddChildView(
      std::make_unique<views::ScrollView>());
  sidebar_scroll->SetClipToBounds(true);
  sidebar_scroll->SetBackgroundColor(sidebar_bg);
  sidebar_layout->SetFlexForView(sidebar_scroll, 1);

  views::View* tabs_container = sidebar_scroll->SetContents(
      std::make_unique<views::View>());
  tabs_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(4, 0),
          kSidebarTabSpacing));

  RebuildSidebarTabs();

  // --- Panel area (shows active panel).
  panel_container_ = container_view_->AddChildView(
      std::make_unique<views::View>());
  panel_container_->SetLayoutManager(
      std::make_unique<views::FillLayout>());
  panel_container_->SetBackground(views::CreateSolidBackground(panel_bg));
  main_layout->SetFlexForView(panel_container_, 1);

  // --- Settings drawer.
  settings_drawer_ = container_view_->AddChildView(
      std::make_unique<views::View>());
  settings_drawer_->SetPreferredSize(gfx::Size(kSettingsDrawerWidth, 0));
  settings_drawer_->SetBackground(views::CreateSolidBackground(sidebar_bg));
  settings_drawer_->SetBorder(views::CreateSolidSidedBorder(
      0, 1, 0, 0, border_color));
  settings_drawer_->SetVisible(false);

  auto* drawer_layout = settings_drawer_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(12, 12),
          8));
  drawer_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  drawer_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  auto* drawer_header = settings_drawer_->AddChildView(
      std::make_unique<views::Label>(u"DevTools Settings"));
  drawer_header->SetEnabledColor(dark_theme ? kDarkText : kLightText);
  drawer_header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  drawer_header->SetFontList(drawer_header->font_list().Derive(
      0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD));

  auto* setting1 = settings_drawer_->AddChildView(
      std::make_unique<views::Label>(u"Show panel icons"));
  setting1->SetEnabledColor(dark_theme ? kDarkTextSecondary : kLightTextSecondary);
  setting1->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  auto* setting2 = settings_drawer_->AddChildView(
      std::make_unique<views::Label>(u"Compact mode"));
  setting2->SetEnabledColor(dark_theme ? kDarkTextSecondary : kLightTextSecondary);
  setting2->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  auto* setting3 = settings_drawer_->AddChildView(
      std::make_unique<views::Label>(u"Remember last panel"));
  setting3->SetEnabledColor(dark_theme ? kDarkTextSecondary : kLightTextSecondary);
  setting3->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Show active panel.
  UpdateActivePanelView();
}

// =========================================================================
// Toolbar
// =========================================================================

AstraDevToolsToolbar* AstraDevToolsIntegration::toolbar() {
  if (!toolbar_) {
    toolbar_ = std::make_unique<AstraDevToolsToolbar>(this);
    toolbar_->SetModel(model_.get());
    toolbar_->SetInspectedWebContents(inspected_contents_);
    toolbar_->UpdateFromModel();
  }
  return toolbar_.get();
}

// =========================================================================
// Workspace panel
// =========================================================================

AstraDevToolsWorkspacePanel* AstraDevToolsIntegration::workspace_panel() {
  EnsureWorkspacePanel();
  return workspace_panel_.get();
}

// =========================================================================
// Panel management (legacy)
// =========================================================================

bool AstraDevToolsIntegration::SwitchToPanel(const std::string& panel_id) {
  if (!model_) {
    return false;
  }
  return model_->SetActivePanel(panel_id);
}

std::string AstraDevToolsIntegration::active_panel_id() const {
  if (!model_) {
    return std::string();
  }
  return model_->active_panel_id();
}

// =========================================================================
// Settings drawer
// =========================================================================

void AstraDevToolsIntegration::SetSettingsDrawerOpen(bool open) {
  if (settings_drawer_open_ == open) {
    return;
  }
  settings_drawer_open_ = open;

  if (settings_drawer_) {
    settings_drawer_->SetVisible(open);
    if (container_view_) {
      container_view_->InvalidateLayout();
    }
  }
}

void AstraDevToolsIntegration::ToggleSettingsDrawer() {
  SetSettingsDrawerOpen(!settings_drawer_open_);
}

// =========================================================================
// Theming
// =========================================================================

void AstraDevToolsIntegration::ApplyTheme() {
  if (!model_) {
    return;
  }

  bool dark_theme =
      model_->GetEffectiveTheme() == AstraDevToolsTheme::kDark;

  if (toolbar_) {
    toolbar_->SetTheme(dark_theme);
  }
  if (workspace_panel_) {
    workspace_panel_->SetTheme(dark_theme);
  }

  SkColor sidebar_bg = dark_theme ? kDarkSidebarBg : kLightSidebarBg;
  SkColor panel_bg = dark_theme ? kDarkPanelBg : kLightPanelBg;
  SkColor border_color = dark_theme ? kDarkBorder : kLightBorder;
  SkColor text_color = dark_theme ? kDarkText : kLightText;
  SkColor text_secondary =
      dark_theme ? kDarkTextSecondary : kLightTextSecondary;
  SkColor active_bg = dark_theme ? kDarkActiveTabBg : kLightActiveTabBg;

  if (sidebar_view_) {
    sidebar_view_->SetBackground(views::CreateSolidBackground(sidebar_bg));
    sidebar_view_->SetBorder(views::CreateSolidSidedBorder(
        0, 0, 0, 1, border_color));
  }

  if (panel_container_) {
    panel_container_->SetBackground(views::CreateSolidBackground(panel_bg));
  }

  if (settings_drawer_) {
    settings_drawer_->SetBackground(
        views::CreateSolidBackground(sidebar_bg));
    settings_drawer_->SetBorder(views::CreateSolidSidedBorder(
        0, 1, 0, 0, border_color));
  }

  // Update sidebar tabs.
  const std::string& active_id = model_->active_panel_id();
  auto panels = model_->GetVisiblePanels();

  for (size_t i = 0; i < sidebar_tabs_.size() && i < panels.size(); ++i) {
    auto* tab = sidebar_tabs_[i];
    tab->SetTextColor(views::Button::STATE_NORMAL, text_color);
    tab->SetTextColor(views::Button::STATE_HOVERED, text_color);
    tab->SetTextColor(views::Button::STATE_PRESSED, text_color);

    if (panels[i].id == active_id) {
      tab->SetBackground(views::CreateSolidBackground(active_bg));
    } else {
      tab->SetBackground(nullptr);
    }
  }
}

// =========================================================================
// Observer management
// =========================================================================

void AstraDevToolsIntegration::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AstraDevToolsIntegration::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AstraDevToolsIntegration::AddIntegrationObserver(
    AstraDevToolsIntegrationObserver* observer) {
  integration_observers_.AddObserver(observer);
}

void AstraDevToolsIntegration::RemoveIntegrationObserver(
    AstraDevToolsIntegrationObserver* observer) {
  integration_observers_.RemoveObserver(observer);
}

// =========================================================================
// Private: notify integration observers
// =========================================================================

void AstraDevToolsIntegration::NotifyPanelShown(AstraDevToolsPanelType type) {
  for (auto& observer : integration_observers_) {
    observer.OnPanelShown(this, type);
  }
}

void AstraDevToolsIntegration::NotifyPanelHidden() {
  for (auto& observer : integration_observers_) {
    observer.OnPanelHidden(this);
  }
}

// =========================================================================
// AstraDevToolsToolbar::Delegate
// =========================================================================

void AstraDevToolsIntegration::OnPanelTabClicked(
    const std::string& panel_id) {
  SwitchToPanel(panel_id);
}

void AstraDevToolsIntegration::OnSettingsClicked() {
  ToggleSettingsDrawer();
}

void AstraDevToolsIntegration::OnDetachClicked() {
  if (model_) {
    model_->CycleDockPosition();
  }
}

void AstraDevToolsIntegration::OnDockClicked() {
  if (model_) {
    model_->ToggleDockSide();
  }
}

void AstraDevToolsIntegration::OnMenuClicked() {
  VLOG(1) << "Astra DevTools: Menu button clicked";
}

void AstraDevToolsIntegration::OnCloseClicked() {
  CloseDevTools();
}

void AstraDevToolsIntegration::OnAstraTabClicked() {
  VLOG(1) << "Astra DevTools: Astra tab clicked";
  // Toggle Astra panel visibility.
  if (panel_open_) {
    ClosePanel();
  } else {
    ShowPanel(AstraDevToolsPanelType::kWorkspacePanel);
  }
}

void AstraDevToolsIntegration::OnBackClicked() {
  VLOG(1) << "Astra DevTools: Back button clicked";
}

void AstraDevToolsIntegration::OnForwardClicked() {
  VLOG(1) << "Astra DevTools: Forward button clicked";
}

void AstraDevToolsIntegration::OnSearchTextChanged(
    const std::u16string& text) {
  if (workspace_panel_) {
    workspace_panel_->SetSearchFilter(text);
  }
}

void AstraDevToolsIntegration::OnFocusModeToggled() {
  VLOG(1) << "Astra DevTools: Focus Mode toggled";
}

// =========================================================================
// AstraDevToolsWorkspacePanel::Delegate
// =========================================================================

void AstraDevToolsIntegration::OnNewWorkspace() {
  VLOG(1) << "Astra DevTools: New workspace requested";
  RefreshAll();
}

void AstraDevToolsIntegration::OnDeleteWorkspace(
    const std::string& workspace_id) {
  VLOG(1) << "Astra DevTools: Delete workspace: " << workspace_id;
  RefreshAll();
}

void AstraDevToolsIntegration::OnRenameWorkspace(
    const std::string& workspace_id,
    const std::string& new_name) {
  VLOG(1) << "Astra DevTools: Rename workspace: " << workspace_id
          << " -> " << new_name;
  RefreshAll();
}

void AstraDevToolsIntegration::OnWorkspaceSelected(
    const std::string& workspace_id) {
  VLOG(1) << "Astra DevTools: Workspace selected: " << workspace_id;
}

void AstraDevToolsIntegration::OnTabSelected(int tab_index) {
  VLOG(1) << "Astra DevTools: Tab selected: " << tab_index;
}

void AstraDevToolsIntegration::OnWorkspaceColorChanged(
    const std::string& workspace_id, SkColor color) {
  VLOG(1) << "Astra DevTools: Workspace color changed: " << workspace_id
          << " -> " << base::StringPrintf("#%06X", color & 0xFFFFFF);
}

// =========================================================================
// AstraDevToolsModelObserver (legacy)
// =========================================================================

void AstraDevToolsIntegration::OnActivePanelChanged(
    const std::string& panel_id) {
  UpdateActivePanelView();
  if (toolbar_) {
    toolbar_->UpdateFromModel();
  }
  RebuildSidebarTabs();

  for (auto& observer : observers_) {
    observer.OnActivePanelChanged(panel_id);
  }
}

void AstraDevToolsIntegration::OnPanelOpened(
    const std::string& panel_id) {
  RebuildSidebarTabs();
}

void AstraDevToolsIntegration::OnPanelClosed(
    const std::string& panel_id) {
  RebuildSidebarTabs();
}

void AstraDevToolsIntegration::OnPanelOrderChanged() {
  RebuildSidebarTabs();
  if (toolbar_) {
    toolbar_->UpdateFromModel();
  }
}

void AstraDevToolsIntegration::OnDevToolsSettingsChanged() {
  if (toolbar_) {
    toolbar_->UpdateFromModel();
  }
  ApplyTheme();

  for (auto& observer : observers_) {
    observer.OnDevToolsSettingsChanged();
  }
}

void AstraDevToolsIntegration::OnDockPositionChanged(
    AstraDevToolsDockPosition position) {
  for (auto& observer : observers_) {
    observer.OnDockPositionChanged(position);
  }
}

void AstraDevToolsIntegration::OnThemeChanged(AstraDevToolsTheme theme) {
  ApplyTheme();
}

// =========================================================================
// AstraDevToolsObserver (deepened)
// =========================================================================

void AstraDevToolsIntegration::OnDevToolsOpened(AstraDevToolsModel* model) {
  for (auto& observer : integration_observers_) {
    observer.OnDevToolsOpened(this);
  }
}

void AstraDevToolsIntegration::OnDevToolsClosed(AstraDevToolsModel* model) {
  for (auto& observer : integration_observers_) {
    observer.OnDevToolsClosed(this);
  }
}

void AstraDevToolsIntegration::OnPanelActivated(
    AstraDevToolsModel* model, const std::string& panel_id) {
  VLOG(1) << "Astra DevTools: Panel activated: " << panel_id;
  UpdateActivePanelView();
}

void AstraDevToolsIntegration::OnPanelEnabledChanged(
    AstraDevToolsModel* model,
    const std::string& panel_id,
    bool enabled) {
  VLOG(1) << "Astra DevTools: Panel " << panel_id
          << " enabled: " << (enabled ? "true" : "false");
  if (toolbar_) {
    toolbar_->UpdateFromModel();
  }
}

void AstraDevToolsIntegration::OnPanelVisibilityChanged(
    AstraDevToolsModel* model,
    const std::string& panel_id,
    bool visible) {
  VLOG(1) << "Astra DevTools: Panel " << panel_id
          << " visible: " << (visible ? "true" : "false");
  if (toolbar_) {
    toolbar_->UpdateFromModel();
  }
  RebuildSidebarTabs();
}

void AstraDevToolsIntegration::OnPanelsReordered(AstraDevToolsModel* model) {
  if (toolbar_) {
    toolbar_->UpdateFromModel();
  }
  RebuildSidebarTabs();
}

void AstraDevToolsIntegration::OnDockStateChanged(
    AstraDevToolsModel* model, AstraDevToolsDockState state) {
  if (toolbar_) {
    toolbar_->SetDockState(state);
  }
}

void AstraDevToolsIntegration::OnDevToolsModelShutdown(
    AstraDevToolsModel* model) {
  // Model is shutting down — nothing to do since integration owns the model.
}

// =========================================================================
// Sidebar tabs
// =========================================================================

void AstraDevToolsIntegration::BuildSidebar() {
  RebuildSidebarTabs();
}

void AstraDevToolsIntegration::RebuildSidebarTabs() {
  if (!model_ || !sidebar_view_ ||
      sidebar_view_->children().size() < 2) {
    return;
  }

  // The scroll view is the second child (index 1).
  auto* scroll_view =
      static_cast<views::ScrollView*>(sidebar_view_->children()[1]);
  if (!scroll_view || !scroll_view->contents()) {
    return;
  }

  views::View* tabs_container = scroll_view->contents();
  sidebar_tabs_.clear();
  tabs_container->RemoveAllChildViews();

  // Use deepened panel system if available.
  auto panels = model_->GetPanels();
  const std::string& active_id = model_->GetActivePanel();

  bool dark_theme =
      model_->GetEffectiveTheme() == AstraDevToolsTheme::kDark;
  SkColor text_color = dark_theme ? kDarkText : kLightText;
  SkColor active_bg = dark_theme ? kDarkActiveTabBg : kLightActiveTabBg;

  for (const auto& panel : panels) {
    if (!panel.is_visible || !panel.is_enabled) {
      continue;
    }

    auto* tab = tabs_container->AddChildView(
        std::make_unique<views::LabelButton>(
            base::BindRepeating(
                &AstraDevToolsIntegration::SwitchToPanel,
                base::Unretained(this), panel.panel_id),
            panel.title));
    tab->SetMinSize(gfx::Size(0, kSidebarTabHeight));
    tab->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    tab->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(0, 12)));
    tab->SetFocusForPlatform();
    tab->SetTooltipText(panel.title);

    tab->SetTextColor(views::Button::STATE_NORMAL, text_color);
    tab->SetTextColor(views::Button::STATE_HOVERED, text_color);
    tab->SetTextColor(views::Button::STATE_PRESSED, text_color);

    if (panel.panel_id == active_id) {
      tab->SetBackground(views::CreateSolidBackground(active_bg));
    }

    sidebar_tabs_.push_back(tab);
  }

  tabs_container->InvalidateLayout();
}

// =========================================================================
// Active panel view
// =========================================================================

void AstraDevToolsIntegration::UpdateActivePanelView() {
  if (!panel_container_ || !model_) {
    return;
  }

  const std::string& active_id = model_->GetActivePanel();

  // Check if workspace panel is active.
  const auto* workspace_panel_info =
      model_->GetPanelByType(AstraDevToolsPanelType::kWorkspacePanel);

  if (workspace_panel_info && active_id == workspace_panel_info->panel_id) {
    // Ensure the panel exists.
    EnsureWorkspacePanel();

    if (workspace_panel_) {
      panel_container_->RemoveAllChildViews();
      panel_container_->AddChildView(std::move(workspace_panel_));
      // workspace_panel_ is now null after move.
      // We'll access via the panel_container_'s children.
    }
  } else if (!active_id.empty()) {
    // Show a placeholder for unimplemented panels.
    panel_container_->RemoveAllChildViews();

    bool dark_theme =
        model_->GetEffectiveTheme() == AstraDevToolsTheme::kDark;

    auto* placeholder = panel_container_->AddChildView(
        std::make_unique<views::Label>(
            base::UTF8ToUTF16(active_id + " panel")));
    placeholder->SetEnabledColor(
        dark_theme ? kDarkTextSecondary : kLightTextSecondary);
    placeholder->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    placeholder->SetMultiLine(true);
  }

  panel_container_->InvalidateLayout();
}

// =========================================================================
// Refresh all
// =========================================================================

void AstraDevToolsIntegration::RefreshAll() {
  if (toolbar_) {
    toolbar_->UpdateFromModel();
  }
  if (workspace_panel_) {
    workspace_panel_->Refresh();
  }
}

// =========================================================================
// Install for testing
// =========================================================================

void AstraDevToolsIntegration::InstallForTesting() {
  // Ensure components are created so tests can access them.
  toolbar();
  EnsureWorkspacePanel();

  VLOG(1) << "Astra DevTools: Integration installed (test mode)";
}

}  // namespace astra
