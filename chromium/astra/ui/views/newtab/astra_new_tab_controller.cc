#include "astra/ui/views/newtab/astra_new_tab_controller.h"

#include <string>
#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "third_party/skia/include/core/SkColor.h"
#include "url/gurl.h"

#include "astra/browser/astra_new_tab_page_service.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/views/newtab/astra_new_tab_view.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraNewTabController::AstraNewTabController(Browser* browser,
                                             AstraNewTabView* view,
                                             Delegate* delegate)
    : browser_(browser), view_(view), delegate_(delegate) {
  DCHECK(view_);

  if (browser_) {
    profile_ = browser_->profile();
    if (profile_) {
      prefs_ = profile_->GetPrefs();
      ntp_service_ =
          AstraNewTabPageServiceFactory::GetForProfile(profile_);
      workspace_service_ =
          AstraWorkspaceServiceFactory::GetForProfile(profile_);
    }
  }

  // Load initial state from prefs.
  if (prefs_) {
    model_.LoadFromPrefs(prefs_);
  }

  // Observe the model for changes.
  model_.AddObserver(this);

  // Load data from services and update the view.
  RefreshFromServices();
}

AstraNewTabController::~AstraNewTabController() {
  model_.RemoveObserver(this);

  // Save state to prefs on destruction.
  if (prefs_) {
    SaveToPrefs();
  }
}

// =========================================================================
// Data loading
// =========================================================================

void AstraNewTabController::RefreshFromServices() {
  LoadShortcutsFromService();
  LoadWorkspacesFromService();
  LoadQuickActions();
  LoadRecentlyClosedFromService();
  UpdateViewFromModel();
}

void AstraNewTabController::LoadShortcutsFromService() {
  if (!ntp_service_) {
    return;
  }

  // Get combined list of custom + most-visited shortcuts.
  size_t count = static_cast<size_t>(model_.shortcut_columns() * 2);
  auto shortcuts = ntp_service_->GetAllShortcuts(count);

  std::vector<AstraNtpShortcutInfo> infos;
  infos.reserve(shortcuts.size());
  for (size_t i = 0; i < shortcuts.size(); ++i) {
    AstraNtpShortcutInfo info;
    info.title = shortcuts[i].title;
    info.url = shortcuts[i].url;
    info.icon_url = shortcuts[i].favicon_url;
    info.is_custom = !shortcuts[i].is_most_visited;
    info.order_index = static_cast<int>(i);
    infos.push_back(info);
  }

  // Set directly on the model without triggering save (these are projected
  // from services, not user-edited state that needs persistence).
  model_.SetShortcuts(std::move(infos));
}

void AstraNewTabController::LoadWorkspacesFromService() {
  if (!ntp_service_) {
    return;
  }

  auto summaries = ntp_service_->GetWorkspaceSummaries();

  std::vector<AstraNtpWorkspaceCardInfo> cards;
  cards.reserve(summaries.size());
  for (size_t i = 0; i < summaries.size(); ++i) {
    AstraNtpWorkspaceCardInfo card;
    card.id = summaries[i].id;
    card.name = base::UTF8ToUTF16(summaries[i].name);
    card.accent_color_hex = summaries[i].accent_color;
    card.tab_count = summaries[i].tab_count;
    card.is_active = summaries[i].is_active;
    card.order_index = static_cast<int>(i);

    // Parse accent color.
    // TODO(astra): Move color parsing to a shared utility.
    SkColor accent = SK_ColorTRANSPARENT;
    if (!summaries[i].accent_color.empty() &&
        summaries[i].accent_color.size() == 7 &&
        summaries[i].accent_color[0] == '#') {
      unsigned int color_val = 0;
      if (base::HexStringToUInt(summaries[i].accent_color.substr(1),
                                &color_val)) {
        accent = SkColorSetRGB(SkColorGetR(color_val),
                              SkColorGetG(color_val),
                              SkColorGetB(color_val));
      }
    }
    card.accent_color = accent;

    cards.push_back(card);
  }

  model_.SetWorkspaceCards(std::move(cards));
}

void AstraNewTabController::LoadQuickActions() {
  // Default quick actions.
  // TODO(astra): Load from prefs / service for user-customizable quick actions.
  std::vector<AstraNtpQuickAction> actions = {
      {"new_workspace", u"New workspace", u"+", true, 0},
      {"screenshot", u"Screenshot", u"📷", true, 1},
      {"focus_mode", u"Focus mode", u"🔒", true, 2},
      {"history", u"History", u"📜", true, 3},
      {"downloads", u"Downloads", u"⬇", true, 4},
      {"bookmarks", u"Bookmarks", u"⭐", true, 5},
  };

  model_.SetQuickActions(std::move(actions));
}

void AstraNewTabController::LoadRecentlyClosedFromService() {
  // TODO(astra): Load from TabRestoreService / sessions::TabRestoreService.
  // For now, the recently closed list is populated from the model's
  // cached data or starts empty.
}

// =========================================================================
// User action handlers
// =========================================================================

void AstraNewTabController::OnShortcutClicked(const GURL& url) {
  if (delegate_) {
    delegate_->OnNavigateToURL(url);
  }
}

void AstraNewTabController::OnShortcutRemove(const GURL& url) {
  model_.RemoveShortcutByUrl(url);
  SaveToPrefs();
}

void AstraNewTabController::OnShortcutEdit(const GURL& url,
                                           const std::u16string& new_title,
                                           const GURL& new_url) {
  int index = model_.FindShortcutByUrl(url);
  if (index >= 0) {
    model_.UpdateShortcutAt(static_cast<size_t>(index), new_title, new_url);
    SaveToPrefs();
  }
}

void AstraNewTabController::OnShortcutReordered(size_t from_index,
                                                size_t to_index) {
  model_.MoveShortcut(from_index, to_index);
  SaveToPrefs();
}

void AstraNewTabController::OnShortcutAdd(const std::u16string& title,
                                          const GURL& url) {
  model_.AddCustomShortcut(title, url);
  SaveToPrefs();
}

void AstraNewTabController::OnWorkspaceClicked(const std::string& workspace_id) {
  if (workspace_id.empty()) {
    if (delegate_) {
      delegate_->OnNewWorkspace();
    }
  } else {
    if (delegate_) {
      delegate_->OnOpenWorkspace(workspace_id);
    }
  }
}

void AstraNewTabController::OnWorkspaceMenu(const std::string& workspace_id,
                                            const gfx::Point& screen_point) {
  if (delegate_ && !workspace_id.empty()) {
    delegate_->OnShowWorkspaceContextMenu(workspace_id, screen_point);
  }
}

void AstraNewTabController::OnQuickAction(const std::string& action_id) {
  if (delegate_) {
    delegate_->OnQuickAction(action_id);
  }
}

void AstraNewTabController::OnRecentlyClosedClicked(int session_id) {
  if (delegate_) {
    delegate_->OnRestoreRecentlyClosed(session_id);
  }
}

// =========================================================================
// Settings actions
// =========================================================================

void AstraNewTabController::ToggleGreeting() {
  model_.set_show_greeting(!model_.show_greeting());
  SaveToPrefs();
}

void AstraNewTabController::ToggleSearchBar() {
  model_.set_show_search_bar(!model_.show_search_bar());
  SaveToPrefs();
}

void AstraNewTabController::ToggleWorkspaceCards() {
  model_.set_show_workspace_cards(!model_.show_workspace_cards());
  SaveToPrefs();
}

void AstraNewTabController::ToggleShortcuts() {
  model_.set_show_shortcuts(!model_.show_shortcuts());
  SaveToPrefs();
}

void AstraNewTabController::ToggleRecentlyClosed() {
  model_.set_show_recently_closed(!model_.show_recently_closed());
  SaveToPrefs();
}

void AstraNewTabController::ToggleQuickActions() {
  model_.set_show_quick_actions(!model_.show_quick_actions());
  SaveToPrefs();
}

void AstraNewTabController::ToggleMostVisited() {
  model_.set_show_most_visited(!model_.show_most_visited());
  SaveToPrefs();
  // Reload shortcuts to reflect the change.
  LoadShortcutsFromService();
}

void AstraNewTabController::SetShortcutColumns(int columns) {
  model_.set_shortcut_columns(columns);
  SaveToPrefs();
}

void AstraNewTabController::SetMaxWorkspacesShown(int max) {
  model_.set_max_workspaces_shown(max);
  SaveToPrefs();
}

void AstraNewTabController::SetMaxRecentlyClosedShown(int max) {
  model_.set_max_recently_closed_shown(max);
  SaveToPrefs();
}

void AstraNewTabController::SetShortcutLayoutMode(
    AstraNtpShortcutLayoutMode mode) {
  model_.set_shortcut_layout_mode(mode);
  SaveToPrefs();
}

void AstraNewTabController::SetWorkspaceCardStyle(
    AstraNtpWorkspaceCardStyle style) {
  model_.set_workspace_card_style(style);
  SaveToPrefs();
}

void AstraNewTabController::SetBackgroundStyle(AstraNtpBackgroundStyle style) {
  model_.set_background_style(style);
  SaveToPrefs();
}

void AstraNewTabController::SetCustomBackgroundUrl(const std::string& url) {
  model_.set_custom_background_url(url);
  SaveToPrefs();
}

void AstraNewTabController::SetGreetingStyle(AstraNtpGreetingStyle style) {
  model_.set_greeting_style(style);
  SaveToPrefs();
}

// =========================================================================
// AstraNewTabModelObserver
// =========================================================================

void AstraNewTabController::OnShortcutsChanged() {
  if (view_) {
    // Update the view's shortcut section.
    // TODO(astra): Add a method on AstraNewTabView to update shortcuts
    // from model data.  For now, we rely on full refresh.
  }
}

void AstraNewTabController::OnWorkspacesChanged() {
  if (view_) {
    // Update the view's workspace section.
  }
}

void AstraNewTabController::OnQuickActionsChanged() {
  if (view_) {
    // Update the view's quick actions section.
  }
}

void AstraNewTabController::OnRecentlyClosedChanged() {
  if (view_) {
    // Update the view's recently closed section.
  }
}

void AstraNewTabController::OnNtpSettingsChanged() {
  if (view_) {
    // Update the view's layout based on new settings.
  }
}

void AstraNewTabController::OnThemeChanged() {
  if (view_) {
    view_->OnThemeChanged();
  }
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraNewTabController::SaveToPrefs() {
  if (prefs_) {
    model_.SaveToPrefs(prefs_);
  }
}

void AstraNewTabController::UpdateViewFromModel() {
  if (!view_) {
    return;
  }
  // TODO(astra): Implement fine-grained view updates from model state.
  // For now, we trigger a full refresh of the view content.
  view_->RefreshContent();
  view_->UpdateFromSettings();
}

// =========================================================================
// AstraNewTabView::Delegate — view action handlers
// =========================================================================

void AstraNewTabController::OnNavigateToURL(const GURL& url) {
  // Forward to delegate for actual navigation.
  if (delegate_) {
    delegate_->OnNavigateToURL(url);
  }
}

void AstraNewTabController::OnOpenWorkspace(const std::string& workspace_id) {
  OnWorkspaceClicked(workspace_id);
}

void AstraNewTabController::OnNewWorkspace() {
  OnWorkspaceClicked(std::string());
}

void AstraNewTabController::OnShowAllWorkspaces() {
  if (delegate_) {
    delegate_->OnShowAllWorkspaces();
  }
}

void AstraNewTabController::OnRestoreRecentlyClosed(int session_id) {
  OnRecentlyClosedClicked(session_id);
}

void AstraNewTabController::OnRemoveShortcut(const GURL& url) {
  OnShortcutRemove(url);
}

void AstraNewTabController::OnShowShortcutContextMenu(
    const GURL& url,
    const gfx::Point& screen_point) {
  if (delegate_) {
    delegate_->OnShowShortcutContextMenu(url, screen_point);
  }
}

void AstraNewTabController::OnShowWorkspaceContextMenu(
    const std::string& workspace_id,
    const gfx::Point& screen_point) {
  OnWorkspaceMenu(workspace_id, screen_point);
}

void AstraNewTabController::OnSettingsGearPressed() {
  if (delegate_) {
    delegate_->OnSettingsGearPressed();
  }
}

void AstraNewTabController::OnToggleGreeting() {
  ToggleGreeting();
}

void AstraNewTabController::OnToggleShortcuts() {
  ToggleShortcuts();
}

void AstraNewTabController::OnShortcutColumnsChanged(int columns) {
  SetShortcutColumns(columns);
}

void AstraNewTabController::OnShortcutLayoutModeChanged(int mode) {
  SetShortcutLayoutMode(static_cast<AstraNtpShortcutLayoutMode>(mode));
}

void AstraNewTabController::OnBackgroundStyleChanged(int style) {
  SetBackgroundStyle(static_cast<AstraNtpBackgroundStyle>(style));
}

}  // namespace astra
