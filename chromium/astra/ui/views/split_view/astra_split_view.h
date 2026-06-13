#ifndef ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_VIEW_H_
#define ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/ui/views/split_view/astra_split_view_model.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/accessibility/ax_enums.mojom-forward.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/view.h"

namespace astra {

// =========================================================================
// AstraSplitOrientation
// =========================================================================
//
// Split orientation for Astra split view.  This is the same semantic as
// SplitViewOrientation from astra_tab_features.h but uses the Astra prefix
// naming convention for the views layer.
//
// TODO(astra): Consider unifying with SplitViewOrientation from
//   astra_tab_features.h to avoid duplicate enum definitions.
//   Chromium owner: astra/browser/astra_tab_features.h
enum class AstraSplitOrientation {
  kHorizontal,  // Side by side (left/right panes).
  kVertical,    // Stacked (top/bottom panes).
};

// =========================================================================
// AstraSplitPane
// =========================================================================
//
// Identifies which pane in a split view is being referenced.
enum class AstraSplitPane {
  kPrimary,    // Primary pane (left or top).
  kSecondary,  // Secondary pane (right or bottom).
};

// =========================================================================
// AstraSplitPreset
// =========================================================================
//
// Named layout presets for quick ratio selection.  Each preset corresponds
// to a specific split ratio that users commonly use.
//
// Ratios:
//   - kEqual:        0.5  (50/50 split)
//   - kPrimaryLarge: 0.7  (primary takes 70%)
//   - kSecondaryLarge: 0.3 (primary takes 30%, secondary is large)
//   - kThreeQuarter: 0.75 (primary takes 75%)
//   - kQuarter:      0.25 (primary takes 25%)
//   - kGoldenRatio:  0.618 (golden ratio, primary is the larger side)
enum class AstraSplitPreset {
  kEqual,
  kPrimaryLarge,
  kSecondaryLarge,
  kThreeQuarter,
  kQuarter,
  kGoldenRatio,
};

// =========================================================================
// AstraResizeMode
// =========================================================================
//
// Controls how the split view resizes when the overall view size changes.
//
//   - kFixedRatio:      The ratio stays constant; both panes grow/shrink
//                       proportionally when the view resizes.
//   - kProportional:    Same as kFixedRatio — ratio is preserved.
//   - kMinSizePriority: The primary pane keeps its pixel size as long as
//                       the secondary pane meets the minimum size;
//                       beyond that, the ratio adjusts.
enum class AstraResizeMode {
  kFixedRatio,
  kProportional,
  kMinSizePriority,
};

// =========================================================================
// AstraSplitViewSettings (expanded)
// =========================================================================
//
// Extended settings struct with all 12+ split view configuration options.
// These are presentation preferences persisted via PrefService.
//
// Truth: PrefService owns persistence; this struct is a convenience bundle.
struct AstraSplitViewSettings {
  // Default orientation for new split views.
  AstraSplitOrientation default_orientation = AstraSplitOrientation::kHorizontal;

  // Default ratio for new split views.
  double default_ratio = 0.5;

  // Default layout preset for new split views.
  AstraSplitPreset default_preset = AstraSplitPreset::kEqual;

  // Whether to remember the last used split state (ratio, orientation)
  // and restore it for new split views.
  bool remember_split_state = true;

  // How the split view behaves when the window is resized.
  AstraResizeMode resize_mode = AstraResizeMode::kFixedRatio;

  // Minimum size of each pane in DIPs.
  int min_pane_size = 100;

  // Width (or height, for vertical splits) of the divider in DIPs.
  int divider_width = 4;

  // Whether the divider handle (drag indicator) is visible.
  bool show_divider_handle = true;

  // Whether double-clicking the divider resets to the default ratio.
  bool double_click_divider_resets = true;

  // Number of DIPs to resize per keyboard arrow key press.
  int keyboard_resize_step = 20;

  // Whether to show pane labels (tab titles) on the divider.
  bool show_pane_labels = false;

  // Whether the split view automatically equalizes (resets to 0.5 ratio)
  // when the window is resized.
  bool auto_equal_on_window_resize = false;

  // -----------------------------------------------------------------------
  // Legacy fields (kept for backward compatibility)
  // -----------------------------------------------------------------------

  // Whether the divider is visible and draggable.
  bool divider_visible = true;

  // Whether split views snap to preset ratios when dragged near them.
  bool snap_to_presets = false;

  // Snap distance in DIPs — how close the divider must be to a preset ratio
  // before it snaps to it.
  int snap_distance_dips = 20;

  // Whether to remember the last used ratio for new split views.
  bool remember_ratio = true;

  // Whether the mini map / thumbnail overlay is enabled.
  bool minimap_enabled = false;

  // Whether keyboard navigation is enabled for the divider.
  bool keyboard_navigation_enabled = true;

  // Legacy default orientation (using SplitViewOrientation).
  // TODO(astra): Remove legacy fields after migration to Astra-prefixed enums.
  SplitViewOrientation legacy_default_orientation = SplitViewOrientation::kHorizontal;

  // Legacy default ratio (float).
  float legacy_default_ratio = 0.5f;

  // Whether to show a split view menu button on the divider.
  bool show_menu_button = false;
};

// SplitViewOrientation is defined in astra/browser/astra_tab_features.h
// because it is shared between the browser layer (metadata) and the views
// layer (presentation).  We include it here via astra_tab_features.h.

// Preset split ratios for quick selection.
// These provide convenient split configurations that users can choose from.
//
// Chromium analog: No direct equivalent — this is Astra-specific presentation.
enum class SplitViewPreset {
  kFiftyFifty,   // Equal split (0.5)
  kSeventyThirty, // Primary takes 70% (0.7)
  kThirtySeventy, // Primary takes 30% (0.3)
  kSixtyForty,   // Primary takes 60% (0.6)
  kFortySixty,   // Primary takes 40% (0.4)
};

// Split view settings that can be persisted via PrefService.
// These are presentation preferences, not per-tab state.
//
// Truth: PrefService owns persistence; this struct is a convenience bundle.
struct AstraSplitViewSettings {
  // Whether the divider is visible and draggable.
  bool divider_visible = true;

  // Whether split views snap to preset ratios when dragged near them.
  bool snap_to_presets = false;

  // Snap distance in DIPs — how close the divider must be to a preset ratio
  // before it snaps to it.
  int snap_distance_dips = 20;

  // Whether to remember the last used ratio for new split views.
  bool remember_ratio = true;

  // Whether the mini map / thumbnail overlay is enabled.
  bool minimap_enabled = false;

  // Whether keyboard navigation is enabled for the divider.
  bool keyboard_navigation_enabled = true;

  // Default orientation for new split views.
  SplitViewOrientation default_orientation = SplitViewOrientation::kHorizontal;

  // Default ratio for new split views.
  float default_ratio = 0.5f;

  // Whether to show a split view menu button on the divider.
  // TODO(astra): Implement split view menu button on the divider.
  //   Chromium owner: views::MenuButton (ui/views/controls/button/menu_button.h)
  bool show_menu_button = false;
};

// Forward declaration.
class AstraSplitView;
class AstraSplitEmptyPaneView;
class AstraSplitDropIndicator;

// =========================================================================
// AstraSplitDivider
// =========================================================================

// Divider view that sits between the two split panes and handles drag-to-resize.
//
// The divider is a thin view that captures mouse events and translates them
// into ratio changes on the parent AstraSplitView.  It follows Chromium's
// views::View pattern for interactive controls.
//
// Accessibility:
//   - Role: kSplitter
//   - Keyboard support: arrow keys to resize by step, Home/End to jump
//     to minimum/maximum
//   - Orientation-aware: reports horizontal/vertical splitter role
//
// TODO(astra): Consider reusing views::SplitPane (ui/views/controls/split_pane/)
//   if it satisfies all our needs (ratio API, orientation swapping, divider
//   appearance customization, keyboard accessibility).  We use a custom
//   implementation for finer control over the split ratio semantics and
//   to avoid coupling to SplitPane's specific ownership model.
class AstraSplitDivider : public views::View {
 public:
  explicit AstraSplitDivider(SplitViewOrientation orientation);
  ~AstraSplitDivider() override;

  // Update the divider's orientation (changes hit-test dimensions and cursor).
  void SetOrientation(SplitViewOrientation orientation);
  SplitViewOrientation orientation() const { return orientation_; }

  // Set the step size for keyboard-based resizing, in DIPs.
  void SetKeyboardStepSize(int step_size) { keyboard_step_size_ = step_size; }
  int keyboard_step_size() const { return keyboard_step_size_; }

  // Set whether the divider is visible.  When invisible, it does not paint
  // and does not receive events, but still takes up layout space.
  void SetDividerVisible(bool visible);
  bool divider_visible() const { return divider_visible_; }

  // views::View:
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  gfx::NativeCursor GetCursor(const ui::MouseEvent& event) override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool OnKeyReleased(const ui::KeyEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;

 private:
  // Compute the accessible name based on orientation.
  std::u16string GetAccessibleName() const;

  // Handle a keyboard resize by |delta| DIPs along the split axis.
  // Positive delta moves the divider toward the secondary pane.
  void HandleKeyboardResize(int delta);

  SplitViewOrientation orientation_;

  // Drag state: the offset within the divider where the press occurred,
  // used to keep the divider under the cursor during drag.
  int drag_offset_ = 0;
  bool is_dragging_ = false;

  // Whether the divider is currently hovered (for visual feedback).
  bool is_hovered_ = false;

  // Whether the divider is visible.
  bool divider_visible_ = true;

  // Step size for keyboard-based resizing, in DIPs.
  int keyboard_step_size_ = 20;

  // The parent split view (cached for convenience).
  // Not owned — parent owns this view.
  raw_ptr<AstraSplitView> split_view_ = nullptr;
};

// =========================================================================
// AstraSplitMinimapView
// =========================================================================

// A small thumbnail / minimap overlay that shows a miniature representation
// of both split panes.  This is a lightweight presentation widget that
// appears on hover or via keyboard shortcut, giving the user a quick
// overview of the split layout.
//
// This is a pure presentation view — it does not own or interact with
// WebContents directly.  It just renders a stylized representation of
// the split layout (two rectangles with the current ratio).
//
// Truth: The split view state (ratio, orientation) is read from the
// parent AstraSplitView.  The minimap never mutates state.
//
// TODO(astra): Implement real thumbnail rendering using WebContents
//   capture APIs.  For now, this is a schematic minimap.
//   Chromium owner: content::WebContents::CopyFromSurface()
//   (content/public/browser/web_contents.h)
class AstraSplitMinimapView : public views::View {
 public:
  AstraSplitMinimapView();
  ~AstraSplitMinimapView() override;

  // Update the minimap to reflect the current split state.
  void UpdateLayout(float ratio, SplitViewOrientation orientation);

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
  float ratio_ = 0.5f;
  SplitViewOrientation orientation_ = SplitViewOrientation::kHorizontal;
};

// =========================================================================
// AstraSplitViewButton
// =========================================================================
//
// Base class for split view control buttons.  These are small icon buttons
// that appear on pane headers or on the divider toolbar.
//
// All buttons follow Chromium's ImageButton pattern and have accessible names.
class AstraSplitViewButton : public views::ImageButton {
 public:
  explicit AstraSplitViewButton(PressedCallback callback);
  ~AstraSplitViewButton() override;

  // Set the accessible name for this button.
  void SetAccessibleName(const std::u16string& name);

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnThemeChanged() override;

 protected:
  // Override to provide the icon glyph or paint the button.
  virtual void PaintButtonContents(gfx::Canvas* canvas) = 0;

  // views::ImageButton:
  void PaintButtonContentsOverride(gfx::Canvas* canvas) override {}

 private:
  std::u16string accessible_name_;
};

// =========================================================================
// AstraSplitCloseButton
// =========================================================================
//
// Close button that appears on a pane header.  Clicking it closes the pane.
//
// Accessibility: Role = kButton, name = "Close pane"
class AstraSplitCloseButton : public AstraSplitViewButton {
 public:
  explicit AstraSplitCloseButton(PressedCallback callback);
  ~AstraSplitCloseButton() override;

  // Set which pane this close button belongs to.
  void SetPane(AstraSplitPane pane) { pane_ = pane; }
  AstraSplitPane pane() const { return pane_; }

 protected:
  void PaintButtonContents(gfx::Canvas* canvas) override;

 private:
  AstraSplitPane pane_ = AstraSplitPane::kPrimary;
};

// =========================================================================
// AstraSplitSwapButton
// =========================================================================
//
// Swap button that swaps the primary and secondary panes.
// Appears on the divider toolbar or as a header action.
//
// Accessibility: Role = kButton, name = "Swap panes"
class AstraSplitSwapButton : public AstraSplitViewButton {
 public:
  explicit AstraSplitSwapButton(PressedCallback callback);
  ~AstraSplitSwapButton() override;

 protected:
  void PaintButtonContents(gfx::Canvas* canvas) override;
};

// =========================================================================
// AstraSplitLayoutToggleButton
// =========================================================================
//
// Button that toggles between horizontal and vertical split orientation.
// For multi-pane layouts, cycles through layout modes.
//
// Accessibility: Role = kButton, name = "Toggle split layout"
class AstraSplitLayoutToggleButton : public AstraSplitViewButton {
 public:
  explicit AstraSplitLayoutToggleButton(PressedCallback callback);
  ~AstraSplitLayoutToggleButton() override;

  // Set the current layout mode to display the appropriate icon.
  void SetLayoutMode(AstraSplitLayoutMode mode);
  AstraSplitLayoutMode layout_mode() const { return layout_mode_; }

 protected:
  void PaintButtonContents(gfx::Canvas* canvas) override;

 private:
  AstraSplitLayoutMode layout_mode_ = AstraSplitLayoutMode::kTwoPaneHorizontal;
};

// =========================================================================
// AstraSplitPaneHeader
// =========================================================================
//
// Header bar that appears at the top of each split pane.  Contains the
// pane title and optional control buttons (close, etc.).
//
// The header is a lightweight presentation widget — it reflects state from
// the model but does not own state.
//
// Accessibility:
//   - Role: kGrouping
//   - Label: the pane title
//   - Buttons inside have their own accessible names
class AstraSplitPaneHeader : public views::View {
 public:
  AstraSplitPaneHeader();
  ~AstraSplitPaneHeader() override;

  // Set the title text displayed in the header.
  void SetTitle(const std::u16string& title);
  const std::u16string& title() const { return title_; }

  // Show or hide the close button.
  void SetShowCloseButton(bool show);
  bool show_close_button() const { return show_close_button_; }

  // Set which pane this header belongs to.
  void SetPane(AstraSplitPane pane) { pane_ = pane; }
  AstraSplitPane pane() const { return pane_; }

  // Set the close button callback.
  void SetCloseCallback(base::RepeatingClosure callback) {
    close_callback_ = callback;
  }

  // views::View:
  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnPaint(gfx::Canvas* canvas) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnThemeChanged() override;

 private:
  void OnCloseButtonPressed();

  std::u16string title_;
  AstraSplitPane pane_ = AstraSplitPane::kPrimary;
  bool show_close_button_ = false;

  raw_ptr<AstraSplitCloseButton> close_button_ = nullptr;
  base::RepeatingClosure close_callback_;
};

// =========================================================================
// AstraSplitDividerToolbar
// =========================================================================
//
// Toolbar that appears on or near the divider, containing split view
// control buttons like swap and layout toggle.
//
// The toolbar is positioned at a fixed location along the divider
// (typically the center or top).  It contains action buttons that
// operate on the split view as a whole.
//
// Accessibility:
//   - Role: kToolbar
//   - Buttons inside have their own accessible names
//   - Keyboard accessible via Tab navigation
class AstraSplitDividerToolbar : public views::View {
 public:
  AstraSplitDividerToolbar();
  ~AstraSplitDividerToolbar() override;

  // Set callbacks for the toolbar buttons.
  void SetSwapCallback(base::RepeatingClosure callback);
  void SetLayoutToggleCallback(base::RepeatingClosure callback);

  // Update the layout toggle button icon based on current mode.
  void UpdateLayoutMode(AstraSplitLayoutMode mode);

  // Show or hide the toolbar.
  void SetToolbarVisible(bool visible);
  bool toolbar_visible() const { return toolbar_visible_; }

  // views::View:
  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnPaint(gfx::Canvas* canvas) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnThemeChanged() override;

 private:
  raw_ptr<AstraSplitSwapButton> swap_button_ = nullptr;
  raw_ptr<AstraSplitLayoutToggleButton> layout_toggle_button_ = nullptr;
  bool toolbar_visible_ = true;
};

// =========================================================================
// AstraSplitEmptyPaneView
// =========================================================================
//
// Placeholder view shown when a split pane has no content.
// Displays a message and an "Open tab" button to encourage the user
// to populate the pane.
//
// This is a pure presentation widget — it does not create tabs or
// manipulate browser state.  It just shows a UI and fires a callback
// when the button is pressed.
//
// Accessibility:
//   - Role: kGrouping
//   - Contains a button with "Open tab" label
class AstraSplitEmptyPaneView : public views::View {
 public:
  explicit AstraSplitEmptyPaneView(base::RepeatingClosure open_tab_callback);
  ~AstraSplitEmptyPaneView() override;

  // Set the message text displayed in the placeholder.
  void SetMessage(const std::u16string& message);
  const std::u16string& message() const { return message_; }

  // Set the button label.
  void SetButtonLabel(const std::u16string& label);

  // Show or hide the "Open tab" button.
  void SetButtonVisible(bool visible);

  // views::View:
  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnPaint(gfx::Canvas* canvas) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnThemeChanged() override;

 private:
  void OnOpenTabButtonPressed();

  std::u16string message_ = u"This pane is empty";
  raw_ptr<AstraSplitViewButton> open_tab_button_ = nullptr;
  base::RepeatingClosure open_tab_callback_;
  bool button_visible_ = true;
};

// =========================================================================
// AstraSplitDropIndicator
// =========================================================================
//
// Visual indicator shown when a tab is being dragged over a split pane.
// Highlights the target pane to show where the dropped tab will land.
//
// This is a transient overlay view that appears during drag-and-drop.
//
// Accessibility:
//   - Role: kGrouping
//   - Name: "Drop target"
class AstraSplitDropIndicator : public views::View {
 public:
  AstraSplitDropIndicator();
  ~AstraSplitDropIndicator() override;

  // Show the drop indicator at the specified pane position.
  // |pane_bounds| is the bounds of the target pane in the split view's
  // coordinate space.
  void ShowForPane(const gfx::Rect& pane_bounds);

  // Hide the drop indicator.
  void Hide();

  // Set whether the drop is currently valid (affects appearance).
  void SetDropValid(bool valid);
  bool drop_valid() const { return drop_valid_; }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnThemeChanged() override;

 private:
  bool drop_valid_ = true;
  bool is_visible_ = false;
};

// =========================================================================
// AstraSplitView
// =========================================================================

// AstraSplitView is a Views container that arranges two child views
// side-by-side (horizontal) or stacked (vertical) with a draggable divider.
//
// This is a pure presentation view — it knows nothing about WebContents,
// tabs, or browser state.  It simply arranges two arbitrary views with
// a configurable ratio and orientation.
//
// State model:
//   - All state changes go through the controller; the view just reflects state.
//   - The view notifies its delegate (the controller) of user-initiated
//     changes (divider drag completion).
//
// Observer pattern:
//   - Views that need to react to split ratio changes can observe via
//     the Observer interface.  The controller typically observes the view
//     to persist state back to AstraTabFeatures.
//
// Chromium subsystem considered: views::SplitPane (ui/views/controls/split_pane/).
class AstraSplitView : public views::View {
 public:
  // Observer interface for split view state changes.
  // Implemented by the controller and any other objects that need to
  // react to ratio or orientation changes initiated from the view.
  //
  // All observer methods have empty default implementations so that
  // observers can override only the methods they care about.
  class Observer : public base::CheckedObserver {
   public:
    // Called when the user finishes dragging the divider and the ratio
    // has settled.  |ratio| is the new split ratio.
    virtual void OnSplitRatioChanged(float ratio) {}

    // Called continuously during a divider drag.  Observers can use this
    // for live updates, but should avoid heavy work.
    virtual void OnSplitRatioChanging(float ratio) {}

    // Called when the split orientation changes.
    virtual void OnSplitOrientationChanged(SplitViewOrientation orientation) {}

    // Called when the primary and secondary panes are swapped.
    virtual void OnSplitViewsSwapped() {}

    // Called when one of the child views is replaced.
    // |side| indicates which side was replaced (true = primary, false = secondary).
    virtual void OnSplitViewReplaced(bool is_primary) {}

    // Called when split view settings change (e.g. divider visibility,
    // snap-to-preset behavior).
    virtual void OnSplitViewSettingsChanged(
        const AstraSplitViewSettings& settings) {}

    // Called when the split view is about to be destroyed.
    virtual void OnSplitViewDestroyed() {}

    // Called when the split view enters a maximized state (one pane
    // takes nearly all the space).
    virtual void OnSplitViewMaximized(bool primary_maximized) {}

    // Called when the split view is reset from a maximized state back
    // to a normal split ratio.
    virtual void OnSplitViewUnmaximized() {}

   protected:
    ~Observer() override = default;
  };

  // Thickness of the draggable divider in DIPs.
  static constexpr int kDividerThickness = 4;

  // Thickness of the visual divider line (smaller than the hit target).
  static constexpr int kDividerVisualThickness = 1;

  // Minimum size of each pane as a fraction [0.0, 1.0].
  static constexpr float kMinPaneRatio = 0.1f;

  // Minimum size of each pane in DIPs (takes precedence over ratio-based
  // minimum when the split view is very small).
  static constexpr int kMinPaneSizeDips = 100;

  // Default minimap size.
  static constexpr int kMinimapWidth = 120;
  static constexpr int kMinimapHeight = 80;

  AstraSplitView();
  ~AstraSplitView() override;

  // -- Observer management ------------------------------------------------

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // ========================================================================
  // Extended API (Astra naming convention)
  // ========================================================================

  // -- Ratio ----------------------------------------------------------------

  // Set the split ratio.  |ratio| is clamped to valid range [0.1, 0.9]
  // and also respects the minimum pane size in pixels.
  void SetRatio(double ratio);
  // Get the current split ratio.
  double GetRatio() const;

  // -- Orientation ----------------------------------------------------------

  // Set the split orientation using AstraSplitOrientation enum.
  void SetOrientation(AstraSplitOrientation orientation);
  // Get the current split orientation.
  AstraSplitOrientation GetOrientation() const;

  // -- Divider configuration ------------------------------------------------

  // Set the divider width (thickness) in DIPs.
  void SetDividerWidth(int width);
  // Get the divider width in DIPs.
  int GetDividerWidth() const;

  // Set whether the divider handle (visual drag indicator) is shown.
  void SetShowHandle(bool show);
  // Returns whether the divider handle is shown.
  bool GetShowHandle() const;

  // -- Pane views -----------------------------------------------------------

  // Set the primary pane content view.
  void SetPrimaryView(views::View* view);
  // Set the secondary pane content view.
  void SetSecondaryView(views::View* view);

  // Get the primary pane view (may be null).
  views::View* GetPrimaryView();
  const views::View* GetPrimaryView() const;

  // Get the secondary pane view (may be null).
  views::View* GetSecondaryView();
  const views::View* GetSecondaryView() const;

  // -- Divider position -----------------------------------------------------

  // Set the divider position in DIPs along the split axis.
  // For horizontal orientation this is the x-position of the divider.
  // For vertical orientation this is the y-position of the divider.
  void SetDividerPosition(int position);
  // Get the current divider position in DIPs.
  int GetDividerPosition() const;

  // -- Focused pane ---------------------------------------------------------

  // Set which pane has focus (for visual focus indicator).
  void SetFocusedPane(AstraSplitPane pane);
  // Get the currently focused pane.
  AstraSplitPane GetFocusedPane() const;

  // -- Pane labels ----------------------------------------------------------

  // Set the label text for both panes (shown on the divider when enabled).
  void SetPaneLabels(const std::u16string& primary_label,
                     const std::u16string& secondary_label);
  // Show or hide pane labels on the divider.
  void ShowPaneLabels(bool show);

  // -- Drag state -----------------------------------------------------------

  // Returns true if the user is currently dragging the divider.
  bool IsDraggingDivider() const;

  // ========================================================================
  // Layout modes and multi-pane support
  // ========================================================================

  // Set the layout mode (2-pane, 3-pane, grid, etc.).
  // Resets all divider positions to equal ratios for the new layout.
  // TODO(astra): Full multi-pane layout support requires additional dividers
  //   and pane containers.  Current implementation supports 2-pane mode.
  //   Chromium owner: views::View hierarchy, similar to views::GridLayout.
  void SetLayoutMode(AstraSplitLayoutMode mode);
  AstraSplitLayoutMode layout_mode() const { return layout_mode_; }

  // Returns the number of panes in the current layout.
  int GetPaneCount() const;

  // Returns true if the current layout is a grid layout.
  bool IsGridLayout() const;

  // Cycle to the next layout mode (2-pane horizontal -> 2-pane vertical ->
  // 3-pane horizontal -> 3-pane vertical -> grid 2x2 -> ... -> wrap).
  void CycleNextLayoutMode();

  // Cycle to the previous layout mode.
  void CyclePreviousLayoutMode();

  // ========================================================================
  // Pane headers and control buttons
  // ========================================================================

  // Show or hide pane headers (title bars with close buttons).
  void SetShowPaneHeaders(bool show);
  bool show_pane_headers() const { return show_pane_headers_; }

  // Set the title text for a specific pane.
  void SetPaneTitle(AstraSplitPane pane, const std::u16string& title);

  // Show or hide the divider toolbar (swap + layout toggle buttons).
  void SetShowDividerToolbar(bool show);
  bool show_divider_toolbar() const { return show_divider_toolbar_; }

  // Access the divider toolbar (may be null if toolbar is not created).
  AstraSplitDividerToolbar* divider_toolbar() { return divider_toolbar_; }
  const AstraSplitDividerToolbar* divider_toolbar() const {
    return divider_toolbar_;
  }

  // Access pane headers (may be null if headers are not shown).
  AstraSplitPaneHeader* primary_header() { return primary_header_; }
  const AstraSplitPaneHeader* primary_header() const { return primary_header_; }
  AstraSplitPaneHeader* secondary_header() { return secondary_header_; }
  const AstraSplitPaneHeader* secondary_header() const {
    return secondary_header_;
  }

  // ========================================================================
  // Pane operations (from user actions on buttons)
  // ========================================================================

  // Close a pane (removes it from the split view).
  // If only one pane remains, split view is conceptually "closed".
  // TODO(astra): Implement multi-pane close semantics.
  //   Currently works for 2-pane: closing one pane leaves the other full-size.
  void ClosePane(AstraSplitPane pane);

  // ========================================================================
  // Focus indicator
  // ========================================================================

  // Show or hide the focus highlight border around the active pane.
  void SetShowFocusIndicator(bool show);
  bool show_focus_indicator() const { return show_focus_indicator_; }

  // ========================================================================
  // Empty pane placeholder
  // ========================================================================

  // Show or hide the empty pane placeholder for a specific pane.
  // The placeholder is shown when a pane has no content.
  void SetEmptyPaneVisible(AstraSplitPane pane, bool visible);
  bool IsEmptyPaneVisible(AstraSplitPane pane) const;

  // Set the message text for the empty pane placeholder.
  void SetEmptyPaneMessage(AstraSplitPane pane, const std::u16string& message);

  // ========================================================================
  // Tab drop indicator
  // ========================================================================

  // Show the drop indicator on the specified pane (during tab drag).
  void ShowDropIndicator(AstraSplitPane pane, bool valid = true);

  // Hide the drop indicator.
  void HideDropIndicator();

  // Returns true if the drop indicator is currently visible.
  bool IsDropIndicatorVisible() const;

  // ========================================================================
  // Divider context menu
  // ========================================================================

  // Show the divider context menu at the given position (in screen coords).
  // The menu provides layout options: orientation toggle, presets, etc.
  // TODO(astra): Implement using views::MenuRunner.
  //   Chromium owner: ui/views/controls/menu/menu_runner.h
  void ShowDividerContextMenu(const gfx::Point& screen_point);

  // ========================================================================
  // Snap points visual feedback
  // ========================================================================

  // Show visual snap point indicators along the divider axis.
  void SetShowSnapIndicators(bool show);
  bool show_snap_indicators() const { return show_snap_indicators_; }

  // Set the snap points (as ratios) for visual indicators and snapping.
  void SetSnapPoints(const std::vector<double>& points);
  const std::vector<double>& snap_points() const { return snap_points_; }

  // Reset snap points to defaults (25%, 33%, 50%, 67%, 75%).
  void ResetSnapPointsToDefaults();

  // ========================================================================
  // Pane action buttons
  // ========================================================================

  // Show or hide the per-pane action buttons (swap, maximize, close, new tab).
  void SetShowPaneActionButtons(bool show);
  bool show_pane_action_buttons() const { return show_pane_action_buttons_; }

  // ========================================================================
  // Smooth resizing animations
  // ========================================================================

  // Set whether smooth resizing animations are enabled.
  void SetAnimateResizing(bool animate);
  bool animate_resizing() const { return animate_resizing_; }

  // Set the animation duration in milliseconds.
  void SetAnimationDurationMs(int duration_ms);
  int animation_duration_ms() const { return animation_duration_ms_; }

  // ========================================================================
  // Accessibility
  // ========================================================================

  // Get the accessible name for the split view as a whole.
  std::u16string GetAccessibleName() const;

  // Set a custom accessible description for the split view.
  void SetAccessibleDescription(const std::u16string& description);
  const std::u16string& accessible_description() const {
    return accessible_description_;
  }

  // ========================================================================
  // Legacy API (float-based, with animation support)
  // ========================================================================
  //
  // These methods use the legacy SplitViewOrientation / SplitViewPreset
  // types and float ratio.  They are kept for backward compatibility.
  // New code should prefer the Astra-prefixed enums and double-based API.

  // Replace the primary view with a new view.  Notifies observers.
  // This is equivalent to SetPrimaryView but with observer notification.
  void ReplacePrimaryView(views::View* new_view);

  // Replace the secondary view with a new view.  Notifies observers.
  void ReplaceSecondaryView(views::View* new_view);

  // -- Ratio and orientation (legacy) --------------------------------------

  // Set the split ratio (float version, with optional animation).
  void SetRatio(float ratio, bool animate = false);
  float ratio() const { return static_cast<float>(ratio_); }

  // Set the split ratio to a named preset (legacy enum).
  void SetPresetRatio(SplitViewPreset preset, bool animate = false);

  // Get the current preset if the ratio matches a preset, or nullopt.
  // Returns the closest preset within |tolerance| if snap mode is active.
  absl::optional<SplitViewPreset> GetCurrentPreset(float tolerance = 0.02f) const;

  // Set the split orientation.
  void SetOrientation(SplitViewOrientation orientation);
  SplitViewOrientation orientation() const { return orientation_; }

  // Toggle between horizontal and vertical orientation.
  void ToggleOrientation();

  // Swap primary and secondary views (without reparenting them outside the
  // split view).  The ratio is preserved but conceptually "inverted" in
  // terms of which view gets which space.
  void SwapViews();

  // Maximize the primary pane (reduce secondary to minimum size).
  // If |primary| is true, primary is maximized; otherwise secondary is.
  void MaximizePane(bool primary);

  // Reset from maximized state back to the previous non-maximized ratio.
  void Unmaximize();

  // Returns true if a pane is currently maximized.
  bool IsMaximized() const { return is_maximized_; }
  bool IsPrimaryMaximized() const { return is_maximized_ && primary_maximized_; }

  // -- Settings -----------------------------------------------------------

  // Apply split view settings.  Notifies observers of changes.
  void ApplySettings(const AstraSplitViewSettings& settings);

  // Get the current settings.
  const AstraSplitViewSettings& settings() const { return settings_; }

  // -- Minimap ------------------------------------------------------------

  // Show or hide the mini map / thumbnail view.
  void SetMinimapVisible(bool visible);
  bool minimap_visible() const { return minimap_visible_; }

  // -- Accessors ----------------------------------------------------------

  AstraSplitDivider* divider() { return divider_; }
  const AstraSplitDivider* divider() const { return divider_; }

  views::View* primary_view() { return primary_view_; }
  const views::View* primary_view() const { return primary_view_; }

  views::View* secondary_view() { return secondary_view_; }
  const views::View* secondary_view() const { return secondary_view_; }

  AstraSplitMinimapView* minimap() { return minimap_; }
  const AstraSplitMinimapView* minimap() const { return minimap_; }

  // -- views::View overrides ----------------------------------------------

  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  // -- Public helper: ratio clamping --------------------------------------
  //
  // Exposed as public so the controller can validate ratio values
  // without needing to create a split view.

  // Clamps |ratio| to the valid range, respecting both ratio-based and
  // pixel-based minimum pane sizes for a view of |total_size| DIPs.
  // If |total_size| is <= 0, only ratio-based clamping is applied.
  static float ClampRatio(float ratio, int total_size = 0);

 private:
  // Called by the divider when a drag begins.
  void OnDividerDragStarted(const gfx::Point& location);

  // Called by the divider during a drag.  |location| is in this view's
  // coordinate space.
  void OnDividerDragged(const gfx::Point& location);

  // Called by the divider when a drag ends.
  void OnDividerDragEnded();

  // Compute the divider position (in DIPs) from the current ratio.
  int ComputeDividerPosition() const;

  // Compute the ratio from a divider position (in DIPs).
  float ComputeRatioFromPosition(int position) const;

  // Animate the divider to a new position.
  void AnimateDividerToPosition(int new_position);

  // Snap |ratio| to the nearest preset if snap-to-preset is enabled
  // and the ratio is within the snap distance.
  float MaybeSnapToPreset(float ratio);

  // Notify observers that the ratio is changing (during drag).
  void NotifyRatioChanging();

  // Notify observers that the ratio has changed (after drag ends).
  void NotifyRatioChanged();

  // Notify observers of orientation change.
  void NotifyOrientationChanged();

  // Notify observers of view swap.
  void NotifyViewsSwapped();

  // Notify observers of view replacement.
  void NotifyViewReplaced(bool is_primary);

  // Notify observers of settings change.
  void NotifySettingsChanged();

  // Notify observers of maximize state change.
  void NotifyMaximized(bool primary_maximized);

  // Notify observers of unmaximize.
  void NotifyUnmaximized();

  // Refresh the minimap display based on current state.
  void UpdateMinimap();

  // Position the minimap in the corner of the split view.
  void LayoutMinimap();

  // -- Multi-pane layout helpers -------------------------------------------

  // Create or destroy pane headers based on show_pane_headers_.
  void UpdatePaneHeaders();

  // Create or destroy the divider toolbar based on show_divider_toolbar_.
  void UpdateDividerToolbar();

  // Layout pane headers within each pane area.
  void LayoutPaneHeaders();

  // Layout the divider toolbar (positioned on the divider).
  void LayoutDividerToolbar();

  // Called when the swap button is pressed.
  void OnSwapButtonPressed();

  // Called when the layout toggle button is pressed.
  void OnLayoutToggleButtonPressed();

  // Called when a pane's close button is pressed.
  void OnCloseButtonPressed(AstraSplitPane pane);

  // Notify observers that a pane was closed.
  void NotifyPaneClosed(AstraSplitPane pane);

  // Paint focus indicator border around the focused pane.
  void PaintFocusIndicator(gfx::Canvas* canvas);

  // Paint snap point indicators along the divider axis.
  void PaintSnapIndicators(gfx::Canvas* canvas);

  // Layout the empty pane placeholders.
  void LayoutEmptyPanes();

  // Layout the drop indicator.
  void LayoutDropIndicator();

  // Handle "open tab" button press in an empty pane.
  void OnEmptyPaneOpenTab(AstraSplitPane pane);

  // Get the bounds of a specific pane in this view's coordinate space.
  gfx::Rect GetPaneBounds(AstraSplitPane pane) const;

  raw_ptr<views::View> primary_view_ = nullptr;
  raw_ptr<views::View> secondary_view_ = nullptr;
  raw_ptr<AstraSplitDivider> divider_ = nullptr;
  raw_ptr<AstraSplitMinimapView> minimap_ = nullptr;
  raw_ptr<AstraSplitPaneHeader> primary_header_ = nullptr;
  raw_ptr<AstraSplitPaneHeader> secondary_header_ = nullptr;
  raw_ptr<AstraSplitDividerToolbar> divider_toolbar_ = nullptr;
  raw_ptr<AstraSplitEmptyPaneView> primary_empty_pane_ = nullptr;
  raw_ptr<AstraSplitEmptyPaneView> secondary_empty_pane_ = nullptr;
  raw_ptr<AstraSplitDropIndicator> drop_indicator_ = nullptr;

  double ratio_ = 0.5;
  SplitViewOrientation orientation_ = SplitViewOrientation::kHorizontal;

  // Current layout mode (2-pane, 3-pane, grid, etc.).
  AstraSplitLayoutMode layout_mode_ = AstraSplitLayoutMode::kTwoPaneHorizontal;

  // Which pane currently has focus (for visual focus indicator).
  AstraSplitPane focused_pane_ = AstraSplitPane::kPrimary;

  // Pane labels (shown on the divider when enabled).
  std::u16string primary_label_;
  std::u16string secondary_label_;
  bool show_pane_labels_ = false;

  // Whether pane headers (title bars with close buttons) are shown.
  bool show_pane_headers_ = false;

  // Whether the divider toolbar is shown.
  bool show_divider_toolbar_ = false;

  // Accessible description.
  std::u16string accessible_description_;

  // Whether the divider handle (visual drag indicator) is shown.
  bool show_handle_ = true;

  // Divider width/thickness in DIPs.
  int divider_width_ = kDividerThickness;

  // Whether a drag is currently in progress (used to suppress observer
  // "changed" notifications until drag ends).
  bool is_dragging_ = false;

  // Maximized state: when true, one pane takes nearly all the space.
  // We remember the ratio before maximization so we can restore it.
  bool is_maximized_ = false;
  bool primary_maximized_ = true;
  double pre_maximize_ratio_ = 0.5;

  // Split view settings.
  AstraSplitViewSettings settings_;

  // Whether the minimap is currently visible.
  bool minimap_visible_ = false;

  // Whether focus indicator (highlight border) is shown around the active pane.
  bool show_focus_indicator_ = false;

  // Whether snap point visual indicators are shown.
  bool show_snap_indicators_ = false;

  // Snap points (ratios) for visual indicators and snapping behavior.
  std::vector<double> snap_points_;

  // Whether per-pane action buttons are shown.
  bool show_pane_action_buttons_ = false;

  // Whether smooth resizing animations are enabled.
  bool animate_resizing_ = true;

  // Animation duration in milliseconds.
  int animation_duration_ms_ = 150;

  // Whether the drop indicator is currently visible.
  bool drop_indicator_visible_ = false;

  // Which pane the drop indicator is shown on (when visible).
  AstraSplitPane drop_indicator_pane_ = AstraSplitPane::kPrimary;

  // Whether the drop is valid (affects indicator appearance).
  bool drop_valid_ = true;

  // Observers for split view state changes.
  base::ObserverList<Observer> observers_;

  // The divider is a friend so it can call the drag callbacks.
  friend class AstraSplitDivider;

  // The pane header is a friend so it can call close button callbacks.
  friend class AstraSplitPaneHeader;
};

// Converts a preset enum value to a ratio.
float SplitViewPresetToRatio(SplitViewPreset preset);

// Converts a ratio to the nearest preset, or nullopt if no preset is
// within |tolerance|.
absl::optional<SplitViewPreset> RatioToSplitViewPreset(
    float ratio, float tolerance = 0.02f);

// Returns the display name of a preset (for UI labels and accessibility).
std::u16string SplitViewPresetToName(SplitViewPreset preset);

// =========================================================================
// Astra-prefixed enum helpers
// =========================================================================

// Converts an AstraSplitPreset enum value to a ratio.
double AstraSplitPresetToRatio(AstraSplitPreset preset);

// Converts a ratio to the nearest AstraSplitPreset, or nullopt if no preset
// is within |tolerance|.
absl::optional<AstraSplitPreset> RatioToAstraSplitPreset(
    double ratio, double tolerance = 0.02);

// Returns the display name of an Astra preset (for UI labels and accessibility).
std::u16string AstraSplitPresetToName(AstraSplitPreset preset);

// Converts between AstraSplitOrientation and SplitViewOrientation.
SplitViewOrientation ToLegacyOrientation(AstraSplitOrientation orientation);
AstraSplitOrientation FromLegacyOrientation(SplitViewOrientation orientation);

// Converts an AstraSplitOrientation to a string (for prefs/storage).
std::string AstraSplitOrientationToString(AstraSplitOrientation orientation);
// Parses a string to an AstraSplitOrientation.
AstraSplitOrientation AstraSplitOrientationFromString(
    const std::string& value);

// Converts an AstraResizeMode to a string (for prefs/storage).
std::string AstraResizeModeToString(AstraResizeMode mode);
// Parses a string to an AstraResizeMode.
AstraResizeMode AstraResizeModeFromString(const std::string& value);

// Converts an AstraSplitPreset to a string (for prefs/storage).
std::string AstraSplitPresetToString(AstraSplitPreset preset);
// Parses a string to an AstraSplitPreset.
AstraSplitPreset AstraSplitPresetFromString(const std::string& value);

// Converts an AstraSplitPane to a string (for prefs/storage).
std::string AstraSplitPaneToString(AstraSplitPane pane);
// Parses a string to an AstraSplitPane.
AstraSplitPane AstraSplitPaneFromString(const std::string& value);

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_VIEW_H_
