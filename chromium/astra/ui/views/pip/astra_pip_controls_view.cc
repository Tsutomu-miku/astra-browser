#include "astra/ui/views/pip/astra_pip_controls_view.h"

#include "base/i18n/rtl.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/browser/astra_pip_service.h"
#include "astra/ui/views/pip/astra_pip_controls_model.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/accessibility/accessibility_paint_checks.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/slider.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Control bar dimensions.
constexpr int kTopBarHeight = 32;
constexpr int kBottomBarHeight = 56;  // Increased to accommodate sliders.
constexpr int kMinimizedBottomBarHeight = 28;  // Height when minimized.
constexpr int kControlButtonSpacing = 6;
constexpr int kBarHorizontalPadding = 12;
constexpr int kBarVerticalPadding = 4;

// Button dimensions.
constexpr int kButtonSize = 28;
constexpr int kPlayPauseButtonSize = 36;
constexpr int kSmallButtonSize = 24;

// Resize handle size.
constexpr int kResizeHandleSize = 16;

// Snap indicator size.
constexpr int kSnapIndicatorSize = 8;
constexpr int kSnapIndicatorMargin = 4;

// Slider dimensions.
constexpr int kSliderWidth = 60;
constexpr int kSliderHeight = 20;

// Semi-transparent background alpha for control bars.
constexpr SkAlpha kBarBackgroundAlpha = 0xCC;  // 80% opaque.

// TODO(astra): Define Astra-specific color IDs for the PiP controls overlay.
// These should use the color provider system for proper dark/light mode
// and theme support.
//
// Chromium color system: ui/color/color_id.h, ui/color/color_provider.h
// For now, use semi-transparent dark background similar to Chrome's video
// PiP controls.
constexpr SkColor kControlsBackgroundBase = SkColorSetARGB(0xCC, 0x20, 0x20, 0x20);
constexpr SkColor kControlsTextColor = SK_ColorWHITE;
constexpr SkColor kControlsIconColor = SK_ColorWHITE;
constexpr SkColor kSnapIndicatorColor = SkColorSetARGB(0x80, 255, 255, 255);
constexpr SkColor kSnapIndicatorActiveColor =
    SkColorSetARGB(0xFF, 0x4CAF50, 255, 0x90);  // Green tint.

// Creates a semi-transparent rounded background for a control bar.
std::unique_ptr<views::Background> CreateBarBackground(double opacity) {
  SkAlpha alpha =
      static_cast<SkAlpha>(kBarBackgroundAlpha * std::clamp(opacity, 0.0, 1.0));
  return views::CreateSolidBackground(SkColorSetA(kControlsBackgroundBase, alpha));
}

// Configures a standard image button for the PiP controls.
void ConfigureImageButton(views::ImageButton* button,
                     const std::u16string& tooltip,
                     int button_size) {
  button->SetPreferredSize(gfx::Size(button_size, button_size));
  button->SetTooltipText(tooltip);
  button->SetFocusForPlatform();
  button->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  button->SetInstallFocusRingOnFocus(true);

  // TODO(astra): Set actual vector icons for each button.
  // Chromium icon system: ui/gfx/vector_icon_types.h
  // For now, buttons are placeholders without icons but have proper
  // accessibility and focus behavior.

  // Accessibility: ensure the button has a proper name.
  button->SetAccessibleName(tooltip);
}

// Configures a label for use in control bars.
void ConfigureControlLabel(views::Label* label, const std::u16string& text) {
  label->SetText(text);
  label->SetEnabledColor(kControlsTextColor);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      label->font_list().DeriveWithSizeDelta(-2)
          .DeriveWithWeight(gfx::Font::Weight::NORMAL));
  label->SetFocusBehavior(views::View::FocusBehavior::NEVER);
}

}  // namespace

// ---------------------------------------------------------------------------
// AstraPipControlsView
// ---------------------------------------------------------------------------

AstraPipControlsView::AstraPipControlsView(AstraPipControlsModel* model,
                                           Delegate* delegate)
    : delegate_(delegate),
      model_(model) {
  DCHECK(delegate_);
  DCHECK(model_);

  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  // Observe the model for state changes.
  model_->AddObserver(this);

  // The overlay fills the entire PiP window.
  BuildLayout();

  // Initialize visual state from model.
  UpdateAllFromModel();
  UpdateControlVisibilityFromSettings();
  UpdateBarBackgrounds();
}

AstraPipControlsView::~AstraPipControlsView() {
  if (model_) {
    model_->RemoveObserver(this);
  }
}

// -- State updates (convenience — delegate to model) ------------------------

void AstraPipControlsView::SetPlaying(bool playing) {
  if (model_) {
    model_->SetPlaying(playing);
  }
}

void AstraPipControlsView::SetMuted(bool muted) {
  if (model_) {
    model_->SetMuted(muted);
  }
}

void AstraPipControlsView::SetActiveSizePreset(PipSizePreset preset) {
  if (model_) {
    model_->SetActiveSizePreset(preset);
  }
}

void AstraPipControlsView::SetAlwaysOnTop(bool pinned) {
  if (model_) {
    model_->SetAlwaysOnTop(pinned);
  }
}

void AstraPipControlsView::SetTitle(const std::u16string& title) {
  title_text_ = title;
  if (title_label_) {
    title_label_->SetText(title);
    title_label_->SetTooltipText(title);
  }
}

void AstraPipControlsView::SetVolume(double volume) {
  if (model_) {
    model_->SetVolume(volume);
  }
}

void AstraPipControlsView::SetPlaybackRate(double rate) {
  if (model_) {
    model_->SetPlaybackRate(rate);
  }
}

void AstraPipControlsView::SetOpacity(double opacity) {
  if (model_) {
    model_->SetOpacity(opacity);
  }
}

// -- Accessors for testing --------------------------------------------------

bool AstraPipControlsView::IsControlsVisible() const {
  return model_ && model_->controls_visible();
}

bool AstraPipControlsView::IsControlsMinimized() const {
  return model_ && model_->controls_minimized();
}

// -- AstraPipControlsModelObserver ------------------------------------------

void AstraPipControlsView::OnPlayStateChanged(bool playing) {
  if (play_pause_button_) {
    play_pause_button_->SetToggled(playing);
    play_pause_button_->SetTooltipText(playing ? u"Pause" : u"Play");
    play_pause_button_->SetAccessibleName(playing ? u"Pause" : u"Play");
  }
}

void AstraPipControlsView::OnMuteStateChanged(bool muted) {
  if (mute_button_) {
    mute_button_->SetToggled(muted);
    mute_button_->SetTooltipText(muted ? u"Unmute" : u"Mute");
    mute_button_->SetAccessibleName(muted ? u"Unmute" : u"Mute");
  }
  if (volume_slider_ && model_) {
    volume_slider_->SetValue(model_->is_muted() ? 0.0f
                                                : static_cast<float>(model_->volume()));
  }
}

void AstraPipControlsView::OnVolumeChanged(double volume) {
  if (volume_slider_) {
    volume_slider_->SetValue(static_cast<float>(volume));
  }
  // Update accessibility.
  UpdateAccessibilityInfo();
}

void AstraPipControlsView::OnPlaybackRateChanged(double rate) {
  if (playback_rate_label_) {
    playback_rate_label_->SetText(
        base::NumberToString16(rate) + u"x");
  }
  if (playback_rate_button_) {
    playback_rate_button_->SetTooltipText(
        base::NumberToString16(rate) + u"x playback speed");
  }
}

void AstraPipControlsView::OnSizePresetChanged(PipSizePreset preset) {
  if (size_small_button_) {
    size_small_button_->SetToggled(preset == PipSizePreset::kSmall);
  }
  if (size_medium_button_) {
    size_medium_button_->SetToggled(preset == PipSizePreset::kMedium);
  }
  if (size_large_button_) {
    size_large_button_->SetToggled(preset == PipSizePreset::kLarge);
  }
}

void AstraPipControlsView::OnAlwaysOnTopChanged(bool pinned) {
  if (always_on_top_button_) {
    always_on_top_button_->SetToggled(pinned);
    always_on_top_button_->SetTooltipText(
        pinned ? u"Unpin from top" : u"Pin on top");
    always_on_top_button_->SetAccessibleName(
        pinned ? u"Unpin from top" : u"Pin on top");
  }
}

void AstraPipControlsView::OnOpacityChanged(double opacity) {
  if (opacity_slider_) {
    opacity_slider_->SetValue(static_cast<float>(opacity));
  }
  // Apply opacity to the widget if possible.
  // TODO(astra): Apply opacity to the PiP widget.
}

void AstraPipControlsView::OnControlsVisibilityChanged(bool visible) {
  SetVisible(visible);
  // If becoming visible, restart the auto-hide timer if applicable.
  if (visible && model_ && model_->GetAutoHideControls() && !mouse_hovering_) {
    StartAutoHideTimer();
  }
}

void AstraPipControlsView::OnControlsSettingsChanged() {
  UpdateControlVisibilityFromSettings();
  UpdateBarBackgrounds();
  UpdateAccessibilityInfo();
}

void AstraPipControlsView::OnSnapPositionChanged(PipSnapPosition position) {
  // Update snap indicator highlights.
  if (snap_indicator_tl_) {
    snap_indicator_tl_->SetBackground(views::CreateSolidBackground(
        position == PipSnapPosition::kTopLeft ? kSnapIndicatorActiveColor
                                               : kSnapIndicatorColor));
  }
  if (snap_indicator_tr_) {
    snap_indicator_tr_->SetBackground(views::CreateSolidBackground(
        position == PipSnapPosition::kTopRight ? kSnapIndicatorActiveColor
                                                : kSnapIndicatorColor));
  }
  if (snap_indicator_bl_) {
    snap_indicator_bl_->SetBackground(views::CreateSolidBackground(
        position == PipSnapPosition::kBottomLeft ? kSnapIndicatorActiveColor
                                                  : kSnapIndicatorColor));
  }
  if (snap_indicator_br_) {
    snap_indicator_br_->SetBackground(views::CreateSolidBackground(
        position == PipSnapPosition::kBottomRight ? kSnapIndicatorActiveColor
                                                   : kSnapIndicatorColor));
  }
}

void AstraPipControlsView::OnControlsMinimizedChanged(bool minimized) {
  // Update bottom bar height and visibility of secondary controls.
  if (bottom_bar_) {
    // Toggle visibility of secondary controls.
    if (volume_slider_) volume_slider_->SetVisible(!minimized);
    if (skip_backward_button_) skip_backward_button_->SetVisible(
        !minimized && model_ && model_->GetShowSkipButtons());
    if (skip_forward_button_) skip_forward_button_->SetVisible(
        !minimized && model_ && model_->GetShowSkipButtons());
    if (size_small_button_) size_small_button_->SetVisible(!minimized);
    if (size_medium_button_) size_medium_button_->SetVisible(!minimized);
    if (size_large_button_) size_large_button_->SetVisible(!minimized);
    if (opacity_slider_) opacity_slider_->SetVisible(!minimized);
    if (settings_button_) settings_button_->SetVisible(!minimized);
    if (playback_rate_button_) playback_rate_button_->SetVisible(!minimized);
    if (playback_rate_label_) playback_rate_label_->SetVisible(!minimized);
  }

  // Update minimize button tooltip.
  if (minimize_button_) {
    minimize_button_->SetTooltipText(
        minimized ? u"Expand controls" : u"Minimize controls");
    minimize_button_->SetAccessibleName(
        minimized ? u"Expand controls" : u"Minimize controls");
  }

  // Trigger relayout.
  InvalidateLayout();
}

// -- views::View -------------------------------------------------------------

gfx::Size AstraPipControlsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = 320;
  int bottom_height = model_ && model_->controls_minimized()
                          ? kMinimizedBottomBarHeight
                          : kBottomBarHeight;
  int height = kTopBarHeight + bottom_height + 100;  // Content area.
  if (available_size.width().is_bounded()) {
    width = available_size.width().value();
  }
  if (available_size.height().is_bounded()) {
    height = available_size.height().value();
  }
  return gfx::Size(width, height);
}

void AstraPipControlsView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateBarBackgrounds();
}

bool AstraPipControlsView::OnMousePressed(const ui::MouseEvent& event) {
  // Start auto-hide timer on mouse press (interaction resets the timer).
  if (model_ && model_->GetAutoHideControls()) {
    StartAutoHideTimer();
  }
  return views::View::OnMousePressed(event);
}

void AstraPipControlsView::OnMouseEntered(const ui::MouseEvent& event) {
  mouse_hovering_ = true;
  CancelAutoHideTimer();

  // If controls were hidden, show them.
  if (model_ && !model_->controls_visible()) {
    model_->SetControlsVisible(true);
  }
}

void AstraPipControlsView::OnMouseExited(const ui::MouseEvent& event) {
  mouse_hovering_ = false;
  if (model_ && model_->GetAutoHideControls()) {
    StartAutoHideTimer();
  }
}

bool AstraPipControlsView::OnKeyPressed(const ui::KeyEvent& event) {
  if (HandleKeyboardShortcut(event)) {
    return true;
  }
  return views::View::OnKeyPressed(event);
}

void AstraPipControlsView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kToolbar;
  node_data->SetName(u"PiP controls");
  node_data->AddState(ax::mojom::State::kFocusable);
}

// -- views::SliderListener --------------------------------------------------

void AstraPipControlsView::SliderValueChanged(views::Slider* sender,
                                              float value,
                                              float old_value,
                                              views::SliderChangeReason reason) {
  if (sender == volume_slider_ && model_) {
    model_->SetVolume(static_cast<double>(value));
    if (delegate_) {
      delegate_->OnVolumeChanged(model_->volume());
    }
  } else if (sender == opacity_slider_ && model_) {
    model_->SetOpacity(static_cast<double>(value));
    if (delegate_) {
      delegate_->OnOpacityChanged(model_->opacity());
    }
  }
}

// -- Layout -----------------------------------------------------------------

void AstraPipControlsView::Layout() {
  views::View::Layout();

  // Position snap indicators at corners.
  gfx::Rect bounds = GetLocalBounds();

  if (snap_indicator_tl_ && snap_indicator_tl_->GetVisible()) {
    snap_indicator_tl_->SetBounds(
        kSnapIndicatorMargin,
        kTopBarHeight + kSnapIndicatorMargin,
        kSnapIndicatorSize,
        kSnapIndicatorSize);
  }
  if (snap_indicator_tr_ && snap_indicator_tr_->GetVisible()) {
    snap_indicator_tr_->SetBounds(
        bounds.width() - kSnapIndicatorSize - kSnapIndicatorMargin,
        kTopBarHeight + kSnapIndicatorMargin,
        kSnapIndicatorSize,
        kSnapIndicatorSize);
  }
  if (snap_indicator_bl_ && snap_indicator_bl_->GetVisible()) {
    int bottom_bar_y = bounds.height() -
        (model_ && model_->controls_minimized() ? kMinimizedBottomBarHeight
                                                : kBottomBarHeight);
    snap_indicator_bl_->SetBounds(
        kSnapIndicatorMargin,
        bottom_bar_y - kSnapIndicatorSize - kSnapIndicatorMargin,
        kSnapIndicatorSize,
        kSnapIndicatorSize);
  }
  if (snap_indicator_br_ && snap_indicator_br_->GetVisible()) {
    int bottom_bar_y = bounds.height() -
        (model_ && model_->controls_minimized() ? kMinimizedBottomBarHeight
                                                : kBottomBarHeight);
    snap_indicator_br_->SetBounds(
        bounds.width() - kSnapIndicatorSize - kSnapIndicatorMargin,
        bottom_bar_y - kSnapIndicatorSize - kSnapIndicatorMargin,
        kSnapIndicatorSize,
        kSnapIndicatorSize);
  }

  // Position resize handle in bottom-right corner.
  if (resize_handle_ && resize_handle_->GetVisible()) {
    resize_handle_->SetBounds(
        bounds.width() - kResizeHandleSize,
        bounds.height() - kResizeHandleSize,
        kResizeHandleSize,
        kResizeHandleSize);
  }
}

// -- Private helpers --------------------------------------------------------

void AstraPipControlsView::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Top bar.
  BuildTopBar();
  AddChildView(top_bar_.ExtractAsOwned());

  // Flexible spacer — fills the middle area.
  auto* spacer = AddChildView(std::make_unique<views::View>());
  layout->SetFlexForView(spacer, 1);

  // Build snap indicators (added as direct children for absolute positioning).
  BuildSnapIndicators();
  AddChildView(snap_indicator_tl_.ExtractAsOwned());
  AddChildView(snap_indicator_tr_.ExtractAsOwned());
  AddChildView(snap_indicator_bl_.ExtractAsOwned());
  AddChildView(snap_indicator_br_.ExtractAsOwned());

  // Bottom bar.
  BuildBottomBar();
  AddChildView(bottom_bar_.ExtractAsOwned());

  // Resize handle — positioned in the bottom-right corner via Layout().
  BuildResizeHandle();
  AddChildView(resize_handle_.ExtractAsOwned());

  // Make the view focusable for keyboard navigation.
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
}

void AstraPipControlsView::BuildTopBar() {
  top_bar_ = std::make_unique<views::View>();
  top_bar_->SetPaintToLayer();
  top_bar_->layer()->SetFillsBoundsOpaquely(false);
  top_bar_->SetBackground(CreateBarBackground(1.0));

  auto* layout = top_bar_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kBarVerticalPadding, kBarHorizontalPadding),
          kControlButtonSpacing));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Badge label.
  badge_label_ = top_bar_->AddChildView(std::make_unique<views::Label>(
      u"Astra PiP"));
  badge_label_->SetFontList(
      badge_label_->font_list().DeriveWithSizeDelta(-2)
          .DeriveWithWeight(gfx::Font::Weight::MEDIUM));
  badge_label_->SetEnabledColor(kControlsTextColor);
  badge_label_->SetAutoColorReadabilityEnabled(false);
  badge_label_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  badge_label_->SetAccessibleName(u"Astra PiP");

  // Title label.
  title_label_ = top_bar_->AddChildView(std::make_unique<views::Label>(u""));
  title_label_->SetFontList(
      title_label_->font_list().DeriveWithSizeDelta(-1));
  title_label_->SetEnabledColor(kControlsTextColor);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  title_label_->SetAccessibleName(u"PiP window title");
  layout->SetFlexForView(title_label_, 1);

  // Minimize button.
  BuildMinimizeButton(top_bar_);

  // Return-to-tab button.
  return_to_tab_button_ =
      top_bar_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnReturnToTabClicked,
              base::Unretained(this))));
  ConfigureImageButton(return_to_tab_button_, u"Return to tab", kSmallButtonSize);

  // Close button.
  close_button_ =
      top_bar_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnCloseClicked,
              base::Unretained(this))));
  ConfigureImageButton(close_button_, u"Close PiP", kSmallButtonSize);
}

void AstraPipControlsView::BuildBottomBar() {
  bottom_bar_ = std::make_unique<views::View>();
  bottom_bar_->SetPaintToLayer();
  bottom_bar_->layer()->SetFillsBoundsOpaquely(false);
  bottom_bar_->SetBackground(CreateBarBackground(1.0));
  bottom_bar_->SetPreferredSize(
      gfx::Size(0, kBottomBarHeight));

  auto* layout = bottom_bar_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kBarVerticalPadding, kBarHorizontalPadding),
          kControlButtonSpacing));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Left side: volume controls.
  BuildVolumeControls(bottom_bar_);

  // Flexible spacer — pushes center controls to the center.
  auto* left_spacer = bottom_bar_->AddChildView(std::make_unique<views::View>());
  layout->SetFlexForView(left_spacer, 1);

  // Center: skip backward, play/pause, skip forward.
  skip_backward_button_ =
      bottom_bar_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnSkipBackwardClicked,
              base::Unretained(this))));
  ConfigureImageButton(skip_backward_button_, u"Skip backward 10 seconds",
                       kButtonSize);

  play_pause_button_ =
      bottom_bar_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnPlayPauseClicked,
              base::Unretained(this))));
  ConfigureImageButton(play_pause_button_, u"Play", kPlayPauseButtonSize);

  skip_forward_button_ =
      bottom_bar_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnSkipForwardClicked,
              base::Unretained(this))));
  ConfigureImageButton(skip_forward_button_, u"Skip forward 10 seconds",
                       kButtonSize);

  // Flexible spacer — pushes right controls to the right.
  auto* right_spacer = bottom_bar_->AddChildView(std::make_unique<views::View>());
  layout->SetFlexForView(right_spacer, 1);

  // Right side: playback rate, size presets, opacity, always-on-top, settings.
  BuildPlaybackRateControl(bottom_bar_);

  // Size preset buttons.
  size_small_button_ =
      bottom_bar_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnSizeSmallClicked,
              base::Unretained(this))));
  ConfigureImageButton(size_small_button_, u"Small size", kButtonSize);

  size_medium_button_ =
      bottom_bar_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnSizeMediumClicked,
              base::Unretained(this))));
  ConfigureImageButton(size_medium_button_, u"Medium size", kButtonSize);

  size_large_button_ =
      bottom_bar_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnSizeLargeClicked,
              base::Unretained(this))));
  ConfigureImageButton(size_large_button_, u"Large size", kButtonSize);

  // Opacity control.
  BuildOpacityControl(bottom_bar_);

  // Always-on-top toggle.
  always_on_top_button_ =
      bottom_bar_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnAlwaysOnTopClicked,
              base::Unretained(this))));
  ConfigureImageButton(always_on_top_button_, u"Pin on top", kButtonSize);

  // Settings button.
  BuildSettingsButton(bottom_bar_);
}

void AstraPipControlsView::BuildResizeHandle() {
  resize_handle_ = std::make_unique<views::View>();
  resize_handle_->SetPaintToLayer();
  resize_handle_->layer()->SetFillsBoundsOpaquely(false);
  resize_handle_->SetPreferredSize(
      gfx::Size(kResizeHandleSize, kResizeHandleSize));
  resize_handle_->SetCursor(ui::mojom::CursorType::kNorthWestResize);
  resize_handle_->SetAccessibleName(u"Resize PiP window");
  resize_handle_->SetTooltipText(u"Drag to resize");

  // TODO(astra): Implement actual resize drag handling.
  //   Chromium owner: Widget::SetBounds / PictureInPictureWindowViews
}

void AstraPipControlsView::BuildSnapIndicators() {
  auto create_indicator = [this](PipSnapPosition position) {
    auto indicator = std::make_unique<views::View>();
    indicator->SetPaintToLayer();
    indicator->layer()->SetFillsBoundsOpaquely(false);
    indicator->SetBackground(
        views::CreateSolidBackground(kSnapIndicatorColor));
    indicator->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    indicator->SetCursor(ui::mojom::CursorType::kPointer);
    // TODO(astra): Click to snap to this position.
    return indicator;
  };

  snap_indicator_tl_ = create_indicator(PipSnapPosition::kTopLeft);
  snap_indicator_tl_->SetAccessibleName(u"Snap to top-left corner");
  snap_indicator_tl_->SetTooltipText(u"Snap to top-left");

  snap_indicator_tr_ = create_indicator(PipSnapPosition::kTopRight);
  snap_indicator_tr_->SetAccessibleName(u"Snap to top-right corner");
  snap_indicator_tr_->SetTooltipText(u"Snap to top-right");

  snap_indicator_bl_ = create_indicator(PipSnapPosition::kBottomLeft);
  snap_indicator_bl_->SetAccessibleName(u"Snap to bottom-left corner");
  snap_indicator_bl_->SetTooltipText(u"Snap to bottom-left");

  snap_indicator_br_ = create_indicator(PipSnapPosition::kBottomRight);
  snap_indicator_br_->SetAccessibleName(u"Snap to bottom-right corner");
  snap_indicator_br_->SetTooltipText(u"Snap to bottom-right");
}

void AstraPipControlsView::BuildVolumeControls(views::View* container) {
  // Mute button.
  mute_button_ = container->AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraPipControlsView::OnMuteClicked,
                          base::Unretained(this))));
  ConfigureImageButton(mute_button_, u"Mute", kButtonSize);

  // Volume slider.
  volume_slider_ = container->AddChildView(
      std::make_unique<views::Slider>(this));
  volume_slider_->SetPreferredSize(gfx::Size(kSliderWidth, kSliderHeight));
  volume_slider_->SetMinValue(0.0f);
  volume_slider_->SetMaxValue(1.0f);
  volume_slider_->SetValue(1.0f);
  volume_slider_->SetTooltipText(u"Volume");
  volume_slider_->SetAccessibleName(u"Volume slider");
  volume_slider_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
}

void AstraPipControlsView::BuildPlaybackRateControl(views::View* container) {
  playback_rate_button_ =
      container->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnPlaybackRateClicked,
              base::Unretained(this))));
  ConfigureImageButton(playback_rate_button_, u"Playback speed: 1.0x",
                       kButtonSize);

  playback_rate_label_ = container->AddChildView(
      std::make_unique<views::Label>(u"1.0x"));
  ConfigureControlLabel(playback_rate_label_, u"1.0x");
}

void AstraPipControlsView::BuildOpacityControl(views::View* container) {
  opacity_slider_ = container->AddChildView(
      std::make_unique<views::Slider>(this));
  opacity_slider_->SetPreferredSize(gfx::Size(kSliderWidth, kSliderHeight));
  opacity_slider_->SetMinValue(static_cast<float>(AstraPipControlsModel::kMinOpacity));
  opacity_slider_->SetMaxValue(static_cast<float>(AstraPipControlsModel::kMaxOpacity));
  opacity_slider_->SetValue(1.0f);
  opacity_slider_->SetTooltipText(u"Opacity");
  opacity_slider_->SetAccessibleName(u"Opacity slider");
  opacity_slider_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
}

void AstraPipControlsView::BuildSettingsButton(views::View* container) {
  settings_button_ =
      container->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnSettingsClicked,
              base::Unretained(this))));
  ConfigureImageButton(settings_button_, u"Settings", kButtonSize);
  settings_button_->SetAccessibleName(u"PiP controls settings");
}

void AstraPipControlsView::BuildMinimizeButton(views::View* container) {
  minimize_button_ =
      container->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraPipControlsView::OnMinimizeClicked,
              base::Unretained(this))));
  ConfigureImageButton(minimize_button_, u"Minimize controls", kSmallButtonSize);
  minimize_button_->SetAccessibleName(u"Minimize PiP controls");
}

// -- Button callbacks -------------------------------------------------------

void AstraPipControlsView::OnPlayPauseClicked(const ui::Event& /*event*/) {
  if (model_) {
    model_->TogglePlay();
  }
  if (delegate_) {
    delegate_->OnPlayPause();
  }
}

void AstraPipControlsView::OnSkipBackwardClicked(const ui::Event& /*event*/) {
  int skip_seconds =
      model_ ? model_->GetSkipDurationSeconds()
             : AstraPipControlsModel::kDefaultSkipDurationSeconds;
  if (delegate_) {
    delegate_->OnSkipBackward(skip_seconds);
  }
}

void AstraPipControlsView::OnSkipForwardClicked(const ui::Event& /*event*/) {
  int skip_seconds =
      model_ ? model_->GetSkipDurationSeconds()
             : AstraPipControlsModel::kDefaultSkipDurationSeconds;
  if (delegate_) {
    delegate_->OnSkipForward(skip_seconds);
  }
}

void AstraPipControlsView::OnMuteClicked(const ui::Event& /*event*/) {
  if (model_) {
    model_->ToggleMute();
  }
  if (delegate_) {
    delegate_->OnMuteToggle();
  }
}

void AstraPipControlsView::OnCloseClicked(const ui::Event& /*event*/) {
  if (delegate_) {
    delegate_->OnClosePip();
  }
}

void AstraPipControlsView::OnReturnToTabClicked(const ui::Event& /*event*/) {
  if (delegate_) {
    delegate_->OnReturnToTab();
  }
}

void AstraPipControlsView::OnSizeSmallClicked(const ui::Event& /*event*/) {
  if (model_) {
    model_->SetActiveSizePreset(PipSizePreset::kSmall);
  }
  if (delegate_) {
    delegate_->OnResizePreset(PipSizePreset::kSmall);
  }
}

void AstraPipControlsView::OnSizeMediumClicked(const ui::Event& /*event*/) {
  if (model_) {
    model_->SetActiveSizePreset(PipSizePreset::kMedium);
  }
  if (delegate_) {
    delegate_->OnResizePreset(PipSizePreset::kMedium);
  }
}

void AstraPipControlsView::OnSizeLargeClicked(const ui::Event& /*event*/) {
  if (model_) {
    model_->SetActiveSizePreset(PipSizePreset::kLarge);
  }
  if (delegate_) {
    delegate_->OnResizePreset(PipSizePreset::kLarge);
  }
}

void AstraPipControlsView::OnAlwaysOnTopClicked(const ui::Event& /*event*/) {
  if (model_) {
    model_->ToggleAlwaysOnTop();
  }
  if (delegate_) {
    delegate_->OnAlwaysOnTopToggle();
  }
}

void AstraPipControlsView::OnSettingsClicked(const ui::Event& /*event*/) {
  // TODO(astra): Open settings menu / bubble for PiP controls.
  //   This could show a menu with auto-hide toggle, bar visibility, etc.
  //   For now, toggling controls minimization as a placeholder action.
  if (model_) {
    model_->ToggleControlsMinimized();
  }
}

void AstraPipControlsView::OnMinimizeClicked(const ui::Event& /*event*/) {
  if (model_) {
    model_->ToggleControlsMinimized();
  }
}

void AstraPipControlsView::OnPlaybackRateClicked(const ui::Event& event) {
  if (!model_) return;

  if (event.IsShiftDown()) {
    model_->CyclePlaybackRateBackward();
  } else {
    model_->CyclePlaybackRateForward();
  }

  if (delegate_) {
    delegate_->OnPlaybackRateChanged(model_->playback_rate());
  }
}

void AstraPipControlsView::OnSnapIndicatorClicked(const ui::Event& /*event*/) {
  // TODO(astra): Click snap indicators to change snap position.
}

// -- Visual updates ---------------------------------------------------------

void AstraPipControlsView::UpdateAllFromModel() {
  if (!model_) return;

  OnPlayStateChanged(model_->is_playing());
  OnMuteStateChanged(model_->is_muted());
  OnVolumeChanged(model_->volume());
  OnPlaybackRateChanged(model_->playback_rate());
  OnSizePresetChanged(model_->active_preset());
  OnAlwaysOnTopChanged(model_->is_pinned());
  OnOpacityChanged(model_->opacity());
  OnSnapPositionChanged(model_->snap_position());
  OnControlsMinimizedChanged(model_->controls_minimized());
}

void AstraPipControlsView::UpdateControlVisibilityFromSettings() {
  if (!model_) return;

  // Top bar visibility.
  if (top_bar_) {
    top_bar_->SetVisible(model_->GetShowTopBar());
  }

  // Bottom bar visibility.
  if (bottom_bar_) {
    bottom_bar_->SetVisible(model_->GetShowBottomBar());
  }

  // Resize handle visibility.
  if (resize_handle_) {
    resize_handle_->SetVisible(model_->GetShowResizeHandle());
  }

  // Always-on-top button.
  if (always_on_top_button_) {
    always_on_top_button_->SetVisible(model_->GetShowAlwaysOnTopButton());
  }

  // Playback controls.
  bool show_playback = model_->GetShowPlaybackControls();
  if (play_pause_button_) play_pause_button_->SetVisible(show_playback);
  if (mute_button_) mute_button_->SetVisible(show_playback);
  if (volume_slider_) volume_slider_->SetVisible(show_playback);
  if (playback_rate_button_) playback_rate_button_->SetVisible(show_playback);
  if (playback_rate_label_) playback_rate_label_->SetVisible(show_playback);

  // Skip buttons.
  bool show_skip = model_->GetShowSkipButtons() && show_playback;
  if (skip_backward_button_) skip_backward_button_->SetVisible(show_skip);
  if (skip_forward_button_) skip_forward_button_->SetVisible(show_skip);
}

void AstraPipControlsView::UpdateBarBackgrounds() {
  double opacity =
      model_ ? model_->GetControlsOpacity() : 1.0;
  if (top_bar_) {
    top_bar_->SetBackground(CreateBarBackground(opacity));
  }
  if (bottom_bar_) {
    bottom_bar_->SetBackground(CreateBarBackground(opacity));
  }
}

void AstraPipControlsView::UpdateAccessibilityInfo() {
  if (play_pause_button_ && model_) {
    play_pause_button_->SetAccessibleName(
        model_->is_playing() ? u"Pause" : u"Play");
  }
  if (mute_button_ && model_) {
    mute_button_->SetAccessibleName(
        model_->is_muted() ? u"Unmute" : u"Mute");
  }
  if (volume_slider_ && model_) {
    volume_slider_->SetAccessibleName(
        u"Volume: " + base::NumberToString16(
            static_cast<int>(model_->volume() * 100)) + u"%");
  }
  if (opacity_slider_ && model_) {
    opacity_slider_->SetAccessibleName(
        u"Opacity: " + base::NumberToString16(
            static_cast<int>(model_->opacity() * 100)) + u"%");
  }
}

// -- Auto-hide --------------------------------------------------------------

void AstraPipControlsView::StartAutoHideTimer() {
  if (!model_ || !model_->GetAutoHideControls()) {
    return;
  }
  base::TimeDelta delay = model_->GetAutoHideDelay();
  if (delay.is_zero()) {
    // Hide immediately if delay is zero.
    OnAutoHideTimerFired();
    return;
  }
  auto_hide_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&AstraPipControlsView::OnAutoHideTimerFired,
                     base::Unretained(this)));
}

void AstraPipControlsView::CancelAutoHideTimer() {
  auto_hide_timer_.Stop();
}

void AstraPipControlsView::OnAutoHideTimerFired() {
  if (mouse_hovering_) {
    // Mouse is still hovering — don't hide.
    return;
  }
  if (model_ && model_->controls_visible()) {
    model_->SetControlsVisible(false);
  }
}

// -- Keyboard shortcuts -----------------------------------------------------

bool AstraPipControlsView::HandleKeyboardShortcut(const ui::KeyEvent& event) {
  if (!model_) return false;

  bool handled = true;
  switch (event.key_code()) {
    case ui::VKEY_SPACE:
    case ui::VKEY_K:
      // Play/pause toggle.
      model_->TogglePlay();
      if (delegate_) delegate_->OnPlayPause();
      break;
    case ui::VKEY_M:
      // Mute toggle.
      model_->ToggleMute();
      if (delegate_) delegate_->OnMuteToggle();
      break;
    case ui::VKEY_UP:
      if (event.IsControlDown()) {
        // Volume up.
        model_->IncreaseVolume();
        if (delegate_) delegate_->OnVolumeChanged(model_->volume());
      } else {
        handled = false;
      }
      break;
    case ui::VKEY_DOWN:
      if (event.IsControlDown()) {
        // Volume down.
        model_->DecreaseVolume();
        if (delegate_) delegate_->OnVolumeChanged(model_->volume());
      } else {
        handled = false;
      }
      break;
    case ui::VKEY_LEFT:
      if (event.IsControlDown()) {
        // Skip backward.
        int skip = model_->GetSkipDurationSeconds();
        if (delegate_) delegate_->OnSkipBackward(skip);
      } else {
        handled = false;
      }
      break;
    case ui::VKEY_RIGHT:
      if (event.IsControlDown()) {
        // Skip forward.
        int skip = model_->GetSkipDurationSeconds();
        if (delegate_) delegate_->OnSkipForward(skip);
      } else {
        handled = false;
      }
      break;
    case ui::VKEY_P:
      // Always-on-top toggle.
      model_->ToggleAlwaysOnTop();
      if (delegate_) delegate_->OnAlwaysOnTopToggle();
      break;
    case ui::VKEY_O:
      // Cycle opacity.
      if (event.IsShiftDown()) {
        model_->DecreaseOpacity();
      } else {
        model_->IncreaseOpacity();
      }
      if (delegate_) delegate_->OnOpacityChanged(model_->opacity());
      break;
    case ui::VKEY_S:
      // Cycle size preset.
      if (event.IsShiftDown()) {
        model_->CycleSizePresetBackward();
      } else {
        model_->CycleSizePresetForward();
      }
      if (delegate_) delegate_->OnResizePreset(model_->active_preset());
      break;
    case ui::VKEY_R:
      // Cycle playback rate.
      if (event.IsShiftDown()) {
        model_->CyclePlaybackRateBackward();
      } else {
        model_->CyclePlaybackRateForward();
      }
      if (delegate_) delegate_->OnPlaybackRateChanged(model_->playback_rate());
      break;
    case ui::VKEY_H:
      // Toggle controls visibility (hide/show).
      model_->ToggleControlsVisible();
      break;
    case ui::VKEY_I:
      // Toggle controls minimization.
      model_->ToggleControlsMinimized();
      break;
    case ui::VKEY_ESCAPE:
      // Close PiP.
      if (delegate_) delegate_->OnClosePip();
      break;
    default:
      handled = false;
      break;
  }

  return handled;
}

}  // namespace astra
