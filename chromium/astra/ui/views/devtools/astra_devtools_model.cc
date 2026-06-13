#include "astra/ui/views/devtools/astra_devtools_model.h"

#include <algorithm>

#include "base/logging.h"
#include "base/values.h"
#include "astra/browser/astra_prefs.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// Helper: convert dock position enum to/from string for prefs.
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
  return AstraDevToolsDockPosition::kBottom;  // default
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
  return AstraDevToolsTheme::kSystem;  // default
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
  return AstraDevToolsPanelPosition::kRight;  // default
}

// Panel state dictionary keys.
constexpr char kPanelIdKey[] = "id";
constexpr char kPanelTitleKey[] = "title";
constexpr char kPanelIconKey[] = "icon";
constexpr char kPanelVisibleKey[] = "visible";
constexpr char kPanelPinnedKey[] = "pinned";
constexpr char kPanelPositionKey[] = "position";

}  // namespace

// =========================================================================
// Static helpers: default panels
// =========================================================================

// static
std::vector<AstraDevToolsPanel> AstraDevToolsModel::GetDefaultPanels() {
  std::vector<AstraDevToolsPanel> panels;

  // Default panel ordering — pinned panels first, then regular ones.
  // Position indices are assigned sequentially.

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
  // Start with default panels.
  panels_ = GetDefaultPanels();

  // Load from prefs if available.
  if (pref_service_) {
    LoadFromPrefs();
  } else {
    // No pref service — use defaults and set first visible panel as active.
    auto visible = GetVisiblePanels();
    if (!visible.empty()) {
      active_panel_id_ = visible.front().id;
    }
  }
}

AstraDevToolsModel::~AstraDevToolsModel() = default;

// =========================================================================
// Observer management
// =========================================================================

void AstraDevToolsModel::AddObserver(
    AstraDevToolsModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraDevToolsModel::RemoveObserver(
    AstraDevToolsModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Panel management — queries
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

// =========================================================================
// Panel management — mutations
// =========================================================================

bool AstraDevToolsModel::AddPanel(const AstraDevToolsPanel& panel) {
  if (panel.id.empty()) {
    return false;
  }
  if (HasPanel(panel.id)) {
    return false;
  }

  // Clamp position to valid range.
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

  // If we removed the active panel, clear it.
  if (active_panel_id_ == panel_id) {
    active_panel_id_.clear();
    // Try to activate the next visible panel.
    auto visible = GetVisiblePanels();
    if (!visible.empty()) {
      // Find a panel near the removed position.
      for (const auto& p : visible) {
        if (p.position >= removed_position ||
            p.position == visible.back().position) {
          active_panel_id_ = p.id;
          break;
        }
      }
    }
    for (auto& observer : observers_) {
      observer.OnActivePanelChanged(active_panel_id_);
    }
  }

  RenormalizePositions();
  NotifyPanelOrderChanged();
  return true;
}

bool AstraDevToolsModel::SetPanelVisible(
    const std::string& panel_id,
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
    for (auto& observer : observers_) {
      observer.OnPanelOpened(panel_id);
    }
  } else {
    // If we're hiding the active panel, activate another one.
    if (active_panel_id_ == panel_id) {
      auto visible_panels = GetVisiblePanels();
      if (!visible_panels.empty()) {
        active_panel_id_ = visible_panels.front().id;
        for (auto& observer : observers_) {
          observer.OnActivePanelChanged(active_panel_id_);
        }
      } else {
        active_panel_id_.clear();
        for (auto& observer : observers_) {
          observer.OnActivePanelChanged(std::string());
        }
      }
    }
    for (auto& observer : observers_) {
      observer.OnPanelClosed(panel_id);
    }
  }

  SaveToPrefs();
  return true;
}

bool AstraDevToolsModel::SetPanelPinned(
    const std::string& panel_id,
    bool pinned) {
  auto it = FindPanel(panel_id);
  if (it == panels_.end()) {
    return false;
  }
  if (it->is_pinned == pinned) {
    return true;
  }

  it->is_pinned = pinned;
  // TODO(astra): Reorder panels so pinned ones come first.
  //   For now, we just update the flag and let position stay the same.
  //   A future improvement would sort: pinned panels first, by position;
  //   then unpinned panels, by position.
  NotifyPanelOrderChanged();
  return true;
}

bool AstraDevToolsModel::ReorderPanel(
    const std::string& panel_id,
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
    // Moving later: shift panels between old+1 and new down by 1.
    for (auto& panel : panels_) {
      if (&panel != &(*it) &&
          panel.position > old_position &&
          panel.position <= clamped_position) {
        panel.position -= 1;
      }
    }
  } else {
    // Moving earlier: shift panels between new and old-1 up by 1.
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
    return false;  // Already at the beginning.
  }
  return ReorderPanel(panel_id, it->position - 1);
}

bool AstraDevToolsModel::MovePanelLater(const std::string& panel_id) {
  auto it = FindPanel(panel_id);
  if (it == panels_.end()) {
    return false;
  }
  if (it->position >= panels_.size() - 1) {
    return false;  // Already at the end.
  }
  return ReorderPanel(panel_id, it->position + 1);
}

void AstraDevToolsModel::ResetPanelsToDefaults() {
  panels_ = GetDefaultPanels();

  // Set active panel based on preferences.
  if (remember_last_panel() && !last_active_panel().empty() &&
      HasPanel(last_active_panel())) {
    active_panel_id_ = last_active_panel();
  } else if (!default_active_panel().empty() &&
             HasPanel(default_active_panel())) {
    active_panel_id_ = default_active_panel();
  } else {
    // Default to the first visible panel.
    auto visible = GetVisiblePanels();
    if (!visible.empty()) {
      active_panel_id_ = visible.front().id;
    }
  }

  NotifyPanelOrderChanged();
  if (!active_panel_id_.empty()) {
    for (auto& observer : observers_) {
      observer.OnActivePanelChanged(active_panel_id_);
    }
  }
}

// =========================================================================
// Active panel
// =========================================================================

bool AstraDevToolsModel::SetActivePanel(const std::string& panel_id) {
  if (panel_id == active_panel_id_) {
    return true;
  }

  const AstraDevToolsPanel* panel = GetPanelById(panel_id);
  if (!panel || !panel->is_visible) {
    return false;
  }

  active_panel_id_ = panel_id;

  if (remember_last_panel()) {
    SetLastActivePanel(panel_id);
  }

  for (auto& observer : observers_) {
    observer.OnActivePanelChanged(active_panel_id_);
  }
  return true;
}

void AstraDevToolsModel::ActivateNextPanel() {
  auto visible = GetVisiblePanels();
  if (visible.empty()) {
    return;
  }

  // Find current index.
  size_t current_index = 0;
  for (size_t i = 0; i < visible.size(); ++i) {
    if (visible[i].id == active_panel_id_) {
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

  // Find current index.
  size_t current_index = 0;
  for (size_t i = 0; i < visible.size(); ++i) {
    if (visible[i].id == active_panel_id_) {
      current_index = i;
      break;
    }
  }

  size_t prev_index =
      (current_index == 0) ? visible.size() - 1 : current_index - 1;
  SetActivePanel(visible[prev_index].id);
}

// =========================================================================
// Presentation settings
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
  // Note: this doesn't trigger OnDevToolsSettingsChanged since it's
  // an automatic side-effect of panel switching, not a user setting change.
}

// =========================================================================
// Dock position
// =========================================================================

void AstraDevToolsModel::SetDockPosition(
    AstraDevToolsDockPosition position) {
  if (dock_position_ == position) {
    return;
  }
  dock_position_ = position;

  // Persist to prefs if available.
  if (pref_service_) {
    pref_service_->SetString(prefs::kPrefDevToolsDefaultDockState,
                             DockPositionToString(position));
  }

  for (auto& observer : observers_) {
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
// Theme
// =========================================================================

void AstraDevToolsModel::SetTheme(AstraDevToolsTheme theme) {
  if (theme_ == theme) {
    return;
  }
  theme_ = theme;

  for (auto& observer : observers_) {
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
  //   Pattern: NativeTheme::GetInstance()->ShouldUseDarkColors()
  return AstraDevToolsTheme::kDark;
}

// =========================================================================
// Persistence — LoadFromPrefs / SaveToPrefs
// =========================================================================

void AstraDevToolsModel::LoadFromPrefs() {
  if (!pref_service_) {
    return;
  }

  // Load dock position.
  dock_position_ = StringToDockPosition(
      pref_service_->GetString(prefs::kPrefDevToolsDefaultDockState));

  // Load theme.
  theme_ = StringToTheme(
      pref_service_->GetString(prefs::kPrefDevToolsTheme));

  // Load panels from prefs if they exist.
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
    // No persisted panels — use defaults.
    panels_ = GetDefaultPanels();
  }

  // Set active panel.
  if (remember_last_panel() && !last_active_panel().empty() &&
      HasPanel(last_active_panel()) &&
      GetPanelById(last_active_panel())->is_visible) {
    active_panel_id_ = last_active_panel();
  } else if (!default_active_panel().empty() &&
             HasPanel(default_active_panel()) &&
             GetPanelById(default_active_panel())->is_visible) {
    active_panel_id_ = default_active_panel();
  } else {
    // Default to first visible panel.
    auto visible = GetVisiblePanels();
    if (!visible.empty()) {
      active_panel_id_ = visible.front().id;
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
// Private helpers
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
  for (auto& observer : observers_) {
    observer.OnDevToolsSettingsChanged();
  }
}

void AstraDevToolsModel::NotifyPanelOrderChanged() {
  SaveToPrefs();
  for (auto& observer : observers_) {
    observer.OnPanelOrderChanged();
  }
}

}  // namespace astra
