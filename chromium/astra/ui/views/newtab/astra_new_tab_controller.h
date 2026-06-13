#ifndef ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_CONTROLLER_H_
#define ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "url/gurl.h"

#include "astra/ui/views/newtab/astra_new_tab_model.h"
#include "astra/ui/views/newtab/astra_new_tab_view.h"

class Browser;
class Profile;
class PrefService;

namespace astra {

class AstraNewTabPageService;
class AstraWorkspaceService;

// =========================================================================
// AstraNewTabController — bridges NTP model and views
// =========================================================================
//
// The controller sits between the model (AstraNewTabModel) and the views
// (AstraNewTabView, shortcut views, workspace card views).  It:
//   - Implements AstraNewTabView::Delegate to receive user actions from views.
//   - Loads data from services and populates the model.
//   - Updates model state in response to user actions.
//   - Persists model changes to PrefService.
//   - Observes the model and updates views when model state changes.
//   - Delegates outward actions (navigation, workspace switching) to its
//     own Delegate interface.
//
// This follows the classic MVC pattern:
//   - Model: AstraNewTabModel (owns state, notifies observers)
//   - View: AstraNewTabView + child views (renders state, sends user actions)
//   - Controller: AstraNewTabController (mediates between model and views)
//
// Chromium subsystems reused:
//   - PrefService (persistence)
//   - Profile (access to services)
//   - Browser (navigation, tab management)
//
// Astra services used:
//   - AstraNewTabPageService (top sites, custom shortcuts)
//   - AstraWorkspaceService (workspace metadata)
//
// TODO(astra): Wire up real service observation.  Currently the controller
//   pulls data once at construction.  In a full Chromium build, it should
//   observe services and update the model when underlying data changes.
// =========================================================================

class AstraNewTabController : public AstraNewTabModelObserver,
                              public AstraNewTabView::Delegate {
 public:
  // Delegate interface for actions that need to be handled outside
  // the controller (e.g., navigation, opening browser windows).
  class Delegate {
   public:
    // Called to navigate to a URL in the active tab.
    virtual void OnNavigateToURL(const GURL& url) = 0;

    // Called to open / switch to a workspace.
    virtual void OnOpenWorkspace(const std::string& workspace_id) = 0;

    // Called to create a new workspace.
    virtual void OnNewWorkspace() = 0;

    // Called to show the full workspace overview.
    virtual void OnShowAllWorkspaces() = 0;

    // Called to trigger a quick action.
    virtual void OnQuickAction(const std::string& action_id) = 0;

    // Called to restore a recently closed tab.
    virtual void OnRestoreRecentlyClosed(int session_id) = 0;

    // Called to show the shortcut context menu.
    virtual void OnShowShortcutContextMenu(const GURL& url,
                                           const gfx::Point& screen_point) = 0;

    // Called to show the workspace context menu.
    virtual void OnShowWorkspaceContextMenu(
        const std::string& workspace_id,
        const gfx::Point& screen_point) = 0;

    // Called when the settings gear button is pressed (to show customize menu).
    virtual void OnSettingsGearPressed() = 0;

   protected:
    ~Delegate() = default;
  };

  AstraNewTabController(Browser* browser,
                        AstraNewTabView* view,
                        Delegate* delegate);
  ~AstraNewTabController() override;

  AstraNewTabController(const AstraNewTabController&) = delete;
  AstraNewTabController& operator=(const AstraNewTabController&) = delete;

  // -- Data loading --------------------------------------------------------

  // Reloads all data from services and refreshes the model.
  void RefreshFromServices();

  // -- Model access --------------------------------------------------------

  AstraNewTabModel* model() { return &model_; }
  const AstraNewTabModel* model() const { return &model_; }

  // -- User action handlers (called by views) ------------------------------

  // Shortcut actions.
  void OnShortcutClicked(const GURL& url);
  void OnShortcutRemove(const GURL& url);
  void OnShortcutEdit(const GURL& url,
                      const std::u16string& new_title,
                      const GURL& new_url);
  void OnShortcutReordered(size_t from_index, size_t to_index) override;
  void OnShortcutAdd(const std::u16string& title, const GURL& url);

  // Workspace actions.
  void OnWorkspaceClicked(const std::string& workspace_id);
  void OnWorkspaceMenu(const std::string& workspace_id,
                       const gfx::Point& screen_point);

  // Quick actions.
  void OnQuickAction(const std::string& action_id) override;

  // Recently closed.
  void OnRecentlyClosedClicked(int session_id);

  // Settings actions.
  void ToggleGreeting();
  void ToggleSearchBar();
  void ToggleWorkspaceCards();
  void ToggleShortcuts();
  void ToggleRecentlyClosed();
  void ToggleQuickActions();
  void ToggleMostVisited();
  void ToggleSuggestedContent();
  void ToggleShortcutTitles();
  void ToggleSeconds();
  void ToggleDate();
  void ToggleSearchEngine();

  void SetShortcutColumns(int columns);
  void SetMaxWorkspacesShown(int max);
  void SetMaxRecentlyClosedShown(int max);

  void SetShortcutLayoutMode(AstraNtpShortcutLayoutMode mode);
  void SetWorkspaceCardStyle(AstraNtpWorkspaceCardStyle style);
  void SetBackgroundStyle(AstraNtpBackgroundStyle style);
  void SetCustomBackgroundUrl(const std::string& url);
  void SetGreetingStyle(AstraNtpGreetingStyle style);
  void SetThemeMode(AstraNtpThemeMode mode);
  void SetLayoutDensity(AstraNtpLayoutDensity density);
  void SetShortcutIconSize(AstraNtpShortcutIconSize size);
  void SetClockFormat(AstraNtpClockFormat format);
  void SetSearchBarStyle(AstraNtpSearchBarStyle style);
  void SetAccentColor(SkColor color);
  void SetGradientSettings(const AstraNtpGradientSettings& settings);
  void SetGreetingName(const std::u16string& name);
  void SetSearchEngineName(const std::string& name);

  // Import/export.
  base::Value::Dict ExportSettings() const;
  bool ImportSettings(const base::Value::Dict& settings);
  void ResetSettingsToDefaults();
  void ResetAllToDefaults();

  // Focus management.
  void FocusSearchBar();
  void FocusFirstShortcut();
  void FocusNextSection();
  void FocusPreviousSection();

  // Animation control.
  void PlayEntranceAnimations();
  void SkipAnimationsForTesting();

  // -- AstraNewTabView::Delegate ------------------------------------------
  // (View delegate interface — implemented by controller to receive
  //  user actions from the NTP view. Some methods share signatures with
  //  the public action handlers above and serve double duty.)

  void OnNavigateToURL(const GURL& url) override;
  void OnOpenWorkspace(const std::string& workspace_id) override;
  void OnNewWorkspace() override;
  void OnShowAllWorkspaces() override;
  void OnRestoreRecentlyClosed(int session_id) override;
  void OnRemoveShortcut(const GURL& url) override;
  void OnShowShortcutContextMenu(const GURL& url,
                                 const gfx::Point& screen_point) override;
  void OnShowWorkspaceContextMenu(
      const std::string& workspace_id,
      const gfx::Point& screen_point) override;
  void OnSettingsGearPressed() override;
  void OnToggleGreeting() override;
  void OnToggleShortcuts() override;
  void OnShortcutColumnsChanged(int columns) override;
  void OnShortcutLayoutModeChanged(int mode) override;
  void OnBackgroundStyleChanged(int style) override;

  // -- AstraNewTabModelObserver -------------------------------------------

  void OnShortcutsChanged() override;
  void OnWorkspacesChanged() override;
  void OnQuickActionsChanged() override;
  void OnRecentlyClosedChanged() override;
  void OnSuggestedContentChanged() override;
  void OnNtpSettingsChanged() override;
  void OnThemeChanged() override;
  void OnLayoutDensityChanged() override;
  void OnAccentColorChanged() override;
  void OnClockFormatChanged() override;
  void OnSearchBarStyleChanged() override;
  void OnGreetingNameChanged() override;
  void OnSuggestedContentSettingsChanged() override;

 private:
  // Load shortcuts from services into the model.
  void LoadShortcutsFromService();

  // Load workspace cards from services into the model.
  void LoadWorkspacesFromService();

  // Load quick actions into the model.
  void LoadQuickActions();

  // Load recently closed tabs into the model.
  void LoadRecentlyClosedFromService();

  // Save current model state to prefs.
  void SaveToPrefs();

  // Update the view from the current model state.
  void UpdateViewFromModel();

  // Update only the shortcut section of the view.
  void UpdateViewShortcuts();

  // Update only the workspace section of the view.
  void UpdateViewWorkspaces();

  // Update only the quick actions section.
  void UpdateViewQuickActions();

  // Update only the recently closed section.
  void UpdateViewRecentlyClosed();

  // Update only the suggested content section.
  void UpdateViewSuggestedContent();

  // Raw pointers to services (not owned).
  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<AstraNewTabPageService> ntp_service_ = nullptr;
  raw_ptr<AstraWorkspaceService> workspace_service_ = nullptr;

  // The model (owned by the controller).
  AstraNewTabModel model_;

  // The view (not owned — controller mediates model to view).
  raw_ptr<AstraNewTabView> view_ = nullptr;

  // The delegate (not owned — handles actions outside the controller).
  raw_ptr<Delegate> delegate_ = nullptr;

  // Animation state.
  bool animations_skipped_ = false;
  bool entrance_animations_played_ = false;

  // Current focus section index (for keyboard navigation between sections).
  int current_focus_section_ = 0;

  base::WeakPtrFactory<AstraNewTabController> weak_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_CONTROLLER_H_
