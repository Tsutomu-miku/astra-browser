#include "astra/browser/astra_window_features.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "ui/gfx/geometry/rect.h"

#include "astra/browser/astra_incognito_handler.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// The key used to identify AstraWindowFeatures in Browser's SupportsUserData.
// We use the address of this static int as the unique key, which is the
// standard Chromium SupportsUserData pattern.
const int AstraWindowFeatures::kUserDataKey = 0;

// static
const void* AstraWindowFeatures::UserDataKey() {
  return &kUserDataKey;
}

// static
base::ObserverList<AstraWindowObserver>& AstraWindowFeatures::GetGlobalObservers() {
  static base::NoDestructor<base::ObserverList<AstraWindowObserver>> observers;
  return *observers;
}

AstraWindowFeatures::AstraWindowFeatures(Browser* browser) {
  // Initialize workspace state based on profile type.
  // In incognito, windows always start in the default workspace and cannot
  // be moved.  See AstraIncognitoHandler for the design rationale.
  //
  // Chromium owner: Profile::IsOffTheRecord()
  // (chrome/browser/profiles/profile.h)
  if (browser && browser->profile() &&
      browser->profile()->IsOffTheRecord()) {
    workspace_id_ = "default";
    workspace_read_only_ = true;
  }

  // TODO(astra): Initialize sidebar and split view defaults from PrefService.
  // Currently hardcoded defaults match the pref defaults in astra_prefs.h.
  // Once we have a PrefService available, read the defaults here.
}

AstraWindowFeatures::~AstraWindowFeatures() {
  NotifyWindowClosing();
}

// static
AstraWindowFeatures* AstraWindowFeatures::GetOrCreateForBrowser(
    Browser* browser) {
  if (!browser) {
    return nullptr;
  }

  AstraWindowFeatures* data = FromBrowser(browser);
  if (data) {
    return data;
  }

  // Create and attach to Browser's SupportsUserData.
  auto features = std::make_unique<AstraWindowFeatures>(browser);
  AstraWindowFeatures* ptr = features.get();
  browser->SetUserData(UserDataKey(), std::move(features));
  ptr->NotifyWindowCreated();
  return ptr;
}

// static
AstraWindowFeatures* AstraWindowFeatures::FromBrowser(Browser* browser) {
  if (!browser) {
    return nullptr;
  }
  return static_cast<AstraWindowFeatures*>(
      browser->GetUserData(UserDataKey()));
}

void AstraWindowFeatures::Reset() {
  workspace_id_ = "default";
  order_index_ = 0;

  sidebar_visible_ = true;
  sidebar_pinned_ = true;
  sidebar_width_ = 280;

  split_view_active_ = false;
  split_view_orientation_ = "horizontal";
  split_view_ratio_ = 0.5;

  saved_bounds_ = gfx::Rect();
  is_minimized_ = false;
  is_maximized_ = false;
  is_fullscreen_ = false;
  is_hibernated_ = false;

  // Re-evaluate read-only state based on the current Browser's profile.
  // If this Browser was repurposed (e.g. session restore into a different
  // profile context), the read-only flag needs to be recomputed.
  //
  // TODO(astra): Verify that Reset() is called in all paths where a
  // Browser changes profile context.  In Chromium, Browsers typically
  // stay with one Profile for their lifetime, but some edge cases may
  // need verification.
  // Chromium owner: Browser / BrowserList.
  //
  // Note: we don't have a direct back-pointer to Browser here.
  // In practice, Reset() is called from code that has the Browser pointer,
  // so we rely on the caller to re-create the features if needed.
  // For now, we leave workspace_read_only_ unchanged — it is set at
  // construction time and Browser profiles don't change.

  NotifyFeaturesChanged();
}

// -- Per-window observers -------------------------------------------------

void AstraWindowFeatures::AddObserver(AstraWindowObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraWindowFeatures::RemoveObserver(AstraWindowObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Global observers ----------------------------------------------------

// static
void AstraWindowFeatures::AddGlobalObserver(AstraWindowObserver* observer) {
  GetGlobalObservers().AddObserver(observer);
}

// static
void AstraWindowFeatures::RemoveGlobalObserver(AstraWindowObserver* observer) {
  GetGlobalObservers().RemoveObserver(observer);
}

// -- Workspace ------------------------------------------------------------

void AstraWindowFeatures::set_workspace_id(std::string workspace_id) {
  if (workspace_read_only_) {
    return;
  }
  if (workspace_id_ == workspace_id) {
    return;
  }
  std::string old_id = workspace_id_;
  workspace_id_ = std::move(workspace_id);

  // Notify per-instance observers.
  for (auto& observer : observers_) {
    observer.OnWindowWorkspaceChanged(this, old_id, workspace_id_);
    observer.OnWindowFeaturesChanged(this);
  }

  // Notify global observers.
  for (auto& observer : GetGlobalObservers()) {
    observer.OnWindowWorkspaceChanged(this, old_id, workspace_id_);
    observer.OnWindowFeaturesChanged(this);
  }
}

// -- Sidebar --------------------------------------------------------------

void AstraWindowFeatures::set_sidebar_visible(bool visible) {
  if (sidebar_visible_ == visible) {
    return;
  }
  sidebar_visible_ = visible;

  for (auto& observer : observers_) {
    observer.OnWindowSidebarVisibilityChanged(this, visible);
    observer.OnWindowFeaturesChanged(this);
  }
  for (auto& observer : GetGlobalObservers()) {
    observer.OnWindowSidebarVisibilityChanged(this, visible);
    observer.OnWindowFeaturesChanged(this);
  }
}

void AstraWindowFeatures::set_sidebar_pinned(bool pinned) {
  if (sidebar_pinned_ == pinned) {
    return;
  }
  sidebar_pinned_ = pinned;

  for (auto& observer : observers_) {
    observer.OnWindowSidebarPinnedChanged(this, pinned);
    observer.OnWindowFeaturesChanged(this);
  }
  for (auto& observer : GetGlobalObservers()) {
    observer.OnWindowSidebarPinnedChanged(this, pinned);
    observer.OnWindowFeaturesChanged(this);
  }
}

void AstraWindowFeatures::set_sidebar_width(int width) {
  width = ClampSidebarWidth(width);
  if (sidebar_width_ == width) {
    return;
  }
  sidebar_width_ = width;

  for (auto& observer : observers_) {
    observer.OnWindowSidebarWidthChanged(this, width);
    observer.OnWindowFeaturesChanged(this);
  }
  for (auto& observer : GetGlobalObservers()) {
    observer.OnWindowSidebarWidthChanged(this, width);
    observer.OnWindowFeaturesChanged(this);
  }
}

bool AstraWindowFeatures::ToggleSidebar() {
  set_sidebar_visible(!sidebar_visible_);
  return sidebar_visible_;
}

// -- Split view -----------------------------------------------------------

void AstraWindowFeatures::set_split_view_active(bool active) {
  if (split_view_active_ == active) {
    return;
  }
  split_view_active_ = active;

  for (auto& observer : observers_) {
    observer.OnWindowSplitViewStateChanged(this, active);
    observer.OnWindowFeaturesChanged(this);
  }
  for (auto& observer : GetGlobalObservers()) {
    observer.OnWindowSplitViewStateChanged(this, active);
    observer.OnWindowFeaturesChanged(this);
  }
}

void AstraWindowFeatures::set_split_view_orientation(std::string orientation) {
  if (split_view_orientation_ == orientation) {
    return;
  }
  split_view_orientation_ = std::move(orientation);
  NotifyFeaturesChanged();
}

void AstraWindowFeatures::set_split_view_ratio(double ratio) {
  ratio = ClampSplitRatio(ratio);
  if (split_view_ratio_ == ratio) {
    return;
  }
  split_view_ratio_ = ratio;
  NotifyFeaturesChanged();
}

bool AstraWindowFeatures::ToggleSplitView() {
  set_split_view_active(!split_view_active_);
  return split_view_active_;
}

// -- Fullscreen -----------------------------------------------------------

bool AstraWindowFeatures::ToggleFullscreen() {
  is_fullscreen_ = !is_fullscreen_;
  return is_fullscreen_;
}

// -- Hibernation ----------------------------------------------------------

void AstraWindowFeatures::set_is_hibernated(bool hibernated) {
  if (is_hibernated_ == hibernated) {
    return;
  }
  is_hibernated_ = hibernated;
  NotifyFeaturesChanged();
}

// -- Window management (static, uses BrowserList) -------------------------

// static
size_t AstraWindowFeatures::GetWindowCount() {
  // Chromium owner: BrowserList (chrome/browser/ui/browser_list.h)
  // BrowserList tracks all browser windows.  We delegate to it directly.
  return BrowserList::GetInstance()->size();
}

// static
AstraWindowFeatures* AstraWindowFeatures::GetActiveWindow() {
  Browser* browser = BrowserList::GetInstance()->GetLastActive();
  if (!browser) {
    return nullptr;
  }
  return FromBrowser(browser);
}

// static
std::vector<AstraWindowFeatures*> AstraWindowFeatures::GetWindowsByWorkspace(
    const std::string& workspace_id) {
  std::vector<AstraWindowFeatures*> windows;

  // Iterate Chromium's BrowserList and filter by Astra workspace metadata.
  // Chromium owns the window list — we just project Astra state onto it.
  for (Browser* browser : *BrowserList::GetInstance()) {
    AstraWindowFeatures* features = FromBrowser(browser);
    if (features && features->workspace_id() == workspace_id) {
      windows.push_back(features);
    }
  }

  return windows;
}

// -- Window arrangement helpers (static) ----------------------------------

// static
void AstraWindowFeatures::TileWindowsInWorkspace(
    const std::string& workspace_id) {
  auto windows = GetWindowsByWorkspace(workspace_id);
  if (windows.empty()) {
    return;
  }

  // Use the first window's saved bounds as the reference area.
  // In production, this would use display::Screen to get the work area.
  //
  // TODO(astra): Use display::Screen::GetDisplayNearestWindow() to get
  //   the actual work area for tiling.
  // Chromium owner: display::Screen (ui/display/screen.h)
  gfx::Rect area = windows[0]->saved_bounds();
  if (area.IsEmpty()) {
    // Default work area if no bounds are set.
    area = gfx::Rect(0, 0, 1280, 800);
  }

  size_t count = windows.size();
  int tile_width = area.width() / static_cast<int>(count);

  for (size_t i = 0; i < count; ++i) {
    gfx::Rect tile_bounds(
        area.x() + static_cast<int>(i) * tile_width,
        area.y(),
        tile_width,
        area.height());
    windows[i]->set_saved_bounds(tile_bounds);
    // Also reset maximized/minimized/fullscreen for tiled windows.
    windows[i]->set_is_maximized(false);
    windows[i]->set_is_minimized(false);
    windows[i]->set_is_fullscreen(false);
  }

  // TODO(astra): Apply actual window positions via views::Widget.
  //   Currently we only update saved_bounds_ metadata.
}

// static
void AstraWindowFeatures::StackWindowsInWorkspace(
    const std::string& workspace_id) {
  auto windows = GetWindowsByWorkspace(workspace_id);
  if (windows.empty()) {
    return;
  }

  // Use the first window's saved bounds as the stack position.
  gfx::Rect stack_bounds = windows[0]->saved_bounds();
  if (stack_bounds.IsEmpty()) {
    stack_bounds = gfx::Rect(0, 0, 1280, 800);
  }

  for (auto* window : windows) {
    window->set_saved_bounds(stack_bounds);
    window->set_is_maximized(false);
    window->set_is_minimized(false);
    window->set_is_fullscreen(false);
  }

  // TODO(astra): Apply actual window positions via views::Widget.
}

// -- Bulk operations ------------------------------------------------------

// static
void AstraWindowFeatures::CloseAllWindowsInWorkspace(
    const std::string& workspace_id) {
  auto windows = GetWindowsByWorkspace(workspace_id);

  // TODO(astra): Actually close the windows using Browser::window()->Close().
  //   For now, this is a metadata-level stub.
  //
  // Chromium owner: Browser::window()->Close()
  //   (chrome/browser/ui/browser_window.h)
  // Closing windows is fully owned by Chromium — this helper would just
  // collect the windows and trigger the close.

  // Mark windows as minimized in saved state as a placeholder.
  // In a real implementation, we'd iterate and close each browser.
  for (auto* window : windows) {
    window->set_is_minimized(true);
  }
}

// -- Test helpers ---------------------------------------------------------

// static
std::unique_ptr<AstraWindowFeatures> AstraWindowFeatures::CreateForTesting() {
  // Pass nullptr for Browser — the constructor handles this gracefully.
  // All state starts at defaults.
  return base::WrapUnique(new AstraWindowFeatures(nullptr));
}

// -- Private helpers ------------------------------------------------------

void AstraWindowFeatures::NotifyWindowCreated() {
  for (auto& observer : GetGlobalObservers()) {
    observer.OnWindowCreated(this);
  }
}

void AstraWindowFeatures::NotifyWindowClosing() {
  for (auto& observer : GetGlobalObservers()) {
    observer.OnWindowClosing(this);
  }
  for (auto& observer : observers_) {
    observer.OnWindowClosing(this);
  }
}

void AstraWindowFeatures::NotifyFeaturesChanged() {
  for (auto& observer : observers_) {
    observer.OnWindowFeaturesChanged(this);
  }
  for (auto& observer : GetGlobalObservers()) {
    observer.OnWindowFeaturesChanged(this);
  }
}

// static
double AstraWindowFeatures::ClampSplitRatio(double ratio) {
  if (ratio < 0.1) {
    return 0.1;
  }
  if (ratio > 0.9) {
    return 0.9;
  }
  return ratio;
}

// static
int AstraWindowFeatures::ClampSidebarWidth(int width) {
  if (width < 120) {
    return 120;
  }
  if (width > 600) {
    return 600;
  }
  return width;
}

}  // namespace astra
