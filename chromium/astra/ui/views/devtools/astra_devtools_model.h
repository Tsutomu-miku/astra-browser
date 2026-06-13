#ifndef ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_MODEL_H_
#define ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_MODEL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "third_party/skia/include/core/SkColor.h"

class PrefService;

namespace astra {

// =========================================================================
// AstraDevToolsPanelType — type identifier for Astra DevTools panels
// =========================================================================
//
// Each Astra DevTools panel has a distinct type that identifies its purpose.
// Types are used for programmatic lookup and panel-specific behavior.
//
// Panels are ordered by their conceptual importance / frequency of use:
//   - Workspace panel is the primary Astra product panel.
//   - Tab Stack panel manages tab grouping.
//   - Notes panel is for note-taking alongside DevTools.
//   - Performance panel provides performance insights.
//   - Accessibility panel offers accessibility auditing tools.
//   - A11y Tree panel shows the accessibility tree view.
// =========================================================================
enum class AstraDevToolsPanelType {
  kWorkspacePanel = 0,
  kTabStackPanel,
  kNotesPanel,
  kPerformancePanel,
  kAccessibilityPanel,
  kA11yTreePanel,
};

// =========================================================================
// AstraDevToolsPanelInfo — rich metadata for an Astra DevTools panel
// =========================================================================
//
// Extended panel descriptor with type information, visibility, enabled state,
// ordering, and description.  This is the primary panel metadata struct used
// by the deepened DevTools integration.
//
// Truth source: AstraDevToolsModel owns the list of AstraDevToolsPanelInfo
// entries.  Views read from the model and never store panel state directly.
//
// Chromium subsystems reused:
//   - DevToolsWindow hosts all panels (native or WebUI).
//   - DevToolsUIBindings for extension-based panel registration.
//
// TODO(astra): Wire panel info to Chromium DevTools frontend for real
//   panel integration.  Chromium owner: chrome/browser/devtools/
//   devtools_window.h and chrome/browser/devtools/devtools_ui_bindings.h
// =========================================================================
struct AstraDevToolsPanelInfo {
  // Stable identifier for the panel (e.g. "workspace-panel").
  std::string panel_id;

  // Type of the panel (programmatic category).
  AstraDevToolsPanelType type = AstraDevToolsPanelType::kWorkspacePanel;

  // Human-readable display title shown in the panel tab.
  std::u16string title;

  // Icon identifier (name of vector icon or resource).
  // TODO(astra): Replace with gfx::VectorIcon reference once icon
  //   resources are added to the Astra resource bundle.
  //   Chromium owner: ui/gfx/vector_icon_*.h
  std::string icon_name;

  // Whether the panel is enabled (can be activated).
  // Disabled panels are still registered but cannot be shown.
  bool is_enabled = true;

  // Whether the panel tab is visible in the tab strip.
  // Invisible panels exist but are not shown in the UI.
  bool is_visible = true;

  // Order index in the tab strip (0-based, lower = earlier).
  int order_index = 0;

  // Whether this is a default (built-in) panel vs. user-added.
  bool is_default = false;

  // Human-readable description shown in settings or tooltip.
  std::u16string description;
};

// =========================================================================
// AstraDevToolsDockState — dock state for the DevTools window
// =========================================================================
//
// Extended dock state enum that includes the minimized state in addition
// to the standard docked and undocked states.
//
// This mirrors Chromium's DevTools dock side concept but adds the
// "minimized" state that some Chromium configurations support.
//
// Chromium owner: DevToolsWindow::DockSide
//   (chrome/browser/devtools/devtools_window.h)
// =========================================================================
enum class AstraDevToolsDockState {
  kDockedBottom = 0,
  kDockedLeft,
  kDockedRight,
  kUndocked,
  kMinimized,
};

// =========================================================================
// AstraDevToolsObserver — observer for DevTools model changes
// =========================================================================
//
// Observer interface for Astra DevTools model state changes.  All methods
// have empty default implementations so observers can override only the
// events they care about.
//
// Observer pattern follows Chromium conventions:
//   - base::CheckedObserver base class
//   - base::ObserverList for management
//   - Empty default implementations in the observer class
//
// The observer receives a pointer to the model so observers can query
// additional state without needing separate references.
// =========================================================================
class AstraDevToolsObserver : public base::CheckedObserver {
 public:
  // Called when DevTools is opened (becomes visible).
  virtual void OnDevToolsOpened(AstraDevToolsModel* model) {}

  // Called when DevTools is closed (becomes hidden).
  virtual void OnDevToolsClosed(AstraDevToolsModel* model) {}

  // Called when a panel is activated (made the active panel).
  virtual void OnPanelActivated(AstraDevToolsModel* model,
                                const std::string& panel_id) {}

  // Called when a panel's enabled state changes.
  virtual void OnPanelEnabledChanged(AstraDevToolsModel* model,
                                     const std::string& panel_id,
                                     bool enabled) {}

  // Called when a panel's visibility changes.
  virtual void OnPanelVisibilityChanged(AstraDevToolsModel* model,
                                        const std::string& panel_id,
                                        bool visible) {}

  // Called when panels are reordered.
  virtual void OnPanelsReordered(AstraDevToolsModel* model) {}

  // Called when the dock state changes.
  virtual void OnDockStateChanged(AstraDevToolsModel* model,
                                  AstraDevToolsDockState state) {}

  // Called when the model is about to be destroyed.
  // Observers should remove themselves here.
  virtual void OnDevToolsModelShutdown(AstraDevToolsModel* model) {}

 protected:
  ~AstraDevToolsObserver() override = default;
};

// =========================================================================
// Legacy types (kept for backward compatibility)
// =========================================================================
//
// These types were part of the initial DevTools integration skeleton.
// They are kept for backward compatibility with existing tests and code.
// The deepened integration uses AstraDevToolsPanelInfo and
// AstraDevToolsPanelType above.
// =========================================================================

struct AstraDevToolsPanel {
  std::string id;
  std::string title;
  std::string icon;
  bool is_visible = true;
  bool is_pinned = false;
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

enum class AstraDevToolsDockPosition {
  kUndocked = 0,
  kBottom = 1,
  kLeft = 2,
  kRight = 3,
  kMaxValue = kRight,
};

enum class AstraDevToolsTheme {
  kLight = 0,
  kDark = 1,
  kSystem = 2,
  kMaxValue = kSystem,
};

enum class AstraDevToolsPanelPosition {
  kLeft = 0,
  kRight = 1,
  kBottom = 2,
  kMaxValue = kBottom,
};

// =========================================================================
// AstraDevToolsModelObserver — legacy observer (kept for compatibility)
// =========================================================================
class AstraDevToolsModelObserver : public base::CheckedObserver {
 public:
  virtual void OnPanelOpened(const std::string& panel_id) {}
  virtual void OnPanelClosed(const std::string& panel_id) {}
  virtual void OnActivePanelChanged(const std::string& panel_id) {}
  virtual void OnPanelOrderChanged() {}
  virtual void OnDevToolsSettingsChanged() {}
  virtual void OnDockPositionChanged(AstraDevToolsDockPosition position) {}
  virtual void OnThemeChanged(AstraDevToolsTheme theme) {}

 protected:
  ~AstraDevToolsModelObserver() override = default;
};

// =========================================================================
// AstraDevToolsModel — model for Astra DevTools panels and settings
// =========================================================================
//
// AstraDevToolsModel is the single source of truth for:
//   - Astra panel definitions and their ordering (legacy and new panel info)
//   - Active / visible panel state
//   - Presentation settings (persisted via PrefService)
//   - Dock state and theme preferences
//   - DevTools open / closed state
//   - Zoom level
//
// Views observe the model and update their display when the model changes.
// Views never store state — they always read from the model.
//
// The model supports two parallel panel systems:
//   1. Legacy AstraDevToolsPanel (AstraDevToolsPanel-based)
//   2. Deepened AstraDevToolsPanelInfo (panel-type-based)
//
// The deepened system is the primary system going forward.  The legacy
// system is kept for backward compatibility.
//
// Persistence:
//   Presentation settings are persisted via PrefService.  Panel state
//   (order, visibility, enabled state) is also persisted.
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
  // ---- Pref keys (public static constexpr) ------------------------------
  //
  // These pref keys correspond to the 12+ settings managed by the model.
  // They are declared as public static constexpr so they can be referenced
  // by pref registration code and tests.

  // Whether Astra DevTools panels are enabled at all.
  static constexpr char kPrefEnableAstraPanels[] =
      "astra.dev_tools.enable_astra_panels";

  // Default active panel ID (shown when DevTools first opens).
  static constexpr char kPrefDefaultActivePanel[] =
      "astra.dev_tools.default_active_panel";

  // Default dock state for DevTools (string: "bottom", "left", etc.).
  static constexpr char kPrefDefaultDockState[] =
      "astra.dev_tools.default_dock_state";

  // Panel order as a list of panel IDs (ordered list).
  static constexpr char kPrefPanelOrder[] =
      "astra.dev_tools.panel_order";

  // Whether to show panel icons on tab buttons.
  static constexpr char kPrefShowPanelIcons[] =
      "astra.dev_tools.show_panel_icons";

  // Whether to show the "Astra" tab in DevTools.
  static constexpr char kPrefShowAstraTab[] =
      "astra.dev_tools.show_astra_tab";

  // DevTools theme (string: "light", "dark", "system").
  static constexpr char kPrefDevToolsTheme[] =
      "astra.dev_tools.theme";

  // DevTools font size in pixels (int).
  static constexpr char kPrefFontSize[] =
      "astra.dev_tools.font_size";

  // Panel visibility defaults (dict mapping panel_id -> bool).
  static constexpr char kPrefPanelVisibilityDefaults[] =
      "astra.dev_tools.panel_visibility_defaults";

  // Whether to auto-open DevTools when an error occurs.
  static constexpr char kPrefAutoOpenOnError[] =
      "astra.dev_tools.auto_open_on_error";

  // Whether the workspace panel auto-syncs with workspace changes.
  static constexpr char kPrefWorkspaceAutoSync[] =
      "astra.dev_tools.workspace_auto_sync";

  // Whether the performance panel auto-records on navigation.
  static constexpr char kPrefPerformanceAutoRecord[] =
      "astra.dev_tools.performance_auto_record";

  // ---- Default values (public static constexpr) -------------------------

  static constexpr int kDefaultFontSize = 12;
  static constexpr double kDefaultZoomLevel = 1.0;
  static constexpr double kMinZoomLevel = 0.5;
  static constexpr double kMaxZoomLevel = 2.0;

  // ---- Construction / destruction --------------------------------------

  explicit AstraDevToolsModel(PrefService* pref_service);
  ~AstraDevToolsModel();

  AstraDevToolsModel(const AstraDevToolsModel&) = delete;
  AstraDevToolsModel& operator=(const AstraDevToolsModel&) = delete;

  // ---- Deepened panel API (AstraDevToolsPanelInfo) ----------------------
  //
  // These methods operate on the deepened panel info system
  // (AstraDevToolsPanelInfo + AstraDevToolsPanelType).

  // Returns all Astra panels in order.
  std::vector<AstraDevToolsPanelInfo> GetPanels() const;

  // Returns the total number of panels.
  size_t GetPanelCount() const;

  // Returns the panel with the given ID, or null if not found.
  const AstraDevToolsPanelInfo* GetPanel(const std::string& panel_id) const;

  // Returns the panel of the given type, or null if not found.
  const AstraDevToolsPanelInfo* GetPanelByType(AstraDevToolsPanelType type) const;

  // Returns true if the panel is enabled.
  bool IsPanelEnabled(const std::string& panel_id) const;

  // Sets whether a panel is enabled.  Returns false if panel not found.
  bool SetPanelEnabled(const std::string& panel_id, bool enabled);

  // Returns true if the panel is visible (shown in tab strip).
  bool IsPanelVisible(const std::string& panel_id) const;

  // Sets whether a panel is visible.  Returns false if panel not found.
  bool SetPanelVisible(const std::string& panel_id, bool visible);

  // Returns the ID of the currently active panel.
  std::string GetActivePanel() const;

  // Sets the active panel by ID.  Returns false if panel not found
  // or not visible/enabled.
  bool SetActivePanel(const std::string& panel_id);

  // Reorders panels according to the given list of panel IDs.
  // Panels not in the list are placed at the end in their original order.
  void ReorderPanels(const std::vector<std::string>& panel_ids_in_order);

  // Returns the default panel configurations.
  static std::vector<AstraDevToolsPanelInfo> GetDefaultPanels();

  // Resets all panels to their default definitions and ordering.
  void ResetPanelsToDefaults();

  // Opens DevTools (if not already open) and shows the panel of the
  // given type.  Returns true if the panel was successfully shown.
  bool ShowAstraPanel(AstraDevToolsPanelType type);

  // ---- DevTools state ---------------------------------------------------

  // Returns true if DevTools is currently open (visible).
  bool IsDevToolsOpen() const;

  // Sets whether DevTools is open.  Notifies observers.
  void SetDevToolsOpen(bool open);

  // Returns the current dock state.
  AstraDevToolsDockState GetDockState() const;

  // Sets the dock state.  Notifies observers.
  void SetDockState(AstraDevToolsDockState state);

  // Returns the current zoom level (1.0 = 100%).
  double GetZoomLevel() const;

  // Sets the zoom level.  Clamped to [kMinZoomLevel, kMaxZoomLevel].
  void SetZoomLevel(double level);

  // Returns true if DevTools is currently docked (not undocked or minimized).
  bool IsDocked() const;

  // Toggles the dock side between bottom/left/right/undocked.
  void ToggleDockSide();

  // ---- Deepened observer management -------------------------------------

  void AddObserver(AstraDevToolsObserver* observer);
  void RemoveObserver(AstraDevToolsObserver* observer);

  // ---- Legacy panel API (kept for backward compatibility) --------------

  void AddObserver(AstraDevToolsModelObserver* observer);
  void RemoveObserver(AstraDevToolsModelObserver* observer);

  std::vector<AstraDevToolsPanel> GetAllPanels() const;
  std::vector<AstraDevToolsPanel> GetVisiblePanels() const;
  const AstraDevToolsPanel* GetPanelById(const std::string& panel_id) const;
  bool HasPanel(const std::string& panel_id) const;
  bool AddPanel(const AstraDevToolsPanel& panel);
  bool RemovePanel(const std::string& panel_id);
  bool SetPanelVisible(const std::string& panel_id, bool visible);  // legacy
  bool SetPanelPinned(const std::string& panel_id, bool pinned);
  bool ReorderPanel(const std::string& panel_id, size_t new_position);
  bool MovePanelEarlier(const std::string& panel_id);
  bool MovePanelLater(const std::string& panel_id);
  void ResetPanelsToDefaults();  // legacy
  static std::vector<AstraDevToolsPanel> GetDefaultPanels();  // legacy

  const std::string& active_panel_id() const;
  bool SetActivePanel(const std::string& panel_id);  // legacy
  void ActivateNextPanel();
  void ActivatePreviousPanel();

  // ---- Legacy presentation settings ------------------------------------

  bool show_astra_panels() const;
  void SetShowAstraPanels(bool show);
  std::string default_active_panel() const;
  void SetDefaultActivePanel(const std::string& panel_id);
  AstraDevToolsPanelPosition panel_position() const;
  void SetPanelPosition(AstraDevToolsPanelPosition position);
  bool show_panel_icons() const;
  void SetShowPanelIcons(bool show);
  bool show_panel_labels() const;
  void SetShowPanelLabels(bool show);
  int panel_width() const;
  void SetPanelWidth(int width);
  static constexpr int kMinPanelWidth = 150;
  static constexpr int kMaxPanelWidth = 500;
  static constexpr int kDefaultPanelWidth = 240;
  bool experiments_enabled() const;
  void SetExperimentsEnabled(bool enabled);
  bool auto_expand_workspace_panel() const;
  void SetAutoExpandWorkspacePanel(bool auto_expand);
  bool show_panel_toolbar() const;
  void SetShowPanelToolbar(bool show);
  bool compact_mode() const;
  void SetCompactMode(bool compact);
  bool remember_last_panel() const;
  void SetRememberLastPanel(bool remember);
  std::string last_active_panel() const;
  void SetLastActivePanel(const std::string& panel_id);

  // ---- Legacy dock position --------------------------------------------

  AstraDevToolsDockPosition dock_position() const;
  void SetDockPosition(AstraDevToolsDockPosition position);
  void CycleDockPosition();

  // ---- Legacy theme ----------------------------------------------------

  AstraDevToolsTheme theme() const;
  void SetTheme(AstraDevToolsTheme theme);
  AstraDevToolsTheme GetEffectiveTheme() const;

  // ---- Legacy utility --------------------------------------------------

  size_t panel_count() const;
  size_t visible_panel_count() const;
  bool empty() const;
  void LoadFromPrefs();
  void SaveToPrefs() const;

 private:
  // ---- Deepened panel helpers ------------------------------------------

  // Finds the index of a panel by ID in panel_infos_.  Returns -1 if not found.
  int FindPanelInfoIndex(const std::string& panel_id) const;

  // Finds a panel by type.  Returns nullptr if not found.
  AstraDevToolsPanelInfo* FindPanelInfoByType(AstraDevToolsPanelType type);
  const AstraDevToolsPanelInfo* FindPanelInfoByType(
      AstraDevToolsPanelType type) const;

  // Re-sorts panel_infos_ by order_index and renormalizes indices.
  void RenormalizePanelOrder();

  // Notifies deepened observers of panel activation.
  void NotifyPanelActivated(const std::string& panel_id);

  // Notifies deepened observers of panel enable change.
  void NotifyPanelEnabledChanged(const std::string& panel_id, bool enabled);

  // Notifies deepened observers of panel visibility change.
  void NotifyPanelVisibilityChanged(const std::string& panel_id, bool visible);

  // Notifies deepened observers of panel reorder.
  void NotifyPanelsReordered();

  // Notifies deepened observers of dock state change.
  void NotifyDockStateChanged(AstraDevToolsDockState state);

  // Notifies deepened observers of DevTools open/close.
  void NotifyDevToolsOpened();
  void NotifyDevToolsClosed();
  void NotifyDevToolsModelShutdown();

  // ---- Legacy panel helpers --------------------------------------------

  std::vector<AstraDevToolsPanel>::iterator FindPanel(
      const std::string& panel_id);
  std::vector<AstraDevToolsPanel>::const_iterator FindPanel(
      const std::string& panel_id) const;
  void RenormalizePositions();
  void NotifySettingsChanged();
  void NotifyPanelOrderChanged();

  // ---- Data members ----------------------------------------------------

  // The pref service for persistence.  Not owned.
  raw_ptr<PrefService> pref_service_;

  // ---- Deepened panel system data --------------------------------------

  // List of all Astra panels (deepened system).
  std::vector<AstraDevToolsPanelInfo> panel_infos_;

  // ID of the currently active panel (deepened system).
  std::string active_panel_id_;

  // Whether DevTools is currently open.
  bool devtools_open_ = false;

  // Current dock state (deepened system).
  AstraDevToolsDockState dock_state_ = AstraDevToolsDockState::kDockedBottom;

  // Current zoom level (1.0 = 100%).
  double zoom_level_ = kDefaultZoomLevel;

  // Deepened observers.
  base::ObserverList<AstraDevToolsObserver> observers_;

  // ---- Legacy panel system data ----------------------------------------

  // List of all panels (legacy).  Kept sorted by position.
  std::vector<AstraDevToolsPanel> panels_;

  // Currently active panel ID (legacy).
  std::string legacy_active_panel_id_;

  // Dock position (legacy).
  AstraDevToolsDockPosition dock_position_ = AstraDevToolsDockPosition::kBottom;

  // Theme (legacy).
  AstraDevToolsTheme theme_ = AstraDevToolsTheme::kSystem;

  // Legacy observers.
  base::ObserverList<AstraDevToolsModelObserver> legacy_observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_MODEL_H_
