#include "astra/ui/views/workspace/astra_workspace_overview_controller.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/views/workspace/astra_workspace_import_export_dialog.h"
#include "astra/ui/views/workspace/astra_workspace_overview_view.h"
#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/animation/tween.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace astra {

namespace {

// Animation durations.
constexpr int kShowAnimationDurationMs = 200;
constexpr int kHideAnimationDurationMs = 150;

// Slide distance for show/hide animation (pixels).
constexpr int kAnimationSlideDistance = 20;

// TODO(astra): Define a proper set of accent colors for new workspaces,
// cycling through them or picking one based on index.  For now, use a
// default accent color.
constexpr char kNewWorkspaceAccentColor[] = "#5AD8A6";

// Number of default accent colors to cycle through.
constexpr const char* kAccentColorPalette[] = {
    "#5B8FF9",  // blue
    "#5AD8A6",  // green
    "#F6BD16",  // yellow
    "#E86452",  // red
    "#6DC8EC",  // cyan
    "#9270CA",  // purple
    "#FF9D4D",  // orange
    "#269A99",  // teal
};
constexpr size_t kAccentColorPaletteSize =
    sizeof(kAccentColorPalette) / sizeof(kAccentColorPalette[0]);

// Pick an accent color from the palette based on workspace index.
const char* PickAccentColor(size_t index) {
  return kAccentColorPalette[index % kAccentColorPaletteSize];
}

}  // namespace

// =========================================================================
// Constructor / destructor
// =========================================================================

AstraWorkspaceOverviewController::AstraWorkspaceOverviewController(
    BrowserView* browser_view)
    : browser_view_(browser_view) {
  // Look up the workspace service from the browser's profile.
  if (browser_view_ && browser_view_->browser() &&
      browser_view_->browser()->profile()) {
    workspace_service_ = AstraWorkspaceServiceFactory::GetForProfile(
        browser_view_->browser()->profile());
  }

  // Subscribe to workspace service changes if available.
  if (workspace_service_) {
    workspace_service_observation_.Observe(workspace_service_);
  }

  // Load presentation settings from PrefService.
  LoadPresentationSettings();
}

AstraWorkspaceOverviewController::~AstraWorkspaceOverviewController() {
  // Clean up the widget if it still exists.
  if (widget_) {
    widget_->CloseNow();
    widget_ = nullptr;
    overview_view_ = nullptr;
  }
}

// =========================================================================
// Public API
// =========================================================================

void AstraWorkspaceOverviewController::Show() {
  if (IsVisible()) {
    // Already visible — just refresh content.
    Update();
    return;
  }

  if (!widget_) {
    CreateWidget();
  }

  if (!widget_) {
    return;
  }

  // Refresh content before showing.
  Update();

  // Ensure first card is selected for keyboard navigation.
  if (overview_view_ && overview_view_->GetWorkspaceCardCount() > 0 &&
      overview_view_->selected_index() < 0) {
    overview_view_->SelectWorkspaceAt(0);
  }

  widget_->Show();
  is_visible_ = true;
  is_animating_ = true;

  // Animate in.
  AnimateShow();

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnOverviewShown();
  }
}

void AstraWorkspaceOverviewController::Hide() {
  if (!IsVisible()) {
    return;
  }

  // Animate out, then actually hide.
  is_animating_ = true;
  AnimateHide();
}

void AstraWorkspaceOverviewController::Toggle() {
  if (IsVisible()) {
    Hide();
  } else {
    Show();
  }
}

bool AstraWorkspaceOverviewController::IsVisible() const {
  return is_visible_ && widget_ && widget_->IsVisible();
}

void AstraWorkspaceOverviewController::Update() {
  if (!widget_ || !workspace_service_ || !overview_view_) {
    return;
  }

  const auto& workspaces = workspace_service_->workspaces();
  const std::string& active_id = workspace_service_->active_workspace_id();
  std::vector<int> tab_counts = ComputeTabCounts();
  std::vector<int> window_counts = ComputeWindowCounts();

  overview_view_->UpdateWorkspaces(workspaces, active_id, tab_counts,
                                   window_counts);
}

void AstraWorkspaceOverviewController::ShowOverview() {
  Show();
}

void AstraWorkspaceOverviewController::HideOverview() {
  Hide();
}

bool AstraWorkspaceOverviewController::IsOverviewVisible() const {
  return IsVisible();
}

AstraWorkspaceOverviewView* AstraWorkspaceOverviewController::GetView() const {
  return overview_view_;
}

// =========================================================================
// Workspace selection
// =========================================================================

void AstraWorkspaceOverviewController::SelectWorkspace(
    const std::string& workspace_id) {
  if (!overview_view_ || !workspace_service_) {
    return;
  }

  const auto& workspaces = workspace_service_->workspaces();
  for (size_t i = 0; i < workspaces.size(); ++i) {
    if (workspaces[i].id == workspace_id) {
      overview_view_->SelectWorkspaceAt(static_cast<int>(i));
      break;
    }
  }
}

std::string AstraWorkspaceOverviewController::GetSelectedWorkspace() const {
  if (!overview_view_) {
    return std::string();
  }
  return overview_view_->GetSelectedWorkspaceId();
}

// =========================================================================
// Workspace operations
// =========================================================================

std::string AstraWorkspaceOverviewController::NewWorkspace() {
  if (!workspace_service_) {
    return std::string();
  }

  size_t count = workspace_service_->workspace_count();
  const char* accent_color = PickAccentColor(count);

  AstraWorkspace workspace;
  workspace.id = base::GenerateGUID();
  workspace.name = "Workspace " + base::NumberToString(count + 1);
  workspace.accent_color = accent_color;
  workspace.created_time = base::Time::Now();
  workspace.last_used_time = base::Time::Now();
  workspace.order_index = count;
  workspace.is_default = false;
  workspace.is_hibernated = false;

  workspace_service_->AddWorkspace(std::move(workspace));

  // Notify simplified observers.
  for (auto& observer : overview_observers_) {
    observer.OnWorkspaceAdded(workspace.id);
  }

  return workspace.id;
}

bool AstraWorkspaceOverviewController::DeleteSelectedWorkspace() {
  if (!workspace_service_) {
    return false;
  }

  std::string selected_id = GetSelectedWorkspace();
  if (selected_id.empty()) {
    return false;
  }

  const AstraWorkspace* ws = workspace_service_->GetWorkspace(selected_id);
  if (!ws || ws->is_default) {
    return false;
  }

  bool result = workspace_service_->DeleteWorkspace(selected_id);

  if (result) {
    // Notify simplified observers.
    for (auto& observer : overview_observers_) {
      observer.OnWorkspaceRemoved(selected_id);
    }
  }

  return result;
}

bool AstraWorkspaceOverviewController::RenameSelectedWorkspace(
    const std::string& new_name) {
  if (!workspace_service_) {
    return false;
  }

  std::string selected_id = GetSelectedWorkspace();
  if (selected_id.empty()) {
    return false;
  }

  bool result = workspace_service_->RenameWorkspace(selected_id, new_name);

  if (result) {
    // Notify simplified observers.
    for (auto& observer : overview_observers_) {
      observer.OnWorkspaceRenamed(selected_id, new_name);
    }
  }

  return result;
}

bool AstraWorkspaceOverviewController::MoveWorkspace(size_t from_index,
                                                     size_t to_index) {
  if (!workspace_service_) {
    return false;
  }

  const auto& workspaces = workspace_service_->workspaces();
  if (from_index >= workspaces.size() || to_index >= workspaces.size() ||
      from_index == to_index) {
    return false;
  }

  // Build the new ordered ID list.
  std::vector<std::string> ordered_ids;
  ordered_ids.reserve(workspaces.size());
  for (size_t i = 0; i < workspaces.size(); ++i) {
    if (i == from_index) {
      continue;
    }
    if (i == to_index) {
      ordered_ids.push_back(workspaces[from_index].id);
    }
    ordered_ids.push_back(workspaces[i].id);
  }
  // Handle edge case: moving to the end.
  if (to_index == workspaces.size() - 1) {
    ordered_ids.push_back(workspaces[from_index].id);
  }

  bool result = workspace_service_->ReorderWorkspaces(ordered_ids);

  if (result) {
    // Notify simplified observers.
    for (auto& observer : overview_observers_) {
      observer.OnWorkspacesReordered();
    }
  }

  return result;
}

size_t AstraWorkspaceOverviewController::ImportWorkspaces(
    const std::string& json_data) {
  if (!workspace_service_) {
    return 0;
  }

  // TODO(astra): Implement full JSON import with workspace parsing.
  //   For now, this is a stub that returns 0.
  //   Chromium component: base::JSONReader / base::Value.
  //   Import logic should match AstraWorkspaceImportExport helper.

  // Notify simplified observers for each imported workspace.
  // for (const auto& ws : imported) {
  //   for (auto& observer : overview_observers_) {
  //     observer.OnWorkspaceAdded(ws.id);
  //   }
  // }

  return 0;
}

std::string AstraWorkspaceOverviewController::ExportWorkspace() const {
  if (!workspace_service_) {
    return std::string();
  }

  // TODO(astra): Implement full JSON export of all workspace metadata.
  //   For now, this is a stub that returns an empty string.
  //   Chromium component: base::JSONWriter / base::Value.
  //   Export logic should match AstraWorkspaceImportExport helper.

  return std::string();
}

// =========================================================================
// Workspace query
// =========================================================================

size_t AstraWorkspaceOverviewController::GetWorkspaceCount() const {
  if (!workspace_service_) {
    return 0;
  }
  return workspace_service_->workspace_count();
}

const AstraWorkspace* AstraWorkspaceOverviewController::GetWorkspaceAt(
    size_t index) const {
  if (!workspace_service_) {
    return nullptr;
  }

  const auto& workspaces = workspace_service_->workspaces();
  if (index >= workspaces.size()) {
    return nullptr;
  }

  return &workspaces[index];
}

std::vector<AstraWorkspace>
AstraWorkspaceOverviewController::GetWorkspaces() const {
  if (!workspace_service_) {
    return {};
  }
  return workspace_service_->workspaces();
}

// =========================================================================
// Overview observer management (simplified observer)
// =========================================================================

void AstraWorkspaceOverviewController::AddOverviewObserver(
    AstraWorkspaceOverviewObserver* observer) {
  overview_observers_.AddObserver(observer);
}

void AstraWorkspaceOverviewController::RemoveOverviewObserver(
    AstraWorkspaceOverviewObserver* observer) {
  overview_observers_.RemoveObserver(observer);
}

// =========================================================================
// Presentation settings
// =========================================================================

void AstraWorkspaceOverviewController::SetViewMode(
    AstraWorkspaceOverviewViewMode mode) {
  if (view_mode_ == mode) {
    return;
  }
  view_mode_ = mode;

  if (overview_view_) {
    overview_view_->SetViewMode(mode);
  }

  SavePresentationSettings();

  for (auto& observer : observers_) {
    observer.OnOverviewViewModeChanged(mode);
  }
}

void AstraWorkspaceOverviewController::SetCardSize(
    AstraWorkspaceOverviewCardSize size) {
  if (card_size_ == size) {
    return;
  }
  card_size_ = size;

  if (overview_view_) {
    overview_view_->SetCardSize(size);
  }

  SavePresentationSettings();

  for (auto& observer : observers_) {
    observer.OnOverviewCardSizeChanged(size);
  }
}

void AstraWorkspaceOverviewController::SetShowStatistics(bool show) {
  if (show_statistics_ == show) {
    return;
  }
  show_statistics_ = show;

  if (overview_view_) {
    overview_view_->SetShowStatistics(show);
  }

  SavePresentationSettings();

  for (auto& observer : observers_) {
    observer.OnOverviewShowStatisticsChanged(show);
  }
}

// =========================================================================
// Bulk operations
// =========================================================================

void AstraWorkspaceOverviewController::HibernateAllWorkspaces() {
  if (!workspace_service_) {
    return;
  }

  const auto& workspaces = workspace_service_->workspaces();
  std::string active_id = workspace_service_->active_workspace_id();

  for (const auto& ws : workspaces) {
    // Skip default and active workspaces.
    if (ws.is_default || ws.id == active_id) {
      continue;
    }
    if (!ws.is_hibernated) {
      workspace_service_->SetWorkspaceHibernated(ws.id, true);
    }
  }

  for (auto& observer : observers_) {
    observer.OnAllWorkspacesHibernated();
  }
}

void AstraWorkspaceOverviewController::DeleteAllNonDefaultWorkspaces() {
  if (!workspace_service_) {
    return;
  }

  // Collect non-default workspace IDs first (can't modify while iterating).
  std::vector<std::string> ids_to_delete;
  const auto& workspaces = workspace_service_->workspaces();
  for (const auto& ws : workspaces) {
    if (!ws.is_default) {
      ids_to_delete.push_back(ws.id);
    }
  }

  // Delete each workspace.
  for (const auto& id : ids_to_delete) {
    workspace_service_->DeleteWorkspace(id);
  }

  for (auto& observer : observers_) {
    observer.OnAllNonDefaultWorkspacesDeleted();
  }
}

// =========================================================================
// Observer management
// =========================================================================

void AstraWorkspaceOverviewController::AddObserver(
    AstraWorkspaceOverviewControllerObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraWorkspaceOverviewController::RemoveObserver(
    AstraWorkspaceOverviewControllerObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// AstraWorkspaceServiceObserver
// =========================================================================
//
// The overview is a projection of workspace state.  When the underlying
// service changes, we refresh the presentation.  This follows the same
// observer pattern as the sidebar (AstraSidebarView).
//
// For simplicity, every change triggers a full Update() which rebuilds all
// cards.  With few workspaces (typically < 10), this is fine.  If the user
// has many workspaces, we could optimize to incremental updates.
// TODO(astra): Optimize to incremental card updates for each observer method
// instead of calling Update() (full rebuild) every time.

void AstraWorkspaceOverviewController::OnWorkspaceAdded(
    const AstraWorkspace& /*workspace*/) {
  Update();
}

void AstraWorkspaceOverviewController::OnWorkspaceRemoved(
    const std::string& /*workspace_id*/) {
  Update();
}

void AstraWorkspaceOverviewController::OnWorkspaceRenamed(
    const std::string& /*workspace_id*/,
    const std::string& /*new_name*/) {
  Update();
}

void AstraWorkspaceOverviewController::OnActiveWorkspaceChanged(
    const std::string& /*old_id*/,
    const std::string& /*new_id*/) {
  Update();
}

void AstraWorkspaceOverviewController::OnWorkspacesReordered() {
  Update();
}

// =========================================================================
// AstraWorkspaceOverviewViewObserver
// =========================================================================

void AstraWorkspaceOverviewController::OnWorkspaceClicked(
    const std::string& workspace_id) {
  ActivateWorkspace(workspace_id);
}

void AstraWorkspaceOverviewController::OnWorkspaceRenameRequested(
    const std::string& workspace_id) {
  // TODO(astra): Show an inline rename text field or a rename dialog.
  //   Chromium pattern: views::Textfield in place of the label, or a
  //   BubbleDialogDelegateView for rename prompt.
  //   For now, just log that rename was requested.
  //
  // Example implementation would be:
  //   ShowRenameDialog(workspace_id);
  //   which shows a textfield dialog and calls RenameWorkspace() on OK.
}

void AstraWorkspaceOverviewController::OnWorkspaceMenuRequested(
    const std::string& workspace_id,
    const gfx::Point& screen_point) {
  ShowWorkspaceMenu(workspace_id, screen_point);
}

void AstraWorkspaceOverviewController::OnWorkspaceDeleteRequested(
    const std::string& workspace_id) {
  DeleteWorkspace(workspace_id);
}

void AstraWorkspaceOverviewController::OnNewWorkspaceRequested() {
  CreateNewWorkspace();
}

void AstraWorkspaceOverviewController::OnExportRequested() {
  ShowExportDialog();
}

void AstraWorkspaceOverviewController::OnImportRequested() {
  ShowImportDialog();
}

void AstraWorkspaceOverviewController::OnSearchQueryChanged(
    const std::u16string& /*query*/) {
  // Search filtering is handled by the view itself (it filters the list
  // of workspaces that were pushed to it).  The controller doesn't need
  // to do anything here — the view handles presentation filtering.
  //
  // TODO(astra): If search should also search tab titles / URLs, the
  //   controller would need to query TabStripModel and pass filtered
  //   results to the view.  For now, workspace name search is view-local.
}

void AstraWorkspaceOverviewController::OnOverviewClosing() {
  // The widget is closing (Escape, click outside, etc.).
  // Update our internal state.
  is_visible_ = false;
  is_animating_ = false;

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnOverviewClosing();
    observer.OnOverviewHidden();
  }

  // Don't destroy the widget — it manages its own lifecycle.
  // We keep widget_ around so we can show it again quickly.
  // The widget will self-destruct when closed, so clear our pointer.
  // TODO(astra): Decide whether to keep the widget alive for fast re-show
  //   or let it be destroyed and recreated.  For memory efficiency, we
  //   could destroy it.  For performance, we could keep it.
  widget_ = nullptr;
  overview_view_ = nullptr;
}

void AstraWorkspaceOverviewController::OnWorkspaceSelected(
    const std::string& workspace_id) {
  // Notify controller observers that a workspace was selected.
  for (auto& observer : observers_) {
    observer.OnWorkspaceSelectedInOverview(workspace_id);
  }
}

void AstraWorkspaceOverviewController::OnWorkspaceActivated(
    const std::string& workspace_id) {
  ActivateWorkspace(workspace_id);
}

void AstraWorkspaceOverviewController::OnWorkspacesReordered(
    const std::vector<std::string>& ordered_ids) {
  ReorderWorkspaces(ordered_ids);

  for (auto& observer : observers_) {
    observer.OnWorkspacesReorderedFromOverview();
  }
}

void AstraWorkspaceOverviewController::OnViewModeChanged(
    AstraWorkspaceOverviewViewMode mode) {
  // Sync to controller state and persist.
  // Guard against recursion: if we initiated the change, the view will
  // already be in the target mode and SetViewMode will early-return.
  SetViewMode(mode);
}

void AstraWorkspaceOverviewController::OnCardSizeChanged(
    AstraWorkspaceOverviewCardSize size) {
  SetCardSize(size);
}

void AstraWorkspaceOverviewController::OnShowStatisticsChanged(bool show) {
  SetShowStatistics(show);
}

void AstraWorkspaceOverviewController::OnHibernateAllRequested() {
  HibernateAllWorkspaces();
}

void AstraWorkspaceOverviewController::OnDeleteAllNonDefaultRequested() {
  DeleteAllNonDefaultWorkspaces();
}

void AstraWorkspaceOverviewController::OnOverviewSettingsRequested() {
  // TODO(astra): Show overview settings dialog/bubble.
  //   For now, the settings button just cycles view modes as a demo.
  //   Chromium pattern: views::BubbleDialogDelegateView for settings.
}

// =========================================================================
// Private helpers - widget lifecycle
// =========================================================================

void AstraWorkspaceOverviewController::CreateWidget() {
  if (!browser_view_) {
    return;
  }

  // Create a views::Widget to host the overview.
  // The widget is a child of the browser window and covers the full content
  // area.  It's not a modal dialog — it's an overlay.
  //
  // TODO(astra): Decide the exact Widget type.  Options:
  //   1. Child widget (TYPE_CONTROL) inside BrowserView's client area.
  //   2. Bubble / dialog anchored to the browser frame.
  //   3. Full-screen overlay widget.
  // For now, we use a popup widget that covers the browser window's client
  // area.  This is similar to how Chrome's tab search / tab switcher works.
  //
  // Chromium pattern: see chrome/browser/ui/views/tab_search/tab_search_bubble_host.cc
  // or chrome/browser/ui/views/frame/browser_frame.cc for overlay-style widgets.

  auto widget = std::make_unique<views::Widget>();
  views::Widget::InitParams params;

  params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.shadow_type = views::Widget::InitParams::ShadowType::kNone;
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;

  // Parent to the browser view's native window so the overlay is clipped to
  // the browser window.
  if (browser_view_->GetWidget()) {
    params.parent = browser_view_->GetWidget()->GetNativeView();
  }

  params.bounds = browser_view_->GetBoundsInScreen();
  params.delegate = new AstraWorkspaceOverviewView();  // Widget takes ownership.

  widget->Init(std::move(params));

  overview_view_ = static_cast<AstraWorkspaceOverviewView*>(
      widget->widget_delegate());

  // Wire up view observer.
  if (overview_view_) {
    overview_view_->AddObserver(this);

    // Apply presentation settings from controller state to the new view.
    overview_view_->SetViewMode(view_mode_);
    overview_view_->SetCardSize(card_size_);
    overview_view_->SetShowStatistics(show_statistics_);
  }

  widget_ = widget.release();  // Widget manages its own lifecycle.

  // Set initial opacity to 0 for the show animation.
  if (widget_ && widget_->GetLayer()) {
    widget_->GetLayer()->SetOpacity(0.0f);
  }
}

void AstraWorkspaceOverviewController::DestroyWidget() {
  if (widget_) {
    widget_->CloseNow();
    widget_ = nullptr;
    overview_view_ = nullptr;
    is_visible_ = false;
    is_animating_ = false;
  }
}

// =========================================================================
// Animation
// =========================================================================

void AstraWorkspaceOverviewController::AnimateShow() {
  if (!widget_ || !widget_->GetLayer()) {
    is_animating_ = false;
    OnShowAnimationComplete();
    return;
  }

  ui::Layer* layer = widget_->GetLayer();

  // Start state: transparent + slight offset down.
  layer->SetOpacity(0.0f);
  gfx::Transform transform;
  transform.Translate(0, kAnimationSlideDistance);
  layer->SetTransform(transform);

  // Animate to end state: fully opaque + no offset.
  {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
    settings.SetTransitionDuration(
        base::Milliseconds(kShowAnimationDurationMs));
    settings.SetTweenType(gfx::Tween::EASE_OUT);
    // Note: We don't use an animation observer here because
    // ui::LayerAnimationObserver is an abstract interface that needs
    // subclassing.  Instead, we use PostDelayedTask for the completion
    // callback, which is reliable enough for the skeleton.
    // TODO(astra): Use a proper AnimationObserver subclass or
    //   ui::ImplicitAnimationObserver for precise animation completion.
    //   Chromium pattern: ui::ImplicitAnimationObserver with
    //   OnImplicitAnimationsCompleted().

    layer->SetOpacity(1.0f);
    layer->SetTransform(gfx::Transform());
  }

  // Schedule completion callback.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&AstraWorkspaceOverviewController::OnShowAnimationComplete,
                     weak_factory_.GetWeakPtr()),
      base::Milliseconds(kShowAnimationDurationMs));

  // TODO(astra): Also animate the cards individually (staggered fade-in).
  //   Arc-style Spaces overview has cards that fade in one by one.
  //   For now, a simple cross-fade of the whole overlay is sufficient.
}

void AstraWorkspaceOverviewController::AnimateHide() {
  if (!widget_ || !widget_->GetLayer()) {
    is_animating_ = false;
    OnHideAnimationComplete();
    return;
  }

  ui::Layer* layer = widget_->GetLayer();

  // Animate from current state to transparent + slight offset down.
  {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
    settings.SetTransitionDuration(
        base::Milliseconds(kHideAnimationDurationMs));
    settings.SetTweenType(gfx::Tween::EASE_IN);

    layer->SetOpacity(0.0f);

    gfx::Transform transform;
    transform.Translate(0, kAnimationSlideDistance);
    layer->SetTransform(transform);
  }

  // Schedule completion callback.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&AstraWorkspaceOverviewController::OnHideAnimationComplete,
                     weak_factory_.GetWeakPtr()),
      base::Milliseconds(kHideAnimationDurationMs));
}

void AstraWorkspaceOverviewController::OnShowAnimationComplete() {
  is_animating_ = false;

  // Ensure focus is in the right place (search field or first card).
  if (overview_view_ && overview_view_->GetWorkspaceCardCount() > 0) {
    // Focus the first card for keyboard navigation.
    // TODO(astra): Consider focusing the search field by default for
    //   quick search.  Arc's Spaces focuses the search field.
    overview_view_->SelectWorkspaceAt(overview_view_->selected_index() >= 0
                                          ? overview_view_->selected_index()
                                          : 0);
  }
}

void AstraWorkspaceOverviewController::OnHideAnimationComplete() {
  is_animating_ = false;

  if (widget_) {
    widget_->Hide();
  }
  is_visible_ = false;

  // Reset transform for next show.
  if (widget_ && widget_->GetLayer()) {
    widget_->GetLayer()->SetTransform(gfx::Transform());
    widget_->GetLayer()->SetOpacity(1.0f);
  }

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnOverviewHidden();
  }
}

// =========================================================================
// Private helpers - data computation
// =========================================================================

std::vector<int> AstraWorkspaceOverviewController::ComputeTabCounts() const {
  // Count tabs per workspace by iterating TabStripModel and checking each
  // tab's AstraTabFeatures workspace_id.
  //
  // This is a presentation-layer computation.  The TabStripModel owns all
  // tabs; we just count how many belong to each workspace.
  //
  // Chromium subsystem: TabStripModel + AstraTabFeatures (WebContentsUserData).
  // This is the same projection pattern used by the sidebar.

  if (!workspace_service_ || !browser_view_ || !browser_view_->browser()) {
    return {};
  }

  TabStripModel* tab_strip = browser_view_->browser()->tab_strip_model();
  if (!tab_strip) {
    return {};
  }

  const auto& workspaces = workspace_service_->workspaces();
  std::vector<int> counts(workspaces.size(), 0);

  // Build a quick lookup from workspace_id to index.
  std::unordered_map<std::string, size_t> id_to_index;
  for (size_t i = 0; i < workspaces.size(); ++i) {
    id_to_index[workspaces[i].id] = i;
  }

  int tab_count = tab_strip->GetTabCount();
  for (int i = 0; i < tab_count; ++i) {
    content::WebContents* web_contents = tab_strip->GetWebContentsAt(i);
    if (!web_contents) {
      continue;
    }

    AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
    if (!features) {
      continue;
    }

    auto it = id_to_index.find(features->workspace_id());
    if (it != id_to_index.end()) {
      counts[it->second]++;
    }
  }

  return counts;
}

std::vector<int> AstraWorkspaceOverviewController::ComputeWindowCounts() const {
  if (!workspace_service_) {
    return {};
  }

  const auto& workspaces = workspace_service_->workspaces();
  std::vector<int> counts(workspaces.size(), 0);

  for (size_t i = 0; i < workspaces.size(); ++i) {
    counts[i] = static_cast<int>(
        workspace_service_->GetWindowCount(workspaces[i].id));
  }

  return counts;
}

// =========================================================================
// Presentation settings persistence
// =========================================================================

void AstraWorkspaceOverviewController::LoadPresentationSettings() {
  Profile* profile = GetProfile();
  if (!profile) {
    return;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return;
  }

  // View mode.
  int view_mode_int = prefs->GetInteger(prefs::kPrefWorkspaceOverviewViewMode);
  switch (view_mode_int) {
    case 0:
      view_mode_ = AstraWorkspaceOverviewViewMode::kGrid;
      break;
    case 1:
      view_mode_ = AstraWorkspaceOverviewViewMode::kList;
      break;
    default:
      view_mode_ = AstraWorkspaceOverviewViewMode::kGrid;
      break;
  }

  // Card size.
  int card_size_int = prefs->GetInteger(prefs::kPrefWorkspaceOverviewCardSize);
  switch (card_size_int) {
    case 0:
      card_size_ = AstraWorkspaceOverviewCardSize::kSmall;
      break;
    case 1:
      card_size_ = AstraWorkspaceOverviewCardSize::kMedium;
      break;
    case 2:
      card_size_ = AstraWorkspaceOverviewCardSize::kLarge;
      break;
    default:
      card_size_ = AstraWorkspaceOverviewCardSize::kMedium;
      break;
  }

  // Show statistics.
  show_statistics_ = prefs->GetBoolean(
      prefs::kPrefWorkspaceOverviewShowStatistics);
}

void AstraWorkspaceOverviewController::SavePresentationSettings() {
  Profile* profile = GetProfile();
  if (!profile) {
    return;
  }

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    return;
  }

  // View mode.
  int view_mode_int = 0;
  switch (view_mode_) {
    case AstraWorkspaceOverviewViewMode::kGrid:
      view_mode_int = 0;
      break;
    case AstraWorkspaceOverviewViewMode::kList:
      view_mode_int = 1;
      break;
  }
  prefs->SetInteger(prefs::kPrefWorkspaceOverviewViewMode, view_mode_int);

  // Card size.
  int card_size_int = 1;  // medium
  switch (card_size_) {
    case AstraWorkspaceOverviewCardSize::kSmall:
      card_size_int = 0;
      break;
    case AstraWorkspaceOverviewCardSize::kMedium:
      card_size_int = 1;
      break;
    case AstraWorkspaceOverviewCardSize::kLarge:
      card_size_int = 2;
      break;
  }
  prefs->SetInteger(prefs::kPrefWorkspaceOverviewCardSize, card_size_int);

  // Show statistics.
  prefs->SetBoolean(prefs::kPrefWorkspaceOverviewShowStatistics,
                    show_statistics_);
}

// =========================================================================
// Action handlers
// =========================================================================

void AstraWorkspaceOverviewController::ActivateWorkspace(
    const std::string& workspace_id) {
  // Dispatch the workspace switch through the service.
  // The service broadcasts OnActiveWorkspaceChanged to all observers,
  // including the sidebar and this controller.
  if (workspace_service_) {
    workspace_service_->ActivateWorkspace(workspace_id);
  }

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnWorkspaceActivatedFromOverview(workspace_id);
  }

  // Close the overview after switching.
  Hide();

  // TODO(astra): Animate the transition between overview and workspace.
  // Arc-style Spaces has a smooth zoom animation from the overview to the
  // selected space.  This would require coordinating with the BrowserView
  // layout and potentially Chromium's window animation system.
  // Chromium subsystem: ui/compositor/ or views::animation.
}

void AstraWorkspaceOverviewController::CreateNewWorkspace() {
  if (!workspace_service_) {
    return;
  }

  // Create a new workspace with a generated id and default name.
  size_t ws_count = workspace_service_->workspace_count();
  std::string name = "Workspace " + base::NumberToString(ws_count + 1);

  AstraWorkspace new_ws;
  new_ws.id = base::GenerateGUID();
  new_ws.name = name;
  new_ws.accent_color = PickAccentColor(ws_count);
  new_ws.created_time = base::Time::Now();
  new_ws.is_default = false;
  new_ws.order_index = ws_count;

  workspace_service_->AddWorkspace(std::move(new_ws));

  // Activate the new workspace and close the overview.
  // TODO(astra): Should we auto-activate?  Arc's Spaces does.
  workspace_service_->ActivateWorkspace(new_ws.id);
  Hide();

  // TODO(astra): Show rename prompt / inline rename for the new workspace.
  // When you create a new space in Arc, the name is immediately editable.
}

void AstraWorkspaceOverviewController::RenameWorkspace(
    const std::string& workspace_id,
    const std::string& new_name) {
  if (!workspace_service_) {
    return;
  }

  if (new_name.empty()) {
    return;
  }

  workspace_service_->RenameWorkspace(workspace_id, new_name);
  // Update will be triggered by OnWorkspaceRenamed observer.
}

void AstraWorkspaceOverviewController::DeleteWorkspace(
    const std::string& workspace_id) {
  if (!workspace_service_) {
    return;
  }

  const AstraWorkspace* ws = workspace_service_->GetWorkspace(workspace_id);
  if (!ws) {
    return;
  }

  // Can't delete the default workspace.
  if (ws->is_default) {
    // TODO(astra): Show an error message or disable the delete action
    //   for the default workspace.  For now, silently ignore.
    return;
  }

  // TODO(astra): Show a confirmation dialog before deleting.
  //   Chromium pattern: views::MessageBoxView or a custom confirmation
  //   bubble.  Arc's Spaces has a "Delete?" confirmation.
  //   For now, delete immediately (skeleton behavior).
  //
  // TODO(astra): Also handle the case where the deleted workspace is the
  //   currently selected one in the overview — should selection move to
  //   the next workspace or the active workspace?

  workspace_service_->DeleteWorkspace(workspace_id);
  // Update will be triggered by OnWorkspaceRemoved observer.
}

void AstraWorkspaceOverviewController::ReorderWorkspaces(
    const std::vector<std::string>& ordered_ids) {
  if (!workspace_service_) {
    return;
  }

  workspace_service_->ReorderWorkspaces(ordered_ids);
  // Update will be triggered by OnWorkspacesReordered observer.
}

void AstraWorkspaceOverviewController::ShowImportDialog() {
  if (!overview_view_ || !widget_) {
    return;
  }

  // Show the import dialog anchored to the import button or centered.
  // TODO(astra): Anchor the dialog to the Import button in the header.
  //   For now, anchor to the overview view at the top-right.
  Profile* profile = GetProfile();
  if (!profile) {
    return;
  }

  import_export_dialog_ = AstraWorkspaceImportExportDialog::ShowBubble(
      overview_view_,
      profile,
      AstraWorkspaceImportExportDialog::Mode::kImport);

  if (import_export_dialog_) {
    import_export_dialog_->SetImportCallback(base::BindRepeating(
        [](AstraWorkspaceOverviewController* controller,
           const std::string& json) {
          // TODO(astra): Actually import the JSON via
          //   AstraWorkspaceImportExport.  For now, we just close the
          //   dialog — import logic is in the browser layer.
          //   Chromium pattern: controller delegates to service/helper.
          if (controller->workspace_service_) {
            // Import goes through AstraWorkspaceImportExport.
            // TODO(astra): Wire up AstraWorkspaceImportExport::ImportFromJson.
          }
        },
        base::Unretained(this)));

    import_export_dialog_->SetCloseCallback(base::BindRepeating(
        [](AstraWorkspaceOverviewController* controller) {
          controller->import_export_dialog_ = nullptr;
        },
        base::Unretained(this)));
  }
}

void AstraWorkspaceOverviewController::ShowExportDialog() {
  if (!overview_view_ || !widget_) {
    return;
  }

  Profile* profile = GetProfile();
  if (!profile) {
    return;
  }

  import_export_dialog_ = AstraWorkspaceImportExportDialog::ShowBubble(
      overview_view_,
      profile,
      AstraWorkspaceImportExportDialog::Mode::kExport);

  if (import_export_dialog_) {
    import_export_dialog_->SetCloseCallback(base::BindRepeating(
        [](AstraWorkspaceOverviewController* controller) {
          controller->import_export_dialog_ = nullptr;
        },
        base::Unretained(this)));
  }
}

void AstraWorkspaceOverviewController::ShowWorkspaceMenu(
    const std::string& workspace_id,
    const gfx::Point& screen_point) {
  // TODO(astra): Show a context menu with workspace actions:
  //   - Rename
  //   - Delete (if not default)
  //   - Duplicate
  //   - Move left / right (reorder)
  //   - Change accent color
  //
  // Chromium pattern: ui/views/controls/menu/menu_runner.h with
  //   ui::MenuModel or views::MenuItemView.
  //   For the skeleton, we skip the actual menu and just log.
  //
  // TODO(astra): Build a proper menu using views::MenuRunner and a
  //   simple menu model.  This requires integrating with Chromium's
  //   menu system which uses ui/models/ or views::MenuItemView.

  // For now, trigger rename as a placeholder action (F2-like behavior).
  // This makes the menu button at least do something useful in the skeleton.
  //
  // TODO(astra): Replace with proper menu.
  if (workspace_service_) {
    const AstraWorkspace* ws =
        workspace_service_->GetWorkspace(workspace_id);
    if (ws && !ws->is_default) {
      // As a placeholder, cycle the accent color when menu is clicked.
      // This demonstrates that the menu button works without building
      // a full menu system.
      size_t current_index = 0;
      for (size_t i = 0; i < kAccentColorPaletteSize; ++i) {
        if (ws->accent_color == kAccentColorPalette[i]) {
          current_index = i;
          break;
        }
      }
      size_t next_index = (current_index + 1) % kAccentColorPaletteSize;
      workspace_service_->SetWorkspaceAccentColor(
          workspace_id, kAccentColorPalette[next_index]);
    }
  }
}

Profile* AstraWorkspaceOverviewController::GetProfile() const {
  if (!browser_view_ || !browser_view_->browser()) {
    return nullptr;
  }
  return browser_view_->browser()->profile();
}

}  // namespace astra
