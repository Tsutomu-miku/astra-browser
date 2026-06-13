#include "astra/ui/views/devtools/astra_devtools_model.h"

#include <algorithm>
#include <string>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "astra/browser/astra_prefs.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// Helper: convert dock state enum to/from string for prefs.
const char* DockStateToString(AstraDevToolsDockState state) {
  switch (state) {
    case AstraDevToolsDockState::kDockedBottom:
      return "bottom";
    case AstraDevToolsDockState::kDockedLeft:
      return "left";
    case AstraDevToolsDockState::kDockedRight:
      return "right";
    case AstraDevToolsDockState::kUndocked:
      return "undocked";
    case AstraDevToolsDockState::kMinimized:
      return "minimized";
  }
  return "bottom";
}

AstraDevToolsDockState StringToDockState(const std::string& str) {
  if (str == "bottom") return AstraDevToolsDockState::kDockedBottom;
  if (str == "left") return AstraDevToolsDockState::kDockedLeft;
  if (str == "right") return AstraDevToolsDockState::kDockedRight;
  if (str == "undocked") return AstraDevToolsDockState::kUndocked;
  if (str == "minimized") return AstraDevToolsDockState::kMinimized;
  return AstraDevToolsDockState::kDockedBottom;  // default
}

// Helper: convert legacy dock position to/from string.
const char* DockPositionToString(AstraDevToolsDockPosition position) {
  switch (position) {
    case AstraDevToolsDockPosition::kUndocked:
      return "undocked";
    case AstraDevToolsDockPosition::kBottom:
      return "bottom";
    case AstraDevToolsDockPosition::kLeft:
      return "left";
    case AstraDevToolsDockPosition::kRight:
      return "right";
  }
  return "bottom";
}

AstraDevToolsDockPosition StringToDockPosition(const std::string& str) {
  if (str == "undocked") return AstraDevToolsDockPosition::kUndocked;
  if (str == "left") return AstraDevToolsDockPosition::kLeft;
  if (str == "right") return AstraDevToolsDockPosition::kRight;
  return AstraDevToolsDockPosition::kBottom;
}

const char* ThemeToString(AstraDevToolsTheme theme) {
  switch (theme) {
    case AstraDevToolsTheme::kLight:
      return "light";
    case AstraDevToolsTheme::kDark:
      return "dark";
    case AstraDevToolsTheme::kSystem:
      return "system";
  }
  return "system";
}

AstraDevToolsTheme StringToTheme(const std::string& str) {
  if (str == "light") return AstraDevToolsTheme::kLight;
  if (str == "dark") return AstraDevToolsTheme::kDark;
  return AstraDevToolsTheme::kSystem;
}

const char* PanelPositionToString(AstraDevToolsPanelPosition position) {
  switch (position) {
    case AstraDevToolsPanelPosition::kLeft:
      return "left";
    case AstraDevToolsPanelPosition::kRight:
      return "right";
    case AstraDevToolsPanelPosition::kBottom:
      return "bottom";
  }
  return "right";
}

AstraDevToolsPanelPosition StringToPanelPosition(const std::string& str) {
  if (str == "left") return AstraDevToolsPanelPosition::kLeft;
  if (str == "bottom") return AstraDevToolsPanelPosition::kBottom;
  return AstraDevToolsPanelPosition::kRight;
}

// Panel state dictionary keys (legacy).
constexpr char kPanelIdKey[] = "id";
constexpr char kPanelTitleKey[] = "title";
constexpr char kPanelIconKey[] = "icon";
constexpr char kPanelVisibleKey[] = "visible";
constexpr char kPanelPinnedKey[] = "pinned";
constexpr char kPanelPositionKey[] = "position";

// Deepened panel info dictionary keys.
constexpr char kPanelInfoIdKey[] = "panel_id";
constexpr char kPanelInfoTypeKey[] = "type";
constexpr char kPanelInfoTitleKey[] = "title";
constexpr char kPanelInfoIconKey[] = "icon_name";
constexpr char kPanelInfoEnabledKey[] = "is_enabled";
constexpr char kPanelInfoVisibleKey[] = "is_visible";
constexpr char kPanelInfoOrderKey[] = "order_index";
constexpr char kPanelInfoDefaultKey[] = "is_default";
constexpr char kPanelInfoDescriptionKey[] = "description";

}  // namespace

// =========================================================================
// Static helpers: default deepened panels
// =========================================================================

// static
std::vector<AstraDevToolsPanelInfo> AstraDevToolsModel::GetDefaultPanels() {
  std::vector<AstraDevToolsPanelInfo> panels;

  // Workspace panel — primary Astra product panel.
  AstraDevToolsPanelInfo workspace;
  workspace.panel_id = "workspace-panel";
  workspace.type = AstraDevToolsPanelType::kWorkspacePanel;
  workspace.title = u"Workspace";
  workspace.icon_name = "workspace";
  workspace.is_enabled = true;
  workspace.is_visible = true;
  workspace.order_index = 0;
  workspace.is_default = true;
  workspace.description = u"Manage workspaces, tabs, and window layout";
  panels.push_back(workspace);

  // Tab Stack panel — tab stack management.
  AstraDevToolsPanelInfo tab_stack;
  tab_stack.panel_id = "tab-stack-panel";
  tab_stack.type = AstraDevToolsPanelType::kTabStackPanel;
  tab_stack.title = u"Tab Stack";
  tab_stack.icon_name = "tab_stack";
  tab_stack.is_enabled = true;
  tab_stack.is_visible = true;
  tab_stack.order_index = 1;
  tab_stack.is_default = true;
  tab_stack.description = u"View and manage tab stacks and groups";
  panels.push_back(tab_stack);

  // Notes panel — note-taking.
  AstraDevToolsPanelInfo notes;
  notes.panel_id = "notes-panel";
  notes.type = AstraDevToolsPanelType::kNotesPanel;
  notes.title = u"Notes";
  notes.icon_name = "notes";
  notes.is_enabled = true;
  notes.is_visible = true;
  notes.order_index = 2;
  notes.is_default = true;
  notes.description = u"Take notes while inspecting pages";
  panels.push_back(notes);

  // Performance panel — performance insights.
  AstraDevToolsPanelInfo performance;
  performance.panel_id = "performance-panel";
  performance.type = AstraDevToolsPanelType::kPerformancePanel;
  performance.title = u"Performance";
  performance.icon_name = "performance";
  performance.is_enabled = true;
  performance.is_visible = true;
  performance.order_index = 3;
  performance.is_default = true;
  performance.description = u"Performance insights and profiling";
  panels.push_back(performance);

  // Accessibility panel — accessibility tools.
  AstraDevToolsPanelInfo accessibility;
  accessibility.panel_id = "accessibility-panel";
  accessibility.type = AstraDevToolsPanelType::kAccessibilityPanel;
  accessibility.title = u"Accessibility";
  accessibility.icon_name = "accessibility";
  accessibility.is_enabled = true;
  accessibility.is_visible = true;
  accessibility.order_index = 4;
  accessibility.is_default = true;
  accessibility.description = u"Accessibility auditing and tools";
  panels.push_back(accessibility);

  // A11y Tree panel — accessibility tree view.
  AstraDevToolsPanelInfo a11y_tree;
  a11y_tree.panel_id = "a11y-tree-panel";
  a11y_tree.type = AstraDevToolsPanelType::kA11yTreePanel;
  a11y_tree.title = u"A11y Tree";
  a11y_tree.icon_name = "a11y_tree";
  a11y_tree.is_enabled = true;
  a11y_tree.is_visible = true;
  a11y_tree.order_index = 5;
  a11y_tree.is_default = true;
  a11y_tree.description = u"Accessibility tree inspector view";
  panels.push_back(a11y_tree);

  return panels;
}

// =========================================================================
// Static helpers: default legacy panels
// =========================================================================

// static
std::vector<AstraDevToolsPanel> AstraDevToolsModel::GetDefaultPanels() {
  std::vector<AstraDevToolsPanel> panels;

  // Default panel ordering — pinned panels first, then regular ones.

  AstraDevToolsPanel workspace;
  workspace.id = "workspace";
  workspace.title = "Workspace";
  workspace.icon = "workspace";
  workspace.is_visible = true;
  workspace.is_pinned = true;
  workspace.position = 0;
  panels.push_back(workspace);

  AstraDevToolsPanel notes;
  notes.id = "notes";
  notes.title = "Notes";
  notes.icon = "notes";
  notes.is_visible = true;
  notes.is_pinned = true;
  notes.position = 1;
  panels.push_back(notes);

  AstraDevToolsPanel focus_mode;
  focus_mode.id = "focus-mode";
  focus_mode.title = "Focus Mode";
  focus_mode.icon = "focus";
  focus_mode.is_visible = true;
  focus_mode.is_pinned = false;
  focus_mode.position = 2;
  panels.push_back(focus_mode);

  AstraDevToolsPanel screenshot;
  screenshot.id = "screenshot";
  screenshot.title = "Screenshot";
  screenshot.icon = "screenshot";
  screenshot.is_visible = true;
  screenshot.is_pinned = false;
  screenshot.position = 3;
  panels.push_back(screenshot);

  AstraDevToolsPanel reading_list;
  reading_list.id = "reading-list";
  reading_list.title = "Reading List";
  reading_list.icon = "reading_list";
  reading_list.is_visible = true;
  reading_list.is_pinned = false;
  reading_list.position = 4;
  panels.push_back(reading_list);

  AstraDevToolsPanel tab_stack;
  tab_stack.id = "tab-stack";
  tab_stack.title = "Tab Stack";
  tab_stack.icon = "tab_stack";
  tab_stack.is_visible = true;
  tab_stack.is_pinned = false;
  tab_stack.position = 5;
  panels.push_back(tab_stack);

  return panels;
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraDevToolsModel::AstraDevToolsModel(PrefService* pref_service)
    : pref_service_(pref_service) {
  // Initialize deepened panel system with defaults.
  panel_infos_ = GetDefaultPanels();

  // Set first visible panel as active.
  auto default_panels = GetDefaultPanels();
  if (!default_panels.empty()) {
    for (const auto& p : default_panels) {
      if (p.is_visible && p.is_enabled) {
        active_panel_id_ = p.panel_id;
        break;
      }
    }
  }

  // Initialize legacy panel system with defaults.
  panels_ = GetDefaultPanels();

  // Set first visible legacy panel as active.
  auto visible_legacy = GetVisiblePanels();
  if (!visible_legacy.empty()) {
    legacy_active_panel_id_ = visible_legacy.front().id;
  }

  // Load from prefs if available.
  if (pref_service_) {
    LoadFromPrefs();
  }
}

AstraDevToolsModel::~AstraDevToolsModel() {
  NotifyDevToolsModelShutdown();
}

// =========================================================================
// Deepened panel API — queries
// =========================================================================

std::vector<AstraDevToolsPanelInfo> AstraDevToolsModel::GetPanels() const {
  std::vector<AstraDevToolsPanelInfo> result = panel_infos_;
  std::sort(result.begin(), result.end(),
            [](const AstraDevToolsPanelInfo& a, const AstraDevToolsPanelInfo& b) {
              return a.order_index < b.order_index;
            });
  return result;
}

size_t AstraDevToolsModel::GetPanelCount() const {
  return panel_infos_.size();
}

const AstraDevToolsPanelInfo* AstraDevToolsModel::GetPanel(
    const std::string& panel_id) const {
  int index = FindPanelInfoIndex(panel_id);
  if (index < 0) {
    return nullptr;
  }
  return &panel_infos_[static_cast<size_t>(index)];
}

const AstraDevToolsPanelInfo* AstraDevToolsModel::GetPanelByType(
    AstraDevToolsPanelType type) const {
  return FindPanelInfoByType(type);
}

bool AstraDevToolsModel::IsPanelEnabled(const std::string& panel_id) const {
  const auto* panel = GetPanel(panel_id);
  if (!panel) {
    return false;
  }
  return panel->is_enabled;
}

bool AstraDevToolsModel::IsPanelVisible(const std::string& panel_id) const {
  const auto* panel = GetPanel(panel_id);
  if (!panel) {
    return false;
  }
  return panel->is_visible;
}

std::string AstraDevToolsModel::GetActivePanel() const {
  return active_panel_id_;
}

// =========================================================================
// Deepened panel API — mutations
// =========================================================================

bool AstraDevToolsModel::SetPanelEnabled(const std::string& panel_id,
                                         bool enabled) {
  int index = FindPanelInfoIndex(panel_id);
  if (index < 0) {
    return false;
  }

  if (panel_infos_[index].is_enabled == enabled) {
    return true;  // No change.
  }

  panel_infos_[index].is_enabled = enabled;

  // If disabling the active panel, switch to another.
  if (!enabled && active_panel_id_ == panel_id) {
    // Find the next visible+enabled panel.
    for (const auto& p : GetPanels()) {
      if (p.panel_id != panel_id && p.is_visible && p.is_enabled) {
        active_panel_id_ = p.panel_id;
        NotifyPanelActivated(active_panel_id_);
        break;
      }
    }
    // If no other panel is available, clear active.
    if (active_panel_id_ == panel_id) {
      active_panel_id_.clear();
      NotifyPanelActivated(std::string());
    }
  }

  NotifyPanelEnabledChanged(panel_id, enabled);
  return true;
}

bool AstraDevToolsModel::SetPanelVisible(const std::string& panel_id,
                                         bool visible) {
  int index = FindPanelInfoIndex(panel_id);
  if (index < 0) {
    return false;
  }

  if (panel_infos_[index].is_visible == visible) {
    return true;  // No change.
  }

  panel_infos_[index].is_visible = visible;

  // If hiding the active panel, switch to another.
  if (!visible && active_panel_id_ == panel_id) {
    for (const auto& p : GetPanels()) {
      if (p.panel_id != panel_id && p.is_visible && p.is_enabled) {
        active_panel_id_ = p.panel_id;
        NotifyPanelActivated(active_panel_id_);
        break;
      }
    }
    if (active_panel_id_ == panel_id) {
      active_panel_id_.clear();
      NotifyPanelActivated(std::string());
    }
  }

  NotifyPanelVisibilityChanged(panel_id, visible);
  return true;
}

bool AstraDevToolsModel::SetActivePanel(const std::string& panel_id) {
  if (active_panel_id_ == panel_id) {
    return true;
  }

  const auto* panel = GetPanel(panel_id);
  if (!panel || !panel->is_visible || !panel->is_enabled) {
    return false;
  }

  active_panel_id_ = panel_id;
  NotifyPanelActivated(active_panel_id_);
  return true;
}

void AstraDevToolsModel::ReorderPanels(
    const std::vector<std::string>& panel_ids_in_order) {
  // Assign order indices based on the provided list.
  int order = 0;
  for (const auto& id : panel_ids_in_order) {
    int index = FindPanelInfoIndex(id);
    if (index >= 0) {
      panel_infos_[index].order_index = order++;
    }
  }

  // Place remaining panels at the end, preserving their relative order.
  std::vector<AstraDevToolsPanelInfo> remaining;
  for (const auto& panel : panel_infos_) {
    bool found = false;
    for (const auto& id : panel_ids_in_order) {
      if (panel.panel_id == id) {
        found = true;
        break;
      }
    }
    if (!found) {
      remaining.push_back(panel);
    }
  }
  std::sort(remaining.begin(), remaining.end(),
            [](const AstraDevToolsPanelInfo& a, const AstraDevToolsPanelInfo& b) {
              return a.order_index < b.order_index;
            });
  for (auto& panel : remaining) {
    int index = FindPanelInfoIndex(panel.panel_id);
    if (index >= 0) {
      panel_infos_[index].order_index = order++;
    }
  }

  RenormalizePanelOrder();
  NotifyPanelsReordered();
}

void AstraDevToolsModel::ResetPanelsToDefaults() {
  panel_infos_ = GetDefaultPanels();

  // Set active panel to first visible/enabled default.
  active_panel_id_.clear();
  for (const auto& p : panel_infos_) {
    if (p.is_visible && p.is_enabled) {
      active_panel_id_ = p.panel_id;
      break;
    }
  }

  NotifyPanelsReordered();
  if (!active_panel_id_.empty()) {
    NotifyPanelActivated(active_panel_id_);
  }
}

bool AstraDevToolsModel::ShowAstraPanel(AstraDevToolsPanelType type) {
  const auto* panel = GetPanelByType(type);
  if (!panel) {
    return false;
  }

  // Open DevTools if not already open.
  if (!devtools_open_) {
    SetDevToolsOpen(true);
  }

  // Ensure the panel is visible and enabled.
  if (!panel->is_visible) {
    SetPanelVisible(panel->panel_id, true);
  }
  if (!panel->is_enabled) {
    SetPanelEnabled(panel->panel_id, true);
  }

  // Activate the panel.
  return SetActivePanel(panel->panel_id);
}

// =========================================================================
// DevTools state
// =========================================================================

bool AstraDevToolsModel::IsDevToolsOpen() const {
  return devtools_open_;
}

void AstraDevToolsModel::SetDevToolsOpen(bool open) {
  if (devtools_open_ == open) {
    return;
  }
  devtools_open_ = open;

  if (open) {
    NotifyDevToolsOpened();
  } else {
    NotifyDevToolsClosed();
  }
}

AstraDevToolsDockState AstraDevToolsModel::GetDockState() const {
  return dock_state_;
}

void AstraDevToolsModel::SetDockState(AstraDevToolsDockState state) {
  if (dock_state_ == state) {
    return;
  }
  dock_state_ = state;

  // Persist to prefs if available.
  if (pref_service_) {
    pref_service_->SetString(kPrefDefaultDockState, DockStateToString(state));
  }

  NotifyDockStateChanged(dock_state_);
}

double AstraDevToolsModel::GetZoomLevel() const {
  return zoom_level_;
}

void AstraDevToolsModel::SetZoomLevel(double level) {
  double clamped = std::clamp(level, kMinZoomLevel, kMaxZoomLevel);
  if (zoom_level_ == clamped) {
    return;
  }
  zoom_level_ = clamped;

  // TODO(astra): Persist zoom level to prefs.
  //   Chromium owner: chrome/browser/devtools/devtools_window.h
  //   DevTools stores zoom level in its own preferences.
}

bool AstraDevToolsModel::IsDocked() const {
  return dock_state_ == AstraDevToolsDockState::kDockedBottom ||
         dock_state_ == AstraDevToolsDockState::kDockedLeft ||
         dock_state_ == AstraDevToolsDockState::kDockedRight;
}

void AstraDevToolsModel::ToggleDockSide() {
  // Cycle through dock sides: bottom -> left -> right -> undocked -> bottom
  switch (dock_state_) {
    case AstraDevToolsDockState::kDockedBottom:
      SetDockState(AstraDevToolsDockState::kDockedLeft);
      break;
    case AstraDevToolsDockState::kDockedLeft:
      SetDockState(AstraDevToolsDockState::kDockedRight);
      break;
    case AstraDevToolsDockState::kDockedRight:
      SetDockState(AstraDevToolsDockState::kUndocked);
      break;
    case AstraDevToolsDockState::kUndocked:
      SetDockState(AstraDevToolsDockState::kDockedBottom);
      break;
    case AstraDevToolsDockState::kMinimized:
      // From minimized, go to bottom dock.
      SetDockState(AstraDevToolsDockState::kDockedBottom);
      break;
  }
}

// =========================================================================
// Deepened observer management
// =========================================================================

void AstraDevToolsModel::AddObserver(AstraDevToolsObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraDevToolsModel::RemoveObserver(AstraDevToolsObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Deepened panel helpers
// =========================================================================

int AstraDevToolsModel::FindPanelInfoIndex(
    const std::string& panel_id) const {
  for (size_t i = 0; i < panel_infos_.size(); ++i) {
    if (panel_infos_[i].panel_id == panel_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

AstraDevToolsPanelInfo* AstraDevToolsModel::FindPanelInfoByType(
    AstraDevToolsPanelType type) {
  for (auto& panel : panel_infos_) {
    if (panel.type == type) {
      return &panel;
    }
  }
  return nullptr;
}

const AstraDevToolsPanelInfo* AstraDevToolsModel::FindPanelInfoByType(
    AstraDevToolsPanelType type) const {
  for (const auto& panel : panel_infos_) {
    if (panel.type == type) {
      return &panel;
    }
  }
  return nullptr;
}

void AstraDevToolsModel::RenormalizePanelOrder() {
  std::sort(panel_infos_.begin(), panel_infos_.end(),
            [](const AstraDevToolsPanelInfo& a, const AstraDevToolsPanelInfo& b) {
              return a.order_index < b.order_index;
            });
  for (size_t i = 0; i < panel_infos_.size(); ++i) {
    panel_infos_[i].order_index = static_cast<int>(i);
  }
}

void AstraDevToolsModel::NotifyPanelActivated(const std::string& panel_id) {
  for (auto& observer : observers_) {
    observer.OnPanelActivated(this, panel_id);
  }
}

void AstraDevToolsModel::NotifyPanelEnabledChanged(
    const std::string& panel_id, bool enabled) {
  for (auto& observer : observers_) {
    observer.OnPanelEnabledChanged(this, panel_id, enabled);
  }
}

void AstraDevToolsModel::NotifyPanelVisibilityChanged(
    const std::string& panel_id, bool visible) {
  for (auto& observer : observers_) {
    observer.OnPanelVisibilityChanged(this, panel_id, visible);
  }
}

void AstraDevToolsModel::NotifyPanelsReordered() {
  for (auto& observer : observers_) {
    observer.OnPanelsReordered(this);
  }
}

void AstraDevToolsModel::NotifyDockStateChanged(AstraDevToolsDockState state) {
  for (auto& observer : observers_) {
    observer.OnDockStateChanged(this, state);
  }
}

void AstraDevToolsModel::NotifyDevToolsOpened() {
  for (auto& observer : observers_) {
    observer.OnDevToolsOpened(this);
  }
}

void AstraDevToolsModel::NotifyDevToolsClosed() {
  for (auto& observer : observers_) {
    observer.OnDevToolsClosed(this);
  }
}

void AstraDevToolsModel::NotifyDevToolsModelShutdown() {
  for (auto& observer : observers_) {
    observer.OnDevToolsModelShutdown(this);
  }
}

// =========================================================================
// Legacy observer management
// =========================================================================

void AstraDevToolsModel::AddObserver(AstraDevToolsModelObserver* observer) {
  legacy_observers_.AddObserver(observer);
}

void AstraDevToolsModel::RemoveObserver(AstraDevToolsModelObserver* observer) {
  legacy_observers_.RemoveObserver(observer);
}

// =========================================================================
// Legacy panel management — queries
// =========================================================================

std::vector<AstraDevToolsPanel> AstraDevToolsModel::GetAllPanels() const {
  std::vector<AstraDevToolsPanel> result = panels_;
  std::sort(result.begin(), result.end(),
            [](const AstraDevToolsPanel& a, const AstraDevToolsPanel& b) {
              return a.position < b.position;
            });
  return result;
}

std::vector<AstraDevToolsPanel> AstraDevToolsModel::GetVisiblePanels() const {
  std::vector<AstraDevToolsPanel> all = GetAllPanels();
  std::vector<AstraDevToolsPanel> result;
  for (const auto& panel : all) {
    if (panel.is_visible) {
      result.push_back(panel);
    }
  }
  return result;
}

const AstraDevToolsPanel* AstraDevToolsModel::GetPanelById(
    const std::string& panel_id) const {
  auto it = FindPanel(panel_id);
  if (it == panels_.end()) {
    return nullptr;
  }
  return &(*it);
}

bool AstraDevToolsModel::HasPanel(const std::string& panel_id) const {
  return FindPanel(panel_id) != panels_.end();
}

size_t AstraDevToolsModel::visible_panel_count() const {
  size_t count = 0;
  for (const auto& panel : panels_) {
    if (panel.is_visible) {
      ++count;
    }
  }
  return count;
}

size_t AstraDevToolsModel::panel_count() const {
  return panels_.size();
}

bool AstraDevToolsModel::empty() const {
  return panels_.empty();
}

const std::string& AstraDevToolsModel::active_panel_id() const {
  return legacy_active_panel_id_;
}

// =========================================================================
// Legacy panel management — mutations
// =========================================================================

bool AstraDevToolsModel::AddPanel(const AstraDevToolsPanel& panel) {
  if (panel.id.empty()) {
    return false;
  }
  if (HasPanel(panel.id)) {
    return false;
  }

  size_t position = std::min(panel.position, panels_.size());

  AstraDevToolsPanel new_panel = panel;
  new_panel.position = position;

  // Shift panels at or after the insertion point forward.
  for (auto& existing : panels_) {
    if (existing.position >= position) {
      existing.position += 1;
    }
  }

  panels_.push_back(new_panel);
  RenormalizePositions();
  NotifyPanelOrderChanged();
  return true;
}

bool AstraDevToolsModel::RemovePanel(const std::string& panel_id) {
  auto it = FindPanel(panel_id);
  if (it == panels_.end()) {
    return false;
  }

  size_t removed_position = it->position;
  panels_.erase(it);

  // If we removed the active panel, clear it and try to activate another.
  if (legacy_active_panel_id_ == panel_id) {
    legacy_active_panel_id_.clear();
    auto visible = GetVisiblePanels();
    if (!visible.empty()) {
      for (const auto& p : visible) {
        if (p.position >= removed_position ||
            p.position == visible.back().position) {
          legacy_active_panel_id_ = p.id;
          break;
        }
      }
    }
    for (auto& observer : legacy_observers_) {
      observer.OnActivePanelChanged(legacy_active_panel_id_);
    }
  }

  RenormalizePositions();
  NotifyPanelOrderChanged();
  return true;
}

bool AstraDevToolsModel::SetPanelVisible(const std::string& panel_id,
                                         bool visible) {
  auto it = FindPanel(panel_id);
  if (it == panels_.end()) {
    return false;
  }
  if (it->is_visible == visible) {
    return true;
  }

  it->is_visible = visible;

  if (visible) {
    for (auto& observer : legacy_observers_) {
      observer.OnPanelOpened(panel_id);
    }
  } else {
    // If hiding the active panel, activate another one.
    if (legacy_active_panel_id_ == panel_id) {
      auto visible_panels = GetVisiblePanels();
      if (!visible_panels.empty()) {
        legacy_active_panel_id_ = visible_panels.front().id;
        for (auto& observer : legacy_observers_) {
          observer.OnActivePanelChanged(legacy_active_panel_id_);
        }
      } else {
        legacy_active_panel_id_.clear();
        for (auto& observer : legacy_observers_) {
          observer.OnActivePanelChanged(std::string());
        }
      }
    }
    for (auto& observer : legacy_observers_) {
      observer.OnPanelClosed(panel_id);
    }
  }

  SaveToPrefs();
  return true;
}

bool AstraDevToolsModel::SetPanelPinned(const std::string& panel_id,
                                        bool pinned) {
  auto it = FindPanel(panel_id);
  if (it == panels_.end()) {
    return false;
  }
  if (it->is_pinned == pinned) {
    return true;
  }

  it->is_pinned = pinned;
  NotifyPanelOrderChanged();
  return true;
}

bool AstraDevToolsModel::ReorderPanel(const std::string& panel_id,
                                      size_t new_position) {
  auto it = FindPanel(panel_id);
  if (it == panels_.end()) {
    return false;
  }

  size_t clamped_position = std::min(new_position, panels_.size() - 1);
  if (it->position == clamped_position) {
    return true;
  }

  size_t old_position = it->position;
  it->position = clamped_position;

  // Shift other panels to make room.
  if (clamped_position > old_position) {
    for (auto& panel : panels_) {
      if (&panel != &(*it) &&
          panel.position > old_position &&
          panel.position <= clamped_position) {
        panel.position -= 1;
      }
    }
  } else {
    for (auto& panel : panels_) {
      if (&panel != &(*it) &&
          panel.position >= clamped_position &&
          panel.position < old_position) {
        panel.position += 1;
      }
    }
  }

  RenormalizePositions();
  NotifyPanelOrderChanged();
  return true;
}

bool AstraDevToolsModel::MovePanelEarlier(const std::string& panel_id) {
  auto it = FindPanel(panel_id);
  if (it == panels_.end()) {
    return false;
  }
  if (it->position == 0) {
    return false;
  }
  return ReorderPanel(panel_id, it->position - 1);
}

bool AstraDevToolsModel::MovePanelLater(const std::string& panel_id) {
  auto it = FindPanel(panel_id);
  if (it == panels_.end()) {
    return false;
  }
  if (it->position >= panels_.size() - 1) {
    return false;
  }
  return ReorderPanel(panel_id, it->position + 1);
}

void AstraDevToolsModel::ResetPanelsToDefaults() {
  panels_ = GetDefaultPanels();

  // Set active panel based on preferences.
  if (remember_last_panel() && !last_active_panel().empty() &&
      HasPanel(last_active_panel())) {
    legacy_active_panel_id_ = last_active_panel();
  } else if (!default_active_panel().empty() &&
             HasPanel(default_active_panel())) {
    legacy_active_panel_id_ = default_active_panel();
  } else {
    auto visible = GetVisiblePanels();
    if (!visible.empty()) {
      legacy_active_panel_id_ = visible.front().id;
    }
  }

  NotifyPanelOrderChanged();
  if (!legacy_active_panel_id_.empty()) {
    for (auto& observer : legacy_observers_) {
      observer.OnActivePanelChanged(legacy_active_panel_id_);
    }
  }
}

bool AstraDevToolsModel::SetActivePanel(const std::string& panel_id) {
  if (legacy_active_panel_id_ == panel_id) {
    return true;
  }

  const AstraDevToolsPanel* panel = GetPanelById(panel_id);
  if (!panel || !panel->is_visible) {
    return false;
  }

  legacy_active_panel_id_ = panel_id;

  if (remember_last_panel()) {
    SetLastActivePanel(panel_id);
  }

  for (auto& observer : legacy_observers_) {
    observer.OnActivePanelChanged(legacy_active_panel_id_);
  }
  return true;
}

void AstraDevToolsModel::ActivateNextPanel() {
  auto visible = GetVisiblePanels();
  if (visible.empty()) {
    return;
  }

  size_t current_index = 0;
  for (size_t i = 0; i < visible.size(); ++i) {
    if (visible[i].id == legacy_active_panel_id_) {
      current_index = i;
      break;
    }
  }

  size_t next_index = (current_index + 1) % visible.size();
  SetActivePanel(visible[next_index].id);
}

void AstraDevToolsModel::ActivatePreviousPanel() {
  auto visible = GetVisiblePanels();
  if (visible.empty()) {
    return;
  }

  size_t current_index = 0;
  for (size_t i = 0; i < visible.size(); ++i) {
    if (visible[i].id == legacy_active_panel_id_) {
      current_index = i;
      break;
    }
  }

  size_t prev_index =
      (current_index == 0) ? visible.size() - 1 : current_index - 1;
  SetActivePanel(visible[prev_index].id);
}

// =========================================================================
// Legacy presentation settings
// =========================================================================

bool AstraDevToolsModel::show_astra_panels() const {
  if (!pref_service_) {
    return kDefaultDevToolsAstraPanelVisible;
  }
  return pref_service_->GetBoolean(prefs::kPrefDevToolsAstraPanelVisible);
}

void AstraDevToolsModel::SetShowAstraPanels(bool show) {
  if (!pref_service_) {
    return;
  }
  if (show_astra_panels() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefDevToolsAstraPanelVisible, show);
  NotifySettingsChanged();
}

std::string AstraDevToolsModel::default_active_panel() const {
  if (!pref_service_) {
    return kDefaultDevToolsDefaultPanel;
  }
  return pref_service_->GetString(prefs::kPrefDevToolsDefaultPanel);
}

void AstraDevToolsModel::SetDefaultActivePanel(const std::string& panel_id) {
  if (!pref_service_) {
    return;
  }
  if (default_active_panel() == panel_id) {
    return;
  }
  pref_service_->SetString(prefs::kPrefDevToolsDefaultPanel, panel_id);
  NotifySettingsChanged();
}

AstraDevToolsPanelPosition AstraDevToolsModel::panel_position() const {
  if (!pref_service_) {
    return StringToPanelPosition(kDefaultDevToolsPanelSide);
  }
  return StringToPanelPosition(
      pref_service_->GetString(prefs::kPrefDevToolsPanelSide));
}

void AstraDevToolsModel::SetPanelPosition(
    AstraDevToolsPanelPosition position) {
  if (!pref_service_) {
    return;
  }
  if (panel_position() == position) {
    return;
  }
  pref_service_->SetString(
      prefs::kPrefDevToolsPanelSide,
      PanelPositionToString(position));
  NotifySettingsChanged();
}

bool AstraDevToolsModel::show_panel_icons() const {
  if (!pref_service_) {
    return kDefaultDevToolsShowPanelIcons;
  }
  return pref_service_->GetBoolean(prefs::kPrefDevToolsShowPanelIcons);
}

void AstraDevToolsModel::SetShowPanelIcons(bool show) {
  if (!pref_service_) {
    return;
  }
  if (show_panel_icons() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefDevToolsShowPanelIcons, show);
  NotifySettingsChanged();
}

bool AstraDevToolsModel::show_panel_labels() const {
  if (!pref_service_) {
    return kDefaultDevToolsShowPanelLabels;
  }
  return pref_service_->GetBoolean(prefs::kPrefDevToolsShowPanelLabels);
}

void AstraDevToolsModel::SetShowPanelLabels(bool show) {
  if (!pref_service_) {
    return;
  }
  if (show_panel_labels() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefDevToolsShowPanelLabels, show);
  NotifySettingsChanged();
}

int AstraDevToolsModel::panel_width() const {
  if (!pref_service_) {
    return kDefaultDevToolsPanelWidth;
  }
  return pref_service_->GetInteger(prefs::kPrefDevToolsPanelWidth);
}

void AstraDevToolsModel::SetPanelWidth(int width) {
  if (!pref_service_) {
    return;
  }
  int clamped = std::clamp(width, kMinPanelWidth, kMaxPanelWidth);
  if (panel_width() == clamped) {
    return;
  }
  pref_service_->SetInteger(prefs::kPrefDevToolsPanelWidth, clamped);
  NotifySettingsChanged();
}

bool AstraDevToolsModel::experiments_enabled() const {
  if (!pref_service_) {
    return kDefaultDevToolsExperimentsEnabled;
  }
  return pref_service_->GetBoolean(prefs::kPrefDevToolsExperimentsEnabled);
}

void AstraDevToolsModel::SetExperimentsEnabled(bool enabled) {
  if (!pref_service_) {
    return;
  }
  if (experiments_enabled() == enabled) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefDevToolsExperimentsEnabled, enabled);
  NotifySettingsChanged();
}

bool AstraDevToolsModel::auto_expand_workspace_panel() const {
  if (!pref_service_) {
    return kDefaultDevToolsAutoExpandWorkspacePanel;
  }
  return pref_service_->GetBoolean(
      prefs::kPrefDevToolsAutoExpandWorkspacePanel);
}

void AstraDevToolsModel::SetAutoExpandWorkspacePanel(bool auto_expand) {
  if (!pref_service_) {
    return;
  }
  if (auto_expand_workspace_panel() == auto_expand) {
    return;
  }
  pref_service_->SetBoolean(
      prefs::kPrefDevToolsAutoExpandWorkspacePanel, auto_expand);
  NotifySettingsChanged();
}

bool AstraDevToolsModel::show_panel_toolbar() const {
  if (!pref_service_) {
    return kDefaultDevToolsShowPanelToolbar;
  }
  return pref_service_->GetBoolean(prefs::kPrefDevToolsShowPanelToolbar);
}

void AstraDevToolsModel::SetShowPanelToolbar(bool show) {
  if (!pref_service_) {
    return;
  }
  if (show_panel_toolbar() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefDevToolsShowPanelToolbar, show);
  NotifySettingsChanged();
}

bool AstraDevToolsModel::compact_mode() const {
  if (!pref_service_) {
    return kDefaultDevToolsCompactMode;
  }
  return pref_service_->GetBoolean(prefs::kPrefDevToolsCompactMode);
}

void AstraDevToolsModel::SetCompactMode(bool compact) {
  if (!pref_service_) {
    return;
  }
  if (compact_mode() == compact) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefDevToolsCompactMode, compact);
  NotifySettingsChanged();
}

bool AstraDevToolsModel::remember_last_panel() const {
  if (!pref_service_) {
    return kDefaultDevToolsRememberLastPanel;
  }
  return pref_service_->GetBoolean(prefs::kPrefDevToolsRememberLastPanel);
}

void AstraDevToolsModel::SetRememberLastPanel(bool remember) {
  if (!pref_service_) {
    return;
  }
  if (remember_last_panel() == remember) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefDevToolsRememberLastPanel, remember);
  NotifySettingsChanged();
}

std::string AstraDevToolsModel::last_active_panel() const {
  if (!pref_service_) {
    return kDefaultDevToolsLastActivePanel;
  }
  return pref_service_->GetString(prefs::kPrefDevToolsLastActivePanel);
}

void AstraDevToolsModel::SetLastActivePanel(const std::string& panel_id) {
  if (!pref_service_) {
    return;
  }
  if (last_active_panel() == panel_id) {
    return;
  }
  pref_service_->SetString(prefs::kPrefDevToolsLastActivePanel, panel_id);
}

// =========================================================================
// Legacy dock position
// =========================================================================

AstraDevToolsDockPosition AstraDevToolsModel::dock_position() const {
  return dock_position_;
}

void AstraDevToolsModel::SetDockPosition(AstraDevToolsDockPosition position) {
  if (dock_position_ == position) {
    return;
  }
  dock_position_ = position;

  if (pref_service_) {
    pref_service_->SetString(prefs::kPrefDevToolsDefaultDockState,
                             DockPositionToString(position));
  }

  for (auto& observer : legacy_observers_) {
    observer.OnDockPositionChanged(dock_position_);
  }
}

void AstraDevToolsModel::CycleDockPosition() {
  switch (dock_position_) {
    case AstraDevToolsDockPosition::kUndocked:
      SetDockPosition(AstraDevToolsDockPosition::kBottom);
      break;
    case AstraDevToolsDockPosition::kBottom:
      SetDockPosition(AstraDevToolsDockPosition::kLeft);
      break;
    case AstraDevToolsDockPosition::kLeft:
      SetDockPosition(AstraDevToolsDockPosition::kRight);
      break;
    case AstraDevToolsDockPosition::kRight:
      SetDockPosition(AstraDevToolsDockPosition::kUndocked);
      break;
  }
}

// =========================================================================
// Legacy theme
// =========================================================================

AstraDevToolsTheme AstraDevToolsModel::theme() const {
  return theme_;
}

void AstraDevToolsModel::SetTheme(AstraDevToolsTheme theme) {
  if (theme_ == theme) {
    return;
  }
  theme_ = theme;

  for (auto& observer : legacy_observers_) {
    observer.OnThemeChanged(theme_);
  }
}

AstraDevToolsTheme AstraDevToolsModel::GetEffectiveTheme() const {
  if (theme_ != AstraDevToolsTheme::kSystem) {
    return theme_;
  }
  // TODO(astra): Query NativeTheme for system dark/light setting.
  //   For now, default to dark (matches DevTools default).
  //   Chromium owner: ui/native_theme/native_theme.h
  return AstraDevToolsTheme::kDark;
}

// =========================================================================
// Legacy persistence
// =========================================================================

void AstraDevToolsModel::LoadFromPrefs() {
  if (!pref_service_) {
    return;
  }

  // Load legacy dock position.
  dock_position_ = StringToDockPosition(
      pref_service_->GetString(prefs::kPrefDevToolsDefaultDockState));

  // Load legacy theme.
  theme_ = StringToTheme(
      pref_service_->GetString(prefs::kPrefDevToolsTheme));

  // Load legacy panels from prefs if they exist.
  const base::Value::List& panel_list =
      pref_service_->GetList(prefs::kPrefDevToolsPanelList);
  if (!panel_list.empty()) {
    panels_.clear();
    for (const auto& entry : panel_list) {
      const base::Value::Dict* dict = entry.GetIfDict();
      if (!dict) continue;

      AstraDevToolsPanel panel;
      const std::string* id = dict->FindString(kPanelIdKey);
      if (!id || id->empty()) continue;
      panel.id = *id;

      const std::string* title = dict->FindString(kPanelTitleKey);
      panel.title = title ? *title : panel.id;

      const std::string* icon = dict->FindString(kPanelIconKey);
      panel.icon = icon ? *icon : "";

      panel.is_visible = dict->FindBool(kPanelVisibleKey).value_or(true);
      panel.is_pinned = dict->FindBool(kPanelPinnedKey).value_or(false);
      panel.position =
          static_cast<size_t>(dict->FindInt(kPanelPositionKey).value_or(0));

      panels_.push_back(panel);
    }
    RenormalizePositions();
  } else {
    panels_ = GetDefaultPanels();
  }

  // Set active panel (legacy).
  if (remember_last_panel() && !last_active_panel().empty() &&
      HasPanel(last_active_panel()) &&
      GetPanelById(last_active_panel())->is_visible) {
    legacy_active_panel_id_ = last_active_panel();
  } else if (!default_active_panel().empty() &&
             HasPanel(default_active_panel()) &&
             GetPanelById(default_active_panel())->is_visible) {
    legacy_active_panel_id_ = default_active_panel();
  } else {
    auto visible = GetVisiblePanels();
    if (!visible.empty()) {
      legacy_active_panel_id_ = visible.front().id;
    }
  }
}

void AstraDevToolsModel::SaveToPrefs() const {
  if (!pref_service_) {
    return;
  }

  base::Value::List panel_list;
  auto all_panels = GetAllPanels();
  for (const auto& panel : all_panels) {
    base::Value::Dict dict;
    dict.Set(kPanelIdKey, panel.id);
    dict.Set(kPanelTitleKey, panel.title);
    dict.Set(kPanelIconKey, panel.icon);
    dict.Set(kPanelVisibleKey, panel.is_visible);
    dict.Set(kPanelPinnedKey, panel.is_pinned);
    dict.Set(kPanelPositionKey, static_cast<int>(panel.position));
    panel_list.Append(std::move(dict));
  }
  pref_service_->Set(prefs::kPrefDevToolsPanelList,
                     base::Value(std::move(panel_list)));
}

// =========================================================================
// Legacy private helpers
// =========================================================================

std::vector<AstraDevToolsPanel>::iterator
AstraDevToolsModel::FindPanel(const std::string& panel_id) {
  return std::find_if(panels_.begin(), panels_.end(),
                      [&panel_id](const AstraDevToolsPanel& p) {
                        return p.id == panel_id;
                      });
}

std::vector<AstraDevToolsPanel>::const_iterator
AstraDevToolsModel::FindPanel(const std::string& panel_id) const {
  return std::find_if(panels_.begin(), panels_.end(),
                      [&panel_id](const AstraDevToolsPanel& p) {
                        return p.id == panel_id;
                      });
}

void AstraDevToolsModel::RenormalizePositions() {
  std::sort(panels_.begin(), panels_.end(),
            [](const AstraDevToolsPanel& a, const AstraDevToolsPanel& b) {
              return a.position < b.position;
            });
  for (size_t i = 0; i < panels_.size(); ++i) {
    panels_[i].position = i;
  }
}

void AstraDevToolsModel::NotifySettingsChanged() {
  SaveToPrefs();
  for (auto& observer : legacy_observers_) {
    observer.OnDevToolsSettingsChanged();
  }
}

void AstraDevToolsModel::NotifyPanelOrderChanged() {
  SaveToPrefs();
  for (auto& observer : legacy_observers_) {
    observer.OnPanelOrderChanged();
  }
}

}  // namespace astra
