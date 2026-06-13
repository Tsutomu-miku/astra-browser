#ifndef ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_MENU_BUBBLE_H_
#define ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_MENU_BUBBLE_H_

#include <cstddef>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

#include "astra/browser/astra_focus_mode_service.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_indicator.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_model.h"

namespace views {
class BoxLayout;
class Label;
}  // namespace views

namespace astra {

class AstraFocusModeModel;

// =========================================================================
// AstraFocusModeMenuBubble — focus mode menu popup
// =========================================================================
//
// Menu bubble shown when the focus mode indicator is clicked.  Provides
// quick access to focus mode actions and information:
//
//   ┌──────────────────────────────┐
//   │ Focus Mode         25:00  ⏱ │
//   │ [ Pause ] [ Break ] [ End ]  │
//   ├──────────────────────────────┤
//   │ Quick Start:                  │
//   │ [ 25m ] [ 45m ] [ 60m ] [ 90m ]│
//   ├──────────────────────────────┤
//   │ Today's Stats:                │
//   │  🕒 2h 15m total focus        │
//   │  📊 4 sessions completed      │
//   │  🔥 3 day streak               │
//   ├──────────────────────────────┤
//   │ 🚫 3 distractions blocked     │
//   ├──────────────────────────────┤
//   │ [ Settings ]                  │
//   └──────────────────────────────┘
//
// Features:
//   - Time remaining display (large)
//   - Duration presets (quick buttons: 25m, 45m, 60m, 90m)
//   - Block level selector
//   - Start/Pause/Resume/End buttons
//   - Settings link
//   - Session stats (today's focus time)
//   - Keyboard navigation support
//
// This view is a presentation-only surface.  All actions delegate upward
// through AstraFocusModeIndicatorDelegate.
//
// Architecture (MVC):
//   - Model: AstraFocusModeModel — owns UI state and settings.
//   - View: AstraFocusModeMenuBubble — pure presentation.
//   - Controller: AstraFocusModeController — relays actions to service.
//
// Chromium subsystems reused:
//   - Widget / Views framework.
//   - BubbleDialogDelegateView pattern.
//   - LabelButton (for action buttons).
// =========================================================================

class AstraFocusModeMenuBubble : public views::View,
                                 public views::WidgetObserver {
 public:
  // Creates and shows the menu bubble anchored to |anchor_view|.
  // |delegate| is used to forward user actions.
  // |model| provides settings and state (may be null).
  // |remaining_time| is the initial remaining time to display.
  // |distraction_count| is the initial number of blocked distractions.
  // |is_paused| whether the session is currently paused.
  // |phase| current focus phase (work/break).
  // Returns a raw pointer to the bubble view.  The widget is self-owned.
  static AstraFocusModeMenuBubble* Show(
      views::View* anchor_view,
      AstraFocusModeIndicatorDelegate* delegate,
      AstraFocusModeModel* model,
      base::TimeDelta remaining_time,
      size_t distraction_count,
      bool is_paused,
      AstraFocusPhase phase);

  // -- Visibility ----------------------------------------------------------

  // Shows the bubble anchored to |anchor_rect| (screen coordinates).
  void Show(const gfx::Rect& anchor_rect);

  // Hides the menu bubble widget.
  void Hide();

  // Returns true if the bubble is currently visible.
  bool IsVisible() const;

  // -- Duration ------------------------------------------------------------

  // Sets the displayed duration.
  void SetDuration(base::TimeDelta duration);

  // Returns the currently displayed duration.
  base::TimeDelta GetDuration() const;

  // -- Block level ---------------------------------------------------------

  // Sets the displayed block level.
  void SetBlockLevel(AstraFocusBlockLevel level);

  // Returns the currently displayed block level.
  AstraFocusBlockLevel GetBlockLevel() const;

  // -- Active state --------------------------------------------------------

  // Sets whether focus mode is active.
  void SetIsActive(bool active);

  // Returns whether focus mode is shown as active.
  bool IsActive() const;

  // -- Time remaining ------------------------------------------------------

  // Sets the displayed remaining time.
  void SetTimeRemaining(base::TimeDelta remaining);

  // Returns the currently displayed remaining time.
  base::TimeDelta GetTimeRemaining() const;

  // -- Presets -------------------------------------------------------------

  // Selects a preset duration by index.
  void SelectPreset(int preset_index);

  // Returns the number of preset buttons.
  int GetPresetCount() const;

  // -- Action buttons ------------------------------------------------------

  // Triggers the start action (starts a new focus session).
  void StartFocusAction();

  // Triggers the pause action.
  void PauseFocusAction();

  // Triggers the resume action.
  void ResumeFocusAction();

  // Triggers the end action.
  void EndFocusAction();

  // Triggers the extend action.
  void ExtendFocusAction(base::TimeDelta extension);

  // -- Legacy API for compatibility ----------------------------------------

  void UpdateRemainingTime(base::TimeDelta remaining);
  void UpdateDistractionCount(size_t count);
  void UpdatePausedState(bool is_paused);
  void UpdatePhase(AstraFocusPhase phase);
  void UpdateStats();
  void Close();

  // -- views::WidgetObserver ----------------------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;

 private:
  AstraFocusModeMenuBubble(AstraFocusModeIndicatorDelegate* delegate,
                           AstraFocusModeModel* model,
                           base::TimeDelta remaining_time,
                           size_t distraction_count,
                           bool is_paused,
                           AstraFocusPhase phase);
  ~AstraFocusModeMenuBubble() override;

  // Creates the widget anchored to the given view.
  void CreateWidget(views::View* anchor_view);

  // Builds the menu content views.
  void BuildLayout();

  // Builds the header section (title + time).
  void BuildHeader();

  // Builds the action buttons row (pause/break/end).
  void BuildActionButtons();

  // Builds the session presets section.
  void BuildPresets();

  // Builds the block level selector section.
  void BuildBlockLevelSelector();

  // Builds the session stats section.
  void BuildStats();

  // Builds the distraction info section.
  void BuildDistractionInfo();

  // Builds the settings link.
  void BuildSettingsLink();

  // Formats a TimeDelta into a compact string.
  std::u16string FormatTime(base::TimeDelta delta) const;

  // Formats a duration as a human-readable string (e.g. "2h 15m").
  std::u16string FormatDurationLong(base::TimeDelta duration) const;

  // Formats a block level as a display string.
  std::u16string FormatBlockLevel(AstraFocusBlockLevel level) const;

  // Button callbacks.
  void OnPauseResumeClicked(const ui::Event& event);
  void OnBreakClicked(const ui::Event& event);
  void OnEndClicked(const ui::Event& event);
  void OnStartClicked(const ui::Event& event);
  void OnPresetClicked(int index);
  void OnBlockLevelClicked(AstraFocusBlockLevel level);
  void OnSettingsClicked(const ui::Event& event);

  // Updates the pause/resume button text.
  void UpdatePauseResumeButton();

  // Updates the start button visibility.
  void UpdateStartButton();

  // Updates the stats section text.
  void UpdateStatsLabels();

  // Updates the block level selector highlight.
  void UpdateBlockLevelHighlight();

  raw_ptr<AstraFocusModeIndicatorDelegate> delegate_;
  raw_ptr<AstraFocusModeModel> model_ = nullptr;
  raw_ptr<views::Widget> widget_ = nullptr;

  // Header row labels.
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> time_label_ = nullptr;

  // Action buttons.
  raw_ptr<views::LabelButton> start_button_ = nullptr;
  raw_ptr<views::LabelButton> pause_resume_button_ = nullptr;
  raw_ptr<views::LabelButton> break_button_ = nullptr;
  raw_ptr<views::LabelButton> end_button_ = nullptr;

  // Preset buttons.
  std::vector<raw_ptr<views::LabelButton>> preset_buttons_;

  // Block level buttons.
  std::vector<raw_ptr<views::LabelButton>> block_level_buttons_;

  // Stats labels.
  raw_ptr<views::Label> stats_title_label_ = nullptr;
  raw_ptr<views::Label> stats_total_label_ = nullptr;
  raw_ptr<views::Label> stats_sessions_label_ = nullptr;
  raw_ptr<views::Label> stats_streak_label_ = nullptr;

  // Distraction info.
  raw_ptr<views::Label> distraction_label_ = nullptr;

  // Settings link button.
  raw_ptr<views::LabelButton> settings_button_ = nullptr;

  // Current state.
  bool is_active_ = false;
  base::TimeDelta remaining_time_;
  base::TimeDelta total_duration_;
  size_t distraction_count_ = 0;
  bool is_paused_ = false;
  AstraFocusPhase current_phase_ = AstraFocusPhase::kWork;
  AstraFocusBlockLevel block_level_ = AstraFocusBlockLevel::kNone;

  // Whether we're observing the widget for destruction.
  bool observing_widget_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_MENU_BUBBLE_H_
