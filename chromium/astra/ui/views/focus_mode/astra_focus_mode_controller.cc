#include "astra/ui/views/focus_mode/astra_focus_mode_controller.h"

#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/gfx/geometry/point.h"

#include "astra/browser/astra_focus_mode_service.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_indicator.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_menu_bubble.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_model.h"
#include "astra/ui/views/sidebar/astra_sidebar_view.h"

namespace astra {

// ---------------------------------------------------------------------------
// AstraFocusModeController
// ---------------------------------------------------------------------------

AstraFocusModeController::AstraFocusModeController(
    BrowserView* browser_view,
    PrefService* pref_service)
    : browser_view_(browser_view),
      model_(std::make_unique<AstraFocusModeModel>(pref_service)) {
  // Observe the model for state changes.
  model_->AddObserver(this);
  observing_model_ = true;

  // Subscribe to focus mode service state changes.
  if (auto* service = GetService()) {
    service->AddObserver(this);
    observing_service_ = true;

    // If focus mode is already active, sync our UI state.
    if (service->IsFocusModeActive()) {
      SyncModelWithService();
      ActivateFocusModeUI();
    }
  }
}

AstraFocusModeController::~AstraFocusModeController() {
  // Clean up: ensure focus mode UI is deactivated and we're removed as
  // an observer before the service goes away.
  if (observing_service_) {
    if (auto* service = GetService()) {
      service->RemoveObserver(this);
    }
    observing_service_ = false;
  }

  // Stop observing the model.
  if (observing_model_ && model_) {
    model_->RemoveObserver(this);
    observing_model_ = false;
  }

  // Hide the menu bubble if it's still open.
  if (menu_bubble_) {
    menu_bubble_->Close();
    menu_bubble_ = nullptr;
  }

  // Make sure we clean up the indicator if it's still around.
  if (indicator_) {
    HideIndicator();
  }

  // Stop the break reminder timer.
  StopBreakReminderTimer();
}

// -- Focus mode control ----------------------------------------------------

void AstraFocusModeController::ToggleFocusMode() {
  if (model_->IsActive()) {
    EndFocusMode();
  } else {
    StartFocusMode();
  }
}

void AstraFocusModeController::StartFocusMode() {
  if (auto* service = GetService()) {
    service->EnterFocusMode(model_->GetDefaultDuration());
  } else {
    // No service available (e.g. in tests). Update model directly.
    base::TimeDelta duration = model_->GetDefaultDuration();
    model_->SetDuration(duration);
    model_->SetRemainingTime(duration);
    model_->SetElapsedTime(base::TimeDelta());
    model_->SetSessionStartTime(base::Time::Now());
    model_->SetActive(true);
    ActivateFocusModeUI();
  }
}

void AstraFocusModeController::EndFocusMode() {
  if (auto* service = GetService()) {
    service->ExitFocusMode();
  } else {
    model_->EndSession();
    DeactivateFocusModeUI();
  }
}

bool AstraFocusModeController::IsActive() const {
  return model_->IsActive();
}

void AstraFocusModeController::PauseFocusMode() {
  if (auto* service = GetService()) {
    service->PauseSession();
  } else {
    model_->PauseSession();
  }
}

void AstraFocusModeController::ResumeFocusMode() {
  if (auto* service = GetService()) {
    service->ResumeSession();
  } else {
    model_->ResumeSession();
  }
}

bool AstraFocusModeController::IsPaused() const {
  return model_->IsSessionPaused();
}

// -- Session duration ------------------------------------------------------

void AstraFocusModeController::SetDuration(base::TimeDelta duration) {
  model_->SetDuration(duration);
  if (indicator_) {
    indicator_->UpdateRemainingTime(model_->GetTimeRemaining());
  }
}

base::TimeDelta AstraFocusModeController::GetDuration() const {
  return model_->GetDuration();
}

base::TimeDelta AstraFocusModeController::GetTimeRemaining() const {
  return model_->GetTimeRemaining();
}

void AstraFocusModeController::ExtendSession(base::TimeDelta extension) {
  if (auto* service = GetService()) {
    service->ExtendFocusSession(extension);
  } else {
    model_->ExtendSession(extension);
  }
}

// -- Model access ----------------------------------------------------------

AstraFocusModeModel* AstraFocusModeController::GetModel() {
  return model_.get();
}

const AstraFocusModeModel* AstraFocusModeController::GetModel() const {
  return model_.get();
}

void AstraFocusModeController::SetModel(
    std::unique_ptr<AstraFocusModeModel> model) {
  // Stop observing old model.
  if (observing_model_ && model_) {
    model_->RemoveObserver(this);
    observing_model_ = false;
  }

  model_ = std::move(model);

  // Start observing new model.
  if (model_) {
    model_->AddObserver(this);
    observing_model_ = true;
  }
}

// -- Menu bubble -----------------------------------------------------------

void AstraFocusModeController::ShowMenu(const gfx::Point& anchor) {
  if (menu_bubble_) {
    return;
  }
  // TODO(astra): Show menu bubble at the given anchor point.
  //   For now, we show it relative to the indicator if available.
  if (indicator_) {
    ShowMenuBubble();
  }
}

void AstraFocusModeController::HideMenu() {
  HideMenuBubble();
}

bool AstraFocusModeController::IsMenuVisible() const {
  return menu_bubble_ != nullptr;
}

// -- Indicator -------------------------------------------------------------

void AstraFocusModeController::ShowIndicator() {
  if (indicator_ || !browser_view_) {
    return;
  }
  ShowIndicatorInternal();
}

void AstraFocusModeController::HideIndicator() {
  if (!indicator_) {
    return;
  }
  HideIndicatorInternal();
}

bool AstraFocusModeController::IsIndicatorVisible() const {
  return indicator_ != nullptr;
}

AstraFocusModeIndicator* AstraFocusModeController::GetIndicatorView() {
  return indicator_;
}

// -- Session control (legacy API) ------------------------------------------

void AstraFocusModeController::StartFocusSession(base::TimeDelta duration) {
  if (auto* service = GetService()) {
    service->EnterFocusMode(duration);
  } else {
    model_->SetDuration(duration);
    model_->SetRemainingTime(duration);
    model_->SetElapsedTime(base::TimeDelta());
    model_->SetSessionStartTime(base::Time::Now());
    model_->SetActive(true);
    ActivateFocusModeUI();
  }
}

void AstraFocusModeController::EndFocusSession() {
  EndFocusMode();
}

void AstraFocusModeController::PauseFocusSession() {
  PauseFocusMode();
}

void AstraFocusModeController::ResumeFocusSession() {
  ResumeFocusMode();
}

void AstraFocusModeController::StartBreak(bool is_long) {
  if (auto* service = GetService()) {
    service->StartBreak(is_long);
  }
  // TODO(astra): Show break UI in indicator if no service available.
}

bool AstraFocusModeController::IsFocusModeActive() const {
  return model_->IsActive();
}

bool AstraFocusModeController::IsSessionPaused() const {
  return model_->IsSessionPaused();
}

// -- Distraction blocking --------------------------------------------------

void AstraFocusModeController::RecordDistractionBlocked(const GURL& url) {
  model_->IncrementDistractionsBlocked(url);
}

size_t AstraFocusModeController::GetDistractionsBlockedCount() const {
  return model_->distractions_blocked();
}

// -- Break reminders -------------------------------------------------------

bool AstraFocusModeController::AreBreakRemindersEnabled() const {
  return model_->GetShowBreakReminder();
}

void AstraFocusModeController::OnBreakReminder() {
  // TODO(astra): Show a break reminder notification.
  //   Chromium owner: NotificationService / message_center.
  //   Patch point: Use NotificationUIManager to show a notification.
  //
  // For now, the break reminder is just a timer event that could trigger
  // UI changes (e.g. pulse the indicator).
  if (indicator_) {
    indicator_->StartPulseAnimation();
  }
}

// -- AstraFocusModeServiceObserver -----------------------------------------

void AstraFocusModeController::OnFocusModeEntered(base::TimeDelta duration) {
  model_->SetDuration(duration);
  model_->SetRemainingTime(duration);
  model_->SetElapsedTime(base::TimeDelta());
  model_->SetSessionStartTime(base::Time::Now());
  model_->SetActive(true);
  ActivateFocusModeUI();
}

void AstraFocusModeController::OnFocusModeExited() {
  model_->EndSession();
  DeactivateFocusModeUI();
}

void AstraFocusModeController::OnFocusTimeUpdated(base::TimeDelta remaining) {
  model_->SetRemainingTime(remaining);
  base::TimeDelta elapsed = model_->GetDuration() - remaining;
  if (elapsed.is_negative()) {
    elapsed = base::TimeDelta();
  }
  model_->SetElapsedTime(elapsed);
  UpdateIndicatorRemainingTime();
}

void AstraFocusModeController::OnDistractionBlocklistChanged() {
  // Blocklist changed — if there's an open menu, refresh the whitelist section.
  // TODO(astra): Refresh the menu bubble's whitelist UI if open.
}

void AstraFocusModeController::OnFocusPhaseChanged(AstraFocusPhase new_phase) {
  // Phase changed (work -> break or break -> work).
  // Update the indicator state/color to reflect the current phase.
  if (indicator_) {
    indicator_->UpdatePhase(new_phase);
  }
}

void AstraFocusModeController::OnPomodoroCycleCompleted(int cycle_count) {
  // Cycle completed — could show a celebration animation or notification.
}

void AstraFocusModeController::OnFocusSessionPaused() {
  model_->PauseSession();
}

void AstraFocusModeController::OnFocusSessionResumed() {
  model_->ResumeSession();
}

void AstraFocusModeController::OnFocusSessionCompleted(
    base::TimeDelta total_duration) {
  // Session completed naturally — update stats.
  model_->AddFocusMinutesToday(
      static_cast<int>(total_duration.InMinutes()));
  model_->IncrementSessionsToday();
}

void AstraFocusModeController::OnWhitelistChanged() {
  // Whitelist changed — refresh any open UI that shows it.
}

void AstraFocusModeController::OnDistractionWarning(const std::string& url) {
  model_->IncrementDistractionsBlocked(GURL(url));
}

void AstraFocusModeController::OnStatsUpdated() {
  // Service stats updated — sync with model if needed.
}

void AstraFocusModeController::OnPresetsChanged() {
  // Presets changed — refresh menu if open.
}

void AstraFocusModeController::OnAutoStartSettingsChanged() {
  // Auto-start settings changed.
}

// -- AstraFocusModeIndicatorDelegate ---------------------------------------

void AstraFocusModeController::OnExtendFocusMode(base::TimeDelta duration) {
  ExtendSession(duration);
}

void AstraFocusModeController::OnEndFocusMode() {
  EndFocusMode();
}

void AstraFocusModeController::OnOpenSettings() {
  // TODO(astra): Open the focus mode settings page.
  //   Chromium owner: Settings UI / chrome://settings
  //   Patch point: Navigate to chrome://settings/focusMode or similar.
}

void AstraFocusModeController::OnPauseFocusMode() {
  PauseFocusMode();
}

void AstraFocusModeController::OnResumeFocusMode() {
  ResumeFocusMode();
}

void AstraFocusModeController::OnStartBreak() {
  StartBreak(false);  // Default to short break.
}

size_t AstraFocusModeController::GetDistractionsBlockedCount() {
  return model_->distractions_blocked();
}

// -- AstraFocusModeObserver ------------------------------------------------

void AstraFocusModeController::OnFocusModeStarted(AstraFocusModeModel* model) {
  DCHECK_EQ(model, model_.get());
  ActivateFocusModeUI();
}

void AstraFocusModeController::OnFocusModeEnded(AstraFocusModeModel* model) {
  DCHECK_EQ(model, model_.get());
  DeactivateFocusModeUI();
}

void AstraFocusModeController::OnFocusModePaused(AstraFocusModeModel* model) {
  DCHECK_EQ(model, model_.get());
  UpdateIndicatorPausedState();
  StopBreakReminderTimer();
}

void AstraFocusModeController::OnFocusModeResumed(AstraFocusModeModel* model) {
  DCHECK_EQ(model, model_.get());
  UpdateIndicatorPausedState();
  StartBreakReminderTimer();
}

void AstraFocusModeController::OnFocusTimeUpdated(AstraFocusModeModel* model,
                                                  base::TimeDelta remaining) {
  DCHECK_EQ(model, model_.get());
  UpdateIndicatorRemainingTime();
}

void AstraFocusModeController::OnBlockLevelChanged(AstraFocusModeModel* model,
                                                    AstraFocusBlockLevel level) {
  DCHECK_EQ(model, model_.get());
  UpdateIndicatorBlockLevel();
}

void AstraFocusModeController::OnBlockedSitesChanged(AstraFocusModeModel* model) {
  DCHECK_EQ(model, model_.get());
  // If menu is open, refresh blocklist section.
  // TODO(astra): Update menu bubble if open.
}

void AstraFocusModeController::OnSessionCompleted(AstraFocusModeModel* model,
                                                   const AstraFocusSession& session) {
  DCHECK_EQ(model, model_.get());
  // Session recorded — stats are already updated in the model.
}

void AstraFocusModeController::OnFocusModeModelShutdown(AstraFocusModeModel* model) {
  DCHECK_EQ(model, model_.get());
  // Model is shutting down — clean up views.
  HideMenuBubble();
  HideIndicatorInternal();
  observing_model_ = false;
}

// -- Private helpers -------------------------------------------------------

void AstraFocusModeController::ActivateFocusModeUI() {
  // Show the indicator if enabled.
  if (model_->GetShowIndicator()) {
    ShowIndicatorInternal();
  }

  // Hide sidebar if configured.
  if (model_->hide_sidebar_in_focus_mode()) {
    HideSidebarForFocusMode();
  }

  // Dim non-focus tabs if configured.
  if (model_->dim_non_focus_tabs()) {
    ApplyNonFocusTabDimming();
  }

  // Start break reminder timer if enabled.
  StartBreakReminderTimer();
}

void AstraFocusModeController::DeactivateFocusModeUI() {
  // Hide the menu bubble first.
  HideMenuBubble();

  // Hide the indicator.
  HideIndicatorInternal();

  // Restore sidebar.
  RestoreSidebarFromFocusMode();

  // Restore tab dimming.
  RestoreNonFocusTabDimming();

  // Stop break reminder timer.
  StopBreakReminderTimer();

  // Reset session state in the model (keeps settings).
  // Note: EndSession already records history, but we reset the runtime state.
  // Don't call ResetSessionState here because EndSession was already called
  // and that would clear the note before history was recorded.
}

void AstraFocusModeController::UpdateIndicatorRemainingTime() {
  if (indicator_) {
    indicator_->UpdateRemainingTime(model_->GetTimeRemaining());
  }
}

void AstraFocusModeController::UpdateIndicatorPausedState() {
  if (indicator_) {
    indicator_->UpdatePausedState(model_->IsSessionPaused());
  }
}

void AstraFocusModeController::UpdateIndicatorBlockLevel() {
  if (indicator_) {
    indicator_->SetBlockLevel(model_->GetBlockLevel());
  }
}

void AstraFocusModeController::ShowIndicatorInternal() {
  if (indicator_ || !browser_view_) {
    return;
  }

  indicator_ = AstraFocusModeIndicator::Show(browser_view_, model_.get());

  // Set ourselves as the delegate so the indicator menu can forward actions.
  if (indicator_) {
    indicator_->SetDelegate(this);
    indicator_->ApplySettings(model_.get());
    indicator_->UpdateRemainingTime(model_->GetTimeRemaining());
    indicator_->UpdatePausedState(model_->IsSessionPaused());
    indicator_->SetBlockLevel(model_->GetBlockLevel());
  }
}

void AstraFocusModeController::HideIndicatorInternal() {
  if (!indicator_) {
    return;
  }

  indicator_->Close();
  indicator_ = nullptr;
}

void AstraFocusModeController::ShowMenuBubble() {
  if (menu_bubble_ || !indicator_) {
    return;
  }

  // The menu bubble is created and shown by the indicator.
  // We track it via the delegate pattern.
  // TODO(astra): Direct menu bubble creation from controller.
}

void AstraFocusModeController::HideMenuBubble() {
  if (menu_bubble_) {
    menu_bubble_->Close();
    menu_bubble_ = nullptr;
  }
}

void AstraFocusModeController::HideSidebarForFocusMode() {
  if (!browser_view_) {
    return;
  }

  // TODO(astra): Use the actual sidebar view to get visibility.
  // For now, we assume it was visible since that's the default.
  sidebar_was_visible_ = true;
  sidebar_was_pinned_ = true;

  // The actual visibility change is handled by the owner (AstraBrowserView).
  // TODO(astra): Call HideSidebar() on AstraBrowserView.
}

void AstraFocusModeController::RestoreSidebarFromFocusMode() {
  if (!browser_view_) {
    return;
  }

  // Restore sidebar to its pre-focus-mode visibility state.
  // The actual visibility change is handled by the owner (AstraBrowserView).
  // TODO(astra): Call ShowSidebar() / HideSidebar() on AstraBrowserView.
}

void AstraFocusModeController::ApplyNonFocusTabDimming() {
  tabs_were_dimmed_ = true;
  // TODO(astra): Apply tab dimming using TabStripModel or TabStyle.
  //   Chromium owner: TabStrip / TabStyle.
  //   Patch point: Set a custom tab style or opacity for non-active tabs.
}

void AstraFocusModeController::RestoreNonFocusTabDimming() {
  tabs_were_dimmed_ = false;
  // TODO(astra): Restore tab opacity to normal.
}

void AstraFocusModeController::StartBreakReminderTimer() {
  if (!model_->GetShowBreakReminder() || !model_->IsActive() ||
      model_->IsSessionPaused()) {
    return;
  }

  base::TimeDelta interval = model_->GetBreakInterval();
  if (interval.is_zero() || interval.is_negative()) {
    return;
  }

  break_reminder_timer_.Start(
      FROM_HERE, interval,
      base::BindRepeating(&AstraFocusModeController::OnBreakReminder,
                          base::Unretained(this)));
}

void AstraFocusModeController::StopBreakReminderTimer() {
  break_reminder_timer_.Stop();
}

void AstraFocusModeController::SyncModelWithService() {
  auto* service = GetService();
  if (!service) {
    return;
  }

  model_->SetActive(service->IsFocusModeActive());
  model_->SetPaused(service->IsSessionPaused());
  model_->SetRemainingTime(service->GetRemainingTime());
  model_->SetDuration(service->GetTotalDuration());
  if (service->IsFocusModeActive() &&
      service->GetRemainingTime() < service->GetTotalDuration()) {
    model_->SetElapsedTime(
        service->GetTotalDuration() - service->GetRemainingTime());
  }
}

AstraFocusModeService* AstraFocusModeController::GetService() const {
  if (!browser_view_ || !browser_view_->browser() ||
      !browser_view_->browser()->profile()) {
    return nullptr;
  }
  return AstraFocusModeServiceFactory::GetForProfile(
      browser_view_->browser()->profile());
}

}  // namespace astra
