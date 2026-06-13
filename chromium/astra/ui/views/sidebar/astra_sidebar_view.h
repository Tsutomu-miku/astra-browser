#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_VIEW_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "astra/browser/astra_favorite_service.h"
#include "astra/browser/astra_memory_saver_service.h"
#include "astra/browser/astra_note_service.h"
#include "astra/browser/astra_reading_list_service.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/views/sidebar/astra_history_item_view.h"
#include "astra/ui/views/sidebar/astra_reading_list_item_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_controller.h"
#include "astra/ui/views/sidebar/astra_sidebar_downloads_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_drag_types.h"
#include "astra/ui/views/sidebar/astra_sidebar_extensions_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_history_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_model.h"
#include "astra/ui/views/sidebar/astra_sidebar_passwords_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_notes_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_recently_closed_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_reading_list_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_section_view.h"
#include "astra/ui/views/tab_hover/astra_tab_hover_peek_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/public/browser/download_manager.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/view.h"

class Browser;
class TabStripModel;

namespace astra {

class AstraSidebarDropIndicatorView;
class AstraSidebarBookmarksView;
class AstraSidebarTabGroupsView;
class AstraWorkspaceSwitcherView;

// The main sidebar widget embedded in Chrome's BrowserView.
//
// This is a presentation-only view: it projects Chromium state (TabStripModel,
// AstraWorkspaceService, AstraTabFeatures, HistoryService, DownloadManager)
// into a vertical sidebar with sections. It never mutates browser state
// directly — all user actions dispatch commands through the command delegate
// or Chromium APIs.
//
// Sections (top to bottom):
//   1. Workspace switcher (active workspace + chevron)
//   2. Favorites section
//   3. Pinned tabs section
//   4. Reading list section (read-later items from Chromium's ReadingListModel)
//   5. Tab groups section (collapsible tree of groups with nested tabs)
//   6. Bookmarks section (Chrome bookmarks tree from BookmarkModel)
//   7. Open tabs (in the current workspace)
//   8. History section (recently visited pages, from Chromium HistoryService)
//   9. Downloads section (active and recent downloads, from Chromium DownloadManager)
//  10. Extensions section (browser action icons, from Chromium ExtensionRegistry)
//
// Implements TabStripModelObserver to receive live tab change notifications
// directly from Chromium's TabStripModel. This is the primary update path —
// the sidebar updates reactively when tabs are added, removed, reordered,
// activated, or have their title/favicon changed.
//
// Implements AstraWorkspaceServiceObserver to receive live updates from the
// workspace service.
//
// Implements AstraSidebarItemDragDelegate and AstraSidebarSectionDropDelegate
// to manage drag-and-drop sessions within the sidebar. The sidebar is the
// drag controller: it creates drag data, validates drops against services,
// and dispatches mutations through services on drop.
//
// Implements AstraSidebarHistoryView::Delegate to handle navigation actions
// from the history section (opening URLs, opening full history page).
//
// TODO(astra): Implement incremental UI updates in each TabStripModelObserver
// method instead of calling UpdateFromModel() (full rebuild) on every change.
// Full rebuilds are correct but O(n) per change; incremental updates would be
// O(1) for most operations. The section view already supports InsertItemAt /
// RemoveItemAt for this purpose.
// Chromium subsystem: TabStripModelObserver (chrome/browser/ui/tabs/tab_strip_model_observer.h)
class AstraSidebarView final : public views::View,
                               public TabStripModelObserver,
                               public AstraWorkspaceServiceObserver,
                               public AstraFavoriteServiceObserver,
                               public AstraMemorySaverServiceObserver,
                               public AstraSidebarModelObserver,
                               public AstraSidebarItemDragDelegate,
                               public AstraSidebarItemHoverDelegate,
                               public AstraSidebarSectionDropDelegate,
                               public AstraSidebarHistoryView::Delegate,
                               public AstraSidebarRecentlyClosedView::Delegate,
                               public AstraSidebarPasswordsView::Delegate {
 public:
  static constexpr int kDefaultWidth = 280;

  explicit AstraSidebarView(Browser* browser);
  AstraSidebarView(const AstraSidebarView&) = delete;
  AstraSidebarView& operator=(const AstraSidebarView&) = delete;
  ~AstraSidebarView() override;

  // Refresh all sidebar sections from the underlying models.
  // Reads from TabStripModel, AstraWorkspaceService, and per-tab
  // AstraTabFeatures metadata.
  //
  // This is the full-rebuild fallback. The primary update path is through
  // TabStripModelObserver notifications which (will) do incremental updates.
  void UpdateFromModel();

  // Set the active workspace filter. Tabs not in this workspace are hidden
  // from the "Open Tabs" section.
  //
  // In regular profile mode, this dispatches through the workspace service
  // so the change is persisted and broadcast to all observers.
  // In incognito mode, this only updates the sidebar's local active workspace
  // state — it does not modify the shared service or persist anything.
  void SetActiveWorkspace(const std::string& workspace_id);

  // Navigate to the next (+1) or previous (-1) workspace relative to the
  // current active workspace.
  //
  // In incognito mode, this is the primary way to navigate workspaces since
  // workspace navigation commands are routed through the UI layer (see
  // AstraCommandDelegate::OnWorkspaceNavigateRequested).
  void NavigateWorkspace(int direction);

  const std::string& active_workspace_id() const { return active_workspace_id_; }

  // Returns true if this sidebar is attached to an incognito browser window.
  bool is_incognito() const { return is_incognito_; }

  // Toggle the extensions panel (show/hide the extensions section).
  // Dispatches a signal to collapse/expand the extensions section.
  //
  // Chromium owner: ExtensionsToolbarButton / ExtensionsToolbarContainer
  //   (chrome/browser/ui/views/toolbar/extensions_toolbar_button.h)
  //   (chrome/browser/ui/views/toolbar/extensions_toolbar_container.h)
  // Patch point: Could be triggered by the extensions toolbar button or
  //   a keyboard shortcut. The extensions section in the sidebar is an
  //   alternative to the toolbar extensions menu.
  void ToggleExtensionsPanel();

  // Toggle the passwords panel (show/hide the passwords section).
  // Dispatches a signal to collapse/expand the passwords section.
  //
  // Chromium owner: PasswordManagerUI / PasswordManagerService
  //   (chrome/browser/password_manager/password_manager_service.h)
  //   (chrome/browser/ui/webui/password_manager/password_manager_ui.cc)
  // Patch point: Could be triggered by the password toolbar button or
  //   a keyboard shortcut. The passwords section in the sidebar is an
  //   alternative to the full password manager page.
  void TogglePasswordsPanel();

  // Toggle the notes panel (show/hide the notes section).
  // Dispatches a signal to collapse/expand the notes section.
  //
  // Chromium owner: No direct equivalent — Astra-specific feature.
  // Patch point: Triggered by a toolbar button or keyboard shortcut.
  void ToggleNotesPanel();

  // -- Model / controller integration -------------------------------------

  // Get the controller associated with this sidebar view.
  // The controller is owned by the view and manages the model and
  // service interactions.
  AstraSidebarController* controller() { return controller_.get(); }
  const AstraSidebarController* controller() const { return controller_.get(); }

  // Get the model from the controller (convenience accessor).
  AstraSidebarModel* model() { return controller_ ? controller_->model() : nullptr; }
  const AstraSidebarModel* model() const {
    return controller_ ? controller_->model() : nullptr;
  }

  // -- AstraSidebarModelObserver ------------------------------------------

  void OnSidebarShown() override;
  void OnSidebarHidden() override;
  void OnSidebarPinnedChanged(bool pinned) override;
  void OnActiveSectionChanged(const std::string& section_id) override;
  void OnSectionVisibilityChanged(const std::string& section_id,
                                  bool visible) override;
  void OnSectionOrderChanged() override;
  void OnSectionCollapsedChanged(const std::string& section_id,
                                 bool collapsed) override;
  void OnSidebarWidthChanged(int width) override;
  void OnSidebarPositionChanged(AstraSidebarPosition position) override;
  void OnSidebarSettingsChanged() override;

  // -- Tab strip observation ---------------------------------------------

  // Start observing |tab_strip| for tab changes. Safe to call when already
  // observing (resets observation to the new model).
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  void StartObservingTabStrip(TabStripModel* tab_strip);

  // Stop observing the current tab strip model. No-op if not observing.
  void StopObservingTabStrip();

  // Called when Astra-specific metadata for a tab changes (e.g., favorite
  // toggled, workspace changed, sidebar pin toggled).
  //
  // TabStripModel does not know about Astra metadata, so changes to
  // AstraTabFeatures must be pushed through this separate path.
  //
  // TODO(astra): Replace this ad-hoc call with a proper observer pattern.
  // Options:
  //   1. Add an ObserverList to AstraTabFeatures (WebContentsUserData)
  //   2. Create a tab-projection service that bridges TabStripModel and
  //      Astra metadata with a single observer interface
  // Option (2) is preferred because it keeps WebContentsUserData lightweight
  // and follows Chromium's service-layer pattern.
  // Chromium patch point: could extend TabStripModelObserver with an
  // Astra-specific hook via a small patch to tab_strip_model_observer.h.
  void OnTabAstraFeaturesChanged(int index);

  // -- AstraWorkspaceServiceObserver ------------------------------------

  void OnWorkspaceAdded(const AstraWorkspace& workspace) override;
  void OnWorkspaceRemoved(const std::string& workspace_id) override;
  void OnWorkspaceRenamed(const std::string& workspace_id,
                          const std::string& new_name) override;
  void OnActiveWorkspaceChanged(const std::string& old_id,
                                const std::string& new_id) override;
  void OnWorkspacesReordered() override;

  // -- AstraFavoriteServiceObserver -------------------------------------

  void OnFolderAdded(const AstraFavoriteFolder& folder) override;
  void OnFolderRemoved(const std::string& folder_id) override;
  void OnFolderRenamed(const std::string& folder_id,
                       const std::string& new_name) override;
  void OnFoldersReordered() override;
  void OnFavoriteMoved(content::WebContents* web_contents,
                       const std::string& old_folder_id,
                       const std::string& new_folder_id) override;
  void OnFavoritesReordered(const std::string& folder_id) override;

  // -- AstraMemorySaverServiceObserver ------------------------------------

  void OnTabSuspended(content::WebContents* web_contents) override;
  void OnTabRestored(content::WebContents* web_contents) override;
  void OnMemorySaverEnabledChanged(bool enabled) override;
  void OnMemorySaverTimeoutChanged(base::TimeDelta timeout) override;

  // -- TabStripModelObserver ---------------------------------------------

  void OnTabStripModelChanged(TabStripModel* tab_strip_model,
                              const TabStripModelChange& change,
                              const TabStripSelectionChange& selection) override;
  void OnTabInsertedAt(TabStripModel* tab_strip_model,
                       int index,
                       bool foreground) override;
  void OnTabRemovedAt(TabStripModel* tab_strip_model,
                      int index,
                      bool was_active) override;
  void OnTabMoved(TabStripModel* tab_strip_model,
                  int from_index,
                  int to_index) override;
  void OnActiveTabChanged(TabStripModel* tab_strip_model,
                          int old_index,
                          int new_index,
                          const TabStripSelectionChange& selection) override;
  void OnTabChanged(TabStripModel* tab_strip_model,
                    int index,
                    TabChangeType change_type) override;
  void OnTabPinnedStateChanged(TabStripModel* tab_strip_model,
                               int index) override;

  // -- views::View -------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;

  // -- AstraSidebarItemDragDelegate --------------------------------------

  void OnItemDragStarted(AstraSidebarItemView* item,
                         const gfx::Point& mouse_location) override;

  // -- AstraSidebarItemHoverDelegate --------------------------------------

  void OnItemHoverStarted(AstraSidebarItemView* item,
                          const gfx::Point& mouse_location) override;
  void OnItemHoverEnded(AstraSidebarItemView* item) override;
  void OnItemHoverMoved(AstraSidebarItemView* item,
                        const gfx::Point& mouse_location) override;

  // -- Peek controller ----------------------------------------------------

  // Controller that manages the peek/hover preview state for sidebar items.
  // Owned by the sidebar view.
  //
  // TODO(astra): Integrate with Chromium's actual tab hover card system.
  //   The real integration would require patching TabHoverCardController
  //   to delegate peek actions to Astra.  For now, the sidebar has its
  //   own peek controller for sidebar-triggered previews.
  //
  //   Files to patch in Chromium:
  //     - chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.cc
  //     - chrome/browser/ui/tabs/tab_hover_card_controller.cc
  //   Hook: add a "Peek" button to the hover card that calls into
  //   AstraTabHoverPeekController::ExpandToGlance().
  //   Chromium owner: TabHoverCardController
  //   (chrome/browser/ui/tabs/tab_hover_card_controller.h).
  AstraTabHoverPeekController* peek_controller() {
    return peek_controller_.get();
  }
  const AstraTabHoverPeekController* peek_controller() const {
    return peek_controller_.get();
  }

  // -- AstraSidebarSectionDropDelegate -----------------------------------

  AstraSidebarDropResult OnDragEnterSection(
      AstraSidebarSectionType section_type,
      const AstraSidebarDragData& drag_data,
      int y_in_section) override;
  AstraSidebarDropResult OnDragOverSection(
      AstraSidebarSectionType section_type,
      const AstraSidebarDragData& drag_data,
      int y_in_section) override;
  void OnDragLeaveSection(AstraSidebarSectionType section_type) override;
  bool OnDropInSection(AstraSidebarSectionType section_type,
                       const AstraSidebarDragData& drag_data,
                       const AstraSidebarDropResult& drop_result) override;

  // -- AstraSidebarHistoryView::Delegate ---------------------------------

  // Open a URL from a history entry. Dispatches through Chromium's
  // Browser::OpenURL or the active tab's NavigationController.
  // |in_new_tab| controls whether the URL opens in the current tab or
  // a new foreground tab.
  //
  // Chromium subsystem: Browser::OpenURL (chrome/browser/ui/browser.h)
  // Chromium subsystem: content::NavigationController
  //   (content/public/browser/navigation_controller.h)
  void OpenHistoryURL(const GURL& url, bool in_new_tab) override;

  // Open the full chrome://history page in a new tab.
  // Chromium WebUI: chrome://history (chrome/browser/ui/webui/history/history_ui.cc)
  void OpenFullHistory() override;

  // -- AstraSidebarRecentlyClosedView::Delegate --------------------------

  // Restore a specific recently closed tab by its TabRestoreService entry id.
  // The tab is restored in the current browser window.
  //
  // Chromium subsystem: sessions::TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  void RestoreRecentlyClosedTab(int entry_id) override;

  // Restore all recently closed tabs.
  //
  // Chromium subsystem: sessions::TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  void RestoreAllRecentlyClosedTabs() override;

  // -- AstraSidebarPasswordsView::Delegate -------------------------------

  // Open a URL from a password entry. Dispatches through Chromium's
  // Browser::OpenURL or the active tab's NavigationController.
  // |in_new_tab| controls whether the URL opens in the current tab or
  // a new foreground tab.
  //
  // Chromium subsystem: Browser::OpenURL (chrome/browser/ui/browser.h)
  // Chromium subsystem: content::NavigationController
  //   (content/public/browser/navigation_controller.h)
  void OpenPasswordURL(const GURL& url, bool in_new_tab) override;

  // Open the password settings page (chrome://settings/passwords).
  // Chromium WebUI: chrome://settings/passwords
  //   (chrome/browser/ui/webui/settings/password_manager_handler.h)
  void OpenPasswordSettings() override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Populate each section from model data.
  void PopulateFavoritesSection(TabStripModel* tab_strip);
  void PopulatePinnedTabsSection(TabStripModel* tab_strip);
  void PopulateOpenTabsSection(TabStripModel* tab_strip);
  void UpdateWorkspaceSwitcher();

  // Refresh the history section from HistoryService.
  // Called after construction and whenever history might have changed.
  // TODO(astra): Wire to HistoryServiceObserver for reactive updates
  // instead of manual refresh calls.
  void RefreshHistorySection();

  // Refresh the passwords section from PasswordStore.
  // Called after construction and whenever passwords might have changed.
  // TODO(astra): Wire to AstraPasswordHelperObserver for reactive updates
  // instead of manual refresh calls.
  // Chromium owner: PasswordStore (components/password_manager/core/browser/)
  void RefreshPasswordsSection();

  // Refresh the recently closed section from TabRestoreService.
  // Called after construction and whenever recently closed tabs change.
  // TODO(astra): Wire to TabRestoreServiceObserver for reactive updates
  // instead of manual refresh calls.
  // Chromium owner: sessions::TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  void RefreshRecentlyClosedSection();

  // Helpers to create an item view for a tab at a given index.
  std::unique_ptr<views::View> CreateTabItem(int tab_index);

  // -- Drag and drop helpers ---------------------------------------------

  // Build drag data from a sidebar item.
  AstraSidebarDragData BuildDragData(AstraSidebarItemView* item) const;

  // Determine which section a point (in sidebar coordinates) is over.
  // Returns nullptr if not over any section.
  AstraSidebarSectionView* GetSectionAtPoint(const gfx::Point& point) const;

  // Get the section type enum for a given section view.
  AstraSidebarSectionId GetSectionIdForType(
      AstraSidebarSectionType type) const;

  // Validate whether |drag_data| can be dropped into |target_section|.
  // Returns a drop result with is_valid = true if the drop is allowed.
  AstraSidebarDropResult ValidateDrop(
      AstraSidebarSectionType target_section,
      const AstraSidebarDragData& drag_data,
      int y_in_section) const;

  // Execute a drop operation. Dispatches to the appropriate service.
  bool ExecuteDrop(AstraSidebarSectionType target_section,
                   const AstraSidebarDragData& drag_data,
                   const AstraSidebarDropResult& drop_result);

  // Show the drag ghost image at the given mouse position (in sidebar coords).
  void UpdateDragGhost(const gfx::Point& mouse_position);

  // Clean up drag state and hide the ghost.
  void EndDragSession();

  raw_ptr<Browser> browser_;
  raw_ptr<AstraWorkspaceService> workspace_service_ = nullptr;

  // Sidebar controller — owns the model and orchestrates sidebar state.
  // Created by the view on construction.
  std::unique_ptr<AstraSidebarController> controller_;

  // Favorite service — manages Astra-specific favorite folder metadata.
  // Not owned — obtained from the profile via AstraFavoriteServiceFactory.
  // Chromium subsystem: BookmarkModel (for bookmark-backed favorites)
  // TODO(astra): Wire up factory integration in profile keyed service setup.
  //   Patch point: chrome/browser/profiles/profile_keyed_service_factory*.cc
  raw_ptr<AstraFavoriteService> favorite_service_ = nullptr;

  // Memory saver service — manages tab suspension/throttling.
  // Not owned — obtained from the profile via AstraMemorySaverServiceFactory.
  // Chromium subsystem: WebContents::WasDiscarded() / DiscardableSharedMemory
  // TODO(astra): Wire up factory integration in profile keyed service setup.
  //   Patch point: chrome/browser/profiles/profile_keyed_service_factory*.cc
  raw_ptr<AstraMemorySaverService> memory_saver_service_ = nullptr;

  // Whether this sidebar is attached to an incognito (off-the-record)
  // browser window.  Cached during construction from browser_->profile().
  //
  // In incognito mode:
  //   - The active workspace is tracked locally (not via the service).
  //   - Favorite mutations (drag-to-favorite, etc.) are disabled.
  //   - Workspace mutations (add/rename/delete) are disabled.
  //   - An incognito visual indicator is shown.
  //
  // See AstraIncognitoHandler for the centralized incognito policy.
  // Chromium owner: Profile::IsOffTheRecord()
  //   (chrome/browser/profiles/profile.h)
  bool is_incognito_ = false;

  // Reading list service — projects Chromium's ReadingListModel for Astra UI.
  // Not owned — obtained from the profile via AstraReadingListServiceFactory.
  // Chromium subsystem: ReadingListModel (components/reading_list/core/)
  // TODO(astra): Wire up factory integration in profile keyed service setup.
  //   Patch point: chrome/browser/profiles/profile_keyed_service_factory*.cc
  raw_ptr<AstraReadingListService> reading_list_service_ = nullptr;

  // Note service — manages Astra-specific notes metadata.
  // Not owned — obtained from the profile via AstraNoteServiceFactory.
  // Chromium subsystem: PrefService (for persistence)
  // TODO(astra): Wire up factory integration in profile keyed service setup.
  //   Patch point: chrome/browser/profiles/profile_keyed_service_factory*.cc
  raw_ptr<AstraNoteService> note_service_ = nullptr;

  // The Chromium DownloadManager we project in the sidebar downloads section.
  // Not owned — obtained from the Browser's profile via
  // content::BrowserContext::GetDownloadManager().
  // TODO(astra): Consider using AllDownloadsNotifier instead of direct
  // DownloadManager access to handle off-the-record profiles properly.
  // Chromium owner: content::DownloadManager (content/public/browser/download_manager.h)
  // Chromium owner: AllDownloadsNotifier (chrome/browser/download/all_downloads_notifier.h)
  raw_ptr<content::DownloadManager> download_manager_ = nullptr;

  // Observation of Chromium's TabStripModel for reactive sidebar updates.
  // Primary update path — replaces manual UpdateFromModel() calls.
  base::ScopedObservation<TabStripModel, TabStripModelObserver>
      tab_strip_observation_{this};

  // Child views (owned by the view hierarchy).
  raw_ptr<AstraWorkspaceSwitcherView> workspace_switcher_ = nullptr;
  raw_ptr<AstraSidebarSectionView> favorites_section_ = nullptr;
  raw_ptr<AstraSidebarSectionView> pinned_tabs_section_ = nullptr;
  raw_ptr<AstraSidebarReadingListView> reading_list_section_ = nullptr;
  raw_ptr<AstraSidebarTabGroupsView> tab_groups_section_ = nullptr;
  raw_ptr<AstraSidebarNotesView> notes_section_ = nullptr;
  raw_ptr<AstraSidebarBookmarksView> bookmarks_section_ = nullptr;
  raw_ptr<AstraSidebarSectionView> open_tabs_section_ = nullptr;
  raw_ptr<AstraSidebarHistoryView> history_section_ = nullptr;
  raw_ptr<AstraSidebarDownloadsView> downloads_section_ = nullptr;
  raw_ptr<AstraSidebarExtensionsView> extensions_section_ = nullptr;
  raw_ptr<AstraSidebarPasswordsView> passwords_section_ = nullptr;
  raw_ptr<AstraSidebarRecentlyClosedView> recently_closed_section_ = nullptr;

  // Tab hover peek controller — shows a preview of the tab on hover.
  std::unique_ptr<AstraTabHoverPeekController> peek_controller_;

  // Current workspace filter for the open tabs section.
  std::string active_workspace_id_ = "default";

  // -- Drag and drop state -----------------------------------------------

  // True during an active drag session.
  bool is_dragging_ = false;

  // Data for the currently dragged item. Only valid when is_dragging_ is true.
  AstraSidebarDragData current_drag_data_;

  // The item view that started the drag. Not owned.
  raw_ptr<AstraSidebarItemView> drag_source_item_ = nullptr;

  // The section currently under the drag cursor. Null if not over any
  // valid section. Not owned.
  raw_ptr<AstraSidebarSectionView> hover_section_ = nullptr;

  // Drag ghost view — a semi-transparent copy of the dragged item that
  // follows the mouse cursor. Owned by the view hierarchy.
  raw_ptr<AstraSidebarItemView> drag_ghost_ = nullptr;

  // Offset of the mouse pointer within the dragged item (in the item's
  // local coordinates). Used to position the ghost correctly.
  gfx::Point drag_offset_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_VIEW_H_
