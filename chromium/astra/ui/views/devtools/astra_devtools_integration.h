#ifndef ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_INTEGRATION_H_
#define ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_INTEGRATION_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
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
// AstraDevToolsIntegrationObserver — observer for integration events
// =========================================================================
//
// Observer interface for high-level Astra DevTools integration events.
// All methods have empty default implementations.
//
// Follows Chromium observer conventions:
//   - base::CheckedObserver base class
//   - base::ObserverList for management
//   - Empty default implementations
// =========================================================================
class AstraDevToolsIntegrationObserver : public base::CheckedObserver {
 public:
  // Called when DevTools is opened.
  virtual void OnDevToolsOpened(AstraDevToolsIntegration* integration) {}

  // Called when DevTools is closed.
  virtual void OnDevToolsClosed(AstraDevToolsIntegration* integration) {}

  // Called when an Astra panel is shown (made active).
  virtual void OnPanelShown(AstraDevToolsIntegration* integration,
                            AstraDevToolsPanelType type) {}

  // Called when the Astra panel is hidden / closed.
  virtual void OnPanelHidden(AstraDevToolsIntegration* integration) {}

 protected:
  ~AstraDevToolsIntegrationObserver() override = default;
};

// =========================================================================
// AstraDevToolsIntegration — coordinator for Astra DevTools extensions
// =========================================================================
//
// This is the top-level coordinator for Astra's DevTools additions.  It
// owns the model, all panels, the toolbar, and coordinates their
// integration with Chromium's DevTools subsystem.
//
// Responsibilities (deepened):
//   - Creates and owns AstraDevToolsModel (the single source of truth).
//   - Creates and owns all Astra DevTools panels (workspace, notes, etc.).
//   - Creates and owns AstraDevToolsToolbar.
//   - Bridges Astra services to the DevTools UI components.
//   - Handles panel switching via the model.
//   - Manages DevTools open/close state.
//   - Manages dock state.
//   - Manages zoom level.
//   - Provides inspect element mode toggle.
//   - Provides device mode toggle.
//   - Provides reload functionality.
//
// Relationship to Chromium DevTools:
//   - Chromium owns the full DevTools experience.
//   - Astra only adds panels, toolbar, and presentation settings.
//   - This coordinator is the glue between Chromium's DevToolsWindow
//     and Astra's Views-based UI additions.
//
// Chromium subsystems reused:
//   - DevToolsWindow — hosts all Astra DevTools additions.
//   - DevToolsUIBindings — panel registration (via patch).
//   - Profile — to look up Astra services.
//   - PrefService — for persistence of presentation settings.
//
// Chromium patch points (documented):
//   1. DevToolsWindow creation / destruction
//   2. DevTools panel injection
//   3. Custom panel registration
//   4. Inspected tab change
//
//   TODO(astra): Implement all patch points in a Chromium checkout.
//     For the overlay repository, this coordinator operates as a
//     standalone object that can be instantiated and attached manually.
// =========================================================================

class AstraDevToolsIntegration
    : public AstraDevToolsToolbar::Delegate,
      public AstraDevToolsWorkspacePanel::Delegate,
      public AstraDevToolsModelObserver,
      public AstraDevToolsObserver {
 public:
  // Observer interface for Astra DevTools integration events (legacy).
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
  explicit AstraDevToolsIntegration(Profile* profile);
  ~AstraDevToolsIntegration() override;

  AstraDevToolsIntegration(const AstraDevToolsIntegration&) = delete;
  AstraDevToolsIntegration& operator=(const AstraDevToolsIntegration&) = delete;

  // -- DevTools lifecycle --------------------------------------------------

  // Opens DevTools.  No-op if already open.
  void ShowDevTools();

  // Closes DevTools.  No-op if already closed.
  void CloseDevTools();

  // Returns true if DevTools is currently open.
  bool IsDevToolsOpen() const;

  // Toggles DevTools open/closed.
  void ToggleDevTools();

  // Reloads DevTools (reloads the DevTools frontend).
  void ReloadDevTools();

  // -- Panel management ----------------------------------------------------

  // Opens DevTools (if not already open) and shows the panel of the
  // given type.  Returns true if the panel was successfully shown.
  bool ShowPanel(AstraDevToolsPanelType panel_type);

  // Closes (hides) the active Astra panel.
  void ClosePanel();

  // Returns true if an Astra panel is currently open/active.
  bool IsPanelOpen() const;

  // -- Model access --------------------------------------------------------

  // Returns the DevTools model.
  AstraDevToolsModel* GetModel() { return model_.get(); }
  const AstraDevToolsModel* GetModel() const { return model_.get(); }

  // -- Dock state ----------------------------------------------------------

  // Sets the dock state.
  void SetDockState(AstraDevToolsDockState state);

  // Returns the current dock state.
  AstraDevToolsDockState GetDockState() const;

  // -- Zoom ----------------------------------------------------------------

  // Returns the current zoom level (1.0 = 100%).
  double GetZoomLevel() const;

  // Sets the zoom level.
  void SetZoomLevel(double level);

  // -- Inspection & emulation ----------------------------------------------

  // Toggles element inspection mode.
  void InspectElement();

  // Toggles device emulation mode.
  void ToggleDeviceMode();

  // -- Legacy API (kept for backward compatibility) ------------------------

  AstraDevToolsModel* model() { return model_.get(); }
  const AstraDevToolsModel* model() const { return model_.get(); }

  void SetInspectedWebContents(content::WebContents* web_contents);
  content::WebContents* inspected_contents() const {
    return inspected_contents_;
  }

  views::View* container_view();
  AstraDevToolsToolbar* toolbar();
  AstraDevToolsWorkspacePanel* workspace_panel();

  bool SwitchToPanel(const std::string& panel_id);
  std::string active_panel_id() const;

  void SetSettingsDrawerOpen(bool open);
  bool IsSettingsDrawerOpen() const { return settings_drawer_open_; }
  void ToggleSettingsDrawer();

  void ApplyTheme();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Deepened observer management.
  void AddIntegrationObserver(AstraDevToolsIntegrationObserver* observer);
  void RemoveIntegrationObserver(AstraDevToolsIntegrationObserver* observer);

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
  void OnCloseClicked() override;
  void OnAstraTabClicked() override;
  void OnDockClicked() override;

  // -- AstraDevToolsWorkspacePanel::Delegate -----------------------------

  void OnNewWorkspace() override;
  void OnDeleteWorkspace(const std::string& workspace_id) override;
  void OnRenameWorkspace(const std::string& workspace_id,
                         const std::string& new_name) override;
  void OnWorkspaceSelected(const std::string& workspace_id) override;
  void OnTabSelected(int tab_index) override;
  void OnWorkspaceColorChanged(const std::string& workspace_id,
                               SkColor color) override;

  // -- AstraDevToolsModelObserver (legacy) --------------------------------

  void OnActivePanelChanged(const std::string& panel_id) override;
  void OnPanelOpened(const std::string& panel_id) override;
  void OnPanelClosed(const std::string& panel_id) override;
  void OnPanelOrderChanged() override;
  void OnDevToolsSettingsChanged() override;
  void OnDockPositionChanged(AstraDevToolsDockPosition position) override;
  void OnThemeChanged(AstraDevToolsTheme theme) override;

  // -- AstraDevToolsObserver (deepened) ------------------------------------

  void OnDevToolsOpened(AstraDevToolsModel* model) override;
  void OnDevToolsClosed(AstraDevToolsModel* model) override;
  void OnPanelActivated(AstraDevToolsModel* model,
                        const std::string& panel_id) override;
  void OnPanelEnabledChanged(AstraDevToolsModel* model,
                             const std::string& panel_id,
                             bool enabled) override;
  void OnPanelVisibilityChanged(AstraDevToolsModel* model,
                                const std::string& panel_id,
                                bool visible) override;
  void OnPanelsReordered(AstraDevToolsModel* model) override;
  void OnDockStateChanged(AstraDevToolsModel* model,
                          AstraDevToolsDockState state) override;
  void OnDevToolsModelShutdown(AstraDevToolsModel* model) override;

  // -- Manual install helpers (for testing / overlay) --------------------

  void InstallForTesting();
  void BuildContainerView();

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

  // Notifies integration observers of panel shown event.
  void NotifyPanelShown(AstraDevToolsPanelType type);

  // Notifies integration observers of panel hidden event.
  void NotifyPanelHidden();

  // The profile associated with this DevTools instance.  Not owned.
  raw_ptr<Profile> profile_;

  // The pref service for persistence.  Not owned.
  raw_ptr<PrefService> pref_service_ = nullptr;

  // The currently inspected WebContents.  Not owned.
  raw_ptr<content::WebContents> inspected_contents_ = nullptr;

  // Astra services — not owned, looked up from profile.
  raw_ptr<AstraWorkspaceService> workspace_service_ = nullptr;

  // Whether element inspection mode is active.
  bool inspect_element_active_ = false;

  // Whether device mode is active.
  bool device_mode_active_ = false;

  // Whether the Astra panel is currently open/active.
  bool panel_open_ = false;

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

  // Legacy observers.
  base::ObserverList<Observer> observers_;

  // Deepened integration observers.
  base::ObserverList<AstraDevToolsIntegrationObserver>
      integration_observers_;

  // Sidebar tab buttons.  Owned by sidebar_view_'s children.
  std::vector<raw_ptr<views::LabelButton>> sidebar_tabs_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_INTEGRATION_H_
