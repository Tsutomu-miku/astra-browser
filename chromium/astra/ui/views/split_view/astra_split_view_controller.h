#ifndef ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_VIEW_CONTROLLER_H_
#define ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_VIEW_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/ui/views/split_view/astra_split_view.h"
#include "astra/ui/views/split_view/astra_split_view_model.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

namespace content {
class WebContents;
}

namespace ui {
class KeyEvent;
}  // namespace ui

namespace views {
class View;
}  // namespace views

class BrowserView;
class Profile;

namespace astra {

class AstraSplitViewController;

// =========================================================================
// AstraSplitViewObserver
// =========================================================================
//
// Observer interface for AstraSplitViewController state changes.
// Implemented by UI components that need to react to split view state
// (e.g., toolbar split view button, sidebar items, status indicators).
//
// All observer methods have empty default implementations so that
// observers can override only the methods they care about.
//
// This follows the Astra naming convention and provides the deepened
// observer API with 9 event types.
class AstraSplitViewObserver : public base::CheckedObserver {
 public:
  // Called when split view is activated (shown).
  virtual void OnSplitViewActivated(AstraSplitViewController* controller) {}

  // Called when split view is deactivated (hidden).
  virtual void OnSplitViewDeactivated(AstraSplitViewController* controller) {}

  // Called when the split ratio changes.
  virtual void OnSplitRatioChanged(AstraSplitViewController* controller,
                                   double new_ratio) {}

  // Called when the split orientation changes.
  virtual void OnSplitOrientationChanged(AstraSplitViewController* controller,
                                         AstraSplitOrientation orientation) {}

  // Called when the primary pane's tab changes.
  virtual void OnPrimaryPaneChanged(AstraSplitViewController* controller,
                                    int tab_index) {}

  // Called when the secondary pane's tab changes.
  virtual void OnSecondaryPaneChanged(AstraSplitViewController* controller,
                                      int tab_index) {}

  // Called when the primary and secondary panes are swapped.
  virtual void OnPaneSwapped(AstraSplitViewController* controller) {}

  // Called when the focused pane changes.
  virtual void OnFocusedPaneChanged(AstraSplitViewController* controller,
                                    AstraSplitPane pane) {}

  // Called when the split view controller is about to be destroyed.
  virtual void OnSplitViewControllerShutdown(AstraSplitViewController* controller) {}

 protected:
  ~AstraSplitViewObserver() override = default;
};

// =========================================================================
// AstraSplitViewController
// =========================================================================

// AstraSplitViewController manages split view presentation state for a browser
// window.  It is NOT a View subclass — it is a controller object that owns
// and configures an AstraSplitView, arranges WebContents views within the
// content area, and synchronizes state with AstraTabFeatures metadata.
//
// This follows the Model-View-Controller pattern:
//   - Model: AstraTabFeatures metadata on WebContents (source of truth).
//   - View: AstraSplitView (pure presentation, no logic).
//   - Controller: AstraSplitViewController (coordinates between model and view).
//
// Ownership boundary (CRITICAL):
//   - Chromium owns all WebContents, WebContentsView, and the contents
//     container within BrowserView.
//   - AstraSplitViewController arranges the *view* representations of
//     WebContents inside an AstraSplitView that is inserted into the
//     content area.
//   - This controller never creates, destroys, or reparents WebContents.
//   - It only reparents *views* within the Views hierarchy, and only within
//     the content area subtree that Chromium delegates to Astra.
//
// State model:
//   - Split view state (active, ratio, orientation, partner) is stored as
//     AstraTabFeatures metadata on each participating WebContents.
//   - The controller reads initial state from the active tab's AstraTabFeatures
//     and writes state changes back to both tabs' metadata.
//   - This follows the "metadata on Chromium objects" pattern: WebContents
//     is the source of truth for Astra per-tab metadata via WebContentsUserData.
//
// Persistence:
//   - Per-tab split view state (partner, ratio, orientation) travels with
//     tabs through session restore via AstraTabFeatures (WebContentsUserData).
//   - Global split view settings (default orientation, default ratio,
//     snap-to-preset, divider visibility) persist via PrefService.
//   - Settings are profile-scoped; each profile has its own split view defaults.
//
// Observer pattern:
//   - Observers are notified when split view state changes (shown, hidden,
//     ratio changed, orientation changed, swapped, etc.).
//   - UI components that need to react to split view state (e.g., toolbar
//     buttons, sidebar items) should observe the controller rather than
//     the view or the WebContents metadata directly.
//
// TODO(astra): The current implementation assumes a simple "active tab + next
//   tab" split.  The full implementation should use TabStripModel to find the
//   partner tab by index or by AstraTabFeatures partner_id, and handle cases
//   where the partner tab has been closed or moved.  Chromium subsystem to
//   integrate: TabStripModelObserver (chrome/browser/ui/tabs/tab_strip_model.h).
//
// TODO(astra): Implement TabStripModelObserver integration.  The controller
//   should observe TabStripModel to handle tab closure, tab reordering, and
//   active tab changes while split view is active.  When the partner tab is
//   closed, split view should deactivate and show only the remaining tab.
//   Chromium owner: TabStripModelObserver
//   (chrome/browser/ui/tabs/tab_strip_model_observer.h).
//   Patch point: No patch needed — TabStripModelObserver is a public interface
//   that can be implemented by any class.  The controller just needs to add
//   itself as an observer when created.
class AstraSplitViewController : public AstraSplitView::Observer {
 public:
  // Observer interface for split view state changes.
  // Implemented by UI components that need to react to split view state
  // (e.g., toolbar split view button, sidebar items, status indicators).
  //
  // All observer methods have empty default implementations so that
  // observers can override only the methods they care about.
  class Observer : public base::CheckedObserver {
   public:
    // Called when split view is shown (activated).
    // |primary| and |secondary| are the two WebContents in the split.
    virtual void OnSplitViewShown(content::WebContents* primary,
                                  content::WebContents* secondary) {}

    // Called when split view is hidden (deactivated).
    virtual void OnSplitViewHidden() {}

    // Called when the split ratio changes.
    // |ratio| is the new ratio (primary pane fraction).
    virtual void OnSplitRatioChanged(float ratio) {}

    // Called continuously during a ratio change (e.g. while dragging).
    // Observers should avoid heavy work in this callback.
    virtual void OnSplitRatioChanging(float ratio) {}

    // Called when the split orientation changes.
    virtual void OnSplitOrientationChanged(SplitViewOrientation orientation) {}

    // Called when the primary and secondary panes are swapped.
    virtual void OnSplitViewsSwapped() {}

    // Called when one side of the split view is replaced with a different
    // tab.  |is_primary| indicates which side was replaced.
    virtual void OnSplitTabReplaced(bool is_primary,
                                    content::WebContents* new_contents) {}

    // Called when split view settings change (e.g. default orientation,
    // snap-to-preset behavior).
    virtual void OnSplitViewSettingsChanged(
        const AstraSplitViewSettings& settings) {}

    // Called when a pane is maximized (takes nearly all the space).
    virtual void OnSplitViewMaximized(bool primary_maximized) {}

    // Called when split view returns from maximized to normal state.
    virtual void OnSplitViewUnmaximized() {}

    // Called when the split view controller is about to be destroyed.
    virtual void OnSplitViewControllerDestroyed() {}

   protected:
    ~Observer() override = default;
  };

  explicit AstraSplitViewController(BrowserView* browser_view);
  ~AstraSplitViewController() override;

  AstraSplitViewController(const AstraSplitViewController&) = delete;
  AstraSplitViewController& operator=(const AstraSplitViewController&) = delete;

  // -- Observer management ------------------------------------------------

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // -- Split view lifecycle -----------------------------------------------

  // Show split view with |primary| and |secondary| WebContents in the given
  // orientation.  Both WebContents must be valid and owned by TabStripModel.
  // Writes split view metadata to both tabs' AstraTabFeatures.
  void ShowSplitView(content::WebContents* primary,
                     content::WebContents* secondary,
                     SplitViewOrientation orientation);

  // Show split view using default settings from PrefService.
  // Uses the default orientation and default ratio from prefs.
  void ShowSplitViewWithDefaults(content::WebContents* primary,
                                 content::WebContents* secondary);

  // Hide split view and restore the normal single-tab content area.
  // Clears split view metadata from both tabs' AstraTabFeatures.
  void HideSplitView();

  // Toggle split view on/off for the given primary tab.
  // If split view is not active, shows it with the next tab as secondary.
  // If split view is active, hides it.
  // TODO(astra): Implement ToggleSplitView — needs TabStripModel to find
  //   the "next tab" to use as secondary.
  void ToggleSplitView(content::WebContents* primary,
                       SplitViewOrientation orientation);

  // -- Split view configuration -------------------------------------------

  // Set the split ratio (0.0 to 1.0, clamped to minimum pane size).
  // Writes the new ratio to both tabs' AstraTabFeatures.
  void SetSplitRatio(float ratio);

  // Set the split ratio to a named preset.
  void SetPresetRatio(SplitViewPreset preset);

  // Set the split orientation.  Writes to both tabs' AstraTabFeatures.
  void SetOrientation(SplitViewOrientation orientation);

  // Toggle between horizontal and vertical orientation.
  void ToggleOrientation();

  // Swap primary and secondary views.  Updates partner metadata on both tabs.
  void SwapViews();

  // Resize the primary pane by |delta| DIPs.
  // Positive delta makes the primary pane larger; negative makes it smaller.
  void ResizePrimaryPane(int delta);

  // Maximize the primary pane (secondary reduced to minimum size).
  void MaximizePrimaryPane();

  // Maximize the secondary pane (primary reduced to minimum size).
  void MaximizeSecondaryPane();

  // Reset from maximized state back to the previous ratio.
  void Unmaximize();

  // Returns true if a pane is currently maximized.
  bool IsMaximized() const;

  // Replace the primary tab with |new_contents|.  Updates metadata on
  // both the old and new tab.  The old tab remains in TabStripModel
  // (Chromium owns tab lifecycle); it is just removed from the split view.
  void ReplacePrimaryTab(content::WebContents* new_contents);

  // Replace the secondary tab with |new_contents|.
  void ReplaceSecondaryTab(content::WebContents* new_contents);

  // Apply a split view preset by name.  Presets are common configurations
  // like "code + docs", "reading + notes", etc.
  // TODO(astra): Define named presets beyond the ratio presets.
  void ApplyNamedPreset(const std::string& preset_name);

  // Returns true if split view is currently active.
  bool IsSplitViewActive() const;

  // -- Settings -----------------------------------------------------------

  // Get the current split view settings (read from PrefService and
  // supplemented with runtime state).
  AstraSplitViewSettings GetSettings() const;

  // Update split view settings and persist to PrefService.
  void UpdateSettings(const AstraSplitViewSettings& settings);

  // -- Accessors ----------------------------------------------------------

  float split_ratio() const { return split_ratio_; }
  SplitViewOrientation orientation() const { return orientation_; }

  // Accessor for the split view widget (may be null if not active).
  AstraSplitView* split_view() { return split_view_; }
  const AstraSplitView* split_view() const { return split_view_; }

  // Accessors for the two WebContents in split view (may be null if not active).
  content::WebContents* primary_web_contents() { return primary_web_contents_; }
  content::WebContents* secondary_web_contents() {
    return secondary_web_contents_;
  }

  Profile* profile();
  const Profile* profile() const;

  // ========================================================================
  // Extended API (Astra naming convention)
  // ========================================================================

  // -- Pref keys (public static constexpr) ---------------------------------
  //
  // These are the preference keys for all 12+ split view settings.
  // They follow the Chromium pref naming pattern and are stored in
  // PrefService at the profile level.

  static constexpr const char* kPrefDefaultOrientation =
      "astra.split_view.default_orientation";
  static constexpr const char* kPrefDefaultRatio =
      "astra.split_view.default_ratio";
  static constexpr const char* kPrefDefaultPreset =
      "astra.split_view.default_preset";
  static constexpr const char* kPrefRememberSplitState =
      "astra.split_view.remember_split_state";
  static constexpr const char* kPrefResizeMode =
      "astra.split_view.resize_mode";
  static constexpr const char* kPrefMinPaneSize =
      "astra.split_view.min_pane_size";
  static constexpr const char* kPrefDividerWidth =
      "astra.split_view.divider_width";
  static constexpr const char* kPrefShowDividerHandle =
      "astra.split_view.show_divider_handle";
  static constexpr const char* kPrefDoubleClickDividerResets =
      "astra.split_view.double_click_divider_resets";
  static constexpr const char* kPrefKeyboardResizeStep =
      "astra.split_view.keyboard_resize_step";
  static constexpr const char* kPrefShowPaneLabels =
      "astra.split_view.show_pane_labels";
  static constexpr const char* kPrefAutoEqualOnWindowResize =
      "astra.split_view.auto_equal_on_window_resize";

  // -- State management ---------------------------------------------------

  // Toggle split view on/off.
  // If activating, uses the default orientation and ratio from settings.
  void ToggleSplitView();

  // Activate split view with default settings.
  void ActivateSplitView();

  // Deactivate split view and return to single-tab view.
  void DeactivateSplitView();

  // Returns true if split view is currently active.
  bool IsActive() const;

  // -- Ratio management ---------------------------------------------------

  // Get the current split ratio (0.0 to 1.0).
  double GetSplitRatio() const;

  // Set the split ratio.  |ratio| is clamped to valid range.
  void SetSplitRatio(double ratio);

  // -- Orientation management ---------------------------------------------

  // Set the split orientation.
  void SetOrientation(AstraSplitOrientation orientation);

  // Get the current split orientation.
  AstraSplitOrientation GetOrientation() const;

  // Toggle between horizontal and vertical orientation.
  void ToggleOrientation();

  // -- Pane management ----------------------------------------------------

  // Set the tab in the primary pane by tab index.
  // TODO(astra): Wire to TabStripModel for real tab contents.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  void SetPrimaryTab(int tab_index);

  // Set the tab in the secondary pane by tab index.
  void SetSecondaryTab(int tab_index);

  // Get the index of the tab in the primary pane.
  // Returns -1 if no tab is set or split view is not active.
  int GetPrimaryTabIndex() const;

  // Get the index of the tab in the secondary pane.
  // Returns -1 if no tab is set or split view is not active.
  int GetSecondaryTabIndex() const;

  // Swap the primary and secondary panes.
  void SwapPanes();

  // Close the primary pane's tab and deactivate split view.
  // TODO(astra): Close via TabStripModel when integrated.
  void ClosePrimaryPane();

  // Close the secondary pane's tab and deactivate split view.
  void CloseSecondaryPane();

  // -- Focus management ---------------------------------------------------

  // Set focus to the primary pane.
  void FocusPrimaryPane();

  // Set focus to the secondary pane.
  void FocusSecondaryPane();

  // Toggle focus between the two panes.
  void ToggleFocus();

  // Get the currently focused pane.
  AstraSplitPane GetFocusedPane() const;

  // -- Layout presets -----------------------------------------------------

  // Apply a named layout preset (sets the ratio to the preset value).
  void SetLayoutPreset(AstraSplitPreset preset);

  // Get the currently active layout preset.
  // Returns the preset matching the current ratio, or kEqual if no match.
  AstraSplitPreset GetLayoutPreset() const;

  // Alias for SetLayoutPreset.
  void ApplyPreset(AstraSplitPreset preset);

  // -- Resize behavior ----------------------------------------------------

  // Set the resize mode (how the split view behaves on window resize).
  void SetResizeMode(AstraResizeMode mode);

  // Get the current resize mode.
  AstraResizeMode GetResizeMode() const;

  // -- Divider and pane size settings -------------------------------------

  // Set the minimum pane size in pixels.
  void SetMinPaneSize(int size_px);

  // Get the minimum pane size in pixels.
  int GetMinPaneSize() const;

  // Set the divider width in pixels.
  void SetDividerWidth(int width_px);

  // Get the divider width in pixels.
  int GetDividerWidth() const;

  // -- Extended observer management ---------------------------------------

  // Add an observer for split view state changes.
  void AddAstraObserver(AstraSplitViewObserver* observer);

  // Remove an observer.
  void RemoveAstraObserver(AstraSplitViewObserver* observer);

  // ========================================================================
  // Layout mode management
  // ========================================================================

  // Get the current layout mode.
  AstraSplitLayoutMode GetLayoutMode() const;

  // Set the layout mode (2-pane, 3-pane, grid, etc.).
  void SetLayoutMode(AstraSplitLayoutMode mode);

  // Cycle to the next layout mode.
  void CycleNextLayoutMode();

  // Cycle to the previous layout mode.
  void CyclePreviousLayoutMode();

  // ========================================================================
  // Keyboard shortcut handling
  // ========================================================================

  // Handle a keyboard shortcut for split view operations.
  // Returns true if the shortcut was handled.
  //
  // Supported shortcuts:
  //   - Ctrl/Cmd+Enter: Toggle split view
  //   - Ctrl/Cmd+Shift+Enter: Toggle orientation
  //   - Ctrl/Cmd+D: Swap panes
  //   - Ctrl/Cmd+1..6: Focus pane N
  //   - Ctrl/Cmd+Shift+1..6: Move current tab to pane N
  //   - Ctrl/Cmd+[ : Focus previous pane
  //   - Ctrl/Cmd+] : Focus next pane
  //   - Ctrl/Cmd+W: Close focused pane
  //
  // TODO(astra): Integrate with Chromium's accelerator system.
  //   Chromium owner: chrome/browser/ui/views/accelerator_table.cc
  //   Patch point: accelerator_table.cc merge with Astra accelerators.
  bool HandleKeyboardShortcut(const ui::KeyEvent& event);

  // ========================================================================
  // Tab drag and drop
  // ========================================================================

  // Handle a tab being dragged over a split pane.
  // |pane_id| identifies which pane the tab is being dragged over.
  // Returns true if the drop is acceptable.
  //
  // TODO(astra): Integrate with Chromium's tab drag-drop system.
  //   Chromium owner: chrome/browser/ui/views/tabs/tab_drag_controller.h
  //   This handles dropping a tab into a specific split pane.
  bool CanDropTabOnPane(AstraSplitPaneId pane_id,
                        content::WebContents* dragged_contents) const;

  // Handle a tab being dropped onto a split pane.
  // Replaces the pane's content with the dropped tab.
  void DropTabOnPane(AstraSplitPaneId pane_id,
                     content::WebContents* dropped_contents);

  // Handle a tab being dragged from a split pane to start a new split.
  // Returns true if a new split can be created from the drag.
  bool StartSplitFromDrag(AstraSplitPaneId source_pane_id,
                          content::WebContents* dragged_contents);

  // ========================================================================
  // Workspace sync
  // ========================================================================

  // Save the current split view state for the given workspace.
  // The state is stored in the model's workspace association and can be
  // restored later when switching back to this workspace.
  //
  // TODO(astra): Persist workspace split state to PrefService or
  //   AstraWorkspaceService.  Chromium owner: PrefService for profile-scoped
  //   persistence, or a dedicated workspace service.
  void SaveSplitStateForWorkspace(const std::string& workspace_id);

  // Restore split view state for the given workspace.
  // Returns true if state was found and restored.
  bool RestoreSplitStateForWorkspace(const std::string& workspace_id);

  // Clear saved split state for a workspace.
  void ClearWorkspaceState(const std::string& workspace_id);

  // Returns true if there is saved split state for the given workspace.
  bool HasWorkspaceState(const std::string& workspace_id) const;

  // ========================================================================
  // Focus cycling
  // ========================================================================

  // Cycle focus to the next pane (wraps around).
  // Bound to F6 by default.
  void CycleFocusNextPane();

  // Cycle focus to the previous pane.
  void CycleFocusPreviousPane();

  // Cycle focus between all panes (F6 behavior).
  // Returns the pane that now has focus.
  AstraSplitPaneId CycleFocus();

  // ========================================================================
  // History / back-forward per pane
  // ========================================================================

  // Navigate back in the focused pane.
  void GoBackInFocusedPane();

  // Navigate forward in the focused pane.
  void GoForwardInFocusedPane();

  // Reload the focused pane.
  void ReloadFocusedPane();

  // Navigate back in the specified pane.
  // TODO(astra): Wire to content::NavigationController.
  //   Chromium owner: content::WebContents::GetController()
  void GoBackInPane(AstraSplitPaneId pane_id);

  // Navigate forward in the specified pane.
  void GoForwardInPane(AstraSplitPaneId pane_id);

  // Reload the specified pane.
  void ReloadPane(AstraSplitPaneId pane_id);

  // ========================================================================
  // Tab-to-split / split-to-tab conversion
  // ========================================================================

  // Convert a regular tab into a split pane (add it to the split view).
  // Returns true if the conversion was successful.
  bool ConvertTabToSplit(content::WebContents* web_contents,
                          AstraSplitPaneId target_pane);

  // Convert a split pane back to a regular tab (remove from split view).
  // Returns true if the conversion was successful.
  bool ConvertSplitToTab(AstraSplitPaneId pane_id);

  // ========================================================================
  // Model access (for testing and advanced use cases)
  // ========================================================================

  // Access the underlying split view model.
  AstraSplitViewModel* model() { return &model_; }
  const AstraSplitViewModel* model() const { return &model_; }

  // -- AstraSplitView::Observer overrides --------------------------------

  void OnSplitRatioChanged(float ratio) override;
  void OnSplitRatioChanging(float ratio) override;
  void OnSplitOrientationChanged(SplitViewOrientation orientation) override;
  void OnSplitViewsSwapped() override;
  void OnSplitViewReplaced(bool is_primary) override;
  void OnSplitViewSettingsChanged(
      const AstraSplitViewSettings& settings) override;
  void OnSplitViewMaximized(bool primary_maximized) override;
  void OnSplitViewUnmaximized() override;
  void OnSplitViewDestroyed() override;

 private:
  // Obtain the native view (views::View*) that represents a WebContents.
  // This is the view we need to arrange in the split view.
  //
  // TODO(astra): The exact Chromium API for accessing the WebContents view
  // depends on the integration point.  Possible approaches:
  //   1. WebContents::GetNativeView() — returns a gfx::NativeView, which
  //      may be wrapped in a views::NativeViewHost on desktop.
  //   2. WebContentsView* view = web_contents->GetView(); then get the
  //      views::View* from the views-based WebContentsView implementation.
  //   3. browser_view->contents_web_view() — if the contents container
  //      is a views::WebView that holds the active WebContents.
  //
  // The correct approach depends on how BrowserView hosts WebContents.
  // In Chrome desktop, the contents container hosts a views::WebView or
  // directly embeds WebContentsView's view.
  //
  // Chromium owner: content::WebContentsView (content/public/browser/web_contents_view.h)
  // and chrome/browser/ui/views/frame/browser_view.cc contents_container().
  views::View* GetWebContentsView(content::WebContents* web_contents);

  // Insert the split view into the content area, replacing or overlaying
  // the normal single-tab contents view.
  void InstallSplitViewInContentsArea();

  // Remove the split view from the content area and restore the normal
  // single-tab content view.
  void UninstallSplitViewFromContentsArea();

  // Write the current split view state to both WebContents' AstraTabFeatures.
  void WriteMetadataToTabs();

  // Clear split view metadata from both tabs.
  void ClearMetadataFromTabs();

  // Generate a partner identifier string from a WebContents.
  // TODO(astra): Use base::Token or WebContents::GetController().GetWindowId()
  // or another stable identifier.  For now, use the WebContents pointer
  // value as a string, which is fine for within a single session.
  std::string GetPartnerId(content::WebContents* web_contents);

  // Find a suitable secondary tab for split view when toggling on.
  // Returns the next tab in the tab strip after |primary|, or nullptr if
  // no suitable secondary tab exists.
  // TODO(astra): Implement using TabStripModel.
  content::WebContents* FindSecondaryTab(content::WebContents* primary);

  // Load default settings from PrefService.
  void LoadSettingsFromPrefs(AstraSplitViewSettings* settings) const;

  // Save settings to PrefService.
  void SaveSettingsToPrefs(const AstraSplitViewSettings& settings);

  // Apply settings to the split view (if active).
  void ApplySettingsToView(const AstraSplitViewSettings& settings);

  // Called when a split view pref changes.
  void OnSplitViewPrefChanged(const std::string& pref_name);

  // -- Observer notifications --------------------------------------------

  void NotifySplitViewShown();
  void NotifySplitViewHidden();
  void NotifySplitRatioChanged();
  void NotifySplitRatioChanging();
  void NotifySplitOrientationChanged();
  void NotifySplitViewsSwapped();
  void NotifySplitTabReplaced(bool is_primary,
                              content::WebContents* new_contents);
  void NotifySplitViewSettingsChanged();
  void NotifySplitViewMaximized(bool primary_maximized);
  void NotifySplitViewUnmaximized();

  raw_ptr<BrowserView> browser_view_;

  // The split view widget (owned by the views hierarchy, not by us directly).
  // We hold a raw_ptr because we create it and add it to the BrowserView's
  // contents container, which owns all children.
  raw_ptr<AstraSplitView> split_view_ = nullptr;

  // The two WebContents in split view.  Both are owned by TabStripModel.
  raw_ptr<content::WebContents> primary_web_contents_ = nullptr;
  raw_ptr<content::WebContents> secondary_web_contents_ = nullptr;

  // Cached state (also stored in AstraTabFeatures metadata).
  float split_ratio_ = 0.5f;
  SplitViewOrientation orientation_ = SplitViewOrientation::kHorizontal;
  bool is_active_ = false;
  bool is_maximized_ = false;
  bool primary_maximized_ = true;

  // Cached settings (mirrored from PrefService).
  mutable AstraSplitViewSettings cached_settings_;

  // Pref change registrar to listen for setting changes.
  // Only used when a Profile/PrefService is available.
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  // Observers for split view state changes.
  base::ObserverList<Observer> observers_;

  // ========================================================================
  // Extended API private members
  // ========================================================================

  // -- Extended observer notifications -------------------------------------

  void NotifyAstraSplitViewActivated();
  void NotifyAstraSplitViewDeactivated();
  void NotifyAstraSplitRatioChanged();
  void NotifyAstraSplitOrientationChanged();
  void NotifyAstraPrimaryPaneChanged();
  void NotifyAstraSecondaryPaneChanged();
  void NotifyAstraPaneSwapped();
  void NotifyAstraFocusedPaneChanged();
  void NotifyAstraShutdown();

  // -- Extended state ------------------------------------------------------

  // Tab indices for primary and secondary panes.
  // -1 means no tab is assigned.
  // TODO(astra): These are presentation indices.  The source of truth for
  //   tab contents is TabStripModel.  These indices are cached hints used
  //   for UI presentation and should be synchronized via TabStripModelObserver.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  int primary_tab_index_ = -1;
  int secondary_tab_index_ = -1;

  // Currently focused pane.
  AstraSplitPane focused_pane_ = AstraSplitPane::kPrimary;

  // Current layout preset.
  AstraSplitPreset current_preset_ = AstraSplitPreset::kEqual;

  // Resize mode (how the split behaves on window resize).
  AstraResizeMode resize_mode_ = AstraResizeMode::kFixedRatio;

  // Minimum pane size in pixels.
  int min_pane_size_ = 100;

  // Divider width in pixels.
  int divider_width_ = 4;

  // Extended observers for the AstraSplitViewObserver interface.
  base::ObserverList<AstraSplitViewObserver> astra_observers_;

  // ========================================================================
  // Model
  // ========================================================================

  // The split view model that owns all layout state.
  // The model is the source of truth for split view state.
  // The controller mediates between the model and the view.
  AstraSplitViewModel model_;

  // Saved workspace split states: workspace_id -> serialized model state.
  std::map<std::string, std::string> workspace_states_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SPLIT_VIEW_ASTRA_SPLIT_VIEW_CONTROLLER_H_
