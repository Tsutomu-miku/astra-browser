#ifndef ASTRA_BROWSER_ASTRA_PIP_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_PIP_SERVICE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
class WebContents;
}

namespace astra {

// =========================================================================
// AstraPipService — Astra Picture-in-Picture enhancement service
// =========================================================================
//
// Profile-scoped keyed service that extends Chromium's built-in PiP system
// with Astra-specific features: tab-level PiP (not just video), multi-PiP
// support, size presets, and sidebar integration.
//
// This service does NOT reimplement PiP. It builds on top of Chromium's
// PictureInPictureWindowController and adds Astra metadata, tracking, and
// convenience methods.  The actual PiP window ownership and rendering
// remain with Chromium.
//
// Truth model:
//   - Chromium owns the actual PiP window state (PictureInPictureWindowController).
//   - This service tracks which tabs are in PiP mode as Astra metadata.
//   - Per-tab PiP metadata (window size, etc.) lives on AstraTabFeatures.
//   - PiP preferences (default size, always-on-top) live in PrefService.
//
// Chromium subsystems reused:
//   - PictureInPictureWindowController (chrome/browser/picture_in_picture/)
//   - content::WebContents (tab content ownership)
//   - PrefService (preference persistence)
//   - views::Widget (window management, via Chromium's PiP window)
//   - TabStripModel (tab iteration and lookup)
//
// Chromium patch points:
//   - PictureInPictureWindowController — to hook into PiP enter/exit events
//     and add Astra controls overlay.
//   - chrome/browser/ui/views/picture_in_picture/ — to customize the PiP
//     window UI with Astra-specific controls.
// =========================================================================

class AstraPipObserver : public base::CheckedObserver {
 public:
  // Called when a tab enters PiP mode.
  // |web_contents| is the tab that was sent to PiP.
  virtual void OnPiPEntered(content::WebContents* web_contents) {}

  // Called when a tab exits PiP mode and is restored to the tab strip.
  virtual void OnPiPExited(content::WebContents* web_contents) {}

  // Called when the PiP window size changes for a tab.
  virtual void OnPiPWindowSizeChanged(content::WebContents* web_contents,
                                      const gfx::Size& new_size) {}

  // Called when the PiP window position changes for a tab.
  virtual void OnPiPWindowPositionChanged(content::WebContents* web_contents,
                                          const gfx::Point& new_position) {}

  // Called when the always-on-top preference changes.
  virtual void OnPiPAlwaysOnTopChanged(bool always_on_top) {}

  // Called when the default size preset preference changes.
  virtual void OnPiPDefaultSizePresetChanged(PipSizePreset preset) {}

  // Called when a PiP preference changes (generic notification).
  // Observers can query the service for the new values.
  virtual void OnPiPPreferencesChanged() {}

 protected:
  ~AstraPipObserver() override = default;
};

// PiP size presets.  These map to concrete pixel dimensions used when
// resizing a PiP window via quick presets.
enum class PipSizePreset {
  kSmall,   // Compact — roughly 240x160 (4:3) or 284x160 (16:9)
  kMedium,  // Default — roughly 360x240 (4:3) or 426x240 (16:9)
  kLarge,   // Large — roughly 480x320 (4:3) or 568x320 (16:9)
};

// PiP window snap positions.  When snap-to-corner is enabled, PiP windows
// snap to the nearest corner of the screen when dragged near the edge.
enum class PipSnapPosition {
  kTopLeft,
  kTopRight,
  kBottomLeft,
  kBottomRight,
  kFreeFloating,  // Not snapped — user can position freely
};

class AstraPipService final : public KeyedService {
 public:
  explicit AstraPipService(Profile* profile);
  AstraPipService(const AstraPipService&) = delete;
  AstraPipService& operator=(const AstraPipService&) = delete;
  ~AstraPipService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraPipObserver* observer);
  void RemoveObserver(AstraPipObserver* observer);

  // -- PiP state queries -------------------------------------------------

  // Returns true if |web_contents| is currently displayed as a PiP window.
  //
  // Truth is owned by Chromium's PictureInPictureWindowController; this
  // method checks the Astra metadata cache which is kept in sync via
  // observer hooks into Chromium's PiP controller.
  //
  // TODO(astra): Integrate with Chromium's PictureInPictureWindowController
  // to query actual PiP state instead of relying on Astra metadata.
  // Chromium owner: PictureInPictureWindowController
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
  // Patch point: Add an Astra hook to PiP window controller lifecycle.
  bool IsTabInPip(content::WebContents* web_contents) const;

  // Returns all tabs currently in PiP mode for this profile.
  //
  // Iterates over the tab strips of all browser windows for the profile
  // and filters for tabs that have PiP active.
  //
  // TODO(astra): Proper multi-window support.  Currently assumes a single
  // browser window per profile; multi-window PiP needs TabStripModel
  // iteration across all windows.
  // Chromium owner: BrowserList (chrome/browser/ui/browser_list.h)
  std::vector<content::WebContents*> GetPiPTabs() const;

  // -- PiP control -------------------------------------------------------

  // Sends |web_contents| to picture-in-picture mode.
  //
  // For video PiP, this uses Chromium's existing PictureInPictureWindowController
  // triggered via the media session API.  For tab-level PiP (showing an
  // entire tab as a PiP window), this either uses Chromium's tab PiP feature
  // or creates a mini widget showing the WebContents.
  //
  // TODO(astra): Proper integration with Chromium's PictureInPictureWindowController.
  // Currently this sets Astra metadata as a stand-in.  Real implementation
  // would call into the PiP controller to show the window.
  // Chromium owner: PictureInPictureWindowController
  // Patch point: None needed for entry — uses public API.
  void EnterPipForTab(content::WebContents* web_contents);

  // Restores |web_contents| from PiP mode back to its tab strip position.
  //
  // TODO(astra): Integrate with PictureInPictureWindowController::Close()
  // or the equivalent exit path.
  // Chromium owner: PictureInPictureWindowController::Close()
  void ExitPipForTab(content::WebContents* web_contents);

  // Toggles PiP state for |web_contents|.
  // Returns true if the tab is in PiP after the toggle.
  bool TogglePipForTab(content::WebContents* web_contents);

  // -- Window sizing -----------------------------------------------------

  // Resizes the PiP window for |web_contents| to |size|.
  //
  // The actual window resize is performed by Chromium's PiP window widget.
  // This method updates the Astra metadata and delegates to the controller.
  //
  // TODO(astra): Wire to PictureInPictureWindowController::UpdateLayerBounds
  // or the widget's SetBounds method.
  // Chromium owner: PictureInPictureWindowController / views::Widget
  void ResizePiPWindow(content::WebContents* web_contents,
                       const gfx::Size& size);

  // Returns the current size of the PiP window for |web_contents|.
  // Returns an empty size if the tab is not in PiP mode.
  gfx::Size GetPiPWindowSize(content::WebContents* web_contents) const;

  // Resizes the PiP window for |web_contents| to a preset size.
  // The actual pixel dimensions depend on the content's aspect ratio and
  // the default PiP size configuration.
  void ResizePiPToPreset(content::WebContents* web_contents,
                         PipSizePreset preset);

  // Returns the default size preset from preferences.
  PipSizePreset GetDefaultSizePreset() const;

  // Sets the default size preset and persists it to prefs.
  void SetDefaultSizePreset(PipSizePreset preset);

  // Returns whether PiP windows are always-on-top by default.
  bool GetAlwaysOnTop() const;

  // Sets the always-on-top preference.
  // Fires OnPiPAlwaysOnTopChanged and OnPiPPreferencesChanged if the value changed.
  void SetAlwaysOnTop(bool always_on_top);

  // Toggles the always-on-top state.  Returns the new state.
  bool ToggleAlwaysOnTop();

  // -- Window position -----------------------------------------------------

  // Returns the current position of the PiP window for |web_contents|.
  // Returns an empty point (0, 0) if the tab is not in PiP mode.
  //
  // Note: this is Astra metadata cached from the actual window position.
  // The real position is owned by Chromium's views::Widget for the PiP window.
  gfx::Point GetPiPWindowPosition(content::WebContents* web_contents) const;

  // Sets the position of the PiP window for |web_contents|.
  // Fires OnPiPWindowPositionChanged.
  //
  // TODO(astra): Wire to views::Widget::SetPosition() on the PiP window.
  // Chromium owner: views::Widget (ui/views/widget/widget.h)
  void SetPiPWindowPosition(content::WebContents* web_contents,
                            const gfx::Point& position);

  // -- Snap position -------------------------------------------------------

  // Returns the current snap position of the PiP window.
  // When snap-to-corner is enabled, PiP windows snap to screen corners.
  PipSnapPosition GetSnapPosition() const;

  // Sets the snap position preference.
  void SetSnapPosition(PipSnapPosition position);

  // Returns whether snap-to-corner is enabled.
  // When true, PiP windows automatically snap to the nearest screen corner.
  bool GetSnapToCornerEnabled() const;

  // Sets whether snap-to-corner is enabled.
  void SetSnapToCornerEnabled(bool enabled);

  // Returns the snap position as a human-readable string.
  static const char* SnapPositionToString(PipSnapPosition position);

  // Parses a snap position string to the enum value.
  // Returns kBottomRight as the default if unrecognized.
  static PipSnapPosition SnapPositionFromString(const std::string& name);

  // -- Opacity -------------------------------------------------------------

  // Returns the PiP window opacity in the range [0.2, 1.0].
  // Default: 1.0 (fully opaque).
  double GetPiPOpacity() const;

  // Sets the PiP window opacity.  Values outside [0.2, 1.0] are clamped.
  //
  // TODO(astra): Apply opacity to the PiP window widget via SetOpacity.
  // Chromium owner: views::Widget::SetOpacity()
  void SetPiPOpacity(double opacity);

  // -- Auto-PiP ------------------------------------------------------------

  // Returns whether auto-PiP on tab switch is enabled.
  // When enabled, switching away from a tab that is playing video
  // automatically sends it to PiP mode.
  bool GetAutoPipOnTabSwitch() const;

  // Sets whether auto-PiP on tab switch is enabled.
  void SetAutoPipOnTabSwitch(bool enabled);

  // -- Multi-PiP limit -----------------------------------------------------

  // Returns the maximum number of concurrent PiP windows allowed.
  // Default: 3.  0 means no limit.
  int GetMaxPipWindows() const;

  // Sets the maximum number of concurrent PiP windows.
  // If |max| is less than 0, it is clamped to 0 (no limit).
  void SetMaxPipWindows(int max);

  // Returns true if the current number of PiP tabs is at or above the limit.
  bool IsAtPipLimit() const;

  // -- Bulk operations -----------------------------------------------------

  // Exits all PiP windows for this profile.
  // Returns the number of tabs that were exited from PiP.
  // Fires OnPiPExited for each tab.
  size_t ExitAllPip();

  // -- Utility methods -----------------------------------------------------

  // Returns the number of tabs currently in PiP mode.
  size_t GetPipCount() const;

  // Cycles the PiP window size to the next preset (small → medium → large → small).
  // Returns the new preset.
  PipSizePreset CycleSizePreset(content::WebContents* web_contents);

  // -- Size preset helpers -----------------------------------------------

  // Returns a human-readable name for the size preset.
  static const char* PresetToString(PipSizePreset preset);

  // Parses a preset name string to a PipSizePreset enum value.
  // Returns kMedium as the default if the string is unrecognized.
  static PipSizePreset PresetFromString(const std::string& name);

  // Returns the pixel dimensions for a given preset, scaled to the given
  // aspect ratio.  |aspect_ratio| is width/height (e.g. 16.0/9.0).
  // The returned size has |height| as the reference dimension, with width
  // computed to maintain the aspect ratio.
  static gfx::Size GetPresetSize(PipSizePreset preset, float aspect_ratio);

 private:
  // Loads persisted preferences from the profile's PrefService.
  void LoadFromPrefs();

  // Saves current preferences to the profile's PrefService.
  void SaveToPrefs();

  // Notifies observers that a preference changed (both specific and generic).
  void NotifyPreferencesChanged();

  raw_ptr<Profile> profile_;
  base::ObserverList<AstraPipObserver> observers_;

  // Default PiP size preset (persisted preference).
  PipSizePreset default_size_preset_ = PipSizePreset::kMedium;

  // Whether PiP windows should be always-on-top (persisted preference).
  bool always_on_top_ = true;

  // Default snap position for new PiP windows (persisted preference).
  PipSnapPosition snap_position_ = PipSnapPosition::kBottomRight;

  // Whether snap-to-corner is enabled (persisted preference).
  bool snap_to_corner_enabled_ = true;

  // PiP window opacity in [0.2, 1.0] (persisted preference).
  double pip_opacity_ = 1.0;

  // Whether auto-PiP on tab switch is enabled (persisted preference).
  bool auto_pip_on_tab_switch_ = false;

  // Maximum number of concurrent PiP windows (persisted preference).
  // 0 means no limit.
  int max_pip_windows_ = 3;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_PIP_SERVICE_H_
