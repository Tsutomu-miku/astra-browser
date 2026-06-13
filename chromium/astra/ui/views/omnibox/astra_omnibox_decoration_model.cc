#include "astra/ui/views/omnibox/astra_omnibox_decoration_model.h"

#include <algorithm>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Helper to get default action data by ID.
AstraDecorationAction MakeDefaultAction(const std::string& id,
                                        int position) {
  AstraDecorationAction action;
  action.id = id;
  action.position = position;
  action.is_visible = true;

  if (id == AstraOmniboxDecorationModel::kActionWorkspace) {
    action.label = u"Workspace";
    action.icon = "workspace";
    action.tooltip = u"Switch workspace";
    action.shortcut = u"⌘⇧W";
  } else if (id == AstraOmniboxDecorationModel::kActionFocusMode) {
    action.label = u"Focus";
    action.icon = "focus_mode";
    action.tooltip = u"Toggle focus mode";
    action.shortcut = u"⌘⇧F";
  } else if (id == AstraOmniboxDecorationModel::kActionScreenshot) {
    action.label = u"Screenshot";
    action.icon = "screenshot";
    action.tooltip = u"Take screenshot";
    action.shortcut = u"⌘⇧S";
  } else if (id == AstraOmniboxDecorationModel::kActionNote) {
    action.label = u"Note";
    action.icon = "note";
    action.tooltip = u"Quick note";
    action.shortcut = u"⌘⇧N";
  } else if (id == AstraOmniboxDecorationModel::kActionSplitView) {
    action.label = u"Split";
    action.icon = "split_view";
    action.tooltip = u"Toggle split view";
    action.shortcut = u"⌘⇧\\\\";
  } else if (id == AstraOmniboxDecorationModel::kActionReadingList) {
    action.label = u"Reading";
    action.icon = "reading_list";
    action.tooltip = u"Add to reading list";
    action.shortcut = u"⌘⇧D";
  } else if (id == AstraOmniboxDecorationModel::kActionTranslate) {
    action.label = u"Translate";
    action.icon = "translate";
    action.tooltip = u"Translate page";
    action.shortcut = u"⌘⇧T";
  } else if (id == AstraOmniboxDecorationModel::kActionShare) {
    action.label = u"Share";
    action.icon = "share";
    action.tooltip = u"Share page";
    action.shortcut = u"⌘⇧E";
  }

  return action;
}

}  // namespace

// =========================================================================
// AstraOmniboxDecorationModel — construction / destruction
// =========================================================================

AstraOmniboxDecorationModel::AstraOmniboxDecorationModel() {
  InitializeDefaultActions();
}

AstraOmniboxDecorationModel::~AstraOmniboxDecorationModel() = default;

// =========================================================================
// Default action order
// =========================================================================

std::vector<std::string> AstraOmniboxDecorationModel::GetDefaultActionOrder() {
  return {
      kActionWorkspace, kActionFocusMode,  kActionScreenshot, kActionNote,
      kActionSplitView, kActionReadingList, kActionTranslate,  kActionShare,
  };
}

void AstraOmniboxDecorationModel::InitializeDefaultActions() {
  actions_.clear();
  auto order = GetDefaultActionOrder();
  for (size_t i = 0; i < order.size(); ++i) {
    actions_.push_back(MakeDefaultAction(order[i], static_cast<int>(i)));
  }
}

// =========================================================================
// Action management
// =========================================================================

bool AstraOmniboxDecorationModel::AddAction(
    const AstraDecorationAction& action) {
  if (HasAction(action.id)) {
    return false;
  }
  actions_.push_back(action);
  // Ensure position is set properly.
  actions_.back().position = static_cast<int>(actions_.size() - 1);

  for (auto& observer : observers_) {
    observer.OnActionAdded(action.id);
  }
  return true;
}

bool AstraOmniboxDecorationModel::RemoveAction(const std::string& action_id) {
  int index = FindActionIndex(action_id);
  if (index < 0) {
    return false;
  }
  actions_.erase(actions_.begin() + index);
  // Recompute positions.
  for (size_t i = 0; i < actions_.size(); ++i) {
    actions_[i].position = static_cast<int>(i);
  }

  for (auto& observer : observers_) {
    observer.OnActionRemoved(action_id);
  }
  return true;
}

bool AstraOmniboxDecorationModel::SetActionVisible(
    const std::string& action_id,
    bool visible) {
  int index = FindActionIndex(action_id);
  if (index < 0) {
    return false;
  }
  if (actions_[index].is_visible == visible) {
    return true;  // No change, but success.
  }
  actions_[index].is_visible = visible;

  for (auto& observer : observers_) {
    observer.OnActionVisibilityChanged(action_id, visible);
  }
  return true;
}

bool AstraOmniboxDecorationModel::HasAction(
    const std::string& action_id) const {
  return FindActionIndex(action_id) >= 0;
}

const AstraDecorationAction* AstraOmniboxDecorationModel::GetAction(
    const std::string& action_id) const {
  int index = FindActionIndex(action_id);
  if (index < 0) {
    return nullptr;
  }
  return &actions_[index];
}

std::vector<AstraDecorationAction>
AstraOmniboxDecorationModel::GetVisibleActions() const {
  std::vector<AstraDecorationAction> visible;
  for (const auto& action : actions_) {
    if (action.is_visible) {
      visible.push_back(action);
    }
  }
  return visible;
}

size_t AstraOmniboxDecorationModel::GetVisibleActionCount() const {
  size_t count = 0;
  for (const auto& action : actions_) {
    if (action.is_visible) {
      ++count;
    }
  }
  return count;
}

int AstraOmniboxDecorationModel::FindActionIndex(
    const std::string& action_id) const {
  for (size_t i = 0; i < actions_.size(); ++i) {
    if (actions_[i].id == action_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

// =========================================================================
// Action ordering
// =========================================================================

bool AstraOmniboxDecorationModel::ReorderAction(size_t from_index,
                                                size_t to_index) {
  if (from_index >= actions_.size() || to_index >= actions_.size()) {
    return false;
  }
  if (from_index == to_index) {
    return true;  // No-op success.
  }

  AstraDecorationAction action = std::move(actions_[from_index]);
  actions_.erase(actions_.begin() + from_index);
  actions_.insert(actions_.begin() + to_index, std::move(action));

  // Update position values.
  for (size_t i = 0; i < actions_.size(); ++i) {
    actions_[i].position = static_cast<int>(i);
  }

  for (auto& observer : observers_) {
    observer.OnActionOrderChanged();
  }
  return true;
}

bool AstraOmniboxDecorationModel::MoveActionTo(const std::string& action_id,
                                               size_t index) {
  int from = FindActionIndex(action_id);
  if (from < 0) {
    return false;
  }
  return ReorderAction(static_cast<size_t>(from), index);
}

void AstraOmniboxDecorationModel::ResetActionOrder() {
  InitializeDefaultActions();
  // Re-apply individual visibility settings.
  SyncActionVisibilityFromSettings();

  for (auto& observer : observers_) {
    observer.OnActionOrderChanged();
  }
}

// =========================================================================
// Omnibox state
// =========================================================================

void AstraOmniboxDecorationModel::SetOmniboxFocused(bool focused) {
  if (omnibox_focused_ == focused) {
    return;
  }
  omnibox_focused_ = focused;

  for (auto& observer : observers_) {
    observer.OnOmniboxFocusChanged(focused);
  }
}

void AstraOmniboxDecorationModel::SetSecurityLevel(AstraSecurityLevel level) {
  if (security_level_ == level) {
    return;
  }
  security_level_ = level;

  for (auto& observer : observers_) {
    observer.OnSecurityStateChanged(level);
  }
}

// =========================================================================
// Presentation settings
// =========================================================================

void AstraOmniboxDecorationModel::SetShowDecoration(bool show) {
  if (show_decoration_ == show) {
    return;
  }
  show_decoration_ = show;
  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SetPosition(AstraDecorationPosition pos) {
  if (position_ == pos) {
    return;
  }
  position_ = pos;
  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SetMaxVisibleActions(int max) {
  int clamped = ClampMaxVisibleActions(max);
  if (max_visible_actions_ == clamped) {
    return;
  }
  max_visible_actions_ = clamped;
  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SetShowLabels(bool show) {
  if (show_labels_ == show) {
    return;
  }
  show_labels_ = show;
  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SetIconSize(AstraDecorationIconSize size) {
  if (icon_size_ == size) {
    return;
  }
  icon_size_ = size;
  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SetButtonStyle(
    AstraDecorationButtonStyle style) {
  if (button_style_ == style) {
    return;
  }
  button_style_ = style;
  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SetShowOnFocusOnly(bool show) {
  if (show_on_focus_only_ == show) {
    return;
  }
  show_on_focus_only_ = show;
  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SetShowWorkspace(bool show) {
  if (show_workspace_ == show) {
    return;
  }
  show_workspace_ = show;
  SetActionVisible(kActionWorkspace, show);
}

void AstraOmniboxDecorationModel::SetShowFocusMode(bool show) {
  if (show_focus_mode_ == show) {
    return;
  }
  show_focus_mode_ = show;
  SetActionVisible(kActionFocusMode, show);
}

void AstraOmniboxDecorationModel::SetShowScreenshot(bool show) {
  if (show_screenshot_ == show) {
    return;
  }
  show_screenshot_ = show;
  SetActionVisible(kActionScreenshot, show);
}

void AstraOmniboxDecorationModel::SetShowNote(bool show) {
  if (show_note_ == show) {
    return;
  }
  show_note_ = show;
  SetActionVisible(kActionNote, show);
}

void AstraOmniboxDecorationModel::SetShowSplitView(bool show) {
  if (show_split_view_ == show) {
    return;
  }
  show_split_view_ = show;
  SetActionVisible(kActionSplitView, show);
}

void AstraOmniboxDecorationModel::SetShowReadingList(bool show) {
  if (show_reading_list_ == show) {
    return;
  }
  show_reading_list_ = show;
  SetActionVisible(kActionReadingList, show);
}

void AstraOmniboxDecorationModel::SetShowTranslate(bool show) {
  if (show_translate_ == show) {
    return;
  }
  show_translate_ = show;
  SetActionVisible(kActionTranslate, show);
}

void AstraOmniboxDecorationModel::SetShowShare(bool show) {
  if (show_share_ == show) {
    return;
  }
  show_share_ = show;
  SetActionVisible(kActionShare, show);
}

void AstraOmniboxDecorationModel::SetShowOverflowMenu(bool show) {
  if (show_overflow_menu_ == show) {
    return;
  }
  show_overflow_menu_ = show;
  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SetHoverExpansion(bool enabled) {
  if (hover_expansion_ == enabled) {
    return;
  }
  hover_expansion_ = enabled;
  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SyncActionVisibilityFromSettings() {
  SetActionVisible(kActionWorkspace, show_workspace_);
  SetActionVisible(kActionFocusMode, show_focus_mode_);
  SetActionVisible(kActionScreenshot, show_screenshot_);
  SetActionVisible(kActionNote, show_note_);
  SetActionVisible(kActionSplitView, show_split_view_);
  SetActionVisible(kActionReadingList, show_reading_list_);
  SetActionVisible(kActionTranslate, show_translate_);
  SetActionVisible(kActionShare, show_share_);
}

void AstraOmniboxDecorationModel::NotifySettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnDecorationSettingsChanged();
  }
}

// =========================================================================
// Utility methods
// =========================================================================

std::u16string AstraOmniboxDecorationModel::FormatActionLabel(
    const std::u16string& label,
    size_t max_length) {
  if (label.size() <= max_length) {
    return label;
  }
  return label.substr(0, max_length) + u"\u2026";  // Ellipsis
}

int AstraOmniboxDecorationModel::ClampMaxVisibleActions(int value) {
  return std::clamp(value, kMinVisibleActions, kMaxVisibleActions);
}

int AstraOmniboxDecorationModel::GetIconSizeDp(AstraDecorationIconSize size) {
  switch (size) {
    case AstraDecorationIconSize::kSmall:
      return 16;
    case AstraDecorationIconSize::kMedium:
      return 20;
    case AstraDecorationIconSize::kLarge:
      return 24;
  }
  return 20;  // Default fallback.
}

// =========================================================================
// Bulk operations
// =========================================================================

void AstraOmniboxDecorationModel::SetBulkVisibility(
    const std::vector<std::string>& action_ids,
    bool visible) {
  for (const auto& id : action_ids) {
    SetActionVisible(id, visible);
  }
}

void AstraOmniboxDecorationModel::ShowAllDefaultActions() {
  show_workspace_ = true;
  show_focus_mode_ = true;
  show_screenshot_ = true;
  show_note_ = true;
  show_split_view_ = true;
  show_reading_list_ = true;
  show_translate_ = true;
  show_share_ = true;

  for (auto& action : actions_) {
    if (!action.is_visible) {
      action.is_visible = true;
      for (auto& observer : observers_) {
        observer.OnActionVisibilityChanged(action.id, true);
      }
    }
  }
}

void AstraOmniboxDecorationModel::HideAllActions() {
  show_workspace_ = false;
  show_focus_mode_ = false;
  show_screenshot_ = false;
  show_note_ = false;
  show_split_view_ = false;
  show_reading_list_ = false;
  show_translate_ = false;
  show_share_ = false;

  for (auto& action : actions_) {
    if (action.is_visible) {
      action.is_visible = false;
      for (auto& observer : observers_) {
        observer.OnActionVisibilityChanged(action.id, false);
      }
    }
  }
}

// =========================================================================
// Persistence
// =========================================================================

void AstraOmniboxDecorationModel::LoadFromPrefs(PrefService* prefs) {
  if (!prefs) {
    return;
  }

  using namespace prefs;

  show_decoration_ = prefs->GetBoolean(kPrefOmniboxDecorationShowDecoration);

  std::string pos = prefs->GetString(kPrefOmniboxDecorationPosition);
  position_ = (pos == "right") ? AstraDecorationPosition::kTrailing
                               : AstraDecorationPosition::kLeading;

  max_visible_actions_ = ClampMaxVisibleActions(
      prefs->GetInteger(kPrefOmniboxDecorationMaxVisibleActions));

  show_labels_ = prefs->GetBoolean(kPrefOmniboxDecorationShowLabels);

  int icon_size_val = prefs->GetInteger(kPrefOmniboxDecorationIconSize);
  icon_size_ = static_cast<AstraDecorationIconSize>(
      std::clamp(icon_size_val, 0, 2));

  int style_val = prefs->GetInteger(kPrefOmniboxDecorationButtonStyle);
  button_style_ = static_cast<AstraDecorationButtonStyle>(
      std::clamp(style_val, 0, 2));

  show_on_focus_only_ = prefs->GetBoolean(kPrefOmniboxDecorationShowOnFocusOnly);
  show_workspace_ = prefs->GetBoolean(kPrefOmniboxDecorationShowWorkspace);
  show_focus_mode_ = prefs->GetBoolean(kPrefOmniboxDecorationShowFocusMode);
  show_screenshot_ = prefs->GetBoolean(kPrefOmniboxDecorationShowScreenshot);
  show_note_ = prefs->GetBoolean(kPrefOmniboxDecorationShowNote);
  show_split_view_ = prefs->GetBoolean(kPrefOmniboxDecorationShowSplitView);
  show_reading_list_ = prefs->GetBoolean(kPrefOmniboxDecorationShowReadingList);
  show_translate_ = prefs->GetBoolean(kPrefOmniboxDecorationShowTranslate);
  show_share_ = prefs->GetBoolean(kPrefOmniboxDecorationShowShare);
  show_overflow_menu_ = prefs->GetBoolean(kPrefOmniboxDecorationOverflowMenu);
  hover_expansion_ = prefs->GetBoolean(kPrefOmniboxDecorationHoverExpansion);

  // Sync visibility flags to action state.
  SyncActionVisibilityFromSettings();

  NotifySettingsChanged();
}

void AstraOmniboxDecorationModel::SaveToPrefs(PrefService* prefs) const {
  if (!prefs) {
    return;
  }

  using namespace prefs;

  prefs->SetBoolean(kPrefOmniboxDecorationShowDecoration, show_decoration_);
  prefs->SetString(kPrefOmniboxDecorationPosition,
                   position_ == AstraDecorationPosition::kLeading ? "left"
                                                                  : "right");
  prefs->SetInteger(kPrefOmniboxDecorationMaxVisibleActions,
                    max_visible_actions_);
  prefs->SetBoolean(kPrefOmniboxDecorationShowLabels, show_labels_);
  prefs->SetInteger(kPrefOmniboxDecorationIconSize,
                    static_cast<int>(icon_size_));
  prefs->SetInteger(kPrefOmniboxDecorationButtonStyle,
                    static_cast<int>(button_style_));
  prefs->SetBoolean(kPrefOmniboxDecorationShowOnFocusOnly, show_on_focus_only_);
  prefs->SetBoolean(kPrefOmniboxDecorationShowWorkspace, show_workspace_);
  prefs->SetBoolean(kPrefOmniboxDecorationShowFocusMode, show_focus_mode_);
  prefs->SetBoolean(kPrefOmniboxDecorationShowScreenshot, show_screenshot_);
  prefs->SetBoolean(kPrefOmniboxDecorationShowNote, show_note_);
  prefs->SetBoolean(kPrefOmniboxDecorationShowSplitView, show_split_view_);
  prefs->SetBoolean(kPrefOmniboxDecorationShowReadingList, show_reading_list_);
  prefs->SetBoolean(kPrefOmniboxDecorationShowTranslate, show_translate_);
  prefs->SetBoolean(kPrefOmniboxDecorationShowShare, show_share_);
  prefs->SetBoolean(kPrefOmniboxDecorationOverflowMenu, show_overflow_menu_);
  prefs->SetBoolean(kPrefOmniboxDecorationHoverExpansion, hover_expansion_);
}

// =========================================================================
// Observers
// =========================================================================

void AstraOmniboxDecorationModel::AddObserver(
    AstraOmniboxDecorationModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraOmniboxDecorationModel::RemoveObserver(
    AstraOmniboxDecorationModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace astra
