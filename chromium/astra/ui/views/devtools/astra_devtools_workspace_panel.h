#ifndef ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_WORKSPACE_PANEL_H_
#define ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_WORKSPACE_PANEL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/view.h"
#include "astra/common/astra_workspace_types.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class Label;
class BoxLayout;
class ScrollView;
class Textfield;
class LabelButton;
}  // namespace views

namespace astra {

class AstraWorkspaceService;
class AstraDevToolsModel;

// =========================================================================
// AstraDevToolsWorkspacePanel — Workspace inspector DevTools panel
// =========================================================================
//
// A native Views panel that displays Astra workspace and tab metadata for
// debugging purposes.  It shows the current workspace, workspace list, and
// per-tab Astra features for the inspected WebContents.
//
// This is a DEBUG-ONLY / developer tool panel.  It is the Astra equivalent
// of the "Application" or "Storage" panel in DevTools, but focused on
// Astra product metadata.
//
// Displayed data (deepened):
//   - Workspace list with cards (name, color, tab count, window count)
//   - Search box for filtering workspaces
//   - New workspace button
//   - Workspace stats (total workspaces, total tabs)
//   - Quick actions (merge, import, export)
//   - Current workspace: name, ID, accent color, creation time
//   - Active tab Astra metadata
//
// Features (deepened):
//   - Workspace list with selection
//   - New / delete / rename workspace
//   - Set workspace color
//   - Tab count and window count per workspace
//   - Search/filter for workspaces
//   - Model-driven data (SetWorkspaces)
//
// Architecture:
//   - Pure projection — all data is read from the model or services at render time.
//   - No state storage — the panel does not cache or own data.
//   - Read-only by default — editing dispatches through the delegate.
//
// Truth sources:
//   - AstraDevToolsModel (for panel state and configuration)
//   - AstraWorkspaceService (profile-scoped keyed service)
//   - AstraTabFeatures (WebContentsUserData on the inspected tab)
//   - Chromium's TabStripModel (for tab counts per workspace)
//
// Chromium subsystems reused:
//   - DevToolsPanel / DevToolsUIBindings — panel registration and hosting.
//   - views::Label, views::BoxLayout — text display and layout.
//   - views::ScrollView — scrollable list containers.
//   - views::Textfield — search/filter input.
//   - views::LabelButton — action buttons.
//   - SkColor — accent color rendering.
//
// Chromium patch point:
//   TODO(astra): Register this panel as a custom DevTools panel.
//   Chromium owner: DevToolsWindow / DevToolsUIBindings
//     (chrome/browser/devtools/devtools_window.h)
//     (chrome/browser/devtools/devtools_ui_bindings.h)
// =========================================================================

class AstraDevToolsWorkspacePanel : public views::View {
 public:
  // Delegate for panel actions that go back to the integration layer.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the user requests a new workspace.
    virtual void OnNewWorkspace() = 0;

    // Called when the user requests deletion of a workspace.
    virtual void OnDeleteWorkspace(const std::string& workspace_id) = 0;

    // Called when the user requests renaming a workspace.
    virtual void OnRenameWorkspace(const std::string& workspace_id,
                                   const std::string& new_name) = 0;

    // Called when a workspace is selected in the list.
    virtual void OnWorkspaceSelected(const std::string& workspace_id) = 0;

    // Called when a tab is selected in the tab list.
    virtual void OnTabSelected(int tab_index) = 0;

    // Called when workspace color is changed.
    virtual void OnWorkspaceColorChanged(const std::string& workspace_id,
                                         SkColor color) = 0;
  };

  AstraDevToolsWorkspacePanel();
  ~AstraDevToolsWorkspacePanel() override;

  AstraDevToolsWorkspacePanel(const AstraDevToolsWorkspacePanel&) = delete;
  AstraDevToolsWorkspacePanel& operator=(
      const AstraDevToolsWorkspacePanel&) = delete;

  // -- Delegate ------------------------------------------------------------

  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

  // -- Model integration ---------------------------------------------------

  // Sets the DevTools model.  The model must outlive this panel.
  void SetModel(AstraDevToolsModel* model);

  // Returns the current model, or null if none is set.
  AstraDevToolsModel* GetModel() const { return model_; }

  // -- Workspace list management (model-driven) ----------------------------

  // Sets the list of workspaces to display.
  void SetWorkspaces(const std::vector<AstraWorkspaceInfo>& workspaces);

  // Returns the number of workspaces currently displayed.
  size_t GetWorkspaceCount() const { return workspaces_.size(); }

  // Returns the workspace info at the given index, or null if out of bounds.
  const AstraWorkspaceInfo* GetWorkspaceAt(int index) const;

  // -- Selection -----------------------------------------------------------

  // Selects the workspace at the given index.
  void SelectWorkspace(int index);

  // Returns the index of the currently selected workspace, or -1 if none.
  int GetSelectedIndex() const { return selected_index_; }

  // -- Workspace operations ------------------------------------------------

  // Creates a new workspace with a default name.
  void NewWorkspace();

  // Deletes the workspace at the given index.
  void DeleteWorkspace(int index);

  // Renames the workspace at the given index.
  void RenameWorkspace(int index, const std::u16string& new_name);

  // Sets the accent color of the workspace at the given index.
  void SetWorkspaceColor(int index, SkColor color);

  // -- Workspace stats -----------------------------------------------------

  // Returns the total number of tabs in the workspace at the given index.
  int GetTabCountForWorkspace(int index) const;

  // Returns the number of windows in the workspace at the given index.
  int GetWindowCountForWorkspace(int index) const;

  // -- New workspace button visibility -------------------------------------

  // Shows or hides the new workspace button.
  void ShowNewWorkspaceButton(bool show);

  // Returns whether the new workspace button is visible.
  bool IsNewWorkspaceButtonVisible() const;

  // -- Search --------------------------------------------------------------

  // Sets the search query text.
  void SetSearchQuery(const std::u16string& query);

  // Returns the current search query.
  std::u16string GetSearchQuery() const { return search_query_; }

  // Shows or hides the search box.
  void ShowSearch(bool show);

  // Returns whether the search box is visible.
  bool IsSearchVisible() const;

  // -- Inspected WebContents -----------------------------------------------

  // Sets the inspected WebContents.  The panel reads Astra tab metadata
  // from this WebContents' AstraTabFeatures user data.
  // Passing nullptr clears the tab info section.
  void SetInspectedWebContents(content::WebContents* web_contents);

  // -- Workspace service ---------------------------------------------------

  // Sets the workspace service to read workspace data from.
  // The service must outlive this panel.
  void SetWorkspaceService(AstraWorkspaceService* service);

  // -- Refresh -------------------------------------------------------------

  // Refreshes all displayed data from services.  Call this when underlying
  // data may have changed.
  void Refresh();

  // -- Search filter (legacy) ----------------------------------------------

  // Sets search filter text.  Filters workspace and tab lists.
  void SetSearchFilter(const std::u16string& filter);

  // -- Theme ---------------------------------------------------------------

  // Applies dark or light theme.
  void SetTheme(bool dark_theme);

  // -- Accessors for testing -----------------------------------------------

  views::Label* workspace_name_label_for_testing() {
    return workspace_name_label_;
  }
  views::Label* tab_metadata_label_for_testing() {
    return tab_metadata_label_;
  }
  views::View* workspace_list_container_for_testing() {
    return workspace_list_container_;
  }
  views::View* tab_list_container_for_testing() {
    return tab_list_container_;
  }
  views::Textfield* search_box_for_testing() { return search_box_; }
  views::LabelButton* new_workspace_button_for_testing() {
    return new_workspace_button_;
  }
  views::LabelButton* delete_workspace_button_for_testing() {
    return delete_workspace_button_;
  }
  views::LabelButton* rename_workspace_button_for_testing() {
    return rename_workspace_button_;
  }
  views::Label* stats_label_for_testing() { return stats_label_; }
  views::View* quick_actions_container_for_testing() {
    return quick_actions_container_;
  }
  size_t workspace_item_count_for_testing() const;
  size_t tab_item_count_for_testing() const;
  std::string selected_workspace_id_for_testing() const;
  int selected_index_for_testing() const { return selected_index_; }

 private:
  // Builds the panel's UI structure.
  void BuildPanel();

  // Creates a section header label.
  views::Label* AddSectionLabel(const std::u16string& text);

  // Creates a key-value row (key label + value label) in a container.
  void AddKeyValueRow(views::View* container,
                      const std::string& key,
                      const std::string& value);

  // Creates a workspace card item in the list.
  void AddWorkspaceCard(const AstraWorkspaceInfo& info, int index);

  // Refreshes the workspace info section from AstraWorkspaceService.
  void RefreshWorkspaceInfo();

  // Refreshes the workspace list section from workspaces_ data.
  void RefreshWorkspaceList();

  // Refreshes the tab list for the selected workspace.
  void RefreshTabList();

  // Refreshes the tab metadata section from AstraTabFeatures.
  void RefreshTabMetadata();

  // Refreshes the workspace stats display.
  void RefreshStats();

  // Applies current theme to all UI elements.
  void ApplyTheme();

  // Handles workspace list item click.
  void OnWorkspaceItemClicked(const std::string& workspace_id);

  // Button action handlers.
  void OnNewWorkspaceButton();
  void OnDeleteWorkspaceButton();
  void OnRenameWorkspaceButton();
  void OnMergeWorkspacesButton();
  void OnImportWorkspacesButton();
  void OnExportWorkspacesButton();

  // Helper: find workspace index by ID.  Returns -1 if not found.
  int FindWorkspaceIndexById(const std::string& id) const;

  // Helper: get filtered workspace indices based on search query.
  std::vector<int> GetFilteredWorkspaceIndices() const;

  // Delegate for actions.  Not owned.
  raw_ptr<Delegate> delegate_ = nullptr;

  // The DevTools model — not owned.
  raw_ptr<AstraDevToolsModel> model_ = nullptr;

  // The workspace service — not owned.
  raw_ptr<AstraWorkspaceService> workspace_service_ = nullptr;

  // The inspected WebContents — not owned.
  raw_ptr<content::WebContents> inspected_contents_ = nullptr;

  // Workspace data (model-driven).
  std::vector<AstraWorkspaceInfo> workspaces_;

  // Index of the currently selected workspace, or -1 if none.
  int selected_index_ = -1;

  // Current search query.
  std::u16string search_query_;

  // Current search filter text (legacy, kept for compatibility).
  std::u16string search_filter_;

  // Currently selected workspace ID (legacy, kept for compatibility).
  std::string selected_workspace_id_;

  // Whether dark theme is active.
  bool dark_theme_ = true;

  // Whether the search box is visible.
  bool search_visible_ = true;

  // Whether the new workspace button is visible.
  bool new_workspace_button_visible_ = true;

  // UI elements — owned by the views hierarchy.

  // Search box.
  raw_ptr<views::Textfield> search_box_ = nullptr;

  // Stats label showing workspace count and total tabs.
  raw_ptr<views::Label> stats_label_ = nullptr;

  // Quick actions container (merge, import, export).
  raw_ptr<views::View> quick_actions_container_ = nullptr;

  // Workspace info section.
  raw_ptr<views::Label> workspace_name_label_ = nullptr;
  raw_ptr<views::Label> workspace_id_label_ = nullptr;
  raw_ptr<views::Label> workspace_color_label_ = nullptr;

  // Workspace list section.
  raw_ptr<views::View> workspace_list_container_ = nullptr;
  raw_ptr<views::ScrollView> workspace_scroll_view_ = nullptr;
  raw_ptr<views::LabelButton> new_workspace_button_ = nullptr;
  raw_ptr<views::LabelButton> delete_workspace_button_ = nullptr;
  raw_ptr<views::LabelButton> rename_workspace_button_ = nullptr;

  // Tab list section.
  raw_ptr<views::View> tab_list_container_ = nullptr;
  raw_ptr<views::ScrollView> tab_scroll_view_ = nullptr;

  // Tab metadata display — a multi-line label showing all tab features.
  raw_ptr<views::Label> tab_metadata_label_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_WORKSPACE_PANEL_H_
