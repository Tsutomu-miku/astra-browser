#ifndef ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_CONTROLLER_H_
#define ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_CONTROLLER_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "ui/gfx/geometry/point.h"
#include "astra/browser/astra_focus_mode_service.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_indicator.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_model.h"

class BrowserView;
class PrefService;

namespace astra {

class AstraSidebarView;
class AstraFocusModeMenuBubble;

// =========================================================================
// AstraFocusModeController — per-window focus mode UI controller
// =========================================================================
//
// Controller that manages focus mode visual changes for a single BrowserView.
// It bridges the AstraFocusModeService (truth source) with the
// AstraFocusModeModel (UI state) and the views (indicator, menu bubble).
//
// Architecture (MVC):
//   - Model: AstraFocusModeModel — owns UI state and presentation settings.
//   - View: AstraFocusModeIndicator + AstraFocusModeMenuBubble — pure
//     presentation, no state.
//   - Controller: AstraFocusModeController — orchestrates model and views,
//     syncs with the service, handles user actions.
//
// The controller is the projection layer — it never stores truth state.
// It reads focus mode state from the service, updates the model, and
// the views reflect model state.
//
// Chromium subsystems reused:
//   - PrefService (via model, for settings persistence).
//   - Widget / Views framework (for the indicator bubble).
//
// Chromium patch points:
//   - BrowserView: to install the controller and manage layout changes.
// =========================================================================

class AstraFocusModeController
    : public AstraFocusModeServiceObserver,
      public AstraFocusModeIndicatorDelegate,
      public AstraFocusModeObserver {
 public:
  // Constructs a controller for |browser_view| using |pref_service|
  // for settings persistence.  If |pref_service| is null, settings
  // fall back to in-memory defaults (useful for tests).
  explicit AstraFocusModeController(BrowserView* browser_view,
                                     PrefService* pref_service = nullptr);
  AstraFocusModeController(const AstraFocusModeController&) = delete;
  AstraFocusModeController& operator=(const AstraFocusModeController&) =
      delete;
  ~AstraFocusModeController() override;

  // -- Focus mode control --------------------------------------------------

  // Toggles focus mode on/off. When turning on, uses the default duration.
  void ToggleFocusMode();

  // Starts focus mode with the default duration.
  void StartFocusMode();

  // Ends the current focus mode session.
  void EndFocusMode();

  // Returns true if focus mode is currently active.
  bool IsActive() const;

  // Pauses the current focus session.
  void PauseFocusMode();

  // Resumes a paused focus session.
  void ResumeFocusMode();

  // Returns true if the current session is paused.
  bool IsPaused() const;

  // -- Session duration ----------------------------------------------------

  // Sets the session duration. If active, resets remaining time.
  void SetDuration(base::TimeDelta duration);

  // Returns the total session duration.
  base::TimeDelta GetDuration() const;

  // Returns the time remaining in the current session.
  base::TimeDelta GetTimeRemaining() const;

  // Extends the current session by |extension|.
  void ExtendSession(base::TimeDelta extension);

  // -- Model access --------------------------------------------------------

  // Returns the focus mode model.
  AstraFocusModeModel* GetModel();
  const AstraFocusModeModel* GetModel() const;

  // Sets the model. Takes ownership.
  void SetModel(std::unique_ptr<AstraFocusModeModel> model);

  // -- Menu bubble ---------------------------------------------------------

  // Shows the focus mode menu bubble anchored at |anchor| point (screen coords).
  void ShowMenu(const gfx::Point& anchor);

  // Hides the focus mode menu bubble if it's visible.
  void HideMenu();

  // Returns true if the menu bubble is currently visible.
  bool IsMenuVisible() const;

  // -- Indicator -----------------------------------------------------------

  // Shows the focus mode indicator.
  void ShowIndicator();

  // Hides the focus mode indicator.
  void HideIndicator();

  // Returns true if the indicator is currently visible.
  bool IsIndicatorVisible() const;

  // Returns the indicator view, or null if not shown.
  AstraFocusModeIndicator* GetIndicatorView();

  // -- Session control (legacy API for compatibility) ---------------------

  void StartFocusSession(base::TimeDelta duration);
  void EndFocusSession();
  void PauseFocusSession();
  void ResumeFocusSession();
  void StartBreak(bool is_long = false);
  bool IsFocusModeActive() const;
  bool IsSessionPaused() const;

  // -- Model access (legacy) ----------------------------------------------

  AstraFocusModeModel* model() { return model_.get(); }
  const AstraFocusModeModel* model() const { return model_.get(); }
  AstraFocusModeIndicator* indicator() { return indicator_; }
  const AstraFocusModeIndicator* indicator() const { return indicator_; }

  // -- Distraction blocking ------------------------------------------------

  void RecordDistractionBlocked(const GURL& url);
  size_t GetDistractionsBlockedCount() const;

  // -- Break reminders -----------------------------------------------------

  bool AreBreakRemindersEnabled() const;
  void OnBreakReminder();

  // -- AstraFocusModeServiceObserver ---------------------------------------

  void OnFocusModeEntered(base::TimeDelta duration) override;
  void OnFocusModeExited() override;
  void OnFocusTimeUpdated(base::TimeDelta remaining) override;
  void OnDistractionBlocklistChanged() override;
  void OnFocusPhaseChanged(AstraFocusPhase new_phase) override;
  void OnPomodoroCycleCompleted(int cycle_count) override;
  void OnFocusSessionPaused() override;
  void OnFocusSessionResumed() override;
  void OnFocusSessionCompleted(base::TimeDelta total_duration) override;
  void OnWhitelistChanged() override;
  void OnDistractionWarning(const std::string& url) override;
  void OnStatsUpdated() override;
  void OnPresetsChanged() override;
  void OnAutoStartSettingsChanged() override;

  // -- AstraFocusModeIndicatorDelegate -------------------------------------

  void OnExtendFocusMode(base::TimeDelta duration) override;
  void OnEndFocusMode() override;
  void OnOpenSettings() override;
  void OnPauseFocusMode() override;
  void OnResumeFocusMode() override;
  void OnStartBreak() override;
  size_t GetDistractionsBlockedCount() override;

  // -- AstraFocusModeObserver ---------------------------------------------

  void OnFocusModeStarted(AstraFocusModeModel* model) override;
  void OnFocusModeEnded(AstraFocusModeModel* model) override;
  void OnFocusModePaused(AstraFocusModeModel* model) override;
  void OnFocusModeResumed(AstraFocusModeModel* model) override;
  void OnFocusTimeUpdated(AstraFocusModeModel* model,
                          base::TimeDelta remaining) override;
  void OnBlockLevelChanged(AstraFocusModeModel* model,
                           AstraFocusBlockLevel level) override;
  void OnBlockedSitesChanged(AstraFocusModeModel* model) override;
  void OnSessionCompleted(AstraFocusModeModel* model,
                          const AstraFocusSession& session) override;
  void OnFocusModeModelShutdown(AstraFocusModeModel* model) override;

 private:
  // Activates focus mode visual changes in the window.
  void ActivateFocusModeUI();

  // Deactivates focus mode visual changes in the window.
  void DeactivateFocusModeUI();

  // Updates the indicator with current remaining time.
  void UpdateIndicatorRemainingTime();

  // Updates the indicator's paused state.
  void UpdateIndicatorPausedState();

  // Updates the indicator's block level display.
  void UpdateIndicatorBlockLevel();

  // Shows the focus mode indicator widget.
  void ShowIndicatorInternal();

  // Hides the focus mode indicator widget.
  void HideIndicatorInternal();

  // Shows the menu bubble anchored to the indicator.
  void ShowMenuBubble();

  // Hides the menu bubble.
  void HideMenuBubble();

  // Hides the sidebar for focus mode.
  void HideSidebarForFocusMode();

  // Restores sidebar from focus mode.
  void RestoreSidebarFromFocusMode();

  // Applies dim effect to non-focus tabs.
  void ApplyNonFocusTabDimming();

  // Restores non-focus tabs from dimmed state.
  void RestoreNonFocusTabDimming();

  // Starts the break reminder timer if enabled.
  void StartBreakReminderTimer();

  // Stops the break reminder timer.
  void StopBreakReminderTimer();

  // Syncs the model state with the current service state.
  void SyncModelWithService();

  // Gets the focus mode service from the browser's profile.
  AstraFocusModeService* GetService() const;

  raw_ptr<BrowserView> browser_view_;
  std::unique_ptr<AstraFocusModeModel> model_;

  // Focus mode indicator widget (owned by the native widget system).
  raw_ptr<AstraFocusModeIndicator> indicator_ = nullptr;

  // Focus mode menu bubble (owned by the widget system).
  raw_ptr<AstraFocusModeMenuBubble> menu_bubble_ = nullptr;

  // Saved sidebar visibility state before focus mode was activated.
  bool sidebar_was_visible_ = true;
  bool sidebar_was_pinned_ = true;

  // Saved tab dim state before focus mode.
  bool tabs_were_dimmed_ = false;

  // Break reminder timer.
  base::RepeatingTimer break_reminder_timer_;

  // Whether we've registered as a service observer.
  bool observing_service_ = false;

  // Whether we're observing the model.
  bool observing_model_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_CONTROLLER_H_
