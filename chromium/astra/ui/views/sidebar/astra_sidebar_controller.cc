#include "astra/ui/views/sidebar/astra_sidebar_controller.h"

#include <string>
#include <vector>

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_prefs.h"
#include "astra/ui/views/sidebar/astra_sidebar_model.h"
#include "astra/ui/views/sidebar/astra_sidebar_view.h"

namespace astra {

// =========================================================================
// AstraSidebarController
// =========================================================================

AstraSidebarController::AstraSidebarController(Browser* browser)
    : browser_(browser) {
  DCHECK(browser_);

  // Get the profile's pref service.
  if (browser_->profile()) {
    pref_service_ = browser_->profile()->GetPrefs();
  }

  // Create the model.
  model_ = std::make_unique<AstraSidebarModel>(pref_service_);
  model_observation_.Observe(model_.get());
}

AstraSidebarController::~AstraSidebarController() {
  // Detach from the view if we have one.
  if (sidebar_view_) {
    // The view will detach from us when it's destroyed, but we should
    // clear our reference to avoid dangling pointers.
    sidebar_view_ = nullptr;
  }
}

// -- Sidebar control -------------------------------------------------------

void AstraSidebarController::ShowSidebar() {
  if (model_) {
    model_->SetVisible(true);
  }
}

void AstraSidebarController::HideSidebar() {
  if (model_) {
    model_->SetVisible(false);
  }
}

void AstraSidebarController::ToggleSidebar() {
  if (model_) {
    model_->ToggleVisible();
  }
}

void AstraSidebarController::TogglePinned() {
  if (model_) {
    model_->TogglePinned();
  }
}

void AstraSidebarController::SetSidebarWidth(int width) {
  if (model_) {
    model_->SetWidth(width);
  }
}

void AstraSidebarController::SetSidebarPosition(AstraSidebarPosition position) {
  if (model_) {
    model_->SetPosition(position);
  }
}

// -- Section control -------------------------------------------------------

bool AstraSidebarController::ActivateSection(const std::string& section_id) {
  if (!model_) {
    return false;
  }

  // Only activate visible sections.
  auto section = model_->GetSectionById(section_id);
  if (!section.has_value() || !section->is_visible) {
    return false;
  }

  model_->SetActiveSection(section_id);
  return true;
}

void AstraSidebarController::ActivateNextSection() {
  if (!model_) {
    return;
  }

  auto visible = model_->GetVisibleSections();
  if (visible.empty()) {
    return;
  }

  int current_index = GetActiveSectionVisibleIndex();
  int next_index = current_index + 1;
  if (next_index >= static_cast<int>(visible.size())) {
    next_index = 0;  // wrap around
  }

  if (next_index >= 0 && next_index < static_cast<int>(visible.size())) {
    model_->SetActiveSection(visible[next_index].id);
  }
}

void AstraSidebarController::ActivatePreviousSection() {
  if (!model_) {
    return;
  }

  auto visible = model_->GetVisibleSections();
  if (visible.empty()) {
    return;
  }

  int current_index = GetActiveSectionVisibleIndex();
  int prev_index = current_index - 1;
  if (prev_index < 0) {
    prev_index = static_cast<int>(visible.size()) - 1;  // wrap around
  }

  if (prev_index >= 0 && prev_index < static_cast<int>(visible.size())) {
    model_->SetActiveSection(visible[prev_index].id);
  }
}

bool AstraSidebarController::ToggleSectionCollapsed(
    const std::string& section_id) {
  if (!model_) {
    return false;
  }
  return model_->ToggleSectionCollapsed(section_id);
}

bool AstraSidebarController::ToggleSectionVisibility(
    const std::string& section_id) {
  if (!model_) {
    return false;
  }
  return model_->ToggleSectionVisible(section_id);
}

// -- View binding ----------------------------------------------------------

void AstraSidebarController::SetSidebarView(AstraSidebarView* view) {
  sidebar_view_ = view;
  // TODO(astra): When the view is set, push current model state to it
  // so the view reflects the model's initial state.
}

// -- Data loading ----------------------------------------------------------

void AstraSidebarController::RefreshAllSections() {
  if (!sidebar_view_) {
    return;
  }
  // The view handles full refresh via UpdateFromModel().
  // TODO(astra): Wire up section-specific refresh calls when the view
  // supports per-section updates.
  sidebar_view_->UpdateFromModel();
}

bool AstraSidebarController::RefreshSection(const std::string& section_id) {
  if (!sidebar_view_ || !model_) {
    return false;
  }
  // Validate the section exists.
  auto section = model_->GetSectionById(section_id);
  if (!section.has_value()) {
    return false;
  }
  // For now, do a full refresh since the view doesn't support per-section
  // incremental updates yet.
  // TODO(astra): Implement per-section refresh in the view and call those
  // methods directly here.
  sidebar_view_->UpdateFromModel();
  return true;
}

// -- AstraSidebarModelObserver ---------------------------------------------

void AstraSidebarController::OnSidebarShown() {
  // The view will handle its own visibility update via observing the model.
  // The controller can also trigger side effects like logging or telemetry.
}

void AstraSidebarController::OnSidebarHidden() {
  // Auto-hide side effects can go here.
}

void AstraSidebarController::OnSidebarPinnedChanged(bool pinned) {
  // Pinned state change — view handles its own layout update.
}

void AstraSidebarController::OnActiveSectionChanged(
    const std::string& section_id) {
  // Persist last active section if remember_last_section is enabled.
  // (Model already handles this in SetActiveSection.)
}

void AstraSidebarController::OnSectionVisibilityChanged(
    const std::string& section_id,
    bool visible) {
  // If the active section was hidden, switch to the first visible section.
  if (!visible && model_ && section_id == model_->active_section_id()) {
    auto visible_sections = model_->GetVisibleSections();
    if (!visible_sections.empty()) {
      // Use the model's SetActiveSection directly to avoid recursion.
      model_->SetActiveSection(visible_sections[0].id);
    }
  }
}

void AstraSidebarController::OnSectionOrderChanged() {
  // Section order changed — view handles its own layout.
}

void AstraSidebarController::OnSectionCollapsedChanged(
    const std::string& section_id,
    bool collapsed) {
  // Collapsed state changed — view handles its own layout.
}

void AstraSidebarController::OnSidebarWidthChanged(int width) {
  // Width changed — view handles its own resize.
}

void AstraSidebarController::OnSidebarPositionChanged(
    AstraSidebarPosition position) {
  // Position changed — view handles its own position update.
}

void AstraSidebarController::OnSidebarSettingsChanged() {
  // Any presentation setting changed — view should refresh its appearance.
}

// -- Private helpers -------------------------------------------------------

int AstraSidebarController::GetActiveSectionVisibleIndex() const {
  if (!model_) {
    return -1;
  }
  auto visible = model_->GetVisibleSections();
  const std::string& active_id = model_->active_section_id();
  for (size_t i = 0; i < visible.size(); ++i) {
    if (visible[i].id == active_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace astra
