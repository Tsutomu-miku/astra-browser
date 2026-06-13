#ifndef ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_INTEGRATION_H_
#define ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_INTEGRATION_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "astra/ui/views/devtools/astra_devtools_model.h"
#include "astra/ui/views/devtools/astra_devtools_toolbar.h"
#include "astra/ui/views/devtools/astra_devtools_workspace_panel.h"
#include "ui/views/view.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class ScrollView;
}

class Profile;
class PrefService;

namespace astra {

class AstraWorkspaceService;

// =========================================================================
// AstraDevToolsIntegration — coordinator for Astra DevTools extensions
// =========================================================================
//
// This is the top-level coordinator for Astra's DevTools additions.  It
// owns the model, all panels, the toolbar, and coordinates their
// integration with Chromium's DevTools subsystem.
//
// Responsibilities:
//   - Creates and owns AstraDevToolsModel (the single source of truth).
//   - Creates and owns all Astra DevTools panels (workspace, notes, etc.).
//   - Creates and owns AstraDevToolsToolbar.
//   - Bridges Astra services to the DevTools UI components.
//   - Handles panel switching via the model.
//   - Manages the panel container (which panel is currently visible).
//   - Provides a sidebar for panel tab selection.
//   - Handles settings drawer open/close.
//   - Applies theming to all components.
//
// Relationship to Chromium DevTools:
//   - Chromium owns the full DevTools experience.
//   - Astra only adds panels, toolbar, and presentation settings.
//   - This coordinator is the glue between Chromium's DevToolsWindow
//     and Astra's Views-based UI additions.
//
// Model/View separation:
//   - Model: AstraDevToolsModel owns all state (panels, settings, theme).
//   - Views: Toolbar, panels, sidebar render state from the model.
//   - Integration: Coordinates between views and the model.
//
// Chromium subsystems reused:
//   - DevToolsWindow — hosts all Astra DevTools additions.
//   - DevToolsUIBindings — panel registration (via patch).
//   - Profile — to look up Astra services.
//   - PrefService — for persistence of presentation settings.
//
// Chromium patch points (documented):
//
//   1. DevToolsWindow creation / destruction
//      - Hook: DevToolsWindow::DevToolsWindow() / ~DevToolsWindow()
//      - Action: Create / destroy an AstraDevToolsIntegration instance.
//
//   2. DevTools panel injection
//      - Hook: DevToolsWindow's panel container construction.
//      - Action: Insert the Astra panel container into the panel area.
//
//   3. Custom panel registration
//      - Hook: DevToolsUIBindings / DevToolsPanel registration.
//
//   4. Inspected tab change
//      - Hook: DevToolsWindow::SetInspectedWebContents()
//
//   TODO(astra): Implement all patch points in a Chromium checkout.
//     For the overlay repository, this coordinator operates as a
//     standalone object that can be instantiated and attached manually.
// =========================================================================

class AstraDevToolsIntegration
    : public AstraDevToolsToolbar::Delegate,
      public AstraDevToolsWorkspacePanel::Delegate,
      public AstraDevToolsModelObserver {
 public:
  // Observer interface for Astra DevTools integration events.
  class Observer {
   public:
    virtual ~Observer() = default;

    // Called when the active Astra panel changes.
    virtual void OnActivePanelChanged(const std::string& panel_id) {}

    // Called when DevTools settings change.
    virtual void OnDevToolsSettingsChanged() {}

    // Called when the dock position changes.
    virtual void OnDockPositionChanged(AstraDevToolsDockPosition position) {}
  };

  // Constructs the integration coordinator for a given profile.
  // The profile is used to look up Astra services and PrefService.
  explicit AstraDevToolsIntegration(Profile* profile);
  ~AstraDevToolsIntegration() override;

  AstraDevToolsIntegration(const AstraDevToolsIntegration&) = delete;
  AstraDevToolsIntegration& operator=(const AstraDevToolsIntegration&) = delete;

  // -- Model access --------------------------------------------------------

  AstraDevToolsModel* model() { return model_.get(); }
  const AstraDevToolsModel* model() const { return model_.get(); }

  // -- Inspected tab management -------------------------------------------

  // Sets the WebContents currently being inspected by DevTools.
  void SetInspectedWebContents(content::WebContents* web_contents);
  content::WebContents* inspected_contents() const {
    return inspected_contents_;
  }

  // -- UI component access ------------------------------------------------

  // Returns the top-level container view for all Astra DevTools UI.
  // This view contains the toolbar, panel sidebar, and active panel content.
  views::View* container_view();

  // Returns the Astra DevTools toolbar.
  AstraDevToolsToolbar* toolbar();

  // Returns the Workspace Inspector panel.
  AstraDevToolsWorkspacePanel* workspace_panel();

  // -- Panel management ----------------------------------------------------

  // Switches to the panel with the given ID.
  // Returns false if the panel doesn't exist or isn't visible.
  bool SwitchToPanel(const std::string& panel_id);

  // Returns the ID of the currently active panel.
  std::string active_panel_id() const;

  // -- Settings drawer -----------------------------------------------------

  // Opens or closes the settings drawer.
  void SetSettingsDrawerOpen(bool open);
  bool IsSettingsDrawerOpen() const { return settings_drawer_open_; }
  void ToggleSettingsDrawer();

  // -- Theming -------------------------------------------------------------

  // Applies the current theme from the model to all UI components.
  void ApplyTheme();

  // -- Observer management -------------------------------------------------

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // -- Service access -----------------------------------------------------

  AstraWorkspaceService* workspace_service() const {
    return workspace_service_;
  }

  // -- AstraDevToolsToolbar::Delegate ------------------------------------

  void OnPanelTabClicked(const std::string& panel_id) override;
  void OnSettingsClicked() override;
  void OnDetachClicked() override;
  void OnMenuClicked() override;
  void OnBackClicked() override;
  void OnForwardClicked() override;
  void OnSearchTextChanged(const std::u16string& text) override;
  void OnFocusModeToggled() override;

  // -- AstraDevToolsWorkspacePanel::Delegate -----------------------------

  void OnNewWorkspace() override;
  void OnDeleteWorkspace(const std::string& workspace_id) override;
  void OnRenameWorkspace(const std::string& workspace_id,
                         const std::string& new_name) override;
  void OnWorkspaceSelected(const std::string& workspace_id) override;
  void OnTabSelected(int tab_index) override;

  // -- AstraDevToolsModelObserver ----------------------------------------

  void OnActivePanelChanged(const std::string& panel_id) override;
  void OnPanelOpened(const std::string& panel_id) override;
  void OnPanelClosed(const std::string& panel_id) override;
  void OnPanelOrderChanged() override;
  void OnDevToolsSettingsChanged() override;
  void OnDockPositionChanged(AstraDevToolsDockPosition position) override;
  void OnThemeChanged(AstraDevToolsTheme theme) override;

  // -- Manual install helpers (for testing / overlay) --------------------

  // Installs the toolbar and panel into their target containers.
  // For the overlay skeleton, this creates standalone widgets so
  // components can be tested independently.
  void InstallForTesting();

  // Builds the full container view with toolbar + panel area.
  // Useful for embedding in a test widget.
  void BuildContainerView();

  // Accessors for testing.
  views::View* panel_container_for_testing() { return panel_container_; }
  views::View* sidebar_view_for_testing() { return sidebar_view_; }
  views::View* settings_drawer_for_testing() { return settings_drawer_; }
  views::View* container_view_for_testing() { return container_view_.get(); }
  bool container_view_built_for_testing() const {
    return container_view_ != nullptr;
  }

 private:
  // Looks up Astra services from the profile.
  void InitializeServices();

  // Ensures the model is created.
  void EnsureModel();

  // Ensures the workspace panel is created.
  void EnsureWorkspacePanel();

  // Refreshes all UI components with current service state.
  void RefreshAll();

  // Updates the panel container to show the active panel.
  void UpdateActivePanelView();

  // Builds the sidebar with panel tabs.
  void BuildSidebar();

  // Rebuilds sidebar tabs from the model.
  void RebuildSidebarTabs();

  // The profile associated with this DevTools instance.  Not owned.
  raw_ptr<Profile> profile_;

  // The pref service for persistence.  Not owned.
  raw_ptr<PrefService> pref_service_ = nullptr;

  // The currently inspected WebContents.  Not owned.
  raw_ptr<content::WebContents> inspected_contents_ = nullptr;

  // Astra services — not owned, looked up from profile.
  raw_ptr<AstraWorkspaceService> workspace_service_ = nullptr;

  // The model — owned by this coordinator.
  std::unique_ptr<AstraDevToolsModel> model_;

  // The toolbar — owned by this coordinator (or the views hierarchy).
  std::unique_ptr<AstraDevToolsToolbar> toolbar_;

  // The workspace panel — owned by this coordinator.
  std::unique_ptr<AstraDevToolsWorkspacePanel> workspace_panel_;

  // Top-level container view — owns the full Astra DevTools UI tree.
  std::unique_ptr<views::View> container_view_;

  // Panel container (shows the active panel).  Owned by container_view_.
  raw_ptr<views::View> panel_container_ = nullptr;

  // Sidebar view with panel tabs.  Owned by container_view_.
  raw_ptr<views::View> sidebar_view_ = nullptr;

  // Settings drawer.  Owned by container_view_.
  raw_ptr<views::View> settings_drawer_ = nullptr;

  // Whether the settings drawer is currently open.
  bool settings_drawer_open_ = false;

  // Observers for Astra DevTools events.
  base::ObserverList<Observer> observers_;

  // Sidebar tab buttons.  Owned by sidebar_view_'s children.
  std::vector<raw_ptr<views::LabelButton>> sidebar_tabs_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_INTEGRATION_H_
