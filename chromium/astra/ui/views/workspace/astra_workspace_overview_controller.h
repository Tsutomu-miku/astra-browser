#ifndef ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_OVERVIEW_CONTROLLER_H_
#define ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_OVERVIEW_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/views/workspace/astra_workspace_overview_view.h"

namespace views {
class Widget;
}

class BrowserView;
class Profile;

namespace astra {

class AstraWorkspaceImportExportDialog;
class AstraWorkspaceOverviewView;

// Simplified observer interface for the workspace overview.
//
// This is a lightweight observer with 6 core events, suitable for
// components that just need to know when the overview is shown/hidden
// and when workspaces are added/removed/reordered.
//
// For the full observer interface with all presentation and action events,
// use AstraWorkspaceOverviewControllerObserver.
//
// All methods have default empty implementations.
class AstraWorkspaceOverviewObserver : public base::CheckedObserver {
 public:
  // Called when the overview is shown.
  virtual void OnOverviewShown() {}

  // Called when the overview is hidden.
  virtual void OnOverviewHidden() {}

  // Called when a new workspace is created through the overview.
  virtual void OnWorkspaceAdded(const std::string& workspace_id) {}

  // Called when a workspace is deleted through the overview.
  virtual void OnWorkspaceRemoved(const std::string& workspace_id) {}

  // Called when a workspace is renamed through the overview.
  virtual void OnWorkspaceRenamed(const std::string& workspace_id,
                                  const std::string& new_name) {}

  // Called when workspaces are reordered through the overview.
  virtual void OnWorkspacesReordered() {}

 protected:
  ~AstraWorkspaceOverviewObserver() override = default;
};

// Observer interface for the workspace overview controller.
//
// Other parts of the UI (e.g. sidebar, browser view) can observe the
// controller to know when the overview is shown or hidden, or when
// workspace actions are performed through the overview.
//
// All methods have default empty implementations so observers can override
// only the methods they care about.
//
// TODO(astra): Use this observer pattern to coordinate overview state
//   with other UI surfaces (sidebar workspace switcher, etc.).
class AstraWorkspaceOverviewControllerObserver
    : public base::CheckedObserver {
 public:
  // -- Overview lifecycle --------------------------------------------------

  // Called when the workspace overview is shown.
  virtual void OnOverviewShown() {}

  // Called when the workspace overview is hidden.
  virtual void OnOverviewHidden() {}

  // Called when the workspace overview is about to be shown.
  virtual void OnOverviewOpening() {}

  // Called when the workspace overview is about to be hidden.
  virtual void OnOverviewClosing() {}

  // -- Workspace actions ---------------------------------------------------

  // Called when a workspace is activated through the overview.
  virtual void OnWorkspaceActivatedFromOverview(
      const std::string& workspace_id) {}

  // Called when a workspace is selected (highlighted) in the overview.
  virtual void OnWorkspaceSelectedInOverview(
      const std::string& workspace_id) {}

  // Called when a workspace is created from the overview.
  virtual void OnWorkspaceCreatedFromOverview(
      const std::string& workspace_id) {}

  // Called when a workspace is deleted from the overview.
  virtual void OnWorkspaceDeletedFromOverview(
      const std::string& workspace_id) {}

  // Called when a workspace is renamed from the overview.
  virtual void OnWorkspaceRenamedFromOverview(
      const std::string& workspace_id,
      const std::string& new_name) {}

  // Called when workspaces are reordered via the overview.
  virtual void OnWorkspacesReorderedFromOverview() {}

  // -- Presentation / settings --------------------------------------------

  // Called when the overview view mode changes (grid <-> list).
  virtual void OnOverviewViewModeChanged(
      AstraWorkspaceOverviewViewMode mode) {}

  // Called when the overview card size changes.
  virtual void OnOverviewCardSizeChanged(
      AstraWorkspaceOverviewCardSize size) {}

  // Called when "show statistics" toggles.
  virtual void OnOverviewShowStatisticsChanged(bool show) {}

  // -- Bulk operations ----------------------------------------------------

  // Called when all non-default workspaces are hibernated.
  virtual void OnAllWorkspacesHibernated() {}

  // Called when all non-default workspaces are deleted.
  virtual void OnAllNonDefaultWorkspacesDeleted() {}

 protected:
  ~AstraWorkspaceOverviewControllerObserver() override = default;
};

// Controller for the workspace overview overlay.
//
// This is the "brain" of the workspace overview feature.  It:
//   - Manages the overview widget and view.
//   - Reads data from AstraWorkspaceService and TabStripModel.
//   - Implements AstraWorkspaceServiceObserver to react to workspace changes.
//   - Implements AstraWorkspaceOverviewViewObserver to react to user actions.
//   - Dispatches user actions (switch workspace, new workspace, etc.) to the
//     appropriate Chromium / Astra service.
//
// Ownership:
//   - Owned by AstraBrowserView (as a unique_ptr / member).
//   - Owns the overview Widget (indirectly, through the Widget's own
//     lifecycle).  The Widget is shown/hidden but the controller manages
//     when it's created and destroyed.
//
// Truth sources:
//   - Workspace metadata: AstraWorkspaceService (ProfileKeyedService).
//   - Tab data: TabStripModel (Chromium).
//   - Tab thumbnails: Chromium thumbnail subsystem (future).
//
// The overview view is strictly a presentation layer — all state changes
// flow through the controller to the services.
//
// TODO(astra): Tab thumbnails need to come from Chromium's thumbnail system.
//   Chromium subsystem: chrome/browser/ui/thumbnails/ or TabThumbnailTracker.
//   Or use content::WebContents::GetCaptureUpdater().
//   Patch point needed: thumbnail capture for Astra workspace overview.
class AstraWorkspaceOverviewController
    : public AstraWorkspaceServiceObserver,
      public AstraWorkspaceOverviewViewObserver {
 public:
  explicit AstraWorkspaceOverviewController(BrowserView* browser_view);
  AstraWorkspaceOverviewController(
      const AstraWorkspaceOverviewController&) = delete;
  AstraWorkspaceOverviewController& operator=(
      const AstraWorkspaceOverviewController&) = delete;
  ~AstraWorkspaceOverviewController() override;

  // -- Public API ---------------------------------------------------------

  // Show the workspace overview overlay.
  // Creates the widget if it doesn't exist yet.
  void Show();

  // Alias for Show() — explicit "ShowOverview" naming.
  void ShowOverview();

  // Hide the workspace overview overlay.
  // Closes the widget (it can be shown again later).
  void Hide();

  // Alias for Hide() — explicit "HideOverview" naming.
  void HideOverview();

  // Toggle the overview (show if hidden, hide if visible).
  void Toggle();

  // Returns true if the overview is currently visible.
  bool IsVisible() const;

  // Alias for IsVisible() — explicit "IsOverviewVisible" naming.
  bool IsOverviewVisible() const;

  // Returns the overview view, or nullptr if not created yet.
  AstraWorkspaceOverviewView* GetView() const;

  // Refresh content from models (workspace service + tab strip).
  // Called after workspace or tab changes.
  void Update();

  // -- Workspace selection ------------------------------------------------

  // Selects (highlights) the workspace with |workspace_id| in the overview.
  // Does not activate the workspace — just changes the selection highlight.
  void SelectWorkspace(const std::string& workspace_id);

  // Returns the ID of the currently selected workspace, or empty string
  // if no workspace is selected.
  std::string GetSelectedWorkspace() const;

  // -- Workspace operations -----------------------------------------------

  // Creates a new workspace from the overview.
  // Returns the ID of the new workspace, or empty string on failure.
  std::string NewWorkspace();

  // Deletes the currently selected workspace.
  // Returns true if a workspace was deleted.
  bool DeleteSelectedWorkspace();

  // Renames the currently selected workspace.
  // Returns true if the rename succeeded.
  bool RenameSelectedWorkspace(const std::string& new_name);

  // Moves (reorders) the workspace at |from_index| to |to_index|.
  // Returns true if the reorder was successful.
  bool MoveWorkspace(size_t from_index, size_t to_index);

  // Imports workspaces from a JSON string.
  // Returns the number of workspaces imported.
  size_t ImportWorkspaces(const std::string& json_data);

  // Exports all workspaces as a JSON string.
  std::string ExportWorkspace() const;

  // -- Workspace query ----------------------------------------------------

  // Returns the total number of workspaces.
  size_t GetWorkspaceCount() const;

  // Returns the workspace at the given |index|, or nullptr if out of range.
  const AstraWorkspace* GetWorkspaceAt(size_t index) const;

  // Returns all workspaces as a vector.
  std::vector<AstraWorkspace> GetWorkspaces() const;

  // -- Overview observer management (simplified observer) -----------------

  void AddOverviewObserver(AstraWorkspaceOverviewObserver* observer);
  void RemoveOverviewObserver(AstraWorkspaceOverviewObserver* observer);

  // -- Presentation settings ----------------------------------------------

  // Sets the view mode (grid or list) and persists the setting.
  void SetViewMode(AstraWorkspaceOverviewViewMode mode);

  // Returns the current view mode.
  AstraWorkspaceOverviewViewMode view_mode() const { return view_mode_; }

  // Sets the card size and persists the setting.
  void SetCardSize(AstraWorkspaceOverviewCardSize size);

  // Returns the current card size.
  AstraWorkspaceOverviewCardSize card_size() const { return card_size_; }

  // Sets whether to show statistics and persists the setting.
  void SetShowStatistics(bool show);

  // Returns whether statistics are shown.
  bool show_statistics() const { return show_statistics_; }

  // -- Bulk operations ---------------------------------------------------

  // Hibernates all non-default, non-active workspaces.
  void HibernateAllWorkspaces();

  // Deletes all non-default workspaces (with confirmation).
  void DeleteAllNonDefaultWorkspaces();

  // -- Observer management ------------------------------------------------

  void AddObserver(AstraWorkspaceOverviewControllerObserver* observer);
  void RemoveObserver(AstraWorkspaceOverviewControllerObserver* observer);

  // -- AstraWorkspaceServiceObserver -------------------------------------

  void OnWorkspaceAdded(const AstraWorkspace& workspace) override;
  void OnWorkspaceRemoved(const std::string& workspace_id) override;
  void OnWorkspaceRenamed(const std::string& workspace_id,
                          const std::string& new_name) override;
  void OnActiveWorkspaceChanged(const std::string& old_id,
                                const std::string& new_id) override;
  void OnWorkspacesReordered() override;

  // -- AstraWorkspaceOverviewViewObserver --------------------------------

  void OnWorkspaceClicked(const std::string& workspace_id) override;
  void OnWorkspaceRenameRequested(const std::string& workspace_id) override;
  void OnWorkspaceMenuRequested(const std::string& workspace_id,
                                const gfx::Point& screen_point) override;
  void OnWorkspaceDeleteRequested(const std::string& workspace_id) override;
  void OnNewWorkspaceRequested() override;
  void OnExportRequested() override;
  void OnImportRequested() override;
  void OnSearchQueryChanged(const std::u16string& query) override;
  void OnOverviewClosing() override;
  void OnWorkspaceSelected(const std::string& workspace_id) override;
  void OnWorkspaceActivated(const std::string& workspace_id) override;
  void OnWorkspacesReordered(
      const std::vector<std::string>& ordered_ids) override;
  void OnViewModeChanged(AstraWorkspaceOverviewViewMode mode) override;
  void OnCardSizeChanged(AstraWorkspaceOverviewCardSize size) override;
  void OnShowStatisticsChanged(bool show) override;
  void OnHibernateAllRequested() override;
  void OnDeleteAllNonDefaultRequested() override;
  void OnOverviewSettingsRequested() override;

 private:
  // Creates the overview widget and view.
  void CreateWidget();

  // Closes and destroys the overview widget.
  void DestroyWidget();

  // Animates the widget in (fade + slide up).
  void AnimateShow();

  // Animates the widget out (fade + slide down), then hides it.
  void AnimateHide();

  // Called when the show animation completes.
  void OnShowAnimationComplete();

  // Called when the hide animation completes.
  void OnHideAnimationComplete();

  // Computes tab counts for each workspace by iterating TabStripModel.
  // Returns a vector parallel to workspaces() with the tab count for each.
  //
  // Chromium subsystem: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  // This is a presentation-layer computation — we count how many tabs in
  // the strip have each workspace_id set in their AstraTabFeatures.
  std::vector<int> ComputeTabCounts() const;

  // Computes window counts for each workspace.
  // Uses AstraWorkspaceService::GetWindowCount().
  std::vector<int> ComputeWindowCounts() const;

  // Loads presentation settings from PrefService.
  void LoadPresentationSettings();

  // Saves presentation settings to PrefService.
  void SavePresentationSettings();

  // -- Action handlers (called from view callbacks) -----------------------

  // Switch to the given workspace and close the overview.
  void ActivateWorkspace(const std::string& workspace_id);

  // Create a new workspace and switch to it.
  void CreateNewWorkspace();

  // Rename a workspace.
  void RenameWorkspace(const std::string& workspace_id,
                       const std::string& new_name);

  // Delete a workspace (with confirmation).
  void DeleteWorkspace(const std::string& workspace_id);

  // Reorder workspaces (drag & drop).
  // TODO(astra): Implement reordering via drag and drop.
  //   Chromium pattern: views::View drag source + drop target.
  void ReorderWorkspaces(const std::vector<std::string>& ordered_ids);

  // Show the import dialog.
  void ShowImportDialog();

  // Show the export dialog.
  void ShowExportDialog();

  // Show the context menu for a workspace card.
  void ShowWorkspaceMenu(const std::string& workspace_id,
                         const gfx::Point& screen_point);

  // Get the profile from the browser view.
  Profile* GetProfile() const;

  // The BrowserView this controller is associated with.
  raw_ptr<BrowserView> browser_view_;

  // Workspace service (obtained from the profile).
  raw_ptr<AstraWorkspaceService> workspace_service_ = nullptr;

  // Observation of the workspace service.
  base::ScopedObservation<AstraWorkspaceService,
                          AstraWorkspaceServiceObserver>
      workspace_service_observation_{this};

  // Observers of this controller (full observer interface).
  base::ObserverList<AstraWorkspaceOverviewControllerObserver> observers_;

  // Simplified overview observers (lightweight 6-event interface).
  base::ObserverList<AstraWorkspaceOverviewObserver> overview_observers_;

  // The overview widget (owned by itself / the Widget system).
  // We hold a raw_ptr and manage its lifecycle.
  raw_ptr<views::Widget> widget_ = nullptr;

  // The overview view (owned by the widget's view hierarchy).
  raw_ptr<AstraWorkspaceOverviewView> overview_view_ = nullptr;

  // The import/export dialog, if currently shown.
  raw_ptr<AstraWorkspaceImportExportDialog> import_export_dialog_ = nullptr;

  // Whether the overview is currently visible.
  bool is_visible_ = false;

  // Whether an animation is currently in progress.
  bool is_animating_ = false;

  // Presentation settings (persisted via PrefService).
  AstraWorkspaceOverviewViewMode view_mode_ =
      AstraWorkspaceOverviewViewMode::kGrid;
  AstraWorkspaceOverviewCardSize card_size_ =
      AstraWorkspaceOverviewCardSize::kMedium;
  bool show_statistics_ = true;

  // Weak pointer factory for animation callbacks.
  base::WeakPtrFactory<AstraWorkspaceOverviewController> weak_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_OVERVIEW_CONTROLLER_H_
