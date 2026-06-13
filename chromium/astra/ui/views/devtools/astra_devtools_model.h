#ifndef ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_MODEL_H_
#define ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"

class PrefService;

namespace astra {

// =========================================================================
// AstraDevToolsPanel — metadata describing an Astra DevTools panel
// =========================================================================
//
// Each panel has a stable ID, display title, icon identifier, and
// presentation properties (visibility, pin state, position index).
// Panels are ordered by their |position| field (lower = earlier in tab bar).
//
// Truth source: AstraDevToolsModel owns the panel list.  Panel metadata
// is presentation state — actual panel content and logic lives in
// individual panel views (e.g. AstraDevToolsWorkspacePanel).
// =========================================================================
struct AstraDevToolsPanel {
  // Stable identifier for the panel (e.g. "workspace", "notes").
  std::string id;

  // Display title shown in the panel tab.
  std::string title;

  // Icon identifier (name of vector icon or resource ID).
  // TODO(astra): Replace with gfx::VectorIcon reference once icon
  //   resources are added to the Astra resource bundle.
  std::string icon;

  // Whether the panel tab is visible in the DevTools tab strip.
  bool is_visible = true;

  // Whether the panel is pinned (shown before unpinned panels).
  // Pinned panels appear first in the tab order.
  bool is_pinned = false;

  // Position index in the tab strip (0-based).
  // Lower indices appear earlier (leftmost for horizontal, top for vertical).
  size_t position = 0;

  bool operator==(const AstraDevToolsPanel& other) const {
    return id == other.id && title == other.title && icon == other.icon &&
           is_visible == other.is_visible && is_pinned == other.is_pinned &&
           position == other.position;
  }
  bool operator!=(const AstraDevToolsPanel& other) const {
    return !(*this == other);
  }
};

// Dock position for the DevTools window.
// Follows Chromium's DevTools dock side pattern.
enum class AstraDevToolsDockPosition {
  kUndocked = 0,
  kBottom = 1,
  kLeft = 2,
  kRight = 3,
  kMaxValue = kRight,
};

// Theme mode for DevTools.
// Follows Chromium DevTools theme pattern (light/dark/system).
enum class AstraDevToolsTheme {
  kLight = 0,
  kDark = 1,
  kSystem = 2,
  kMaxValue = kSystem,
};

// Panel position — which side of DevTools the Astra panel sidebar appears on.
enum class AstraDevToolsPanelPosition {
  kLeft = 0,
  kRight = 1,
  kBottom = 2,
  kMaxValue = kBottom,
};

// =========================================================================
// AstraDevToolsModelObserver — observer interface for DevTools model
// =========================================================================
//
// All observer methods have empty default implementations so observers
// can override only the methods they care about.
//
// Observer pattern follows Chromium conventions:
//   - base::CheckedObserver base class
//   - base::ObserverList for management
//   - Empty default implementations in the observer class
// =========================================================================
class AstraDevToolsModelObserver : public base::CheckedObserver {
 public:
  // Called when a panel is opened (made active and visible).
  virtual void OnPanelOpened(const std::string& panel_id) {}

  // Called when a panel is closed (hidden if not the active panel).
  virtual void OnPanelClosed(const std::string& panel_id) {}

  // Called when the active (currently displayed) panel changes.
  // panel_id is the newly active panel ID.
  virtual void OnActivePanelChanged(const std::string& panel_id) {}

  // Called when panel ordering changes (panels reordered, added, removed).
  virtual void OnPanelOrderChanged() {}

  // Called when any DevTools presentation setting changes.
  virtual void OnDevToolsSettingsChanged() {}

  // Called when the dock position changes.
  virtual void OnDockPositionChanged(AstraDevToolsDockPosition position) {}

  // Called when the theme changes.
  virtual void OnThemeChanged(AstraDevToolsTheme theme) {}

 protected:
  ~AstraDevToolsModelObserver() override = default;
};

// =========================================================================
// AstraDevToolsModel — model for Astra DevTools panels and settings
// =========================================================================
//
// AstraDevToolsModel is the single source of truth for:
//   - Astra panel definitions and their ordering
//   - Active / visible panel state
//   - Presentation settings (persisted via PrefService)
//   - Dock position and theme preferences
//
// Views observe the model and update their display when the model changes.
// Views never store state — they always read from the model.
//
// Persistence:
//   Presentation settings are persisted via PrefService.  Panel state
//   (order, visibility, pin state) is also persisted.  The active panel
//   may be persisted if "remember last panel" is enabled.
//
// Chromium subsystems reused:
//   - PrefService — setting persistence
//   - base::ObserverList — observer pattern
//   - DevToolsWindow — owns the actual DevTools window lifecycle
//
// Truth source: This model owns all Astra DevTools presentation state.
// =========================================================================
class AstraDevToolsModel {
 public:
  // Constructs the model with a PrefService for persistence.
  // |pref_service| must outlive this model.
  explicit AstraDevToolsModel(PrefService* pref_service);
  ~AstraDevToolsModel();

  AstraDevToolsModel(const AstraDevToolsModel&) = delete;
  AstraDevToolsModel& operator=(const AstraDevToolsModel&) = delete;

  // -- Observer management -------------------------------------------------

  void AddObserver(AstraDevToolsModelObserver* observer);
  void RemoveObserver(AstraDevToolsModelObserver* observer);

  // -- Panel management ----------------------------------------------------

  // Returns all panels (including hidden ones), ordered by position.
  std::vector<AstraDevToolsPanel> GetAllPanels() const;

  // Returns only visible panels, ordered by position.
  std::vector<AstraDevToolsPanel> GetVisiblePanels() const;

  // Returns the panel with the given ID, or null if not found.
  const AstraDevToolsPanel* GetPanelById(const std::string& panel_id) const;

  // Returns true if a panel with the given ID exists.
  bool HasPanel(const std::string& panel_id) const;

  // Adds a new panel.  Returns false if a panel with the same ID exists.
  bool AddPanel(const AstraDevToolsPanel& panel);

  // Removes the panel with the given ID.  Returns false if not found.
  bool RemovePanel(const std::string& panel_id);

  // Sets the visibility of a panel.  Returns false if panel not found.
  bool SetPanelVisible(const std::string& panel_id, bool visible);

  // Sets the pinned state of a panel.  Returns false if panel not found.
  bool SetPanelPinned(const std::string& panel_id, bool pinned);

  // Reorders a panel to a new position index.  Returns false if not found.
  // The position is clamped to the valid range.
  bool ReorderPanel(const std::string& panel_id, size_t new_position);

  // Moves a panel one position earlier (toward index 0).
  bool MovePanelEarlier(const std::string& panel_id);

  // Moves a panel one position later (toward last index).
  bool MovePanelLater(const std::string& panel_id);

  // Resets all panels to their default definitions and ordering.
  void ResetPanelsToDefaults();

  // Returns the default panel list used for initial state and reset.
  static std::vector<AstraDevToolsPanel> GetDefaultPanels();

  // -- Active panel --------------------------------------------------------

  // Returns the ID of the currently active panel.
  const std::string& active_panel_id() const { return active_panel_id_; }

  // Sets the active panel by ID.  The panel must exist and be visible.
  // Returns false if the panel doesn't exist or isn't visible.
  bool SetActivePanel(const std::string& panel_id);

  // Activates the next visible panel (wraps around).
  void ActivateNextPanel();

  // Activates the previous visible panel (wraps around).
  void ActivatePreviousPanel();

  // -- Presentation settings -----------------------------------------------

  // Whether Astra panels are shown in DevTools at all.
  bool show_astra_panels() const;
  void SetShowAstraPanels(bool show);

  // Default active panel ID (used on first open).
  std::string default_active_panel() const;
  void SetDefaultActivePanel(const std::string& panel_id);

  // Position of the Astra panel sidebar relative to DevTools content.
  AstraDevToolsPanelPosition panel_position() const;
  void SetPanelPosition(AstraDevToolsPanelPosition position);

  // Whether to show icons on panel tabs.
  bool show_panel_icons() const;
  void SetShowPanelIcons(bool show);

  // Whether to show text labels on panel tabs.
  bool show_panel_labels() const;
  void SetShowPanelLabels(bool show);

  // Width of the panel sidebar in pixels (clamped).
  int panel_width() const;
  void SetPanelWidth(int width);

  // Minimum and maximum panel width for clamping.
  static constexpr int kMinPanelWidth = 150;
  static constexpr int kMaxPanelWidth = 500;
  static constexpr int kDefaultPanelWidth = 240;

  // Whether experimental Astra DevTools features are enabled.
  bool experiments_enabled() const;
  void SetExperimentsEnabled(bool enabled);

  // Whether the workspace panel auto-expands when DevTools opens.
  bool auto_expand_workspace_panel() const;
  void SetAutoExpandWorkspacePanel(bool auto_expand);

  // Whether the panel toolbar is shown.
  bool show_panel_toolbar() const;
  void SetShowPanelToolbar(bool show);

  // Whether compact panel mode is enabled (smaller tabs, less padding).
  bool compact_mode() const;
  void SetCompactMode(bool compact);

  // Whether to remember the last active panel across sessions.
  bool remember_last_panel() const;
  void SetRememberLastPanel(bool remember);

  // Last active panel ID (persisted if remember_last_panel is true).
  std::string last_active_panel() const;
  void SetLastActivePanel(const std::string& panel_id);

  // -- Dock position -------------------------------------------------------

  AstraDevToolsDockPosition dock_position() const { return dock_position_; }
  void SetDockPosition(AstraDevToolsDockPosition position);

  // Cycles through dock positions: undocked -> bottom -> left -> right -> undocked
  void CycleDockPosition();

  // -- Theme ---------------------------------------------------------------

  AstraDevToolsTheme theme() const { return theme_; }
  void SetTheme(AstraDevToolsTheme theme);

  // Returns the effective theme (resolves "system" to light/dark).
  // TODO(astra): Implement system theme detection using NativeTheme.
  //   Chromium owner: ui/native_theme/native_theme.h
  AstraDevToolsTheme GetEffectiveTheme() const;

  // -- Utility -------------------------------------------------------------

  // Returns the number of panels.
  size_t panel_count() const { return panels_.size(); }

  // Returns the number of visible panels.
  size_t visible_panel_count() const;

  // Returns true if there are no panels.
  bool empty() const { return panels_.empty(); }

  // Loads persisted state from PrefService.
  // Called automatically by the constructor, but exposed for testing.
  void LoadFromPrefs();

  // Saves current state to PrefService.
  // Called automatically when state changes, but exposed for testing.
  void SaveToPrefs() const;

 private:
  // Finds the iterator for a panel by ID.  Returns panels_.end() if not found.
  std::vector<AstraDevToolsPanel>::iterator FindPanel(
      const std::string& panel_id);
  std::vector<AstraDevToolsPanel>::const_iterator FindPanel(
      const std::string& panel_id) const;

  // Re-normalizes panel positions to be contiguous 0..N-1.
  // Called after add/remove/reorder to keep positions consistent.
  void RenormalizePositions();

  // Notifies observers of settings changes and persists to prefs.
  void NotifySettingsChanged();

  // Notifies observers of panel order changes and persists to prefs.
  void NotifyPanelOrderChanged();

  // The pref service for persistence.  Not owned.
  raw_ptr<PrefService> pref_service_;

  // List of all panels.  Kept sorted by position.
  std::vector<AstraDevToolsPanel> panels_;

  // Currently active panel ID.
  std::string active_panel_id_;

  // Dock position (in-memory, also persisted via PrefService).
  AstraDevToolsDockPosition dock_position_ =
      AstraDevToolsDockPosition::kBottom;

  // Theme (in-memory, also persisted via PrefService).
  AstraDevToolsTheme theme_ = AstraDevToolsTheme::kSystem;

  // Observers.
  base::ObserverList<AstraDevToolsModelObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_MODEL_H_
