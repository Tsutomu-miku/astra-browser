// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_ZOOM_ASTRA_ZOOM_BUTTON_H_
#define ASTRA_UI_VIEWS_ZOOM_ASTRA_ZOOM_BUTTON_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class Label;
class LabelButton;
}  // namespace views

namespace astra {

// =========================================================================
// AstraZoomButton — toolbar zoom button with popup
// =========================================================================
//
// A toolbar button that shows the current zoom level and opens a popup
// with zoom controls (zoom in, zoom out, reset, default zoom setting).
//
// Button states:
//   - Normal: shows current zoom percentage
//   - Hover: highlights the button
//   - Active: shows the zoom control bubble
//
// Bubble layout:
//   - Zoom level display (large percentage)
//   - Zoom out / reset / zoom in buttons
//   - "Use default" checkbox
//   - "Manage default zoom" link
//
// Chromium pattern: ZoomButton / ZoomBubbleView
//   (chrome/browser/ui/views/zoom/zoom_bubble_view.h)
//
// TODO(astra): Integrate with BrowserView toolbar via a patch.
//   Chromium owner: ZoomBubbleView (chrome/browser/ui/views/zoom/)
//   Patch point: ToolbarView — add zoom button to right side
// =========================================================================

class AstraZoomBubbleView;

class AstraZoomButton : public views::ImageButton {
 public:
  // Delegate for zoom button actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the zoom button is clicked (to show/hide bubble).
    virtual void OnZoomButtonClicked() = 0;

    // Called to get the current zoom level.
    virtual double GetZoomLevel() = 0;

    // Called to zoom in.
    virtual void OnZoomIn() = 0;

    // Called to zoom out.
    virtual void OnZoomOut() = 0;

    // Called to reset zoom to default.
    virtual void OnZoomReset() = 0;

    // Called when default zoom setting is changed.
    virtual void OnDefaultZoomChanged(double default_zoom) = 0;

    // Called to get the default zoom level.
    virtual double GetDefaultZoom() = 0;
  };

  AstraZoomButton();
  ~AstraZoomButton() override;

  AstraZoomButton(const AstraZoomButton&) = delete;
  AstraZoomButton& operator=(const AstraZoomButton&) = delete;

  // -- Zoom state -----------------------------------------------------------

  // Set the current zoom level (1.0 = 100%).
  void SetZoomLevel(double zoom_level);
  double zoom_level() const { return zoom_level_; }

  // Set the default zoom level.
  void SetDefaultZoom(double default_zoom);
  double default_zoom() const { return default_zoom_; }

  // Show or hide the zoom bubble.
  void ShowBubble();
  void HideBubble();
  bool IsBubbleShowing() const;

  // -- Delegate -------------------------------------------------------------

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // -- views::ImageButton ---------------------------------------------------

  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  // Update the button label/icon.
  void UpdateButtonDisplay();

  // Handle button press.
  void HandleButtonPress();

  // Delegate.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Zoom state.
  double zoom_level_ = 1.0;
  double default_zoom_ = 1.0;

  // Bubble reference (not owned).
  raw_ptr<AstraZoomBubbleView> bubble_ = nullptr;

  // -- Constants ------------------------------------------------------------

  static constexpr int kButtonSize = 28;
};

// =========================================================================
// AstraZoomBubbleView — zoom control bubble
// =========================================================================

class AstraZoomBubbleView : public views::BubbleDialogDelegateView {
 public:
  // Delegate for bubble actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void OnZoomIn() = 0;
    virtual void OnZoomOut() = 0;
    virtual void OnZoomReset() = 0;
    virtual void OnDefaultZoomChanged(double zoom) = 0;
    virtual void OnBubbleClosed() = 0;
  };

  explicit AstraZoomBubbleView(views::View* anchor_view);
  ~AstraZoomBubbleView() override;

  AstraZoomBubbleView(const AstraZoomBubbleView&) = delete;
  AstraZoomBubbleView& operator=(const AstraZoomBubbleView&) = delete;

  // -- Zoom state -----------------------------------------------------------

  void SetZoomLevel(double zoom_level);
  void SetDefaultZoom(double default_zoom);

  // -- Delegate -------------------------------------------------------------

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // -- views::BubbleDialogDelegateView --------------------------------------

  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  void WindowClosing() override;

 private:
  // Build the bubble layout.
  void BuildLayout();

  // Update zoom level display.
  void UpdateZoomDisplay();

  // Update button states.
  void UpdateButtonStates();

  // Button handlers.
  void OnZoomOutPressed();
  void OnZoomResetPressed();
  void OnZoomInPressed();

  // Delegate.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Zoom state.
  double zoom_level_ = 1.0;
  double default_zoom_ = 1.0;

  // Child views.
  raw_ptr<views::Label> zoom_label_ = nullptr;
  raw_ptr<views::ImageButton> zoom_out_button_ = nullptr;
  raw_ptr<views::LabelButton> reset_button_ = nullptr;
  raw_ptr<views::ImageButton> zoom_in_button_ = nullptr;
  raw_ptr<views::Label> default_label_ = nullptr;

  // -- Constants ------------------------------------------------------------

  static constexpr int kBubbleWidth = 200;
  static constexpr int kBubbleHeight = 100;
  static constexpr int kButtonSize = 28;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_ZOOM_ASTRA_ZOOM_BUTTON_H_
