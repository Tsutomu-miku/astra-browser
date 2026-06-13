#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_CONTROLLER_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "astra/ui/views/sidebar/astra_sidebar_model.h"

class Browser;
class PrefService;

namespace astra {

class AstraSidebarView;

// =========================================================================
// AstraSidebarController — bridges the sidebar model and views
// =========================================================================
//
// The controller sits between the sidebar model (state/settings) and the
// sidebar view (presentation). It manages:
//
// 1. Sidebar show/hide lifecycle — toggling visibility, handling auto-hide.
// 2. Section switching — coordinating active section changes.
// 3. Service integration — loading data from Chromium services into views.
// 4. Command handling — dispatching sidebar-related commands.
//
// The controller owns the model and observes it for state changes. It does
// not own the view — the view is owned by the BrowserView hierarchy. The
// controller is the view's delegate for user actions that affect model
// state.
//
// Truth hierarchy:
//   - Model — owns state and settings (source of truth).
//   - Controller — orchestrates state changes and service interactions.
//   - View — pure presentation, delegates user actions to controller.
//
// Chromium subsystems reused:
//   - PrefService (persistence, via model).
//   - Observer pattern.
//
// Chromium patch point: none — this is pure Astra presentation-layer code.
class AstraSidebarController : public AstraSidebarModelObserver {
 public:
  explicit AstraSidebarController(Browser* browser);
  ~AstraSidebarController() override;

  AstraSidebarController(const AstraSidebarController&) = delete;
  AstraSidebarController& operator=(const AstraSidebarController&) = delete;

  // -- Sidebar control -----------------------------------------------------

  // Show the sidebar (set visible = true).
  void ShowSidebar();

  // Hide the sidebar (set visible = false).
  void HideSidebar();

  // Toggle sidebar visibility.
  void ToggleSidebar();

  // Toggle sidebar pinned state.
  void TogglePinned();

  // Set the sidebar width in pixels (clamped to valid range).
  void SetSidebarWidth(int width);

  // Set the sidebar position (left or right).
  void SetSidebarPosition(AstraSidebarPosition position);

  // -- Section control -----------------------------------------------------

  // Switch to the section with the given ID.
  // Returns true if the section exists and was activated.
  bool ActivateSection(const std::string& section_id);

  // Switch to the next visible section (wraps around).
  void ActivateNextSection();

  // Switch to the previous visible section (wraps around).
  void ActivatePreviousSection();

  // Toggle a section's collapsed state.
  bool ToggleSectionCollapsed(const std::string& section_id);

  // Toggle a section's visibility.
  bool ToggleSectionVisibility(const std::string& section_id);

  // -- View binding --------------------------------------------------------

  // Set the view this controller manages.  Not owned by the controller.
  // The view calls this when it's constructed and sets itself as the
  // controller's view.
  void SetSidebarView(AstraSidebarView* view);

  // -- Data loading --------------------------------------------------------

  // Refresh all sidebar sections from their underlying data sources.
  void RefreshAllSections();

  // Refresh a specific section by ID.
  // Returns true if the section exists and was refreshed.
  bool RefreshSection(const std::string& section_id);

  // -- Accessors -----------------------------------------------------------

  AstraSidebarModel* model() { return model_.get(); }
  const AstraSidebarModel* model() const { return model_.get(); }

  AstraSidebarView* sidebar_view() { return sidebar_view_; }
  const AstraSidebarView* sidebar_view() const { return sidebar_view_; }

  Browser* browser() { return browser_; }
  const Browser* browser() const { return browser_; }

  // -- AstraSidebarModelObserver -------------------------------------------

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

 private:
  // Find the index of the active section within the visible sections list.
  // Returns -1 if the active section is not visible or doesn't exist.
  int GetActiveSectionVisibleIndex() const;

  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<PrefService> pref_service_ = nullptr;

  // The model owned by this controller.
  std::unique_ptr<AstraSidebarModel> model_;

  // The view managed by this controller.  Not owned.
  raw_ptr<AstraSidebarView> sidebar_view_ = nullptr;

  // Observation of the model.
  base::ScopedObservation<AstraSidebarModel, AstraSidebarModelObserver>
      model_observation_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_CONTROLLER_H_
