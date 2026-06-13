#ifndef ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_INDICATOR_H_
#define ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_INDICATOR_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

#include "astra/browser/astra_focus_mode_service.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_model.h"

class BrowserView;

namespace astra {

class AstraFocusModeModel;
class AstraFocusModeMenuBubble;

// =========================================================================
// AstraFocusModeIndicator — subtle focus mode status indicator
// =========================================================================
//
// A small, subtle indicator widget shown when focus mode is active.
// It displays the focus mode status and remaining time and provides
// quick access to focus mode controls (extend, end, settings).
//
// Features:
//   - Timer display (MM:SS or HH:MM:SS format)
//   - Different visual styles (minimal / full / badge)
//   - Pulse animation when active (or when time is low)
//   - Click to show menu bubble
//   - Drag to reposition on screen
//   - Different colors based on state (active / paused / break)
//   - Accessibility support
//
// Design principles:
//   - Minimal UI — doesn't distract from content.
//   - Subtle styling — low contrast, small size, semi-transparent.
//   - Click to show menu of focus mode actions.
//   - Configurable position on screen.
//
// Architecture (MVC):
//   - Model: AstraFocusModeModel — owns UI state and settings.
//   - View: AstraFocusModeIndicator — pure presentation, no truth state.
//   - Controller: AstraFocusModeController — manages show/hide, updates.
//
// The indicator reads presentation state from the model and forwards
// user actions through the delegate interface.
//
// Chromium subsystems reused:
//   - Widget / Views framework.
//   - BubbleDialogDelegateView pattern (for the menu).
//   - LabelButton (for the clickable indicator).
//
// Chromium similar features:
//   - Picture-in-Picture window controls (chrome/browser/ui/views/pip/).
//   - Media notification / mini player.
//   - Status tray icons.
// =========================================================================

// Delegate interface for focus mode indicator actions.
// Implemented by AstraFocusModeController to forward actions to the service.
class AstraFocusModeIndicatorDelegate {
 public:
  virtual ~AstraFocusModeIndicatorDelegate() = default;

  // Called when the user requests extending the focus session.
  virtual void OnExtendFocusMode(base::TimeDelta duration) = 0;

  // Called when the user requests ending focus mode.
  virtual void OnEndFocusMode() = 0;

  // Called when the user requests opening focus mode settings.
  virtual void OnOpenSettings() = 0;

  // Called when the user pauses the focus session.
  virtual void OnPauseFocusMode() = 0;

  // Called when the user resumes the focus session.
  virtual void OnResumeFocusMode() = 0;

  // Called when the user starts a break.
  virtual void OnStartBreak() = 0;

  // Returns the number of distractions blocked in the current session.
  virtual size_t GetDistractionsBlockedCount() = 0;
};

class AstraFocusModeIndicator : public views::View,
                                public views::WidgetObserver {
 public:
  // Creates and shows the focus mode indicator widget anchored to the
  // given BrowserView. Returns a raw pointer to the indicator view.
  // The widget is self-owned and manages its own lifetime.
  // |model| provides presentation settings and UI state.
  static AstraFocusModeIndicator* Show(
      BrowserView* browser_view,
      AstraFocusModeModel* model = nullptr);

  // Sets the delegate that handles focus mode actions.
  void SetDelegate(AstraFocusModeIndicatorDelegate* delegate);

  // -- Active state --------------------------------------------------------

  // Sets whether focus mode is active.
  void SetActive(bool active);

  // Returns true if the indicator shows active state.
  bool IsActive() const;

  // -- Time display --------------------------------------------------------

  // Sets the remaining time display.
  void SetTimeRemaining(base::TimeDelta remaining);

  // Returns the current displayed remaining time.
  base::TimeDelta GetTimeRemaining() const;

  // -- Timer visibility ----------------------------------------------------

  // Sets whether the timer text is visible.
  void SetTimerVisible(bool visible);

  // Returns whether the timer text is visible.
  bool IsTimerVisible() const;

  // -- Block level ---------------------------------------------------------

  // Sets the block level for visual indication.
  void SetBlockLevel(AstraFocusBlockLevel level);

  // Returns the current block level display.
  AstraFocusBlockLevel GetBlockLevel() const;

  // -- Paused state --------------------------------------------------------

  // Sets the paused state.
  void SetPaused(bool paused);

  // Returns whether the indicator shows paused state.
  bool IsPaused() const;

  // -- Menu on click -------------------------------------------------------

  // Sets whether clicking the indicator shows the menu.
  void SetShowMenuOnClick(bool show);

  // Returns whether clicking shows the menu.
  bool GetShowMenuOnClick() const;

  // -- Accent color --------------------------------------------------------

  // Sets the accent color for the indicator border/background.
  void SetAccentColor(SkColor color);

  // Returns the current accent color.
  SkColor GetAccentColor() const;

  // -- Pulse animation -----------------------------------------------------

  // Starts the pulse animation (e.g. when time is low).
  void StartPulseAnimation();

  // Stops the pulse animation.
  void StopPulseAnimation();

  // Returns true if the pulse animation is running.
  bool IsPulsing() const;

  // -- Legacy API for compatibility ----------------------------------------

  void UpdateRemainingTime(base::TimeDelta remaining);
  void UpdatePausedState(bool is_paused);
  void UpdatePhase(AstraFocusPhase phase);
  void ApplySettings(const AstraFocusModeModel* model);
  void UpdateStats();
  void Close();

  // -- views::View ---------------------------------------------------------

  views::Widget* GetWidget() override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // -- views::WidgetObserver ----------------------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetBoundsChanged(views::Widget* widget,
                              const gfx::Rect& new_bounds) override;

 private:
  AstraFocusModeIndicator(BrowserView* browser_view,
                          AstraFocusModeModel* model);
  ~AstraFocusModeIndicator() override;

  // Creates the widget and adds this view to it.
  void CreateWidget();

  // Formats a TimeDelta into a human-readable time string.
  std::u16string FormatTime(base::TimeDelta delta) const;

  // Called when the indicator is clicked. Shows the focus mode menu.
  void OnIndicatorClicked();

  // Shows or hides the menu bubble.
  void ToggleMenuBubble();
  void ShowMenuBubble();
  void CloseMenuBubble();

  // Pulse animation tick.
  void OnPulseTick();

  // Updates the indicator text based on current state and settings.
  void UpdateIndicatorText();

  // Updates the indicator color based on current state.
  void UpdateIndicatorColor();

  // Updates the indicator size based on style setting.
  void UpdateIndicatorSize();

  // Positions the indicator based on the model's position setting.
  void PositionIndicator();

  // Returns the background color for the current state.
  SkColor GetCurrentBackgroundColor() const;

  raw_ptr<BrowserView> browser_view_;
  raw_ptr<AstraFocusModeModel> model_ = nullptr;
  raw_ptr<views::Widget> widget_ = nullptr;

  // Delegate that handles focus mode actions (not owned).
  raw_ptr<AstraFocusModeIndicatorDelegate> delegate_ = nullptr;

  // Main label button showing the indicator text.
  raw_ptr<views::LabelButton> indicator_button_ = nullptr;

  // Current state.
  bool is_active_ = false;
  base::TimeDelta remaining_time_;
  bool timer_visible_ = true;
  AstraFocusBlockLevel block_level_ = AstraFocusBlockLevel::kNone;
  bool is_paused_ = false;
  bool show_menu_on_click_ = true;
  SkColor accent_color_ = SK_ColorTRANSPARENT;
  AstraFocusPhase current_phase_ = AstraFocusPhase::kWork;

  // Drag state.
  bool is_dragging_ = false;
  gfx::Point drag_start_point_;
  gfx::Point widget_start_position_;

  // Pulse animation timer.
  base::RepeatingTimer pulse_timer_;
  bool pulse_visible_ = true;
  int pulse_cycle_ = 0;
  bool is_pulsing_ = false;

  // Whether we're observing the widget for destruction.
  bool observing_widget_ = false;

  // The open menu bubble, if any. Not owned.
  raw_ptr<AstraFocusModeMenuBubble> menu_bubble_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_FOCUS_MODE_ASTRA_FOCUS_MODE_INDICATOR_H_
