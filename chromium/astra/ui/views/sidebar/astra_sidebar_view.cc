#include "astra/ui/views/sidebar/astra_sidebar_view.h"

#include <memory>
#include <utility>

#include "astra/browser/astra_favorite_service.h"
#include "astra/browser/astra_incognito_handler.h"
#include "astra/browser/astra_recent_tabs_helper.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_sidebar_bookmarks_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_drag_types.h"
#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_recently_closed_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_section_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_tab_groups_view.h"
#include "astra/ui/views/sidebar/astra_workspace_switcher_view.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/web_contents.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Sidebar layout constants.
constexpr int kSidebarSectionSpacing = 12;
constexpr int kSidebarTopPadding = 8;
constexpr int kSidebarBottomPadding = 8;

// Section titles.
const char16_t kFavoritesTitle[] = u"Favorites";
const char16_t kPinnedTabsTitle[] = u"Pinned";
const char16_t kOpenTabsTitle[] = u"Tabs";
const char16_t kReadingListTitle[] = u"Reading List";

// Astra color ID for the sidebar background.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kSidebarBackgroundColorId = kColorAstraSidebarBackground;

// TODO(astra): Add proper incognito visual treatment for the sidebar.
// Options:
//   - Subtle background tint (e.g. slightly darker in dark mode, or
//     a purple/gray tint like Chrome's incognito title bar).
//   - An incognito icon badge in the workspace switcher area.
//   - An "Incognito" label in the sidebar header.
// Chromium subsystem: ColorProvider + ThemeService for incognito colors.
// Chromium owner: ThemeService / BrowserFrameView (incognito coloring).
// Patch point: could use Chrome's existing incognito color scheme.

}  // namespace

AstraSidebarView::AstraSidebarView(Browser* browser) : browser_(browser) {
  // Create the controller which owns the model.
  // The controller is the view's delegate for model state changes.
  controller_ = std::make_unique<AstraSidebarController>(browser);
  controller_->SetSidebarView(this);

  // Subscribe to model changes via the controller.
  // The controller observes the model and forwards changes; the view also
  // observes the model directly for immediate UI updates.
  if (controller_ && controller_->model()) {
    controller_->model()->AddObserver(this);
  }

  // Look up the workspace service from the browser's profile.
  if (browser_ && browser_->profile()) {
    workspace_service_ =
        AstraWorkspaceServiceFactory::GetForProfile(browser_->profile());
  }

  // Look up the favorite service from the browser's profile.
  if (browser_ && browser_->profile()) {
    favorite_service_ =
        AstraFavoriteServiceFactory::GetForProfile(browser_->profile());
  }

  // Look up the memory saver service from the browser's profile.
  if (browser_ && browser_->profile()) {
    memory_saver_service_ =
        AstraMemorySaverServiceFactory::GetForProfile(browser_->profile());
  }

  // Look up the reading list service from the browser's profile.
  if (browser_ && browser_->profile()) {
    reading_list_service_ =
        AstraReadingListServiceFactory::GetForProfile(browser_->profile());
  }

  // Look up the note service from the browser's profile.
  if (browser_ && browser_->profile()) {
    note_service_ = AstraNoteServiceFactory::GetForProfile(browser_->profile());
  }

  // Determine if we're in incognito mode.
  // This affects workspace activation behavior and favorite mutations.
  // See AstraIncognitoHandler for the centralized incognito policy.
  if (browser_ && browser_->profile()) {
    is_incognito_ = browser_->profile()->IsOffTheRecord();
  }

  // Subscribe to workspace service changes.
  if (workspace_service_) {
    workspace_service_->AddObserver(this);
    if (is_incognito_) {
      // In incognito, the active workspace starts at "default" rather than
      // using the service's active_workspace_id().  This is because:
      //   1. The service is shared with the original profile, so its
      //      active workspace reflects the main profile's state.
      //   2. Starting on whatever the main profile had active would leak
      //      information about the user's main profile into incognito.
      //   3. "default" is a clean, predictable starting point.
      //
      // TODO(astra): Consider persisting the incognito active workspace
      // in the OTR profile's in-memory PrefService so it survives across
      // incognito window opens/closes within the same browser session.
      // Chromium owner: Profile::GetOffTheRecordPrefs() or equivalent.
      active_workspace_id_ = "default";
    } else {
      active_workspace_id_ = workspace_service_->active_workspace_id();
    }
  }

  // Subscribe to favorite service changes.
  if (favorite_service_) {
    favorite_service_->AddObserver(this);
  }

  // Subscribe to memory saver service changes.
  if (memory_saver_service_) {
    memory_saver_service_->AddObserver(this);
  }

  // Create the peek controller for sidebar item hover previews.
  peek_controller_ = std::make_unique<AstraTabHoverPeekController>();

  // Get the DownloadManager from the browser's profile.
  // TODO(astra): Consider using AllDownloadsNotifier to handle OTR profiles
  // and provide a unified view across all browser contexts.
  // Chromium owner: content::BrowserContext::GetDownloadManager
  //   (content/public/browser/browser_context.h)
  // Chromium owner: AllDownloadsNotifier
  //   (chrome/browser/download/all_downloads_notifier.h)
  if (browser_ && browser_->profile()) {
    download_manager_ =
        content::BrowserContext::GetDownloadManager(browser_->profile());
  }

  BuildLayout();

  // Paint background.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(true);
}

AstraSidebarView::~AstraSidebarView() {
  // Unsubscribe from model changes.
  if (controller_ && controller_->model()) {
    controller_->model()->RemoveObserver(this);
  }
  // Detach the view from the controller.
  if (controller_) {
    controller_->SetSidebarView(nullptr);
  }
  // Unsubscribe from workspace service changes.
  if (workspace_service_) {
    workspace_service_->RemoveObserver(this);
  }
  // Unsubscribe from favorite service changes.
  if (favorite_service_) {
    favorite_service_->RemoveObserver(this);
  }
  // Unsubscribe from memory saver service changes.
  if (memory_saver_service_) {
    memory_saver_service_->RemoveObserver(this);
  }
  // Tab strip observation is automatically cleaned up by
  // base::ScopedObservation destructor.
}

void AstraSidebarView::BuildLayout() {
  // Vertical box layout for the entire sidebar.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kSidebarTopPadding, 0), kSidebarSectionSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // 1. Workspace switcher (top area).
  workspace_switcher_ = AddChildView(
      std::make_unique<AstraWorkspaceSwitcherView>(workspace_service_));

  // 2. Favorites section.
  favorites_section_ = AddChildView(
      std::make_unique<AstraSidebarSectionView>(
          kFavoritesTitle, AstraSidebarSectionType::kFavorites));
  favorites_section_->set_drop_delegate(this);

  // 3. Pinned tabs section.
  pinned_tabs_section_ = AddChildView(
      std::make_unique<AstraSidebarSectionView>(
          kPinnedTabsTitle, AstraSidebarSectionType::kPinnedTabs));
  pinned_tabs_section_->set_drop_delegate(this);

  // 4. Reading list section (read-later items from Chromium's ReadingListModel).
  //
  // Projects Chromium's ReadingListModel into the sidebar via
  // AstraReadingListService. Items are grouped into Unread and Read sections.
  // The section observes the reading list service for reactive updates.
  //
  // Chromium subsystem: ReadingListModel (components/reading_list/core/)
  // Chromium owner: ReadingListModel (components/reading_list/core/reading_list_model.h)
  reading_list_section_ = AddChildView(
      std::make_unique<AstraSidebarReadingListView>(reading_list_service_));

  // 5. Tab groups section (collapsible tree of tab groups with nested tabs).
  //
  // Projects Chromium's tab groups from TabStripModel into the sidebar.
  // Each group has a header (colored dot, name, tab count, expand/collapse)
  // and nested tab items. The section observes TabStripModel for reactive
  // updates to groups and their tabs.
  //
  // Chromium subsystem: TabGroup / TabGroupId (chrome/browser/ui/tabs/tab_group.h)
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  tab_groups_section_ = AddChildView(
      std::make_unique<AstraSidebarTabGroupsView>(browser_));

  // 6. Bookmarks section (Chrome bookmarks tree).
  //
  // Projects Chromium's BookmarkModel into a tree view in the sidebar.
  // The bookmarks section observes BookmarkModel for changes and rebuilds
  // the tree reactively.  Folders are expandable/collapsible.
  //
  // TODO(astra): Get BookmarkModel from profile via BookmarkModelFactory
  // and pass it explicitly to the view, instead of the view looking it up
  // internally.  This follows the same pattern as other sections where the
  // sidebar (controller) obtains services and passes them to views.
  // Chromium owner: BookmarkModelFactory
  //   (chrome/browser/bookmarks/bookmark_model_factory.h)
  //
  // TODO(astra): Handle BookmarkModel loading state properly.  The model may
  // not be loaded immediately when the sidebar is created.  The bookmarks
  // view already handles this via BookmarkModelLoaded observer, but we
  // should consider showing a loading indicator or delaying section creation
  // until the model is ready.
  // Chromium owner: BookmarkModel::loaded() and BookmarkModelLoaded()
  //   (components/bookmarks/browser/bookmark_model.h)
  bookmarks_section_ = AddChildView(
      std::make_unique<AstraSidebarBookmarksView>(browser_));

  // 5. Open tabs section (fills remaining space).
  open_tabs_section_ = AddChildView(
      std::make_unique<AstraSidebarSectionView>(
          kOpenTabsTitle, AstraSidebarSectionType::kOpenTabs));
  open_tabs_section_->set_drop_delegate(this);
  layout->SetFlexForView(open_tabs_section_, 1);

  // 5. History section (recent browsing history from Chromium HistoryService).
  // Shows ~15 recent items grouped by Today / Yesterday / Last 7 days.
  //
  // TODO(astra): Get HistoryService from the profile via HistoryServiceFactory
  // and pass it explicitly to the history view, rather than having the view
  // look it up internally.  This makes the dependency explicit and follows
  // Chromium's service-layer injection pattern.
  // Chromium owner: HistoryServiceFactory
  //   (chrome/browser/history/history_service_factory.h)
  history_section_ = AddChildView(
      std::make_unique<AstraSidebarHistoryView>(browser_->profile()));
  history_section_->set_delegate(this);

  // 6. Recently closed tabs section (near bottom, after open tabs).
  // Shows recently closed tabs from Chromium's TabRestoreService.
  // Clicking an item reopens the tab. The section is collapsible.
  //
  // Recently closed tabs are per-profile (not per-workspace) since
  // TabRestoreService is profile-scoped. They span all workspaces.
  //
  // Chromium owner: sessions::TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  // Chromium observer: TabRestoreServiceObserver
  //   (chrome/browser/sessions/tab_restore_service_observer.h)
  //
  // TODO(astra): Wire to TabRestoreServiceObserver for reactive updates
  // instead of manual refresh calls. The sidebar should update in real-time
  // when tabs are closed or restored.
  // Patch point: None needed — TabRestoreService has a public observer list.
  recently_closed_section_ = AddChildView(
      std::make_unique<AstraSidebarRecentlyClosedView>(browser_->profile()));
  recently_closed_section_->set_delegate(this);

  // 7. Downloads section (at the bottom of the sidebar).
  // Shows active downloads first (with progress), then recent completed downloads.
  // The section observes Chromium's DownloadManager for live updates.
  //
  // TODO(astra): Make the downloads section collapsible like other sections,
  // with a header that toggles visibility.  Currently the section has its own
  // built-in header, but it should follow the same pattern as other sidebar sections.
  // Chromium owner: DownloadShelf (chrome/browser/ui/views/download/download_shelf_view.h)
  // Chromium owner: DownloadItemView (chrome/browser/ui/views/download/download_item_view.h)
  downloads_section_ = AddChildView(
      std::make_unique<AstraSidebarDownloadsView>(download_manager_));

  // 7. Extensions section (at the bottom of the sidebar, below downloads).
  // Shows extension browser action icons in a grid.
  // Clicking an icon opens the extension popup.
  // Right-clicking shows the extension context menu.
  //
  // The section projects Chromium's extension state from ExtensionRegistry
  // via AstraExtensionHelper. It never stores extension truth state.
  //
  // Chromium owner: ExtensionRegistry (extensions/browser/extension_registry.h)
  // Chromium owner: ExtensionsToolbarContainer
  //   (chrome/browser/ui/views/toolbar/extensions_toolbar_container.h)
  // Chromium owner: ExtensionAction (extensions/browser/extension_action.h)
  //
  // TODO(astra): Get AstraExtensionHelper from a proper factory once the
  //   factory pattern is implemented. For now, the view tries to get it
  //   from the profile directly.
  //   Chromium pattern: ProfileKeyedServiceFactory
  // Patch point: None needed — the helper is an Astra KeyedService.
  extensions_section_ = AddChildView(
      std::make_unique<AstraSidebarExtensionsView>(browser_->profile()));

  // 8. Passwords section (near other tool sections at the bottom).
  // Shows saved passwords from Chromium's PasswordStore, with search
  // and copy-to-clipboard functionality.
  //
  // The section projects Chromium's password data via AstraPasswordHelper.
  // It never stores passwords — it only shows metadata (site, username)
  // and delegates copy/navigation to Chromium's password manager.
  //
  // Chromium owner: PasswordManagerService
  //   (chrome/browser/password_manager/password_manager_service.h)
  // Chromium owner: PasswordStore
  //   (components/password_manager/core/browser/password_store.h)
  //
  // TODO(astra): Get AstraPasswordHelper from a proper factory once the
  //   factory pattern is implemented. For now, the view creates its own
  //   helper or accesses via the profile.
  //   Chromium pattern: ProfileKeyedServiceFactory
  // Patch point: None needed — the helper is an Astra KeyedService.
  passwords_section_ = AddChildView(
      std::make_unique<AstraSidebarPasswordsView>(browser_->profile()));
  passwords_section_->set_delegate(this);

  // 9. Notes section (Astra-specific notes tied to URLs).
  //
  // Shows notes from AstraNoteService, with page notes for the current URL
  // and all notes grouped by recency. Includes a search box and an inline
  // note editor.
  //
  // The section observes AstraNoteService for reactive updates. All user
  // actions (create, edit, delete) go through the service — the view is
  // pure projection.
  //
  // Chromium subsystem: PrefService (for persistence)
  // Chromium owner: AstraNoteService (astra/browser/astra_note_service.h)
  notes_section_ = AddChildView(
      std::make_unique<AstraSidebarNotesView>(note_service_));

  // Initial model sync.
  UpdateFromModel();
}

void AstraSidebarView::UpdateFromModel() {
  if (!browser_) {
    return;
  }

  TabStripModel* tab_strip = browser_->tab_strip_model();
  if (!tab_strip) {
    return;
  }

  // Refresh workspace switcher from service.
  UpdateWorkspaceSwitcher();

  // Clear and repopulate each section.
  //
  // Full rebuild fallback. The primary update path is TabStripModelObserver
  // notifications, which currently delegate here for simplicity.
  //
  // TODO(astra): For better performance, implement incremental updates in
  // each TabStripModelObserver method instead of rebuilding everything.
  // The section view already supports InsertItemAt / RemoveItemAt / GetItemAt.
  // See the comment block on the TabStripModelObserver section for details.
  favorites_section_->ClearItems();
  pinned_tabs_section_->ClearItems();
  open_tabs_section_->ClearItems();

  PopulateFavoritesSection(tab_strip);
  PopulatePinnedTabsSection(tab_strip);
  PopulateOpenTabsSection(tab_strip);

  // Hide empty sections to keep the sidebar clean.
  favorites_section_->SetSectionVisible(favorites_section_->GetItemCount() > 0);
  pinned_tabs_section_->SetSectionVisible(
      pinned_tabs_section_->GetItemCount() > 0);

  InvalidateLayout();
}

void AstraSidebarView::SetActiveWorkspace(const std::string& workspace_id) {
  if (active_workspace_id_ == workspace_id) {
    return;
  }

  if (is_incognito_) {
    // In incognito mode, workspace activation is local to this sidebar.
    // We must NOT call workspace_service_->ActivateWorkspace() because:
    //   1. The service is shared with the original profile.
    //   2. ActivateWorkspace() persists the change to prefs (on the original
    //      profile), which would leak incognito activity.
    //   3. It would also affect other incognito windows and the main profile.
    //
    // Instead, we just update our local active_workspace_id_ and refresh.
    // See AstraIncognitoHandler::DoesWorkspaceActivationAffectService.
    active_workspace_id_ = workspace_id;
    UpdateFromModel();
    return;
  }

  // Dispatch through the service so it broadcasts the change to all observers.
  if (workspace_service_) {
    workspace_service_->ActivateWorkspace(workspace_id);
    // The sidebar will be updated via OnActiveWorkspaceChanged observer.
  } else {
    // Fallback: update local state and refresh if no service is available.
    active_workspace_id_ = workspace_id;
    UpdateFromModel();
  }
}

void AstraSidebarView::NavigateWorkspace(int direction) {
  if (!workspace_service_) {
    return;
  }

  const auto& workspaces = workspace_service_->workspaces();
  if (workspaces.size() <= 1) {
    return;  // Nowhere to navigate.
  }

  // Find the index of the current active workspace.
  size_t current_idx = 0;
  for (size_t i = 0; i < workspaces.size(); ++i) {
    if (workspaces[i].id == active_workspace_id_) {
      current_idx = i;
      break;
    }
  }

  // Compute next/previous index with wrap-around.
  size_t next_idx;
  if (direction > 0) {
    next_idx = (current_idx + 1) % workspaces.size();
  } else {
    next_idx = (current_idx + workspaces.size() - 1) % workspaces.size();
  }

  // Use SetActiveWorkspace which handles incognito vs regular mode correctly.
  SetActiveWorkspace(workspaces[next_idx].id);
}

void AstraSidebarView::ToggleExtensionsPanel() {
  if (!extensions_section_) {
    return;
  }

  // Toggle the expanded/collapsed state of the extensions section.
  // This matches the behavior of other collapsible sidebar sections.
  extensions_section_->ToggleExpanded();
}

// Toggle the passwords panel (show/hide the passwords section).
void AstraSidebarView::TogglePasswordsPanel() {
  if (!passwords_section_) {
    return;
  }

  // Toggle the expanded/collapsed state of the passwords section.
  passwords_section_->ToggleExpanded();
}

// Toggle the notes panel (show/hide the notes section).
void AstraSidebarView::ToggleNotesPanel() {
  if (!notes_section_) {
    return;
  }

  // Toggle the expanded/collapsed state of the notes section.
  notes_section_->ToggleExpanded();
}

// =========================================================================
// Auto-hide
// =========================================================================

void AstraSidebarView::StartAutoHideTimer() {
  // TODO(astra): Implement auto-hide timer with base::OneShotTimer.
  //   When the timer fires, hide the sidebar if auto-hide mode is enabled.
  //   Chromium pattern: base::OneShotTimer (base/timer/one_shot_timer.h)
  //
  //   The delay should be configurable, e.g. 300ms by default.
  //   If the mouse re-enters the sidebar before the timer fires,
  //   the hide should be cancelled.
  if (!model() || !model()->is_visible()) {
    return;
  }

  auto mode = model()->auto_hide_mode();
  if (mode == AstraSidebarAutoHideMode::kOnHoverLeave ||
      mode == AstraSidebarAutoHideMode::kOnClickOutside) {
    auto_hide_active_ = true;
    // TODO(astra): Start the actual hide timer.
    // For now, we just set the state flag.
  }
}

void AstraSidebarView::CancelAutoHideTimer() {
  auto_hide_active_ = false;
  // TODO(astra): Stop the auto-hide timer if it's running.
}

// =========================================================================
// Keyboard navigation
// =========================================================================

bool AstraSidebarView::NavigateSection(int direction) {
  if (!model()) {
    return false;
  }

  auto visible = model()->GetVisibleSections();
  if (visible.empty()) {
    return false;
  }

  if (selected_section_index_ < 0 ||
      selected_section_index_ >= static_cast<int>(visible.size())) {
    // Start at the first visible section.
    selected_section_index_ = 0;
  } else {
    // Move to next/previous with wrap-around.
    selected_section_index_ =
        (selected_section_index_ + direction +
         static_cast<int>(visible.size())) %
        static_cast<int>(visible.size());
  }

  if (selected_section_index_ >= 0 &&
      selected_section_index_ < static_cast<int>(visible.size())) {
    model()->SetActiveSection(visible[selected_section_index_].id);
    return true;
  }
  return false;
}

bool AstraSidebarView::ActivateSelectedSection() {
  if (selected_section_index_ < 0 || !model()) {
    return false;
  }
  // The active section is already tracked by the model.
  // Activating it means expanding it or performing its primary action.
  auto active_section = model()->active_section_id();
  if (!active_section.empty()) {
    // Toggle expanded state of the active section.
    model()->ToggleSectionCollapsed(active_section);
    return true;
  }
  return false;
}

// =========================================================================
// Tab strip observation
// =========================================================================

void AstraSidebarView::StartObservingTabStrip(TabStripModel* tab_strip) {
  if (!tab_strip) {
    StopObservingTabStrip();
    return;
  }

  // Reset observation to the new model. Safe even if already observing.
  tab_strip_observation_.Observe(tab_strip);

  // Initial sync — populate the sidebar with current tab state.
  UpdateFromModel();
}

void AstraSidebarView::StopObservingTabStrip() {
  tab_strip_observation_.Reset();
}

void AstraSidebarView::OnTabAstraFeaturesChanged(int /*index*/) {
  // Astra metadata for a tab changed (favorite, workspace, sidebar pin, etc.).
  // Since Astra metadata affects which section(s) a tab appears in, we need
  // to rebuild the sidebar projection.
  //
  // TODO(astra): Optimize this to only move the affected tab item between
  // sections instead of rebuilding everything. For example, if a tab is
  // favorited, only add/remove it from the favorites section and update its
  // item style in the open tabs section.
  UpdateFromModel();
}

gfx::Size AstraSidebarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Fixed width, height follows parent.
  // TODO(astra): Read width from user prefs / profile prefs.
  int height = 0;
  if (available_size.height().is_bounded()) {
    height = available_size.height().value();
  }
  return gfx::Size(kDefaultWidth, height);
}

void AstraSidebarView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }
  if (layer()) {
    layer()->SetColor(color_provider->GetColor(kSidebarBackgroundColorId));
  }
}

// =========================================================================
// AstraWorkspaceServiceObserver
// =========================================================================

void AstraSidebarView::OnWorkspaceAdded(const AstraWorkspace& /*workspace*/) {
  // Rebuild everything — the workspace switcher and potentially tab
  // sections need to reflect the new workspace.
  UpdateFromModel();
}

void AstraSidebarView::OnWorkspaceRemoved(const std::string& /*workspace_id*/) {
  // After a workspace is removed, tabs are reassigned to the default
  // workspace by the service. We need to repopulate all sections.
  UpdateFromModel();
}

void AstraSidebarView::OnWorkspaceRenamed(const std::string& /*workspace_id*/,
                                          const std::string& /*new_name*/) {
  // Only the workspace switcher display needs updating, but it's simpler
  // to refresh the whole model.
  // TODO(astra): Optimize by only updating the workspace switcher when
  // a workspace is renamed, since no tab section content changes.
  UpdateFromModel();
}

void AstraSidebarView::OnActiveWorkspaceChanged(const std::string& /*old_id*/,
                                                const std::string& new_id) {
  // Active workspace changed on the service.
  if (is_incognito_) {
    // In incognito mode, ignore service-level active workspace changes.
    // The service's active workspace belongs to the original (non-OTR)
    // profile and reflects the main profile's state.  Our incognito
    // sidebar has its own independent active workspace tracked locally.
    //
    // Rationale: if the user switches workspaces in their main profile,
    // their incognito window should not suddenly switch workspaces too —
    // that would be surprising and leak information between contexts.
    return;
  }

  // Regular profile: update our local filter and refresh the open tabs section.
  active_workspace_id_ = new_id;
  UpdateFromModel();
}

void AstraSidebarView::OnWorkspacesReordered() {
  // Workspace order changed — refresh the workspace switcher display.
  // Tab sections are unaffected by workspace reordering.
  UpdateWorkspaceSwitcher();
}

// =========================================================================
// AstraFavoriteServiceObserver
// =========================================================================
//
// Observer for AstraFavoriteService changes. The sidebar projects favorite
// state into the favorites section and (eventually) updates tab items to
// show favorite indicators.
//
// TODO(astra): Implement incremental UI updates for favorite changes.
//   For now, we fall back to UpdateFromModel() (full rebuild) on every change.
//   The section view already supports InsertItemAt / RemoveItemAt for
//   incremental updates.
//
// Chromium owner: AstraFavoriteService (astra/browser/astra_favorite_service.h)

void AstraSidebarView::OnFolderAdded(const AstraFavoriteFolder& /*folder*/) {
  // A new favorite folder was added. Rebuild the favorites section.
  // TODO(astra): Incrementally add the new folder instead of full rebuild.
  UpdateFromModel();
}

void AstraSidebarView::OnFolderRemoved(const std::string& /*folder_id*/) {
  // A favorite folder was removed. Rebuild the favorites section.
  // TODO(astra): Incrementally remove the folder instead of full rebuild.
  UpdateFromModel();
}

void AstraSidebarView::OnFolderRenamed(const std::string& /*folder_id*/,
                                       const std::string& /*new_name*/) {
  // A favorite folder was renamed. Update the display.
  // TODO(astra): Incrementally update the folder name label instead of full rebuild.
  UpdateFromModel();
}

void AstraSidebarView::OnFoldersReordered() {
  // Favorite folders were reordered. Rebuild the favorites section.
  // TODO(astra): Incrementally reorder folders instead of full rebuild.
  UpdateFromModel();
}

void AstraSidebarView::OnFavoriteMoved(content::WebContents* /*web_contents*/,
                                      const std::string& /*old_folder_id*/,
                                      const std::string& /*new_folder_id*/) {
  // A tab was moved between favorite folders (or added/removed from favorites).
  // Rebuild both the favorites section and the open tabs section (to update
  // the favorite indicator on the tab item).
  // TODO(astra): Incrementally move the item between sections.
  UpdateFromModel();
}

void AstraSidebarView::OnFavoritesReordered(const std::string& /*folder_id*/) {
  // Favorites within a folder were reordered. Rebuild the favorites section.
  // TODO(astra): Incrementally reorder items within the folder.
  UpdateFromModel();
}

// =========================================================================
// AstraMemorySaverServiceObserver
// =========================================================================
//
// Observer for AstraMemorySaverService changes. When tabs are suspended or
// restored, the sidebar updates the visual state of the affected tab items.
//
// TODO(astra): Implement incremental UI updates for memory saver changes.
//   For now, we fall back to UpdateFromModel() (full rebuild) on every change.
//
// Chromium owner: AstraMemorySaverService (astra/browser/astra_memory_saver_service.h)

void AstraSidebarView::OnTabSuspended(content::WebContents* /*web_contents*/) {
  // A tab was suspended (discarded / put to sleep). Update its sidebar item
  // to show the suspended state (faded text, sleep indicator).
  // TODO(astra): Find the tab item by WebContents and update its suspended
  //   state incrementally instead of doing a full rebuild.
  UpdateFromModel();
}

void AstraSidebarView::OnTabRestored(content::WebContents* /*web_contents*/) {
  // A suspended tab was restored (reloaded). Update its sidebar item to
  // remove the suspended state visual treatment.
  // TODO(astra): Find the tab item by WebContents and update its suspended
  //   state incrementally instead of doing a full rebuild.
  UpdateFromModel();
}

void AstraSidebarView::OnMemorySaverEnabledChanged(bool /*enabled*/) {
  // Memory saver was toggled on/off. If it was turned off, all suspended
  // tabs may be restored; if turned on, tabs may start being suspended.
  // Full rebuild is the safe default.
  // TODO(astra): Only update the visual state (e.g. show/hide sleep indicators)
  //   without full rebuild if the actual suspension state hasn't changed yet.
  UpdateFromModel();
}

void AstraSidebarView::OnMemorySaverTimeoutChanged(base::TimeDelta /*timeout*/) {
  // The auto-suspend timeout changed. No immediate UI change is needed
  // since this only affects future suspensions.
  // TODO(astra): If we ever show the timeout in settings UI, update it here.
}

// =========================================================================
// TabStripModelObserver
// =========================================================================
//
// Primary reactive update path for the sidebar. All tab state changes
// originate from Chromium's TabStripModel and flow through these observer
// methods. The sidebar is a pure projection — it never mutates TabStripModel.
//
// TODO(astra): Implement incremental updates in each method instead of
// calling UpdateFromModel() (full rebuild). Full rebuilds are correct but
// inefficient for frequent operations like tab title updates. The section
// view already has InsertItemAt / RemoveItemAt / GetItemAt for this.
//
// Performance rationale: With many tabs (100+), full rebuilds cause
// noticeable jank on every tab change. Incremental updates are O(1) for
// single-tab changes and O(n) only for batch operations.
//
// Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
// Chromium observer: TabStripModelObserver (chrome/browser/ui/tabs/tab_strip_model_observer.h)

void AstraSidebarView::OnTabStripModelChanged(
    TabStripModel* /*tab_strip_model*/,
    const TabStripModelChange& /*change*/,
    const TabStripSelectionChange& /*selection*/) {
  // Generic change handler — catches any change not handled by the more
  // specific methods below. Full rebuild is the safe default.
  // TODO(astra): Switch on change.type() and do incremental updates for
  // each change type (kInserted, kRemoved, kMoved, kReplaced, kSelectionOnly).
  UpdateFromModel();
}

void AstraSidebarView::OnTabInsertedAt(TabStripModel* /*tab_strip_model*/,
                                       int /*index*/,
                                       bool /*foreground*/) {
  // A new tab was inserted at |index|.
  // TODO(astra): Incrementally insert a new sidebar item at the correct
  // position in the appropriate section (pinned or open tabs), and add
  // to favorites section if the tab has is_favorite() set.
  UpdateFromModel();
}

void AstraSidebarView::OnTabRemovedAt(TabStripModel* /*tab_strip_model*/,
                                      int /*index*/,
                                      bool /*was_active*/) {
  // A tab was removed from |index|.
  // TODO(astra): Incrementally remove the corresponding sidebar item(s)
  // from all sections (favorites, pinned, open tabs) that contained it.
  UpdateFromModel();
}

void AstraSidebarView::OnTabMoved(TabStripModel* /*tab_strip_model*/,
                                  int /*from_index*/,
                                  int /*to_index*/) {
  // A tab was moved from |from_index| to |to_index|.
  // TODO(astra): Incrementally reorder items within the appropriate
  // section. If the move crosses the pinned/non-pinned boundary, also
  // move the item between pinned_tabs_section_ and open_tabs_section_.
  UpdateFromModel();
}

void AstraSidebarView::OnActiveTabChanged(
    TabStripModel* /*tab_strip_model*/,
    int /*old_index*/,
    int /*new_index*/,
    const TabStripSelectionChange& /*selection*/) {
  // The active/selected tab changed.
  // TODO(astra): Incrementally update the active highlight: clear active
  // state from the old tab's item(s) and set it on the new tab's item(s).
  // This is one of the most frequent operations and should be O(1).
  UpdateFromModel();
}

void AstraSidebarView::OnTabChanged(TabStripModel* /*tab_strip_model*/,
                                    int /*index*/,
                                    TabChangeType /*change_type*/) {
  // A tab's display state changed (title, favicon, loading state, etc.).
  // |change_type| indicates what changed (kAll, kTitleOnly, etc.).
  // TODO(astra): Incrementally update just that tab item's title/icon.
  // Since a tab can appear in multiple sections (favorites + open tabs),
  // find all matching items and update them.
  UpdateFromModel();
}

void AstraSidebarView::OnTabPinnedStateChanged(
    TabStripModel* /*tab_strip_model*/,
    int /*index*/) {
  // A tab's pinned state changed (pinned or unpinned).
  // TODO(astra): Move the tab item between pinned_tabs_section_ and
  // open_tabs_section_. If the tab is also a favorite, it stays in the
  // favorites section regardless of pin state.
  UpdateFromModel();
}

// =========================================================================
// Section population helpers
// =========================================================================

void AstraSidebarView::PopulateFavoritesSection(TabStripModel* tab_strip) {
  const int count = tab_strip->GetTabCount();
  for (int i = 0; i < count; ++i) {
    content::WebContents* web_contents = tab_strip->GetWebContentsAt(i);
    if (!web_contents) {
      continue;
    }
    AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
    if (features && features->is_favorite()) {
      favorites_section_->AddItem(CreateTabItem(i));
    }
  }
}

void AstraSidebarView::PopulatePinnedTabsSection(TabStripModel* tab_strip) {
  // Chromium's TabStripModel keeps pinned tabs at the start of the strip.
  // Use IndexOfFirstNonPinnedTab() to find the boundary.
  //
  // TODO(astra): Use TabStripModel::IsTabPinned(int index) for clarity
  // once building against a full Chromium checkout. The index-based
  // approach is equivalent but less readable.
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  int first_non_pinned = tab_strip->IndexOfFirstNonPinnedTab();
  for (int i = 0; i < first_non_pinned; ++i) {
    pinned_tabs_section_->AddItem(CreateTabItem(i));
  }
}

void AstraSidebarView::PopulateOpenTabsSection(TabStripModel* tab_strip) {
  const int count = tab_strip->GetTabCount();
  int first_non_pinned = tab_strip->IndexOfFirstNonPinnedTab();

  for (int i = first_non_pinned; i < count; ++i) {
    content::WebContents* web_contents = tab_strip->GetWebContentsAt(i);
    if (!web_contents) {
      continue;
    }

    // Skip tabs hidden from sidebar presentation.
    AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
    if (features && features->sidebar_hidden()) {
      continue;
    }

    // Filter by workspace membership.
    if (features && features->workspace_id() != active_workspace_id_) {
      continue;
    }

    open_tabs_section_->AddItem(CreateTabItem(i));
  }
}

void AstraSidebarView::UpdateWorkspaceSwitcher() {
  if (workspace_switcher_) {
    workspace_switcher_->UpdateFromService();
  }
}

std::unique_ptr<views::View> AstraSidebarView::CreateTabItem(int tab_index) {
  content::WebContents* web_contents =
      browser_->tab_strip_model()->GetWebContentsAt(tab_index);

  // Determine the tab title. Prefer the WebContents title if available.
  std::u16string title;
  if (web_contents && !web_contents->GetTitle().empty()) {
    title = web_contents->GetTitle();
  } else {
    // Fallback placeholder title.
    title = u"Tab " + base::NumberToString16(tab_index + 1);
  }

  // Determine item type for styling.
  auto type = AstraSidebarItemView::Type::kTab;
  if (web_contents) {
    AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
    if (features) {
      if (features->is_favorite()) {
        type = AstraSidebarItemView::Type::kFavorite;
      } else if (features->sidebar_pinned()) {
        type = AstraSidebarItemView::Type::kPinnedTab;
      }
    }
  }

  auto item = std::make_unique<AstraSidebarItemView>(title, type);

  // Mark as active if this is the currently selected tab.
  if (browser_->tab_strip_model()->active_index() == tab_index) {
    item->SetActive(true);
  }

  // Attach metadata for drag-and-drop lookup.
  // These are cached on the view for convenience during drag operations;
  // the authoritative source is still the services and TabStripModel.
  item->set_tab_index(tab_index);
  item->set_workspace_id(active_workspace_id_);
  if (web_contents) {
    AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
    if (features) {
      item->set_workspace_id(features->workspace_id());
      item->set_favorite_folder_id(features->favorite_folder_id());
    }
  }

  // Make items draggable for reordering and moving between sections.
  item->SetDraggable(true);
  item->set_drag_delegate(this);

  // Set hover delegate for peek/preview on hover.
  item->set_hover_delegate(this);

  // -- Audio indicator ---------------------------------------------------
  // Project audio state from Chromium's WebContents into the sidebar item.
  // Audio state is owned by WebContents — the sidebar is a pure projection
  // and never stores mute/audio state.
  //
  // Priority: muted state takes precedence over playing state.  A muted tab
  // shows the muted icon even if audio is technically playing silently.
  //
  // Chromium owner: content::WebContents (content/public/browser/web_contents.h)
  //   - IsCurrentlyAudible() -> bool (audio is playing and audible)
  //   - IsAudioMuted() -> bool (tab audio output is muted)
  //   - SetAudioMuted(bool) -> void (toggle mute)
  //   - Observer: WebContentsObserver::OnAudioStateChanged()
  //
  // TODO(astra): Use TabStripModelObserver or WebContentsObserver for
  //   reactive audio state updates instead of polling on each
  //   UpdateFromModel() refresh.  The proper approach is to observe each
  //   tab's WebContents for OnAudioStateChanged() and update only the
  //   affected sidebar item.  For the overlay skeleton, we update audio
  //   state during the full UpdateFromModel() refresh cycle.
  //
  // TODO(astra): Handle tab capturing state (camera/mic indicator) as a
  //   bonus indicator state.  Chromium tracks this via
  //   WebContents::IsCapturing() and has a separate indicator icon.
  // Chromium owner: TabRenderer (chrome/browser/ui/views/tabs/tab_renderer.h)
  if (web_contents) {
    AstraSidebarItemView::AudioState audio_state =
        AstraSidebarItemView::AudioState::kNone;
    if (web_contents->IsAudioMuted()) {
      audio_state = AstraSidebarItemView::AudioState::kMuted;
    } else if (web_contents->IsCurrentlyAudible()) {
      audio_state = AstraSidebarItemView::AudioState::kPlaying;
    }
    item->SetAudioState(audio_state);

    // Wire the audio toggle callback.  Clicking the audio indicator toggles
    // the mute state on the underlying WebContents.
    // The callback captures |tab_index| at item creation time.
    // Note: with the current full-rebuild approach, the captured index stays
    // valid for the lifetime of this item view.  When incremental updates
    // are implemented, indices must be updated on tab move/insert/remove.
    item->set_audio_toggle_callback(base::BindRepeating(
        [](Browser* browser, int index) {
          if (!browser || !browser->tab_strip_model()) {
            return;
          }
          content::WebContents* contents =
              browser->tab_strip_model()->GetWebContentsAt(index);
          if (contents) {
            contents->SetAudioMuted(!contents->IsAudioMuted());
          }
          // TODO(astra): After toggling, the sidebar should update the
          //   audio indicator immediately.  With full rebuilds this happens
          //   on the next UpdateFromModel() cycle.  With
          //   WebContentsObserver::OnAudioStateChanged() it would update
          //   reactively.  For now, rely on the next refresh or observer.
        },
        base::Unretained(browser_), tab_index));
  }

  // Click handler: switch to the tab.
  // Dispatches through Chromium's TabStripModel API — the sidebar projects
  // tabs, it does not own them.
  // TODO(astra): Use base::Unretained(this) with a member function instead
  // of capturing browser_ directly, for stronger ownership guarantees.
  // The raw base::Unretained is safe here because the item is a child of
  // this view and will be destroyed before `this`.
  item->SetCallback(base::BindRepeating(
      [](Browser* browser, int index) {
        if (browser && browser->tab_strip_model()) {
          browser->tab_strip_model()->ActivateTabAt(index);
        }
      },
      base::Unretained(browser_), tab_index));

  return item;
}

// =========================================================================
// Drag and drop — AstraSidebarItemDragDelegate
// =========================================================================

void AstraSidebarView::OnItemDragStarted(AstraSidebarItemView* item,
                                         const gfx::Point& mouse_location) {
  DCHECK(item);

  is_dragging_ = true;
  drag_source_item_ = item;
  current_drag_data_ = BuildDragData(item);
  hover_section_ = nullptr;

  // Record the mouse offset within the item for proper ghost positioning.
  drag_offset_ = mouse_location;

  // Create a drag ghost view — a semi-transparent copy of the dragged item.
  // The ghost follows the mouse cursor during the drag.
  // TODO(astra): Use Chromium's views drag image system or
  // ui/base/dragdrop/drag_utils for proper OS-level drag images when
  // dragging between windows or to external targets.
  // Chromium DnD subsystem: ui/base/dragdrop/drag_utils.h,
  //   views/widget/native_widget.h (StartDragForViewFrom)
  drag_ghost_ = AddChildView(std::make_unique<AstraSidebarItemView>(
      item->GetText(), item->type()));
  drag_ghost_->SetActive(item->IsActive());
  drag_ghost_->SetSize(item->size());
  drag_ghost_->SetOpacity(0.7f);
  drag_ghost_->set_drag_delegate(nullptr);
  drag_ghost_->SetDraggable(false);
  // Paint above all other children.
  drag_ghost_->layer()->SetOpacity(0.7f);
  // TODO(astra): Use proper Z-ordering with views::View::StackAtTop or
  // the layer system to ensure the ghost renders on top. For now, we
  // just add it as the last child.

  // Position the ghost at the mouse, offset by drag_offset_.
  gfx::Point sidebar_mouse = mouse_location;
  ConvertPointToTarget(item, this, &sidebar_mouse);
  UpdateDragGhost(sidebar_mouse);

  // Capture the mouse so we receive all drag events.
  // TODO(astra): Use the proper Chromium views mouse capture mechanism.
  // For now, rely on the widget's implicit capture during drag.
  // Chromium views: views::Widget::RunShellDrag or native widget drag.
}

// =========================================================================
// Hover / peek — AstraSidebarItemHoverDelegate
// =========================================================================

void AstraSidebarView::OnItemHoverStarted(AstraSidebarItemView* item,
                                          const gfx::Point& mouse_location) {
  DCHECK(item);

  // Don't trigger peek during a drag operation.
  if (is_dragging_) {
    return;
  }

  // Only trigger peek for tab items (not workspace switcher, etc.).
  if (item->type() != AstraSidebarItemView::Type::kTab &&
      item->type() != AstraSidebarItemView::Type::kPinnedTab &&
      item->type() != AstraSidebarItemView::Type::kFavorite) {
    return;
  }

  // Get the WebContents for this tab.
  content::WebContents* web_contents = nullptr;
  if (browser_ && browser_->tab_strip_model()) {
    int tab_index = item->tab_index();
    if (tab_index >= 0 &&
        tab_index < browser_->tab_strip_model()->GetTabCount()) {
      web_contents = browser_->tab_strip_model()->GetWebContentsAt(tab_index);
    }
  }

  if (!web_contents) {
    return;
  }

  // Compute the anchor rect — the item's bounds in the item's local coords.
  // The peek controller will use the item view as the anchor.
  gfx::Rect anchor_rect(item->size());

  // Start the hover → preview sequence.
  peek_controller_->OnHoverStarted(
      item, anchor_rect, web_contents,
      AstraTabHoverPeekController::Source::kSidebarItem);
}

void AstraSidebarView::OnItemHoverEnded(AstraSidebarItemView* item) {
  DCHECK(item);

  // If the preview/glance is currently showing for this item, hide it.
  // TODO(astra): We should check if the mouse is moving from the sidebar
  // item into the preview bubble.  If so, we should keep the preview
  // visible so the user can interact with it (e.g., click the Peek button).
  // This is a common pattern in hover UIs — the preview stays visible
  // while the mouse is within the "hover corridor" between source and
  // preview.
  //
  // Chromium pattern: TabHoverCardController uses a delay before hiding
  // when the mouse leaves the tab, to give the user time to move into
  // the hover card.
  //
  // For now, hide immediately when the mouse leaves the item.
  peek_controller_->OnHoverEnded();
}

void AstraSidebarView::OnItemHoverMoved(AstraSidebarItemView* item,
                                        const gfx::Point& mouse_location) {
  // Forward mouse movement to the peek controller.
  // The peek controller may use this to update the anchor position or
  // to detect when the mouse is moving toward the preview bubble.
  peek_controller_->OnHoverMoved(mouse_location);
}

// =========================================================================
// Mouse events (auto-hide + drag)
// =========================================================================

void AstraSidebarView::OnMouseEntered(const ui::MouseEvent& event) {
  // When mouse enters the sidebar in auto-hide mode, show the sidebar
  // if it's currently hidden (peek-on-hover behavior).
  if (model() && !model()->is_visible() &&
      model()->auto_hide_mode() == AstraSidebarAutoHideMode::kOnHoverLeave) {
    model()->SetVisible(true);
  }
  CancelAutoHideTimer();
  views::View::OnMouseEntered(event);
}

void AstraSidebarView::OnMouseExited(const ui::MouseEvent& event) {
  // When mouse leaves the sidebar in auto-hide mode, start the hide timer.
  if (model() && model()->is_visible() &&
      (model()->auto_hide_mode() == AstraSidebarAutoHideMode::kOnHoverLeave ||
       model()->auto_hide_mode() == AstraSidebarAutoHideMode::kOnClickOutside)) {
    StartAutoHideTimer();
  }
  views::View::OnMouseExited(event);
}

bool AstraSidebarView::OnKeyPressed(const ui::KeyEvent& event) {
  // Handle keyboard navigation at the sidebar level.
  if (!GetVisible()) {
    return views::View::OnKeyPressed(event);
  }

  switch (event.key_code()) {
    case ui::VKEY_UP:
      if (NavigateSection(-1)) {
        return true;
      }
      break;
    case ui::VKEY_DOWN:
      if (NavigateSection(1)) {
        return true;
      }
      break;
    case ui::VKEY_RETURN:
    case ui::VKEY_SPACE:
      if (ActivateSelectedSection()) {
        return true;
      }
      break;
    case ui::VKEY_HOME:
      if (model() && !model()->GetVisibleSections().empty()) {
        selected_section_index_ = 0;
        model()->SetActiveSection(
            model()->GetVisibleSections()[0].id);
        return true;
      }
      break;
    case ui::VKEY_END:
      if (model()) {
        auto visible = model()->GetVisibleSections();
        if (!visible.empty()) {
          selected_section_index_ =
              static_cast<int>(visible.size()) - 1;
          model()->SetActiveSection(visible.back().id);
          return true;
        }
      }
      break;
    case ui::VKEY_ESCAPE:
      // Escape: close/collapse the sidebar in auto-hide mode.
      if (model() && model()->is_visible() &&
          model()->auto_hide_mode() != AstraSidebarAutoHideMode::kDisabled) {
        model()->SetVisible(false);
        return true;
      }
      break;
    default:
      // Check for number key shortcuts (1-9) to jump to sections.
      if (event.key_code() >= ui::VKEY_1 &&
          event.key_code() <= ui::VKEY_9) {
        int section_index = event.key_code() - ui::VKEY_1;
        if (model()) {
          auto visible = model()->GetVisibleSections();
          if (section_index < static_cast<int>(visible.size())) {
            model()->SetActiveSection(visible[section_index].id);
            return true;
          }
        }
      }
      break;
  }

  return views::View::OnKeyPressed(event);
}

// =========================================================================
// Drag and drop — mouse event handlers
// =========================================================================

bool AstraSidebarView::OnMouseDragged(const ui::MouseEvent& event) {
  if (!is_dragging_) {
    return views::View::OnMouseDragged(event);
  }

  gfx::Point mouse_pos = event.location();

  // Update the drag ghost position.
  UpdateDragGhost(mouse_pos);

  // Determine which section (if any) the mouse is over.
  AstraSidebarSectionView* target_section = GetSectionAtPoint(mouse_pos);

  if (target_section != hover_section_) {
    // Left the previous section.
    if (hover_section_) {
      hover_section_->OnDragLeave();
    }
    hover_section_ = target_section;
    // Entered a new section.
    if (hover_section_) {
      gfx::Point section_point = mouse_pos;
      ConvertPointToTarget(this, hover_section_, &section_point);
      hover_section_->OnDragEnter(current_drag_data_, section_point.y());
    }
  } else if (hover_section_) {
    // Still in the same section — update position.
    gfx::Point section_point = mouse_pos;
    ConvertPointToTarget(this, hover_section_, &section_point);
    hover_section_->OnDragOver(current_drag_data_, section_point.y());
  }

  return true;
}

void AstraSidebarView::OnMouseReleased(const ui::MouseEvent& event) {
  if (!is_dragging_) {
    views::View::OnMouseReleased(event);
    return;
  }

  // Drop the item on the currently hovered section.
  bool handled = false;
  if (hover_section_) {
    handled = hover_section_->OnDrop(current_drag_data_);
  }

  // TODO(astra): If not handled or dropped outside, show a "revert"
  // animation or shake for invalid drops.

  EndDragSession();
}

void AstraSidebarView::OnMouseCaptureLost() {
  if (is_dragging_) {
    // Drag aborted — clean up without performing any action.
    if (hover_section_) {
      hover_section_->OnDragLeave();
    }
    EndDragSession();
  }
  views::View::OnMouseCaptureLost();
}

// =========================================================================
// Drag and drop — AstraSidebarSectionDropDelegate
// =========================================================================

AstraSidebarDropResult AstraSidebarView::OnDragEnterSection(
    AstraSidebarSectionType section_type,
    const AstraSidebarDragData& drag_data,
    int y_in_section) {
  return ValidateDrop(section_type, drag_data, y_in_section);
}

AstraSidebarDropResult AstraSidebarView::OnDragOverSection(
    AstraSidebarSectionType section_type,
    const AstraSidebarDragData& drag_data,
    int y_in_section) {
  return ValidateDrop(section_type, drag_data, y_in_section);
}

void AstraSidebarView::OnDragLeaveSection(AstraSidebarSectionType section_type) {
  // Nothing to do — the section handles hiding its own indicator.
}

bool AstraSidebarView::OnDropInSection(
    AstraSidebarSectionType section_type,
    const AstraSidebarDragData& drag_data,
    const AstraSidebarDropResult& drop_result) {
  return ExecuteDrop(section_type, drag_data, drop_result);
}

// =========================================================================
// Drag and drop — helpers
// =========================================================================

AstraSidebarDragData AstraSidebarView::BuildDragData(
    AstraSidebarItemView* item) const {
  AstraSidebarDragData data;

  // Determine item type from the item's Type enum.
  switch (item->type()) {
    case AstraSidebarItemView::Type::kTab:
    case AstraSidebarItemView::Type::kPinnedTab:
      data.item_type = AstraSidebarDragItemType::kTab;
      break;
    case AstraSidebarItemView::Type::kFavorite:
      data.item_type = AstraSidebarDragItemType::kTab;
      break;
    case AstraSidebarItemView::Type::kWorkspace:
      data.item_type = AstraSidebarDragItemType::kWorkspace;
      break;
  }

  // Determine source section. We need to figure out which section the item
  // is in. Walk up the view hierarchy to find the parent section.
  // TODO(astra): Cache the source section on the item view for faster lookup
  // and to avoid walking the hierarchy during drag start.
  data.source_section = AstraSidebarSectionId::kUnknown;
  views::View* parent = item->parent();
  if (parent) {
    // The item is inside the items_container, which is inside the section.
    views::View* section_view = parent->parent();
    if (section_view == favorites_section_) {
      data.source_section = AstraSidebarSectionId::kFavorites;
    } else if (section_view == pinned_tabs_section_) {
      data.source_section = AstraSidebarSectionId::kPinnedTabs;
    } else if (section_view == open_tabs_section_) {
      data.source_section = AstraSidebarSectionId::kOpenTabs;
    }
  }

  data.source_workspace_id = item->workspace_id();
  data.tab_index = item->tab_index();
  data.favorite_folder_id = item->favorite_folder_id();
  data.workspace_id = item->workspace_id();

  return data;
}

AstraSidebarSectionView* AstraSidebarView::GetSectionAtPoint(
    const gfx::Point& point) const {
  // Check each section in order.
  if (favorites_section_ && favorites_section_->GetVisible() &&
      favorites_section_->bounds().Contains(point)) {
    return favorites_section_;
  }
  if (pinned_tabs_section_ && pinned_tabs_section_->GetVisible() &&
      pinned_tabs_section_->bounds().Contains(point)) {
    return pinned_tabs_section_;
  }
  if (open_tabs_section_ && open_tabs_section_->GetVisible() &&
      open_tabs_section_->bounds().Contains(point)) {
    return open_tabs_section_;
  }
  // TODO(astra): Add workspace section when it's implemented as a list.
  // The workspace switcher currently only shows the active workspace.
  return nullptr;
}

AstraSidebarSectionId AstraSidebarView::GetSectionIdForType(
    AstraSidebarSectionType type) const {
  switch (type) {
    case AstraSidebarSectionType::kFavorites:
      return AstraSidebarSectionId::kFavorites;
    case AstraSidebarSectionType::kPinnedTabs:
      return AstraSidebarSectionId::kPinnedTabs;
    case AstraSidebarSectionType::kOpenTabs:
      return AstraSidebarSectionId::kOpenTabs;
    case AstraSidebarSectionType::kWorkspaces:
      return AstraSidebarSectionId::kWorkspaces;
  }
  return AstraSidebarSectionId::kUnknown;
}

AstraSidebarDropResult AstraSidebarView::ValidateDrop(
    AstraSidebarSectionType target_section,
    const AstraSidebarDragData& drag_data,
    int y_in_section) const {
  AstraSidebarDropResult result;
  result.is_valid = false;
  result.insert_index = -1;

  // Only tab items can be dropped in tab sections.
  if (drag_data.item_type != AstraSidebarDragItemType::kTab) {
    return result;
  }

  AstraSidebarSectionId target_id = GetSectionIdForType(target_section);
  if (target_id == AstraSidebarSectionId::kUnknown) {
    return result;
  }

  // Get the target section view to compute insert index.
  AstraSidebarSectionView* section = nullptr;
  switch (target_section) {
    case AstraSidebarSectionType::kFavorites:
      section = favorites_section_;
      break;
    case AstraSidebarSectionType::kPinnedTabs:
      section = pinned_tabs_section_;
      break;
    case AstraSidebarSectionType::kOpenTabs:
      section = open_tabs_section_;
      break;
    default:
      break;
  }
  if (!section) {
    return result;
  }

  // Compute insertion index from y position.
  int insert_index = section->GetInsertIndexFromY(y_in_section);
  result.insert_index = insert_index;

  // TODO(astra): Validate that the tab at tab_index still exists.
  // The tab index may be stale if tabs were added/removed during drag.
  // We should use a stable tab identifier instead.
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)

  switch (target_section) {
    case AstraSidebarSectionType::kFavorites:
      // In incognito mode, favorite mutations are disabled.
      // Dropping a tab onto the favorites section would set is_favorite,
      // which is a no-op in incognito (read-only).  So we reject the drop.
      // See AstraIncognitoHandler::AreFavoritesMutable.
      if (is_incognito_) {
        result.is_valid = false;
        break;
      }
      // Any tab can be added to favorites by dropping here.
      // This is a valid drop target.
      result.is_valid = true;
      // TODO(astra): If dropping onto a folder item, set target_folder_id
      // to that folder's id to indicate "drop into folder" vs "reorder".
      break;

    case AstraSidebarSectionType::kPinnedTabs:
      // Tab can be pinned by dropping here.
      // TODO(astra): Validate that the tab is not already pinned.
      // For now, allow the drop — the service will handle duplicates.
      result.is_valid = true;
      break;

    case AstraSidebarSectionType::kOpenTabs:
      // Can reorder within open tabs, or move tabs between workspaces.
      // Always valid as a drop target for tabs.
      result.is_valid = true;
      // TODO(astra): Validate same-workspace or cross-workspace rules.
      break;

    case AstraSidebarSectionType::kWorkspaces:
      // Dropping a tab onto a workspace moves it to that workspace.
      // TODO(astra): Implement workspace section list UI.
      result.is_valid = false;
      break;
  }

  return result;
}

bool AstraSidebarView::ExecuteDrop(
    AstraSidebarSectionType target_section,
    const AstraSidebarDragData& drag_data,
    const AstraSidebarDropResult& drop_result) {
  if (!browser_ || !browser_->tab_strip_model()) {
    return false;
  }

  TabStripModel* tab_strip = browser_->tab_strip_model();
  int tab_index = drag_data.tab_index;

  // Validate the tab still exists.
  // TODO(astra): Use a stable tab identifier instead of an index.
  if (tab_index < 0 || tab_index >= tab_strip->GetTabCount()) {
    return false;
  }

  content::WebContents* web_contents = tab_strip->GetWebContentsAt(tab_index);
  if (!web_contents) {
    return false;
  }

  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);

  AstraSidebarSectionId target_id = GetSectionIdForType(target_section);

  // Handle different drop scenarios based on source and target sections.
  if (target_id == drag_data.source_section) {
    // -- Reorder within the same section --
    // TODO(astra): Decision: should sidebar reordering affect TabStripModel
    // order, or is sidebar order independent (Arc-style)?
    //
    // Options:
    //   A) Sidebar order = TabStripModel order (synchronized).
    //      Reordering in the sidebar reorders tabs in the tab strip.
    //      Pro: consistent between tab strip and sidebar.
    //      Con: mixing favorites/pinned with open tabs reordering gets complex.
    //
    //   B) Sidebar order is independent (Arc-style Spaces sidebar).
    //      The sidebar has its own ordering, stored in AstraTabFeatures metadata.
    //      TabStripModel order is unaffected.
    //      Pro: cleaner separation of concerns, matches Arc's UX.
    //      Con: two sources of truth for tab ordering.
    //
    // For now, we implement approach B (independent sidebar order) for
    // favorites and pinned tabs, and approach A (sync) for open tabs.
    // TODO(astra): Make this a user preference or finalize the design.
    // Chromium owner for tab reordering: TabStripModel::MoveWebContentsAt
    // Chromium owner for metadata: AstraTabFeatures

    if (target_section == AstraSidebarSectionType::kFavorites) {
      // Reorder favorites — update favorite_order_index on the tab.
      // TODO(astra): Implement proper reordering that shifts other
      // favorites' order_index values. For now, just set the index.
      // This should go through AstraFavoriteService::ReorderFavoritesInFolder.
      if (drop_result.insert_index >= 0) {
        features->set_favorite_order_index(drop_result.insert_index);
        // Notify that Astra features changed so the sidebar updates.
        OnTabAstraFeaturesChanged(tab_index);
      }
      return true;
    }

    if (target_section == AstraSidebarSectionType::kOpenTabs) {
      // Reorder open tabs — update TabStripModel order.
      // This approach syncs sidebar order with tab strip order.
      // TODO(astra): This is approach A. Decide on final approach.
      if (drop_result.insert_index >= 0) {
        // TODO(astra): Map sidebar insert index to TabStripModel index.
        // The sidebar open tabs section only shows tabs in the current
        // workspace, so the indices don't match TabStripModel indices.
        // For now, we use a no-op and mark as handled.
        //
        // The proper implementation would:
        // 1. Find the actual TabStripModel index for the target position.
        // 2. Call tab_strip->MoveWebContentsAt(from, to, false).
        //
        // Chromium subsystem: TabStripModel::MoveWebContentsAt
        // (chrome/browser/ui/tabs/tab_strip_model.h)
        return true;  // Handled (no-op for now).
      }
      return true;
    }

    if (target_section == AstraSidebarSectionType::kPinnedTabs) {
      // Reorder pinned tabs within the pinned section.
      // TODO(astra): Implement pinned tab reordering via TabStripModel.
      return true;
    }

    return true;
  }

  // -- Move between different sections --

  if (target_section == AstraSidebarSectionType::kFavorites) {
    // Add to favorites (or move within favorites if already a favorite).
    // Dispatch through AstraFavoriteService.
    // TODO(astra): Look up AstraFavoriteService from the profile.
    // For now, directly update AstraTabFeatures (not ideal — should go
    // through the service).
    if (!features->is_favorite()) {
      features->set_is_favorite(true);
      features->set_favorite_folder_id("root");
      OnTabAstraFeaturesChanged(tab_index);
    }
    return true;
  }

  if (target_section == AstraSidebarSectionType::kPinnedTabs) {
    // Pin the tab. Dispatch through TabStripModel.
    // TODO(astra): Use TabStripModel::SetTabPinned for pinning.
    // Chromium subsystem: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
    // Note: pinning changes TabStripModel order (pinned tabs move to front).
    if (!tab_strip->IsTabPinned(tab_index)) {
      // TODO(astra): tab_strip->SetTabPinned(tab_index, true);
      // The sidebar will update via TabStripModelObserver::OnTabPinnedStateChanged.
      // For now, we mark as sidebar pinned via AstraTabFeatures.
      features->set_sidebar_pinned(true);
      OnTabAstraFeaturesChanged(tab_index);
    }
    return true;
  }

  if (target_section == AstraSidebarSectionType::kOpenTabs) {
    // Move to open tabs section.
    // If the tab was a favorite, it stays a favorite (favorites appear in
    // both sections). If it was pinned, unpin it.
    // TODO(astra): Unpin the tab if it was pinned.
    // For now, this is a no-op since tabs are always in open tabs.
    return true;
  }

  // -- Cross-workspace drops --
  // TODO(astra): When dropping a tab onto a workspace item in the workspace
  // switcher/list, update the tab's workspace_id via AstraTabFeatures.
  // The tab will disappear from the current workspace's sidebar.
  //
  // Implementation steps:
  //   1. Identify the target workspace from the drop position.
  //   2. Call features->set_workspace_id(target_workspace_id).
  //   3. Notify AstraWorkspaceService observers.
  //   4. The sidebar updates via OnTabAstraFeaturesChanged.
  //
  // Chromium subsystem: none — this is Astra metadata.

  // -- Workspace reorder --
  // TODO(astra): When a workspace item is dragged to reorder workspaces,
  // call AstraWorkspaceService::ReorderWorkspaces with the new order.
  // The sidebar workspace switcher updates via OnWorkspaceAdded/Removed/Renamed.

  return false;  // Drop not handled.
}

void AstraSidebarView::UpdateDragGhost(const gfx::Point& mouse_position) {
  if (!drag_ghost_) {
    return;
  }

  // Position the ghost so the mouse is at the same relative position
  // within the ghost as it was in the original item.
  int x = mouse_position.x() - drag_offset_.x();
  int y = mouse_position.y() - drag_offset_.y();

  drag_ghost_->SetPosition(gfx::Point(x, y));
}

void AstraSidebarView::EndDragSession() {
  is_dragging_ = false;
  drag_source_item_ = nullptr;
  hover_section_ = nullptr;

  // Remove and destroy the drag ghost.
  if (drag_ghost_) {
    RemoveChildViewT(drag_ghost_);
    drag_ghost_ = nullptr;
  }

  // Reset drag data.
  current_drag_data_ = AstraSidebarDragData();
}

}  // namespace astra

// =========================================================================
// AstraSidebarHistoryView::Delegate
// =========================================================================
//
// Handle navigation actions from the history section.  All navigation is
// dispatched through Chromium's Browser API — the sidebar view is a pure
// projection and never owns WebContents or navigation state.

void AstraSidebarView::OpenHistoryURL(const GURL& url, bool in_new_tab) {
  if (!browser_ || !url.is_valid()) {
    return;
  }

  // TODO(astra): Use Browser::OpenURL with proper OpenURLParams for
  // consistent navigation behavior (including transition type, tab
  // strip model index, and disposition).
  //
  // For in_new_tab = false (default), we navigate the active tab.
  // For in_new_tab = true, we open a new foreground tab.
  //
  // Chromium API: Browser::OpenURL (chrome/browser/ui/browser.h)
  // Chromium struct: OpenURLParams (content/public/browser/open_url_params.h)
  //
  // Patch point: None needed — uses standard Chromium API.
  if (in_new_tab) {
    // Open in a new foreground tab.
    // TODO(astra): Use NavigateParams for proper browser navigation.
    // content::OpenURLParams params(url, content::Referrer(),
    //     WindowOpenDisposition::NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_AUTO_BOOKMARK, false);
    // browser_->OpenURL(params);
    //
    // For now, navigate the active tab as a placeholder.
    content::WebContents* active_contents =
        browser_->tab_strip_model()->GetActiveWebContents();
    if (active_contents) {
      active_contents->GetController().LoadURL(
          url, content::Referrer(), ui::PAGE_TRANSITION_AUTO_BOOKMARK,
          std::string());
    }
  } else {
    // Navigate the active tab to the history URL.
    content::WebContents* active_contents =
        browser_->tab_strip_model()->GetActiveWebContents();
    if (active_contents) {
      active_contents->GetController().LoadURL(
          url, content::Referrer(), ui::PAGE_TRANSITION_AUTO_BOOKMARK,
          std::string());
    }
  }
}

void AstraSidebarView::OpenFullHistory() {
  if (!browser_) {
    return;
  }

  // Open chrome://history in a new foreground tab.
  // TODO(astra): Use Browser::OpenURL with OpenURLParams for proper
  // WebUI page navigation.  chrome://history is a WebUI page managed by
  // Chromium's HistoryUI.
  //
  // Chromium WebUI: chrome/browser/ui/webui/history/history_ui.cc
  // Chromium owner: HistoryUI (chrome/browser/ui/webui/history/)
  //
  // Patch point: None needed — uses standard WebUI navigation.
  GURL history_url("chrome://history");
  content::WebContents* active_contents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (active_contents) {
    active_contents->GetController().LoadURL(
        history_url, content::Referrer(), ui::PAGE_TRANSITION_AUTO_BOOKMARK,
        std::string());
  }
}

void AstraSidebarView::RefreshHistorySection() {
  if (history_section_) {
    history_section_->Refresh();
  }
}

// =========================================================================
// AstraSidebarRecentlyClosedView::Delegate
// =========================================================================
//
// Handle tab restore actions from the recently closed section.  All restore
// operations are dispatched through Chromium's TabRestoreService via
// AstraRecentTabsHelper — the sidebar view is a pure projection and never
// owns session restore state.

void AstraSidebarView::RestoreRecentlyClosedTab(int entry_id) {
  if (!browser_ || !browser_->profile()) {
    return;
  }

  // Restore the tab via the helper (which delegates to TabRestoreService).
  // The restored tab will appear in the tab strip, and the sidebar will
  // update via TabStripModelObserver.
  //
  // Chromium subsystem: sessions::TabRestoreService::RestoreEntryById
  //   (chrome/browser/sessions/tab_restore_service.h)
  //
  // TODO(astra): Also refresh the recently closed section explicitly so
  // the restored tab is removed from the list immediately, instead of
  // waiting for the next TabRestoreServiceObserver notification.
  content::WebContents* restored =
      AstraRecentTabsHelper::RestoreTabById(browser_->profile(), entry_id);

  // If the restore succeeded, refresh the recently closed section to
  // remove the restored entry from the list.
  if (restored) {
    RefreshRecentlyClosedSection();
  }
}

void AstraSidebarView::RestoreAllRecentlyClosedTabs() {
  if (!browser_ || !browser_->profile()) {
    return;
  }

  // Restore all recently closed tabs.
  // Dispatches through AstraRecentTabsHelper, which calls TabRestoreService.
  size_t restored = AstraRecentTabsHelper::RestoreAll(browser_->profile());

  // Refresh the section to update the list.
  if (restored > 0) {
    RefreshRecentlyClosedSection();
  }
}

void AstraSidebarView::RefreshRecentlyClosedSection() {
  if (recently_closed_section_) {
    recently_closed_section_->Refresh();
  }
}

// =========================================================================
// AstraSidebarModelObserver implementation
// =========================================================================
//
// The sidebar view observes the model for state changes and updates its
// presentation accordingly.  All changes originate from the model — the
// view never mutates model state directly.  User actions on the view are
// dispatched through the controller or model methods, which then notify
// observers (including this view).

void AstraSidebarView::OnSidebarShown() {
  // The sidebar was shown — make the view visible and refresh data.
  SetVisible(true);
  UpdateFromModel();
}

void AstraSidebarView::OnSidebarHidden() {
  // The sidebar was hidden — hide the view.
  SetVisible(false);
}

void AstraSidebarView::OnSidebarPinnedChanged(bool pinned) {
  // Pin state changed — update visual treatment and layout.
  // TODO(astra): Add visual distinction between pinned and overlay mode.
  InvalidateLayout();
}

void AstraSidebarView::OnActiveSectionChanged(const std::string& section_id) {
  // Active section changed — update section header highlighting.
  // TODO(astra): Highlight the active section header and scroll to it.
  InvalidateLayout();
}

void AstraSidebarView::OnSectionVisibilityChanged(
    const std::string& section_id,
    bool visible) {
  // A section's visibility changed — update the section view.
  // TODO(astra): Map section_id to section view and update visibility.
  InvalidateLayout();
}

void AstraSidebarView::OnSectionOrderChanged() {
  // Section order changed — rebuild the section layout.
  // TODO(astra): Reorder section views instead of full rebuild.
  UpdateFromModel();
}

void AstraSidebarView::OnSectionCollapsedChanged(
    const std::string& section_id,
    bool collapsed) {
  // A section's collapsed state changed — update the section view.
  // TODO(astra): Map section_id to section view and update collapsed state.
  InvalidateLayout();
}

void AstraSidebarView::OnSidebarWidthChanged(int width) {
  // Sidebar width changed — update preferred size and relayout.
  SetPreferredSize(gfx::Size(width, height()));
  InvalidateLayout();
}

void AstraSidebarView::OnSidebarPositionChanged(AstraSidebarPosition position) {
  // Sidebar position (left/right) changed.
  // Actual repositioning is handled by the parent BrowserView.
  // TODO(astra): Update position-dependent styling (e.g., inner border).
  InvalidateLayout();
}

void AstraSidebarView::OnSidebarSettingsChanged() {
  // Any presentation setting changed — refresh visual appearance.
  // This covers compact mode, show icons, show labels, etc.
  OnThemeChanged();
  InvalidateLayout();
}

void AstraSidebarView::OnCompactModeChanged(bool compact) {
  // Compact mode toggled — update item views to show/hide labels.
  // TODO(astra): Iterate through all section item views and update their
  //   compact mode state. For now, rely on OnSidebarSettingsChanged.
  InvalidateLayout();
}

void AstraSidebarView::OnAutoHideModeChanged(AstraSidebarAutoHideMode mode) {
  // Auto-hide mode changed — update sidebar behavior.
  if (mode == AstraSidebarAutoHideMode::kDisabled) {
    CancelAutoHideTimer();
    if (model() && !model()->is_visible()) {
      model()->SetVisible(true);
    }
  }
  InvalidateLayout();
}

void AstraSidebarView::OnWidthPresetChanged(AstraSidebarWidthPreset preset) {
  // Width preset changed — the actual width update is already handled
  // by OnSidebarWidthChanged. This is just an additional notification
  // for views that care about the preset classification.
  InvalidateLayout();
}

void AstraSidebarView::OnSectionBadgeChanged(const std::string& section_id) {
  // A section's badge changed — update the corresponding section view.
  // TODO(astra): Map section_id to section view and update the badge.
  //   For now, just invalidate layout.
  InvalidateLayout();
}

void AstraSidebarView::OnSectionAddButtonChanged(const std::string& section_id,
                                                  bool visible) {
  // A section's add button visibility changed.
  // TODO(astra): Map section_id to section view and update the add button.
  //   For now, just invalidate layout.
  InvalidateLayout();
}

}  // namespace astra
