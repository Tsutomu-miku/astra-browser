#ifndef ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_VIEW_MODEL_H_
#define ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_VIEW_MODEL_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace astra {

// =========================================================================
// AstraSplitLayoutMode
// =========================================================================
//
// Layout mode for the split view.  Determines how panes are arranged.
//
//   - kTwoPaneHorizontal: Two panes side by side (left/right).
//   - kTwoPaneVertical:   Two panes stacked (top/bottom).
//   - kThreePaneHorizontal: Three panes side by side (left/middle/right).
//   - kThreePaneVertical:   Three panes stacked (top/middle/bottom).
//   - kGridTwoByTwo:  2x2 grid of four panes.
//   - kGridThreeByTwo: 3x2 grid of six panes.
//   - kPictureInPicture: Small floating pane over a main content pane.
//   - kTabShift: One main pane with a narrow tab-shift sidebar pane.
enum class AstraSplitLayoutMode {
  kTwoPaneHorizontal,
  kTwoPaneVertical,
  kThreePaneHorizontal,
  kThreePaneVertical,
  kGridTwoByTwo,
  kGridThreeByTwo,
  kPictureInPicture,
  kTabShift,
};

// =========================================================================
// AstraSplitPaneId
// =========================================================================
//
// Identifies a pane within the split view.  Pane indices are 0-based and
// follow row-major order within the layout grid.
//
// For 2-pane layouts:
//   kPane0 = primary (left or top)
//   kPane1 = secondary (right or bottom)
//
// For 3-pane layouts:
//   kPane0 = left/top, kPane1 = middle, kPane2 = right/bottom
//
// For 2x2 grid:
//   kPane0 = top-left, kPane1 = top-right
//   kPane2 = bottom-left, kPane3 = bottom-right
//
// For 3x2 grid:
//   kPane0 = top-left, kPane1 = top-middle, kPane2 = top-right
//   kPane3 = bottom-left, kPane4 = bottom-middle, kPane5 = bottom-right
enum class AstraSplitPaneId {
  kPane0 = 0,
  kPane1 = 1,
  kPane2 = 2,
  kPane3 = 3,
  kPane4 = 4,
  kPane5 = 5,
};

// =========================================================================
// AstraSplitDividerId
// =========================================================================
//
// Identifies a divider within the split view.  Dividers are numbered
// sequentially based on the layout mode.
//
// For 2-pane layouts: single divider (kDivider0).
// For 3-pane layouts: two dividers (kDivider0, kDivider1).
// For grid layouts: one set of vertical dividers and one set of horizontal.
enum class AstraSplitDividerId {
  kDivider0 = 0,
  kDivider1 = 1,
  kDivider2 = 2,
  kDivider3 = 3,
  kDivider4 = 4,
};

// =========================================================================
// AstraSplitResizeBehavior
// =========================================================================
//
// Controls how panes resize when the overall split view size changes.
enum class AstraSplitResizeBehavior {
  kFixedRatio,       // Pane ratios stay constant.
  kFixedPixelSize,   // Primary pane keeps its pixel size; secondary absorbs.
  kMinSizePriority,  // Each pane stays above its minimum; ratios adjust.
};

// =========================================================================
// AstraSplitViewModel::Observer
// =========================================================================
//
// Observer interface for AstraSplitViewModel state changes.
// Implemented by views and controllers that need to react to model changes.
//
// All observer methods have empty default implementations so that
// observers can override only the methods they care about.
//
// Follows the Model-View-Controller pattern: the model is the source of
// truth, and the view reflects model state via observer notifications.
class AstraSplitViewModelObserver : public base::CheckedObserver {
 public:
  // Called when the layout mode changes (e.g., from 2-pane to 3-pane).
  virtual void OnSplitLayoutModeChanged(AstraSplitLayoutMode mode) {}

  // Called when a divider position changes (ratio between panes changes).
  // |divider_id| identifies which divider moved; |new_ratio| is the
  // fraction of space allocated to the pane(s) before the divider.
  virtual void OnSplitDividerPositionChanged(AstraSplitDividerId divider_id,
                                              double new_ratio) {}

  // Called continuously during a divider drag.
  // Observers should avoid heavy work in this callback.
  virtual void OnSplitDividerPositionChanging(AstraSplitDividerId divider_id,
                                               double new_ratio) {}

  // Called when a divider drag starts.
  virtual void OnSplitDividerDragStarted(AstraSplitDividerId divider_id) {}

  // Called when a divider drag ends.
  virtual void OnSplitDividerDragEnded(AstraSplitDividerId divider_id) {}

  // Called when the focused pane changes.
  virtual void OnSplitFocusedPaneChanged(AstraSplitPaneId pane_id) {}

  // Called when a pane is closed (removed from the layout).
  virtual void OnSplitPaneClosed(AstraSplitPaneId pane_id) {}

  // Called when panes are swapped.
  virtual void OnSplitPanesSwapped(AstraSplitPaneId pane_a,
                                   AstraSplitPaneId pane_b) {}

  // Called when a pane's minimum size changes.
  virtual void OnSplitMinPaneSizeChanged(int min_size_dips) {}

  // Called when the resize behavior changes.
  virtual void OnSplitResizeBehaviorChanged(
      AstraSplitResizeBehavior behavior) {}

  // Called when pane header visibility changes.
  virtual void OnSplitPaneHeadersVisibilityChanged(bool visible) {}

  // Called when pane toolbar visibility changes.
  virtual void OnSplitPaneToolbarsVisibilityChanged(bool visible) {}

  // Called when the workspace association changes.
  virtual void OnSplitWorkspaceIdChanged(const std::string& workspace_id) {}

  // Called when a named layout preset is saved.
  virtual void OnSplitLayoutPresetSaved(const std::string& preset_name) {}

  // Called when a named layout preset is loaded/applied.
  virtual void OnSplitLayoutPresetLoaded(const std::string& preset_name) {}

  // Called when a tab is converted into a split pane (tab-to-split).
  virtual void OnSplitTabToSplitConverted(AstraSplitPaneId pane_id) {}

  // Called when a split pane is converted back to a regular tab (split-to-tab).
  virtual void OnSplitSplitToTabConverted(AstraSplitPaneId pane_id) {}

  // Called when snap-to-points behavior is toggled.
  virtual void OnSplitSnapEnabledChanged(bool enabled) {}

  // Called when a pane is maximized (takes nearly all available space).
  virtual void OnSplitPaneMaximized(AstraSplitPaneId pane_id) {}

  // Called when a pane is restored from maximized state.
  virtual void OnSplitPaneRestored(AstraSplitPaneId pane_id) {}

  // Called when the model is about to be destroyed.
  virtual void OnSplitViewModelDestroyed() {}

 protected:
  ~AstraSplitViewModelObserver() override = default;
};

// =========================================================================
// AstraSplitViewModel
// =========================================================================
//
// Model for Astra split view state.  Owns all split view configuration
// state and notifies observers of changes.
//
// This is a pure model class — it knows nothing about views or rendering.
// It holds state, validates state changes, and notifies observers.
//
// State managed:
//   - Layout mode (2-pane horizontal, 2-pane vertical, 3-pane, grid)
//   - Divider positions (ratios between panes)
//   - Focused pane
//   - Minimum pane size
//   - Resize behavior
//   - Drag state (which divider is being dragged, if any)
//
// Truth: This model is the source of truth for split view layout state.
// The view reads from the model and reflects state changes via observers.
// The controller mediates user actions into model mutations.
//
// TODO(astra): Consider using base::RepeatingTimer or base::TimeDelta for
//   animation-related state if smooth divider animations are added.
//   Chromium owner: ui/gfx/animation/tween.h
class AstraSplitViewModel {
 public:
  // Default number of panes and dividers for 2-pane mode.
  static constexpr int kDefaultPaneCount = 2;
  static constexpr int kDefaultDividerCount = 1;

  // Minimum pane size in DIPs.
  static constexpr int kDefaultMinPaneSize = 100;

  // Minimum ratio for any pane (as a fraction of total space).
  static constexpr double kMinPaneRatio = 0.1;

  explicit AstraSplitViewModel(AstraSplitLayoutMode mode =
                                   AstraSplitLayoutMode::kTwoPaneHorizontal);
  ~AstraSplitViewModel();

  AstraSplitViewModel(const AstraSplitViewModel&) = delete;
  AstraSplitViewModel& operator=(const AstraSplitViewModel&) = delete;

  // -- Observer management ------------------------------------------------

  void AddObserver(AstraSplitViewModelObserver* observer);
  void RemoveObserver(AstraSplitViewModelObserver* observer);

  // -- Layout mode --------------------------------------------------------

  // Set the layout mode.  Resets all divider positions to equal ratios.
  // Notifies observers of layout mode change.
  void SetLayoutMode(AstraSplitLayoutMode mode);
  AstraSplitLayoutMode layout_mode() const { return layout_mode_; }

  // Get the number of panes in the current layout mode.
  int GetPaneCount() const;

  // Get the number of dividers in the current layout mode.
  int GetDividerCount() const;

  // Returns true if the current layout is grid-based (rows + columns).
  bool IsGridLayout() const;

  // Returns the number of columns in the current layout.
  int GetColumnCount() const;

  // Returns the number of rows in the current layout.
  int GetRowCount() const;

  // -- Divider positions --------------------------------------------------

  // Set the position of a divider as a ratio [0.0, 1.0].
  // The ratio represents the fraction of space allocated to the panes
  // before this divider.  The ratio is clamped to valid range.
  // Notifies observers.
  void SetDividerRatio(AstraSplitDividerId divider_id, double ratio);

  // Get the current ratio of a divider.
  double GetDividerRatio(AstraSplitDividerId divider_id) const;

  // Set all dividers to equal spacing.
  void SetEqualRatios();

  // Reset all dividers to their default positions.
  void ResetToDefaults();

  // -- Drag state ---------------------------------------------------------

  // Start a drag on the specified divider.
  void StartDividerDrag(AstraSplitDividerId divider_id);

  // Update divider position during a drag.  Notifies "changing" observers.
  void UpdateDividerDrag(AstraSplitDividerId divider_id, double ratio);

  // End a drag on the specified divider.  Notifies "changed" observers.
  void EndDividerDrag(AstraSplitDividerId divider_id);

  // Returns true if any divider is currently being dragged.
  bool IsDragging() const;

  // Returns the divider being dragged, or nullopt if no drag in progress.
  absl::optional<AstraSplitDividerId> GetDraggedDivider() const;

  // -- Focused pane -------------------------------------------------------

  // Set which pane has focus.
  void SetFocusedPane(AstraSplitPaneId pane_id);
  AstraSplitPaneId focused_pane() const { return focused_pane_; }

  // Move focus to the next pane (wraps around).
  void FocusNextPane();

  // Move focus to the previous pane (wraps around).
  void FocusPreviousPane();

  // -- Pane operations ----------------------------------------------------

  // Close (remove) a pane.  If the layout has only 2 panes, closing one
  // deactivates split view conceptually, but the model still tracks one
  // remaining pane.  Returns false if the pane cannot be closed.
  bool ClosePane(AstraSplitPaneId pane_id);

  // Swap two panes.  The dividers stay in place; only the pane contents
  // conceptually swap positions.  Notifies observers.
  void SwapPanes(AstraSplitPaneId pane_a, AstraSplitPaneId pane_b);

  // Swap the focused pane with the next pane.
  void SwapFocusedWithNext();

  // Swap the focused pane with the previous pane.
  void SwapFocusedWithPrevious();

  // -- Layout configuration ----------------------------------------------

  // Set the minimum pane size in DIPs.
  void SetMinPaneSize(int min_size_dips);
  int min_pane_size() const { return min_pane_size_; }

  // Set the resize behavior.
  void SetResizeBehavior(AstraSplitResizeBehavior behavior);
  AstraSplitResizeBehavior resize_behavior() const {
    return resize_behavior_;
  }

  // -- Orientation helpers (for 2-pane compatibility) --------------------

  // Toggle between horizontal and vertical (for 2-pane mode).
  // In grid or 3-pane modes, this toggles the primary axis.
  void ToggleOrientation();

  // Returns true if the primary split axis is horizontal (side-by-side).
  bool IsHorizontal() const;

  // -- Validation ---------------------------------------------------------

  // Returns true if the pane ID is valid for the current layout.
  bool IsValidPaneId(AstraSplitPaneId pane_id) const;

  // Returns true if the divider ID is valid for the current layout.
  bool IsValidDividerId(AstraSplitDividerId divider_id) const;

  // Clamp a divider ratio to valid range for the current layout.
  double ClampRatio(double ratio) const;

  // -- State serialization (for persistence) ------------------------------

  // Serialize the model state to a string (for prefs/storage).
  std::string SerializeToString() const;

  // Deserialize state from a string.  Returns true on success.
  bool DeserializeFromString(const std::string& state);

  // -- Pane headers -------------------------------------------------------

  // Set whether pane header bars are shown.
  // Header bars contain the tab title and a close button.
  void SetShowPaneHeaders(bool show);
  bool show_pane_headers() const { return show_pane_headers_; }

  // -- Pane toolbars ------------------------------------------------------

  // Set whether per-pane toolbars are shown (back/forward/reload buttons).
  void SetShowPaneToolbars(bool show);
  bool show_pane_toolbars() const { return show_pane_toolbars_; }

  // -- Workspace association ----------------------------------------------

  // Associate this split layout with a workspace ID.
  // Workspace association allows split state to be saved/restored per workspace.
  void SetWorkspaceId(const std::string& workspace_id);
  const std::string& workspace_id() const { return workspace_id_; }

  // Returns true if this split layout is associated with a workspace.
  bool HasWorkspaceAssociation() const;

  // -- Saved layout presets -----------------------------------------------

  // Save the current layout state as a named preset.
  // Presets are stored in memory and can be applied later.
  // TODO(astra): Persist presets to PrefService for cross-session storage.
  //   Chromium owner: PrefService (components/prefs/pref_service.h)
  void SaveLayoutPreset(const std::string& name);

  // Load and apply a named preset.  Returns true if the preset was found.
  bool LoadLayoutPreset(const std::string& name);

  // Delete a saved preset.  Returns true if the preset existed.
  bool DeleteLayoutPreset(const std::string& name);

  // Returns true if a preset with the given name exists.
  bool HasLayoutPreset(const std::string& name) const;

  // Get a list of all saved preset names.
  std::vector<std::string> GetLayoutPresetNames() const;

  // Number of built-in and saved presets.
  size_t GetPresetCount() const;

  // -- Snap points --------------------------------------------------------

  // Enable or disable snap-to-points during divider drag.
  void SetSnapEnabled(bool enabled);
  bool snap_enabled() const { return snap_enabled_; }

  // Get the list of snap points (as ratios).
  // Default snap points: 0.25, 0.333, 0.5, 0.667, 0.75.
  const std::vector<double>& snap_points() const { return snap_points_; }

  // Set custom snap points.  Each value is a ratio [0.0, 1.0].
  void SetSnapPoints(const std::vector<double>& points);

  // Reset snap points to the default set.
  void ResetSnapPointsToDefaults();

  // Snap a ratio to the nearest snap point if within |tolerance|.
  // Returns the snapped ratio, or the original if no snap point is close enough.
  double SnapToNearestPoint(double ratio, double tolerance = 0.02) const;

  // -- Pane maximize / restore --------------------------------------------

  // Maximize a specific pane (reduces other panes to minimum size).
  void MaximizePane(AstraSplitPaneId pane_id);

  // Restore from maximized state back to the previous layout.
  void RestoreFromMaximized();

  // Returns true if any pane is currently maximized.
  bool IsPaneMaximized() const;

  // Returns the maximized pane, or nullopt if no pane is maximized.
  absl::optional<AstraSplitPaneId> GetMaximizedPane() const;

  // Toggle maximize state for the focused pane.
  void ToggleFocusedPaneMaximized();

  // -- Tab-to-split / split-to-tab conversion -----------------------------

  // Notify that a regular tab has been converted into a split pane.
  // This is a semantic notification — the actual tab manipulation is
  // handled by the controller via TabStripModel.
  void NotifyTabToSplitConverted(AstraSplitPaneId pane_id);

  // Notify that a split pane has been converted back to a regular tab.
  void NotifySplitToTabConverted(AstraSplitPaneId pane_id);

  // -- Cycle helpers ------------------------------------------------------

  // Cycle to the next layout mode.
  void CycleNextLayoutMode();

  // Cycle to the previous layout mode.
  void CyclePreviousLayoutMode();

 private:
  // Initialize divider ratios based on the current layout mode.
  void InitializeDividers();

  // Get the number of dividers for a given layout mode.
  static int DividerCountForMode(AstraSplitLayoutMode mode);

  // Get the number of panes for a given layout mode.
  static int PaneCountForMode(AstraSplitLayoutMode mode);

  // Validate and clamp a divider index, returning the clamped index.
  int ValidateDividerIndex(int index) const;

  // Validate and clamp a pane index, returning the clamped index.
  int ValidatePaneIndex(int index) const;

  // Get the divider ratio at a specific index.
  double GetRatioAtIndex(int index) const;

  // Set the divider ratio at a specific index.
  void SetRatioAtIndex(int index, double ratio);

  // Notify observers that the layout mode changed.
  void NotifyLayoutModeChanged();

  // Notify observers that a divider position changed (settled).
  void NotifyDividerPositionChanged(AstraSplitDividerId divider_id);

  // Notify observers that a divider position is changing (during drag).
  void NotifyDividerPositionChanging(AstraSplitDividerId divider_id);

  // Notify observers that a divider drag started.
  void NotifyDividerDragStarted(AstraSplitDividerId divider_id);

  // Notify observers that a divider drag ended.
  void NotifyDividerDragEnded(AstraSplitDividerId divider_id);

  // Notify observers that the focused pane changed.
  void NotifyFocusedPaneChanged();

  // Notify observers that a pane was closed.
  void NotifyPaneClosed(AstraSplitPaneId pane_id);

  // Notify observers that panes were swapped.
  void NotifyPanesSwapped(AstraSplitPaneId pane_a, AstraSplitPaneId pane_b);

  // Notify observers that the minimum pane size changed.
  void NotifyMinPaneSizeChanged();

  // Notify observers that the resize behavior changed.
  void NotifyResizeBehaviorChanged();

  // Notify observers that pane header visibility changed.
  void NotifyPaneHeadersVisibilityChanged();

  // Notify observers that pane toolbar visibility changed.
  void NotifyPaneToolbarsVisibilityChanged();

  // Notify observers that the workspace ID changed.
  void NotifyWorkspaceIdChanged();

  // Notify observers that a layout preset was saved.
  void NotifyLayoutPresetSaved(const std::string& name);

  // Notify observers that a layout preset was loaded.
  void NotifyLayoutPresetLoaded(const std::string& name);

  // Notify observers that snap-to-points was toggled.
  void NotifySnapEnabledChanged();

  // Notify observers that a pane was maximized.
  void NotifyPaneMaximized(AstraSplitPaneId pane_id);

  // Notify observers that a pane was restored from maximized.
  void NotifyPaneRestored(AstraSplitPaneId pane_id);

  // Notify all observers that the model is being destroyed.
  void NotifyDestroyed();

  AstraSplitLayoutMode layout_mode_;

  // Divider ratios.  Each ratio is the fraction of space before that divider.
  // For 2-pane: one divider at e.g. 0.5.
  // For 3-pane: two dividers at e.g. 0.33 and 0.67.
  // For grid: first N-1 are column dividers, rest are row dividers.
  std::vector<double> divider_ratios_;

  // Currently focused pane.
  AstraSplitPaneId focused_pane_ = AstraSplitPaneId::kPane0;

  // Minimum pane size in DIPs.
  int min_pane_size_ = kDefaultMinPaneSize;

  // Resize behavior.
  AstraSplitResizeBehavior resize_behavior_ =
      AstraSplitResizeBehavior::kFixedRatio;

  // Drag state.
  bool is_dragging_ = false;
  int dragged_divider_index_ = -1;

  // Whether pane header bars are shown.
  bool show_pane_headers_ = false;

  // Whether per-pane toolbars are shown.
  bool show_pane_toolbars_ = false;

  // Workspace association ID (empty if not associated).
  std::string workspace_id_;

  // Saved layout presets: name -> serialized state string.
  std::map<std::string, std::string> saved_presets_;

  // Whether divider snap points are enabled.
  bool snap_enabled_ = false;

  // List of snap point ratios.
  std::vector<double> snap_points_;

  // Maximized pane state.
  bool is_pane_maximized_ = false;
  AstraSplitPaneId maximized_pane_ = AstraSplitPaneId::kPane0;

  // Saved divider ratios before maximization (for restore).
  std::vector<double> pre_maximize_ratios_;

  // Observers.
  base::ObserverList<AstraSplitViewModelObserver> observers_;
};

// =========================================================================
// Helper functions
// =========================================================================

// Convert a layout mode to a string (for prefs/storage).
std::string AstraSplitLayoutModeToString(AstraSplitLayoutMode mode);

// Parse a string to a layout mode.
AstraSplitLayoutMode AstraSplitLayoutModeFromString(const std::string& value);

// Convert a resize behavior to a string.
std::string AstraSplitResizeBehaviorToString(AstraSplitResizeBehavior behavior);

// Parse a string to a resize behavior.
AstraSplitResizeBehavior AstraSplitResizeBehaviorFromString(
    const std::string& value);

// Convert a pane ID to a string.
std::string AstraSplitPaneIdToString(AstraSplitPaneId pane_id);

// Parse a string to a pane ID.
AstraSplitPaneId AstraSplitPaneIdFromString(const std::string& value);

// Returns a human-readable name for a layout mode.
std::u16string AstraSplitLayoutModeToName(AstraSplitLayoutMode mode);

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_VIEW_MODEL_H_
