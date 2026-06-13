#ifndef ASTRA_UI_VIEWS_PIP_ASTRA_PIP_CONTROLS_VIEW_H_
#define ASTRA_UI_VIEWS_PIP_ASTRA_PIP_CONTROLS_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/slider.h"
#include "ui/views/view.h"

namespace gfx {
class Size;
}

namespace astra {

class AstraPipControlsModel;
enum class PipSizePreset;
enum class PipSnapPosition;
enum class AstraPipPlaybackSpeed;

// =========================================================================
// AstraPipControlsView — custom PiP window controls overlay
// =========================================================================
//
// Full-window controls overlay shown on top of Chromium's PiP window.
// Adds Astra-specific functionality that extends the standard video PiP
// controls with playback, window management, sizing, and presentation actions.
//
// The view observes AstraPipControlsModel for state changes and renders
// accordingly.  User interactions update the model and also notify the
// delegate so actual PiP actions can be performed.
//
// Layout (expanded mode):
//   ┌─────────────────────────────────────┐
//   │ [Astra PiP]        Tab Title    [X] │  ← top bar
//   │                                     │
//   │                                     │
//   │ 🔊⎽⎽⎽⎽⎽  1.0x  [◀◀] [▶/❚❚] [▶▶] 📌 │  ← bottom bar
//   │           S/M/L  opacity  settings  │
//   └─────────────────────────────────── ╱┘
//                                       ↳ resize handle
//
// Top bar:    Astra PiP badge + tab title + close button + minimize
// Bottom bar: Volume slider, playback rate, skip, play/pause,
//             size presets, opacity, always-on-top, settings
// Resize grip: bottom-right corner resize handle
//
// Model/view separation:
//   - Model owns control state (playing, muted, volume, etc.) and
//     presentation settings (auto-hide, bar visibility, etc.)
//   - View renders the controls and handles user input
//   - View observes the model and updates visuals on state changes
//
// Chromium subsystems reused:
//   - PictureInPictureWindowViews (chrome/browser/ui/views/picture_in_picture/)
//   - VideoPiPView (chrome/browser/ui/views/picture_in_picture/video_pip_view.h)
//   - views::ImageButton / views::Button (ui/views/controls/button/)
//   - views::Slider (ui/views/controls/slider/)
//   - views::Label (ui/views/controls/label.h)
//   - base::OneShotTimer (base/timer/one_shot_timer.h)
//
// Chromium patch points:
//   - picture_in_picture_window_views.cc — to add the Astra controls as
//     a child view overlay on top of the standard PiP content.
//   - video_pip_view.cc — to extend or replace the standard controls bar.
//
// TODO(astra): Overlay custom controls on Chromium's PiP window.
//   The overlay needs to be added as a child of the PiP widget's client
//   view, filling the entire bounds.  This requires a small patch to
//   PictureInPictureWindowViews or VideoPiPView to inject the Astra
//   controls view.
//   Chromium owner: PictureInPictureWindowViews
// =========================================================================

class AstraPipControlsView : public views::View,
                             public AstraPipControlsModelObserver,
                             public views::SliderListener {
 public:
  // Delegate interface for control actions.
  //
  // Implemented by the controller (e.g., the PiP window controller or a
  // per-window Astra PiP helper) that bridges between the view and the
  // underlying PiP service / Chromium PiP controller.
  //
  // All actions are fire-and-forget from the view's perspective — the
  // controller is responsible for updating the model's state via
  // Set*() methods after the underlying state changes.
  class Delegate {
   public:
    // Called when the user clicks the play/pause button.
    virtual void OnPlayPause() = 0;

    // Called when the user clicks the skip backward button.
    // |seconds| is the number of seconds to skip backward.
    virtual void OnSkipBackward(int seconds) = 0;

    // Called when the user clicks the skip forward button.
    // |seconds| is the number of seconds to skip forward.
    virtual void OnSkipForward(int seconds) = 0;

    // Called when the user clicks the mute/unmute button.
    virtual void OnMuteToggle() = 0;

    // Called when the volume changes via the slider.
    virtual void OnVolumeChanged(double volume) = 0;

    // Called when the user clicks the close (X) button.
    virtual void OnClosePip() = 0;

    // Called when the user clicks the "return to tab" button.
    virtual void OnReturnToTab() = 0;

    // Called when the user clicks a size preset button.
    virtual void OnResizePreset(PipSizePreset preset) = 0;

    // Called when the user toggles the always-on-top (pin) button.
    virtual void OnAlwaysOnTopToggle() = 0;

    // Called when the playback rate changes.
    virtual void OnPlaybackRateChanged(double rate) = 0;

    // Called when the window opacity changes.
    virtual void OnOpacityChanged(double opacity) = 0;

    // Called when the snap position changes.
    virtual void OnSnapPositionChanged(PipSnapPosition position) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  AstraPipControlsView(AstraPipControlsModel* model, Delegate* delegate);
  AstraPipControlsView(const AstraPipControlsView&) = delete;
  AstraPipControlsView& operator=(const AstraPipControlsView&) = delete;
  ~AstraPipControlsView() override;

  // -- Model management ----------------------------------------------------

  void SetModel(AstraPipControlsModel* model);
  AstraPipControlsModel* GetModel() { return model_; }
  const AstraPipControlsModel* GetModel() const { return model_; }

  // -- Video content view --------------------------------------------------

  void SetVideoView(views::View* video_view);
  views::View* GetVideoView() { return video_view_; }
  const views::View* GetVideoView() const { return video_view_; }

  // -- Controls visibility -------------------------------------------------

  void SetControlsVisible(bool visible);
  bool GetControlsVisible() const;

  // Show controls temporarily — they will auto-hide after the configured
  // delay if auto-hide is enabled.
  void ShowControlsTemporarily();

  // -- State updates (convenience — delegates to model) -------------------

  void SetTitle(const std::u16string& title);
  const std::u16string& GetTitle() const { return title_text_; }

  void SetIsPlaying(bool playing);
  bool IsPlaying() const;

  void SetProgress(double progress);
  double GetProgress() const;

  void SetVolume(double volume);
  double GetVolume() const;

  void SetMuted(bool muted);
  bool IsMuted() const;

  void SetPlaybackSpeed(AstraPipPlaybackSpeed speed);
  AstraPipPlaybackSpeed GetPlaybackSpeed() const;

  void SetAlwaysOnTop(bool on_top);
  bool GetAlwaysOnTop() const;

  void SetIsDraggable(bool draggable);
  bool IsDraggable() const { return is_draggable_; }

  void SetIsResizable(bool resizable);
  bool IsResizable() const { return is_resizable_; }

  // -- Backward-compatible state updates -----------------------------------

  void SetPlaying(bool playing);
  void SetMuted(bool muted);
  void SetActiveSizePreset(PipSizePreset preset);
  void SetAlwaysOnTop(bool pinned);
  void SetVolume(double volume);
  void SetPlaybackRate(double rate);
  void SetOpacity(double opacity);

  // -- Accessors for testing ----------------------------------------------

  AstraPipControlsModel* model() { return model_; }
  const AstraPipControlsModel* model() const { return model_; }

  bool IsControlsVisible() const;
  bool IsControlsMinimized() const;

  // Control element accessors for testing.
  views::ImageButton* close_button() { return close_button_; }
  views::ImageButton* minimize_button() { return minimize_button_; }
  views::ImageButton* maximize_button() { return maximize_button_; }
  views::ImageButton* play_pause_button() { return play_pause_button_; }
  views::ImageButton* skip_backward_button() { return skip_backward_button_; }
  views::ImageButton* skip_forward_button() { return skip_forward_button_; }
  views::ImageButton* mute_button() { return mute_button_; }
  views::Slider* volume_slider() { return volume_slider_; }
  views::Slider* progress_bar() { return progress_bar_; }
  views::ImageButton* speed_button() { return playback_rate_button_; }
  views::ImageButton* pip_expand_button() { return return_to_tab_button_; }
  views::ImageButton* always_on_top_button() { return always_on_top_button_; }
  views::View* resize_handle() { return resize_handle_; }
  views::Label* title_label() { return title_label_; }
  views::View* top_bar() { return top_bar_; }
  views::View* bottom_bar() { return bottom_bar_; }

  // -- AstraPipControlsModelObserver --------------------------------------

  void OnPlayStateChanged(bool playing) override;
  void OnMuteStateChanged(bool muted) override;
  void OnVolumeChanged(double volume) override;
  void OnPlaybackRateChanged(double rate) override;
  void OnSizePresetChanged(PipSizePreset preset) override;
  void OnAlwaysOnTopChanged(bool pinned) override;
  void OnOpacityChanged(double opacity) override;
  void OnControlsVisibilityChanged(bool visible) override;
  void OnControlsSettingsChanged() override;
  void OnSnapPositionChanged(PipSnapPosition position) override;
  void OnControlsMinimizedChanged(bool minimized) override;
  void OnProgressChanged(double progress) override;
  void OnPlaybackSpeedChanged(AstraPipPlaybackSpeed speed) override;
  void OnLoopingChanged(bool looping) override;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // -- views::SliderListener ----------------------------------------------

  void SliderValueChanged(views::Slider* sender,
                          float value,
                          float old_value,
                          views::SliderChangeReason reason) override;

 protected:
  // Override Layout for custom positioning of certain children.
  void Layout() override;

 private:
  // Build the child views and layout.
  void BuildLayout();

  // Creates the top bar (badge, title, window buttons).
  void BuildTopBar();

  // Creates the bottom bar (playback, size, pin controls).
  void BuildBottomBar();

  // Creates the resize handle in the bottom-right corner.
  void BuildResizeHandle();

  // Creates snap position indicators (corner highlights).
  void BuildSnapIndicators();

  // Creates the progress bar (above the bottom bar).
  void BuildProgressBar();

  // Creates the volume control row (slider + mute button).
  void BuildVolumeControls(views::View* container);

  // Creates the playback rate selector.
  void BuildPlaybackRateControl(views::View* container);

  // Creates the opacity control.
  void BuildOpacityControl(views::View* container);

  // Creates the settings menu button.
  void BuildSettingsButton(views::View* container);

  // Creates the minimize controls button.
  void BuildMinimizeButton(views::View* container);

  // Creates the maximize/restore button.
  void BuildMaximizeButton(views::View* container);

  // Button callbacks.
  void OnPlayPauseClicked(const ui::Event& event);
  void OnSkipBackwardClicked(const ui::Event& event);
  void OnSkipForwardClicked(const ui::Event& event);
  void OnMuteClicked(const ui::Event& event);
  void OnCloseClicked(const ui::Event& event);
  void OnReturnToTabClicked(const ui::Event& event);
  void OnSizeSmallClicked(const ui::Event& event);
  void OnSizeMediumClicked(const ui::Event& event);
  void OnSizeLargeClicked(const ui::Event& event);
  void OnAlwaysOnTopClicked(const ui::Event& event);
  void OnSettingsClicked(const ui::Event& event);
  void OnMinimizeClicked(const ui::Event& event);
  void OnMaximizeClicked(const ui::Event& event);
  void OnPlaybackRateClicked(const ui::Event& event);
  void OnSnapIndicatorClicked(const ui::Event& event);

  // Updates the visual state of all controls from the model.
  void UpdateAllFromModel();

  // Updates button visibility based on presentation settings.
  void UpdateControlVisibilityFromSettings();

  // Updates the background color of the control bars based on the
  // current color provider and controls opacity.
  void UpdateBarBackgrounds();

  // Updates accessibility names and descriptions for all controls.
  void UpdateAccessibilityInfo();

  // Updates the progress bar value.
  void UpdateProgressFromModel();

  // Auto-hide: starts the hide timer.
  void StartAutoHideTimer();

  // Auto-hide: cancels the hide timer.
  void CancelAutoHideTimer();

  // Auto-hide: callback when the timer fires.
  void OnAutoHideTimerFired();

  // Handles keyboard shortcuts.
  bool HandleKeyboardShortcut(const ui::KeyEvent& event);

  // Not owned.  The delegate outlives the view.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Not owned.  The model outlives the view.
  raw_ptr<AstraPipControlsModel> model_ = nullptr;

  // Not owned.  Video content view, if any.
  raw_ptr<views::View> video_view_ = nullptr;

  // -- Child views (owned by the view hierarchy) -------------------------

  // Top bar container.
  raw_ptr<views::View> top_bar_ = nullptr;
  raw_ptr<views::Label> badge_label_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::ImageButton> minimize_button_ = nullptr;
  raw_ptr<views::ImageButton> maximize_button_ = nullptr;
  raw_ptr<views::ImageButton> return_to_tab_button_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;

  // Progress bar (between video and bottom bar).
  raw_ptr<views::Slider> progress_bar_ = nullptr;

  // Bottom bar container.
  raw_ptr<views::View> bottom_bar_ = nullptr;
  raw_ptr<views::ImageButton> mute_button_ = nullptr;
  raw_ptr<views::Slider> volume_slider_ = nullptr;
  raw_ptr<views::ImageButton> skip_backward_button_ = nullptr;
  raw_ptr<views::ImageButton> play_pause_button_ = nullptr;
  raw_ptr<views::ImageButton> skip_forward_button_ = nullptr;
  raw_ptr<views::ImageButton> size_small_button_ = nullptr;
  raw_ptr<views::ImageButton> size_medium_button_ = nullptr;
  raw_ptr<views::ImageButton> size_large_button_ = nullptr;
  raw_ptr<views::ImageButton> always_on_top_button_ = nullptr;
  raw_ptr<views::ImageButton> playback_rate_button_ = nullptr;
  raw_ptr<views::Label> playback_rate_label_ = nullptr;
  raw_ptr<views::Slider> opacity_slider_ = nullptr;
  raw_ptr<views::ImageButton> settings_button_ = nullptr;

  // Resize handle in the bottom-right corner.
  raw_ptr<views::View> resize_handle_ = nullptr;

  // Snap position indicator views (one per corner).
  raw_ptr<views::View> snap_indicator_tl_ = nullptr;
  raw_ptr<views::View> snap_indicator_tr_ = nullptr;
  raw_ptr<views::View> snap_indicator_bl_ = nullptr;
  raw_ptr<views::View> snap_indicator_br_ = nullptr;

  // -- Auto-hide -----------------------------------------------------------

  // Timer for auto-hiding controls after inactivity.
  base::OneShotTimer auto_hide_timer_;

  // Whether the mouse is currently over the controls view.
  bool mouse_hovering_ = false;

  // -- Display state (cached for presentation) ---------------------------

  // Current title text.
  std::u16string title_text_;

  // Whether the window is draggable.
  bool is_draggable_ = true;

  // Whether the window is resizable (cached for view state).
  bool is_resizable_ = true;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PIP_ASTRA_PIP_CONTROLS_VIEW_H_
