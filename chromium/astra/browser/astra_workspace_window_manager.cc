#include "astra/browser/astra_workspace_window_manager.h"

#include <algorithm>
#include <cmath>

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"

#include "astra/browser/astra_incognito_handler.h"
#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_window_features.h"
#include "astra/browser/astra_workspace_service.h"

namespace astra {

// ---------------------------------------------------------------------------
// AstraWorkspaceWindowManager
// ---------------------------------------------------------------------------

AstraWorkspaceWindowManager::AstraWorkspaceWindowManager() {
  // Observe BrowserList for window add/remove events.
  //
  // Chromium owner: BrowserList (chrome/browser/ui/browser_list.h)
  // BrowserList is a singleton that owns all Browser windows.
  // We observe it to track when windows are created and destroyed.
  BrowserList::AddObserver(this);
}

AstraWorkspaceWindowManager::~AstraWorkspaceWindowManager() {
  BrowserList::RemoveObserver(this);
}

// static
AstraWorkspaceWindowManager* AstraWorkspaceWindowManager::GetInstance() {
  static base::NoDestructor<AstraWorkspaceWindowManager> instance;
  return instance.get();
}

// -- Observers ---------------------------------------------------------------

void AstraWorkspaceWindowManager::AddObserver(
    AstraWorkspaceWindowManagerObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraWorkspaceWindowManager::RemoveObserver(
    AstraWorkspaceWindowManagerObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Window-to-workspace query ------------------------------------------------

std::vector<Browser*> AstraWorkspaceWindowManager::GetWindowsForWorkspace(
    Profile* profile,
    const std::string& workspace_id) const {
  std::vector<Browser*> result;

  if (!profile) {
    return result;
  }

  // Iterate all browsers and filter by profile + workspace_id.
  //
  // Chromium owner: BrowserList — we iterate all Browser windows.
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (!BrowserBelongsToProfile(browser, profile)) {
      continue;
    }

    std::string ws_id = GetWorkspaceForWindow(browser);
    if (ws_id == workspace_id) {
      result.push_back(browser);
    }
  }

  return result;
}

std::vector<Browser*> AstraWorkspaceWindowManager::GetWindowsInWorkspace(
    Profile* profile,
    const std::string& workspace_id) const {
  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);

  // Sort by order_index (ascending).
  // Windows with lower order_index come first in the list.
  std::sort(windows.begin(), windows.end(),
            [](Browser* a, Browser* b) {
              AstraWindowFeatures* fa = AstraWindowFeatures::FromBrowser(a);
              AstraWindowFeatures* fb = AstraWindowFeatures::FromBrowser(b);
              size_t ia = fa ? fa->order_index() : 0;
              size_t ib = fb ? fb->order_index() : 0;
              return ia < ib;
            });

  return windows;
}

std::string AstraWorkspaceWindowManager::GetWorkspaceForWindow(
    Browser* browser) const {
  if (!browser) {
    return "default";
  }

  AstraWindowFeatures* features = AstraWindowFeatures::FromBrowser(browser);
  if (!features) {
    return "default";
  }
  return features->workspace_id();
}

size_t AstraWorkspaceWindowManager::GetWindowCount(
    Profile* profile,
    const std::string& workspace_id) const {
  return GetWindowsForWorkspace(profile, workspace_id).size();
}

size_t AstraWorkspaceWindowManager::GetWindowCountInWorkspace(
    Profile* profile,
    const std::string& workspace_id) const {
  return GetWindowCount(profile, workspace_id);
}

size_t AstraWorkspaceWindowManager::GetTabCount(
    Profile* profile,
    const std::string& workspace_id) const {
  size_t count = 0;

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* browser : windows) {
    if (browser && browser->tab_strip_model()) {
      count += browser->tab_strip_model()->count();
    }
  }

  return count;
}

size_t AstraWorkspaceWindowManager::GetTabCountInWorkspace(
    Profile* profile,
    const std::string& workspace_id) const {
  return GetTabCount(profile, workspace_id);
}

Browser* AstraWorkspaceWindowManager::GetActiveWindowInWorkspace(
    Profile* profile,
    const std::string& workspace_id) const {
  if (!profile) {
    return nullptr;
  }

  // Find the first window in the workspace that is currently active (focused).
  //
  // Chromium owner: BrowserWindow::IsActive()
  //   (chrome/browser/ui/browser_window.h)
  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);

  // Prefer the last-active window from BrowserList ordering, which is typically
  // most-recently-focused first.
  for (Browser* browser : windows) {
    if (browser && browser->window() && browser->window()->IsActive()) {
      return browser;
    }
  }

  // If no window is currently active, return the first window (or nullptr).
  return windows.empty() ? nullptr : windows[0];
}

// -- Window manipulation -----------------------------------------------------

void AstraWorkspaceWindowManager::MoveWindowToWorkspace(
    Browser* browser,
    const std::string& workspace_id) {
  if (!browser) {
    return;
  }

  AstraWindowFeatures* features =
      AstraWindowFeatures::GetOrCreateForBrowser(browser);
  DCHECK(features);

  if (features->workspace_read_only()) {
    // Incognito windows cannot be moved between workspaces.
    return;
  }

  if (features->workspace_id() == workspace_id) {
    return;
  }

  std::string old_id = features->workspace_id();
  features->set_workspace_id(workspace_id);

  // Assign a default order index at the end of the target workspace's list.
  AssignDefaultOrderIndex(browser);

  // Notify observers that the window has moved.
  for (auto& observer : observers_) {
    observer.OnWindowRemovedFromWorkspace(browser, old_id);
  }
  for (auto& observer : observers_) {
    observer.OnWindowAddedToWorkspace(browser, workspace_id);
  }

  // Notify count changes for both workspaces.
  NotifyWindowCountChanged(browser->profile(), old_id);
  NotifyWindowCountChanged(browser->profile(), workspace_id);
}

void AstraWorkspaceWindowManager::MoveAllTabsToWorkspace(
    Profile* profile,
    const std::string& source_workspace_id,
    const std::string& target_workspace_id) {
  if (!profile || source_workspace_id == target_workspace_id) {
    return;
  }

  // Iterate all windows in the source workspace, and for each tab,
  // update its workspace_id to the target workspace.
  //
  // Chromium owner: TabStripModel — owns all tabs.
  // Astra only updates the per-tab workspace metadata projection.
  //
  // TODO(astra): When actually moving tabs between workspaces (not just
  //   updating metadata), we should use TabStripModel::DetachWebContentsAt
  //   and InsertWebContentsAt to physically move tabs between windows.
  //   For now, this is a metadata-level operation.
  std::vector<Browser*> windows = GetWindowsForWorkspace(profile,
                                                        source_workspace_id);
  for (Browser* browser : windows) {
    if (!browser || !browser->tab_strip_model()) {
      continue;
    }

    TabStripModel* model = browser->tab_strip_model();
    for (int i = 0; i < model->count(); ++i) {
      content::WebContents* web_contents = model->GetWebContentsAt(i);
      if (!web_contents) {
        continue;
      }

      AstraTabFeatures* features =
          AstraTabFeatures::GetOrCreateForWebContents(web_contents);
      if (features) {
        features->set_workspace_id(target_workspace_id);
      }
    }
  }
}

Browser* AstraWorkspaceWindowManager::CreateNewWindowInWorkspace(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return nullptr;
  }

  // Create a new Browser window using Chromium's Browser::Create().
  //
  // Chromium owner: Browser (chrome/browser/ui/browser.h)
  // Browser::Create() creates a new top-level browser window with a
  // TabStripModel and an empty tab.
  //
  // TODO(astra): Use Browser::CreateParams with the appropriate
  // configuration (type, profile, initial bounds, etc.).
  // Chromium component: Browser::CreateParams.
  Browser::CreateParams params(profile, true /* user_gesture */);
  Browser* browser = Browser::Create(params);
  DCHECK(browser);

  // Assign the window to the workspace.
  AstraWindowFeatures* features =
      AstraWindowFeatures::GetOrCreateForBrowser(browser);
  if (!features->workspace_read_only()) {
    features->set_workspace_id(workspace_id);
  }

  // Assign default order index (at the end of the workspace's window list).
  AssignDefaultOrderIndex(browser);

  // Show the window.
  browser->window()->Show();

  return browser;
}

// -- Window ordering ---------------------------------------------------------

bool AstraWorkspaceWindowManager::ReorderWindowsInWorkspace(
    Profile* profile,
    const std::string& workspace_id,
    const std::vector<Browser*>& ordered_windows) {
  if (!profile) {
    return false;
  }

  // Get current windows in the workspace to verify the list is complete.
  std::vector<Browser*> current = GetWindowsForWorkspace(profile, workspace_id);

  // Verify all current windows are in the ordered list and vice versa.
  if (current.size() != ordered_windows.size()) {
    return false;
  }

  // Verify every window in ordered_windows belongs to this workspace and profile.
  for (Browser* browser : ordered_windows) {
    if (!browser || !BrowserBelongsToProfile(browser, profile)) {
      return false;
    }
    if (GetWorkspaceForWindow(browser) != workspace_id) {
      return false;
    }
  }

  // Assign order indices based on position in the list.
  for (size_t i = 0; i < ordered_windows.size(); ++i) {
    AstraWindowFeatures* features =
        AstraWindowFeatures::FromBrowser(ordered_windows[i]);
    if (features) {
      features->set_order_index(i);
    }
  }

  return true;
}

// -- Bulk operations ---------------------------------------------------------

void AstraWorkspaceWindowManager::CloseAllWindowsInWorkspace(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  // Collect all windows first — we can't iterate while closing because
  // closing windows modifies BrowserList.
  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);

  // Close each window.  Actual closing is done by Chromium.
  //
  // Chromium owner: BrowserWindow::Close()
  //   (chrome/browser/ui/browser_window.h)
  // Note: window closing is asynchronous and may be cancelled by the user
  // (e.g., beforeunload handlers).
  for (Browser* browser : windows) {
    if (browser && browser->window()) {
      browser->window()->Close();
    }
  }
}

void AstraWorkspaceWindowManager::MoveAllWindowsToWorkspace(
    Profile* profile,
    const std::string& source_workspace_id,
    const std::string& target_workspace_id) {
  if (!profile || source_workspace_id == target_workspace_id) {
    return;
  }

  // Collect all windows first since MoveWindowToWorkspace may modify
  // iteration state.
  std::vector<Browser*> windows =
      GetWindowsForWorkspace(profile, source_workspace_id);

  for (Browser* browser : windows) {
    MoveWindowToWorkspace(browser, target_workspace_id);
  }
}

// -- Stats -------------------------------------------------------------------

size_t AstraWorkspaceWindowManager::GetTotalWindowCount(
    Profile* profile) const {
  if (!profile) {
    return 0;
  }

  size_t count = 0;
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (BrowserBelongsToProfile(browser, profile)) {
      ++count;
    }
  }
  return count;
}

size_t AstraWorkspaceWindowManager::GetTotalTabCount(
    Profile* profile) const {
  if (!profile) {
    return 0;
  }

  size_t count = 0;
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (BrowserBelongsToProfile(browser, profile) &&
        browser->tab_strip_model()) {
      count += browser->tab_strip_model()->count();
    }
  }
  return count;
}

double AstraWorkspaceWindowManager::GetAverageTabsPerWindow(
    Profile* profile) const {
  size_t window_count = GetTotalWindowCount(profile);
  if (window_count == 0) {
    return 0.0;
  }
  return static_cast<double>(GetTotalTabCount(profile)) /
         static_cast<double>(window_count);
}

double AstraWorkspaceWindowManager::GetAverageTabsPerWindowInWorkspace(
    Profile* profile,
    const std::string& workspace_id) const {
  size_t window_count = GetWindowCount(profile, workspace_id);
  if (window_count == 0) {
    return 0.0;
  }
  return static_cast<double>(GetTabCount(profile, workspace_id)) /
         static_cast<double>(window_count);
}

// -- Hibernation -------------------------------------------------------------

void AstraWorkspaceWindowManager::HibernateWorkspace(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile || IsWorkspaceHibernated(profile, workspace_id)) {
    return;
  }

  // Mark all windows in the workspace as hibernated.
  //
  // Chromium owner: TabStripModel + WebContents discarding.
  // TODO(astra): Actually discard the WebContents for each tab to free memory.
  //   For now, this is a metadata-level operation that sets the hibernated
  //   flag on each window's AstraWindowFeatures.
  //   Chromium component: TabStripModel::DiscardWebContentsAt or
  //   memory::TabManager.
  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* browser : windows) {
    AstraWindowFeatures* features =
        AstraWindowFeatures::FromBrowser(browser);
    if (features) {
      features->set_is_hibernated(true);
    }
  }

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnWorkspaceHibernationChanged(workspace_id, true);
  }
}

void AstraWorkspaceWindowManager::WakeUpWorkspace(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile || !IsWorkspaceHibernated(profile, workspace_id)) {
    return;
  }

  // Unmark all windows in the workspace as hibernated.
  //
  // Chromium owner: TabStripModel — tabs reload when activated.
  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* browser : windows) {
    AstraWindowFeatures* features =
        AstraWindowFeatures::FromBrowser(browser);
    if (features) {
      features->set_is_hibernated(false);
    }
  }

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnWorkspaceHibernationChanged(workspace_id, false);
  }
}

bool AstraWorkspaceWindowManager::IsWorkspaceHibernated(
    Profile* profile,
    const std::string& workspace_id) const {
  if (!profile) {
    return false;
  }

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  if (windows.empty()) {
    return false;
  }

  // A workspace is hibernated if all of its windows are hibernated.
  for (Browser* browser : windows) {
    AstraWindowFeatures* features =
        AstraWindowFeatures::FromBrowser(browser);
    if (!features || !features->is_hibernated()) {
      return false;
    }
  }
  return true;
}

// -- Window arrangement ------------------------------------------------------

void AstraWorkspaceWindowManager::TileWindows(
    Profile* profile,
    const std::string& workspace_id,
    AstraTileDirection direction) {
  if (!profile) {
    return;
  }

  std::vector<Browser*> windows = GetWindowsInWorkspace(profile, workspace_id);
  if (windows.empty()) {
    return;
  }

  // Get the screen work area for the primary display.
  //
  // TODO(astra): Use the display where the first window currently lives
  //   instead of always using the primary display.
  //   Chromium component: display::Screen::GetDisplayNearestWindow().
  display::Screen* screen = display::Screen::GetScreen();
  gfx::Rect work_area = screen ? screen->GetPrimaryDisplay().work_area()
                                : gfx::Rect(0, 0, 1280, 800);

  int gap = GetTileGap(profile);
  int padding = GetTilePadding(profile);

  // Apply padding to the work area.
  work_area.Inset(padding, padding);

  size_t count = windows.size();

  if (direction == AstraTileDirection::kHorizontal) {
    // Split width equally among windows.
    int total_gap = static_cast<int>(count - 1) * gap;
    int window_width = (work_area.width() - total_gap) / static_cast<int>(count);
    int window_height = work_area.height();

    for (size_t i = 0; i < count; ++i) {
      int x = work_area.x() + static_cast<int>(i) * (window_width + gap);
      gfx::Rect bounds(x, work_area.y(), window_width, window_height);
      if (windows[i] && windows[i]->window()) {
        SaveWindowState(windows[i]);
        windows[i]->window()->SetBounds(bounds);
        windows[i]->window()->Restore();
      }
    }
  } else if (direction == AstraTileDirection::kVertical) {
    // Split height equally among windows.
    int total_gap = static_cast<int>(count - 1) * gap;
    int window_height = (work_area.height() - total_gap) / static_cast<int>(count);
    int window_width = work_area.width();

    for (size_t i = 0; i < count; ++i) {
      int y = work_area.y() + static_cast<int>(i) * (window_height + gap);
      gfx::Rect bounds(work_area.x(), y, window_width, window_height);
      if (windows[i] && windows[i]->window()) {
        SaveWindowState(windows[i]);
        windows[i]->window()->SetBounds(bounds);
        windows[i]->window()->Restore();
      }
    }
  } else if (direction == AstraTileDirection::kGrid) {
    // Arrange in a grid.  Calculate rows and columns to be as close to
    // square as possible.
    int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(count))));
    int rows = static_cast<int>(std::ceil(static_cast<double>(count) / static_cast<double>(cols)));

    int total_h_gap = (cols - 1) * gap;
    int total_v_gap = (rows - 1) * gap;
    int cell_width = (work_area.width() - total_h_gap) / cols;
    int cell_height = (work_area.height() - total_v_gap) / rows;

    for (size_t i = 0; i < count; ++i) {
      int col = static_cast<int>(i) % cols;
      int row = static_cast<int>(i) / cols;
      int x = work_area.x() + col * (cell_width + gap);
      int y = work_area.y() + row * (cell_height + gap);
      gfx::Rect bounds(x, y, cell_width, cell_height);
      if (windows[i] && windows[i]->window()) {
        SaveWindowState(windows[i]);
        windows[i]->window()->SetBounds(bounds);
        windows[i]->window()->Restore();
      }
    }
  }

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnWindowsArranged(workspace_id, direction);
  }
}

void AstraWorkspaceWindowManager::TileWindowsHorizontal(
    Profile* profile,
    const std::string& workspace_id) {
  TileWindows(profile, workspace_id, AstraTileDirection::kHorizontal);
}

void AstraWorkspaceWindowManager::TileWindowsVertical(
    Profile* profile,
    const std::string& workspace_id) {
  TileWindows(profile, workspace_id, AstraTileDirection::kVertical);
}

void AstraWorkspaceWindowManager::TileWindowsGrid(
    Profile* profile,
    const std::string& workspace_id) {
  TileWindows(profile, workspace_id, AstraTileDirection::kGrid);
}

void AstraWorkspaceWindowManager::StackWindows(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  std::vector<Browser*> windows = GetWindowsInWorkspace(profile, workspace_id);
  if (windows.empty()) {
    return;
  }

  // Use the first window's bounds as the stack bounds, or a default.
  gfx::Rect stack_bounds;
  if (windows[0] && windows[0]->window()) {
    stack_bounds = windows[0]->window()->GetBounds();
  } else {
    stack_bounds = gfx::Rect(100, 100, 1024, 768);
  }

  for (Browser* browser : windows) {
    if (browser && browser->window()) {
      SaveWindowState(browser);
      browser->window()->SetBounds(stack_bounds);
      browser->window()->Restore();
    }
  }

  // Notify observers (using kGrid as the direction for "stacked" —
  // could add a kStacked value later).
  //
  // TODO(astra): Add AstraTileDirection::kStacked for proper semantics.
  for (auto& observer : observers_) {
    observer.OnWindowsArranged(workspace_id, AstraTileDirection::kGrid);
  }
}

void AstraWorkspaceWindowManager::CascadeWindows(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  std::vector<Browser*> windows = GetWindowsInWorkspace(profile, workspace_id);
  if (windows.empty()) {
    return;
  }

  int offset = GetCascadeOffset(profile);
  gfx::Size window_size(1024, 768);

  // Get starting position from first window or use default.
  gfx::Point start_pos(100, 100);
  if (windows[0] && windows[0]->window()) {
    start_pos = windows[0]->window()->GetBounds().origin();
    window_size = windows[0]->window()->GetBounds().size();
  }

  for (size_t i = 0; i < windows.size(); ++i) {
    if (windows[i] && windows[i]->window()) {
      gfx::Rect bounds(
          start_pos.x() + static_cast<int>(i) * offset,
          start_pos.y() + static_cast<int>(i) * offset,
          window_size.width(),
          window_size.height());
      SaveWindowState(windows[i]);
      windows[i]->window()->SetBounds(bounds);
      windows[i]->window()->Restore();
    }
  }

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnWindowsArranged(workspace_id, AstraTileDirection::kHorizontal);
  }
}

void AstraWorkspaceWindowManager::MaximizeAllWindows(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* browser : windows) {
    if (browser && browser->window()) {
      SaveWindowState(browser);
      browser->window()->Maximize();
    }
  }

  for (auto& observer : observers_) {
    observer.OnAllWindowsStateChanged(workspace_id, /*is_maximized=*/true,
                                      /*is_minimized=*/false);
  }
}

void AstraWorkspaceWindowManager::MinimizeAllWindows(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* browser : windows) {
    if (browser && browser->window()) {
      SaveWindowState(browser);
      browser->window()->Minimize();
    }
  }

  for (auto& observer : observers_) {
    observer.OnAllWindowsStateChanged(workspace_id, /*is_maximized=*/false,
                                      /*is_minimized=*/true);
  }
}

void AstraWorkspaceWindowManager::RestoreAllWindows(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* browser : windows) {
    if (browser && browser->window()) {
      RestoreWindowState(browser);
    }
  }

  for (auto& observer : observers_) {
    observer.OnAllWindowsStateChanged(workspace_id, /*is_maximized=*/false,
                                      /*is_minimized=*/false);
  }
}

// -- Focus cycling -----------------------------------------------------------

Browser* AstraWorkspaceWindowManager::GetFocusedWindow(
    Profile* profile,
    const std::string& workspace_id) const {
  return GetActiveWindowInWorkspace(profile, workspace_id);
}

Browser* AstraWorkspaceWindowManager::FocusNextWindow(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return nullptr;
  }

  std::vector<Browser*> windows = GetWindowsInWorkspace(profile, workspace_id);
  if (windows.empty()) {
    return nullptr;
  }

  // Find the currently focused window's index.
  Browser* active = GetActiveWindowInWorkspace(profile, workspace_id);
  size_t next_index = 0;

  if (active) {
    for (size_t i = 0; i < windows.size(); ++i) {
      if (windows[i] == active) {
        next_index = (i + 1) % windows.size();
        break;
      }
    }
  }

  Browser* next = windows[next_index];
  if (next && next->window()) {
    next->window()->Show();
    next->window()->Activate();
  }

  for (auto& observer : observers_) {
    observer.OnWindowFocusChanged(workspace_id, next);
  }

  return next;
}

Browser* AstraWorkspaceWindowManager::FocusPreviousWindow(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return nullptr;
  }

  std::vector<Browser*> windows = GetWindowsInWorkspace(profile, workspace_id);
  if (windows.empty()) {
    return nullptr;
  }

  Browser* active = GetActiveWindowInWorkspace(profile, workspace_id);
  size_t prev_index = windows.size() - 1;

  if (active) {
    for (size_t i = 0; i < windows.size(); ++i) {
      if (windows[i] == active) {
        prev_index = (i == 0) ? windows.size() - 1 : i - 1;
        break;
      }
    }
  }

  Browser* prev = windows[prev_index];
  if (prev && prev->window()) {
    prev->window()->Show();
    prev->window()->Activate();
  }

  for (auto& observer : observers_) {
    observer.OnWindowFocusChanged(workspace_id, prev);
  }

  return prev;
}

// -- New window behavior -----------------------------------------------------

AstraNewWindowBehavior AstraWorkspaceWindowManager::GetNewWindowBehavior(
    Profile* profile) const {
  if (!profile) {
    return AstraNewWindowBehavior::kActiveWorkspace;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return AstraNewWindowBehavior::kActiveWorkspace;
  }

  // TODO(astra): Store behavior as an enum pref instead of deriving from
  //   boolean prefs.  For now, map from existing boolean prefs.
  //   Chromium component: PrefService with integer enum storage.
  bool in_active = prefs->GetBoolean(prefs::kPrefWorkspaceWindowNewInActiveWorkspace);
  return in_active ? AstraNewWindowBehavior::kActiveWorkspace
                   : AstraNewWindowBehavior::kDefaultWorkspace;
}

void AstraWorkspaceWindowManager::SetNewWindowBehavior(
    Profile* profile,
    AstraNewWindowBehavior behavior) {
  if (!profile) {
    return;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return;
  }

  // Map behavior to existing boolean pref.
  //
  // TODO(astra): Replace with dedicated enum pref.
  bool in_active = (behavior == AstraNewWindowBehavior::kActiveWorkspace);
  prefs->SetBoolean(prefs::kPrefWorkspaceWindowNewInActiveWorkspace, in_active);
}

// -- Workspace switching helpers ---------------------------------------------

void AstraWorkspaceWindowManager::SwitchToNextWorkspace(Profile* profile) {
  if (!profile) {
    return;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return;
  }

  std::string next_id = service->GetNextWorkspaceId();
  SwitchToWorkspace(profile, next_id);
}

void AstraWorkspaceWindowManager::SwitchToPreviousWorkspace(Profile* profile) {
  if (!profile) {
    return;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return;
  }

  std::string prev_id = service->GetPreviousWorkspaceId();
  SwitchToWorkspace(profile, prev_id);
}

void AstraWorkspaceWindowManager::SwitchToWorkspaceAtIndex(
    Profile* profile,
    size_t index) {
  if (!profile) {
    return;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return;
  }

  const auto& workspaces = service->workspaces();
  if (index >= workspaces.size()) {
    return;
  }

  SwitchToWorkspace(profile, workspaces[index].id);
}

size_t AstraWorkspaceWindowManager::GetWorkspaceCount(Profile* profile) const {
  if (!profile) {
    return 0;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return 0;
  }

  return service->workspace_count();
}

size_t AstraWorkspaceWindowManager::GetActiveWorkspaceIndex(
    Profile* profile) const {
  if (!profile) {
    return 0;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return 0;
  }

  return service->GetWorkspaceIndex(service->active_workspace_id());
}

size_t AstraWorkspaceWindowManager::GetWorkspaceIndex(
    Profile* profile,
    const std::string& workspace_id) const {
  if (!profile) {
    return 0;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return 0;
  }

  return service->GetWorkspaceIndex(workspace_id);
}

std::vector<std::string> AstraWorkspaceWindowManager::GetRecentWorkspaces(
    Profile* profile,
    size_t max_count) const {
  if (!profile) {
    return {};
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return {};
  }

  std::vector<AstraWorkspace> recent = service->GetRecentlyUsedWorkspaces();
  std::vector<std::string> result;
  result.reserve(std::min(max_count, recent.size()));

  for (size_t i = 0; i < max_count && i < recent.size(); ++i) {
    result.push_back(recent[i].id);
  }

  return result;
}

// -- Workspace switch animation ----------------------------------------------

int AstraWorkspaceWindowManager::GetSwitchAnimationDurationMs(
    Profile* profile) const {
  // TODO(astra): Use a dedicated pref for animation duration.
  //   For now, return a fixed default.
  //   Chromium pattern: PrefService with integer pref for animation timing.
  if (!profile) {
    return 200;
  }
  return 200;  // Default 200ms
}

void AstraWorkspaceWindowManager::SetSwitchAnimationDurationMs(
    Profile* profile,
    int duration_ms) {
  // TODO(astra): Persist to a pref when one exists.
  //   For now, this is a no-op (no dedicated pref yet).
  //   Pref key would be kPrefWorkspaceWindowSwitchAnimationDuration.
}

// -- Saved window state ------------------------------------------------------

void AstraWorkspaceWindowManager::SaveAllWindowState(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* browser : windows) {
    SaveWindowState(browser);
  }
}

void AstraWorkspaceWindowManager::RestoreAllWindowState(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* browser : windows) {
    RestoreWindowState(browser);
  }
}

bool AstraWorkspaceWindowManager::HasSavedWindowState(
    Profile* profile,
    const std::string& workspace_id) const {
  if (!profile) {
    return false;
  }

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  if (windows.empty()) {
    return false;
  }

  // A workspace has saved state if at least one window has non-empty saved bounds.
  for (Browser* browser : windows) {
    AstraWindowFeatures* features =
        AstraWindowFeatures::FromBrowser(browser);
    if (features && !features->saved_bounds().IsEmpty()) {
      return true;
    }
  }

  return false;
}

void AstraWorkspaceWindowManager::ClearSavedWindowState(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* browser : windows) {
    AstraWindowFeatures* features =
        AstraWindowFeatures::FromBrowser(browser);
    if (features) {
      features->set_saved_bounds(gfx::Rect());
      features->set_is_minimized(false);
      features->set_is_maximized(false);
    }
  }
}

// -- Settings (PrefService-based) --------------------------------------------

bool AstraWorkspaceWindowManager::GetRememberPlacement(Profile* profile) const {
  if (!profile) {
    return true;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return true;
  }

  return prefs->GetBoolean(prefs::kPrefWorkspaceWindowRememberPlacement);
}

void AstraWorkspaceWindowManager::SetRememberPlacement(Profile* profile,
                                                      bool remember) {
  if (!profile) {
    return;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefWorkspaceWindowRememberPlacement, remember);
}

bool AstraWorkspaceWindowManager::GetNewInActiveWorkspace(
    Profile* profile) const {
  if (!profile) {
    return true;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return true;
  }

  return prefs->GetBoolean(prefs::kPrefWorkspaceWindowNewInActiveWorkspace);
}

void AstraWorkspaceWindowManager::SetNewInActiveWorkspace(Profile* profile,
                                                          bool in_active) {
  if (!profile) {
    return;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefWorkspaceWindowNewInActiveWorkspace, in_active);
}

int AstraWorkspaceWindowManager::GetTileGap(Profile* profile) const {
  // TODO(astra): Use a dedicated tile gap pref.
  //   For now, return a fixed default of 8px.
  //   Pref key: kPrefWorkspaceWindowTileGap
  //   Chromium pattern: integer pref for layout spacing.
  if (!profile) {
    return 8;
  }
  return 8;  // Default 8px gap
}

void AstraWorkspaceWindowManager::SetTileGap(Profile* profile, int gap_px) {
  // TODO(astra): Persist to a tile gap pref when one exists.
  //   For now, this is a no-op.
}

int AstraWorkspaceWindowManager::GetTilePadding(Profile* profile) const {
  // TODO(astra): Use a dedicated tile padding pref.
  //   For now, return a fixed default of 24px.
  //   Pref key: kPrefWorkspaceWindowTilePadding
  if (!profile) {
    return 24;
  }
  return 24;  // Default 24px padding
}

void AstraWorkspaceWindowManager::SetTilePadding(Profile* profile,
                                                 int padding_px) {
  // TODO(astra): Persist to a tile padding pref when one exists.
  //   For now, this is a no-op.
}

int AstraWorkspaceWindowManager::GetCascadeOffset(Profile* profile) const {
  // TODO(astra): Use a dedicated cascade offset pref.
  //   For now, return a fixed default of 24px.
  //   Pref key: kPrefWorkspaceWindowCascadeOffset
  if (!profile) {
    return 24;
  }
  return 24;  // Default 24px cascade offset
}

void AstraWorkspaceWindowManager::SetCascadeOffset(Profile* profile,
                                                   int offset_px) {
  // TODO(astra): Persist to a cascade offset pref when one exists.
  //   For now, this is a no-op.
}

bool AstraWorkspaceWindowManager::GetAutoTile(Profile* profile) const {
  if (!profile) {
    return false;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return false;
  }

  return prefs->GetBoolean(prefs::kPrefWorkspaceWindowAutoTile);
}

void AstraWorkspaceWindowManager::SetAutoTile(Profile* profile,
                                              bool auto_tile) {
  if (!profile) {
    return;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefWorkspaceWindowAutoTile, auto_tile);
}

std::string AstraWorkspaceWindowManager::GetDefaultWorkspaceId(
    Profile* profile) const {
  if (!profile) {
    return "default";
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return "default";
  }

  return service->GetDefaultWorkspaceId();
}

void AstraWorkspaceWindowManager::SetDefaultWorkspaceId(
    Profile* profile,
    const std::string& workspace_id) {
  // TODO(astra): Add a "default workspace" setting pref.
  //   The default workspace cannot be changed through the service alone
  //   since default workspace status is stored on the AstraWorkspace struct.
  //   For now, this is a no-op.
}

// -- Workspace indicator position --------------------------------------------

AstraWorkspaceWindowManager::IndicatorPosition
AstraWorkspaceWindowManager::GetWorkspaceIndicatorPosition(
    Profile* profile) const {
  // TODO(astra): Use a dedicated pref for indicator position.
  //   For now, return top-center as the default.
  //   Pref key: kPrefWorkspaceWindowIndicatorPosition
  if (!profile) {
    return IndicatorPosition::kTopCenter;
  }
  return IndicatorPosition::kTopCenter;
}

void AstraWorkspaceWindowManager::SetWorkspaceIndicatorPosition(
    Profile* profile,
    IndicatorPosition position) {
  // TODO(astra): Persist to an indicator position pref when one exists.
  //   For now, this is a no-op.
}

// -- Workspace switching -----------------------------------------------------

void AstraWorkspaceWindowManager::SwitchToWorkspace(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  std::string old_workspace_id = GetActiveWorkspaceId(profile);
  if (old_workspace_id == workspace_id) {
    return;  // Already active.
  }

  // Iterate all browsers for this profile:
  //   - Windows in the target workspace: show and restore state.
  //   - Windows NOT in the target workspace: save state and hide.
  //
  // Chromium owner: Browser window management (Show/Hide, minimize/restore).
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (!BrowserBelongsToProfile(browser, profile)) {
      continue;
    }

    std::string ws_id = GetWorkspaceForWindow(browser);
    if (ws_id == workspace_id) {
      // Show the window and restore its saved state.
      RestoreWindowState(browser);
    } else {
      // Save the window's state and hide it.
      SaveWindowState(browser);
      // TODO(astra): Use appropriate hide mechanism.
      // Options:
      //   1. Minimize the window (browser->window()->Minimize())
      //   2. Hide the window (browser->window()->Hide())
      //   3. Close the window (would lose state without session restore)
      //
      // Minimize is the safest approach on most platforms:
      //   - Window stays in the taskbar/dock.
      //   - State is preserved by the OS.
      //   - Easy to restore.
      //
      // However, Arc-style workspaces typically hide windows completely.
      // For now, we use Hide() to match Arc's "out of sight" behavior,
      // and saved_bounds_ for position recall.
      //
      // Chromium owner: BrowserWindow (chrome/browser/ui/browser_window.h)
      // Patch point: BrowserWindow::Hide() / Show().
      if (browser->window()) {
        browser->window()->Hide();
      }
    }
  }

  // Notify observers of the workspace change.
  for (auto& observer : observers_) {
    observer.OnActiveWorkspaceChanged(old_workspace_id, workspace_id);
  }
}

std::string AstraWorkspaceWindowManager::GetActiveWorkspaceId(
    Profile* profile) const {
  if (!profile) {
    return "default";
  }

  // Find the workspace with visible windows.
  // The "active" workspace is the one whose windows are currently shown.
  //
  // TODO(astra): This derivation from visibility is fragile.
  // Consider tracking active workspace explicitly on the window manager
  // or using AstraWorkspaceService's active_workspace_id().
  // For now, we look for the first workspace with at least one visible window.
  std::string first_visible_ws = "default";
  bool found_visible = false;

  for (Browser* browser : *BrowserList::GetInstance()) {
    if (!BrowserBelongsToProfile(browser, profile)) {
      continue;
    }

    // Check if the window is visible.
    //
    // Chromium owner: BrowserWindow (chrome/browser/ui/browser_window.h)
    bool is_visible = browser->window() && browser->window()->IsActive();
    // TODO(astra): Use a better visibility check.
    // IsActive() returns true for the focused window.
    // We may want IsVisible() or !IsMinimized().
    // For now, if the window exists and isn't hidden, consider it visible.
    //
    // Alternative: check if the window has been explicitly hidden by us.
    // We could track this on AstraWindowFeatures.

    if (is_visible) {
      std::string ws_id = GetWorkspaceForWindow(browser);
      if (!found_visible) {
        first_visible_ws = ws_id;
        found_visible = true;
      }
      // If we find a window from a different workspace that's also visible,
      // return the one with more windows as a tiebreaker.
      // For simplicity, return the first found.
    }
  }

  // Fallback: use AstraWorkspaceService's active workspace id.
  // This handles the case where all windows are hidden (e.g. startup).
  //
  // TODO(astra): Add dependency on AstraWorkspaceService for fallback.
  // The dependency graph should be: window manager -> workspace service.
  // But to avoid circular deps, we may want to track active workspace
  // explicitly on the window manager.

  return first_visible_ws;
}

// -- BrowserListObserver -----------------------------------------------------

void AstraWorkspaceWindowManager::OnBrowserAdded(Browser* browser) {
  if (!browser) {
    return;
  }

  // Ensure the new browser has AstraWindowFeatures.
  // This attaches workspace metadata to every new Browser window.
  //
  // TODO(astra): Determine the workspace for new windows.
  // Currently defaults to "default".  In the future, new windows should
  // be created in the current active workspace.
  //
  // The active workspace can come from:
  //   - AstraWorkspaceService::active_workspace_id() (profile-level).
  //   - The current active window's workspace (window-level).
  //
  // For now, we create with default and let explicit assignment happen
  // via CreateNewWindowInWorkspace() or MoveWindowToWorkspace().
  AstraWindowFeatures* features =
      AstraWindowFeatures::GetOrCreateForBrowser(browser);
  DCHECK(features);

  // Assign a default order index at the end of the workspace's window list.
  AssignDefaultOrderIndex(browser);

  std::string workspace_id = features->workspace_id();

  for (auto& observer : observers_) {
    observer.OnWindowAddedToWorkspace(browser, workspace_id);
  }

  NotifyWindowCountChanged(browser->profile(), workspace_id);
}

void AstraWorkspaceWindowManager::OnBrowserRemoved(Browser* browser) {
  if (!browser) {
    return;
  }

  std::string workspace_id = GetWorkspaceForWindow(browser);

  for (auto& observer : observers_) {
    observer.OnWindowRemovedFromWorkspace(browser, workspace_id);
  }

  NotifyWindowCountChanged(browser->profile(), workspace_id);
}

// -- Private helpers ---------------------------------------------------------

bool AstraWorkspaceWindowManager::BrowserBelongsToProfile(
    Browser* browser,
    Profile* profile) const {
  if (!browser || !profile) {
    return false;
  }
  // Compare the browser's profile with the given profile.
  //
  // Chromium owner: Browser::profile() (chrome/browser/ui/browser.h)
  return browser->profile() == profile;
}

void AstraWorkspaceWindowManager::SaveWindowState(Browser* browser) {
  if (!browser || !browser->window()) {
    return;
  }

  AstraWindowFeatures* features =
      AstraWindowFeatures::GetOrCreateForBrowser(browser);
  DCHECK(features);

  // Save current window bounds.
  //
  // Chromium owner: BrowserWindow::GetBounds()
  // (chrome/browser/ui/browser_window.h)
  features->set_saved_bounds(browser->window()->GetBounds());

  // Save minimized/maximized state.
  //
  // TODO(astra): Use proper BrowserWindow methods for minimized/maximized.
  // Chromium component: BrowserWindow state methods.
  features->set_is_minimized(browser->window()->IsMinimized());
  features->set_is_maximized(browser->window()->IsMaximized());
}

void AstraWorkspaceWindowManager::RestoreWindowState(Browser* browser) {
  if (!browser || !browser->window()) {
    return;
  }

  AstraWindowFeatures* features =
      AstraWindowFeatures::GetOrCreateForBrowser(browser);
  DCHECK(features);

  // Show the window first.
  browser->window()->Show();

  // Restore saved bounds if available.
  //
  // TODO(astra): Only restore bounds if saved_bounds is non-empty.
  // For now, we always try to restore; empty rect is a no-op for SetBounds.
  const gfx::Rect& bounds = features->saved_bounds();
  if (!bounds.IsEmpty()) {
    browser->window()->SetBounds(bounds);
  }

  // Restore minimized/maximized state.
  //
  // TODO(astra): Handle the various window states properly.
  // Order matters: show first, then maximize if needed, or minimize.
  // Chromium owner: BrowserWindow::Maximize() / Minimize() / Restore().
  if (features->is_maximized()) {
    browser->window()->Maximize();
  } else if (features->is_minimized()) {
    browser->window()->Minimize();
  }
}

void AstraWorkspaceWindowManager::AssignDefaultOrderIndex(Browser* browser) {
  if (!browser) {
    return;
  }

  Profile* profile = browser->profile();
  std::string workspace_id = GetWorkspaceForWindow(browser);

  // Find the highest current order_index in the workspace and assign + 1.
  // This places the new window at the end of the workspace's window list.
  size_t max_index = 0;
  bool found_any = false;

  std::vector<Browser*> windows = GetWindowsForWorkspace(profile, workspace_id);
  for (Browser* w : windows) {
    if (w == browser) {
      continue;  // Skip the window being added.
    }
    AstraWindowFeatures* features = AstraWindowFeatures::FromBrowser(w);
    if (features) {
      if (!found_any || features->order_index() > max_index) {
        max_index = features->order_index();
        found_any = true;
      }
    }
  }

  AstraWindowFeatures* features =
      AstraWindowFeatures::GetOrCreateForBrowser(browser);
  if (features) {
    features->set_order_index(found_any ? max_index + 1 : 0);
  }
}

void AstraWorkspaceWindowManager::NotifyWindowCountChanged(
    Profile* profile,
    const std::string& workspace_id) {
  if (!profile) {
    return;
  }

  size_t count = GetWindowCount(profile, workspace_id);

  for (auto& observer : observers_) {
    observer.OnWindowCountChanged(workspace_id, count);
  }
}

}  // namespace astra
