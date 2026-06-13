#include "astra/ui/views/newtab/astra_new_tab_model.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "third_party/skia/include/core/SkColor.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Helper to parse a shortcut from a base::Value::Dict.
AstraNtpShortcutInfo ShortcutFromDict(const base::Value::Dict& dict) {
  AstraNtpShortcutInfo info;
  const std::string* title_str = dict.FindString("title").value_or(std::string());
  info.title = base::UTF8ToUTF16(title_str);
  const std::string* url_str = dict.FindString("url").value_or(&std::string());
  info.url = GURL(*url_str);
  const std::string* icon_str = dict.FindString("icon_url").value_or(&std::string());
  info.icon_url = GURL(*icon_str);
  info.is_custom = dict.FindBool("is_custom").value_or(true);
  info.order_index = dict.FindInt("order_index").value_or(0);
  return info;
}

// Helper to serialize a shortcut to a base::Value::Dict.
base::Value::Dict ShortcutToDict(const AstraNtpShortcutInfo& info) {
  base::Value::Dict dict;
  dict.Set("title", base::UTF16ToUTF8(info.title));
  dict.Set("url", info.url.spec());
  dict.Set("icon_url", info.icon_url.spec());
  dict.Set("is_custom", info.is_custom);
  dict.Set("order_index", info.order_index);
  return dict;
}

// Helper to parse a recently closed tab from a base::Value::Dict.
AstraNtpRecentlyClosedTab RecentlyClosedFromDict(const base::Value::Dict& dict) {
  AstraNtpRecentlyClosedTab tab;
  const std::string* title_str = dict.FindString("title").value_or(std::string());
  tab.title = base::UTF8ToUTF16(title_str);
  const std::string* url_str = dict.FindString("url").value_or(&std::string());
  tab.url = GURL(*url_str);
  const std::string* favicon_str =
      dict.FindString("favicon_url").value_or(&std::string());
  tab.favicon_url = GURL(*favicon_str);
  double close_time = dict.FindDouble("close_time").value_or(0.0);
  tab.close_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(static_cast<int64_t>(close_time)));
  tab.session_id = dict.FindInt("session_id").value_or(-1);
  return tab;
}

// Helper to serialize a recently closed tab to a base::Value::Dict.
base::Value::Dict RecentlyClosedToDict(const AstraNtpRecentlyClosedTab& tab) {
  base::Value::Dict dict;
  dict.Set("title", base::UTF16ToUTF8(tab.title));
  dict.Set("url", tab.url.spec());
  dict.Set("favicon_url", tab.favicon_url.spec());
  dict.Set("close_time",
         static_cast<double>(tab.close_time.ToDeltaSinceWindowsEpoch()
                                 .InMicroseconds()));
  dict.Set("session_id", tab.session_id);
  return dict;
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraNewTabModel::AstraNewTabModel() = default;

AstraNewTabModel::AstraNewTabModel(PrefService* prefs) {
  if (prefs) {
    LoadFromPrefs(prefs);
  }
}

AstraNewTabModel::~AstraNewTabModel() = default;

// =========================================================================
// Persistence
// =========================================================================

void AstraNewTabModel::LoadFromPrefs(PrefService* prefs) {
  if (!prefs) {
    return;
  }

  // Load visibility settings.
  show_greeting_ = prefs->GetBoolean(prefs::kPrefNtpShowGreeting);
  show_search_bar_ = prefs->GetBoolean(prefs::kPrefNtpShowSearchBar);
  show_workspace_cards_ = prefs->GetBoolean(prefs::kPrefNtpShowWorkspaceCards);
  show_shortcuts_ = prefs->GetBoolean(prefs::kPrefNtpShowShortcuts);
  show_recently_closed_ = prefs->GetBoolean(prefs::kPrefNtpShowRecentlyClosed);
  show_quick_actions_ = prefs->GetBoolean(prefs::kPrefNtpShowQuickActions);

  // Load layout settings.
  shortcut_columns_ = ClampInt(
      prefs->GetInteger(prefs::kPrefNtpShortcutColumns),
      kMinShortcutColumns, kMaxShortcutColumns);
  max_workspaces_shown_ = ClampInt(
      prefs->GetInteger(prefs::kPrefNtpMaxWorkspacesShown),
      kMinMaxWorkspacesShown, kMaxMaxWorkspacesShown);
  max_recently_closed_shown_ = ClampInt(
      prefs->GetInteger(prefs::kPrefNtpMaxRecentlyClosedShown),
      kMinMaxRecentlyClosedShown, kMaxMaxRecentlyClosedShown);

  // Load layout modes.
  int layout_mode_int = prefs->GetInteger(prefs::kPrefNtpShortcutLayoutMode);
  shortcut_layout_mode_ =
      static_cast<AstraNtpShortcutLayoutMode>(std::max(0, std::min(1, layout_mode_int)));

  int card_style_int = prefs->GetInteger(prefs::kPrefNtpWorkspaceCardStyle);
  workspace_card_style_ =
      static_cast<AstraNtpWorkspaceCardStyle>(std::max(0, std::min(1, card_style_int)));

  int bg_style_int = prefs->GetInteger(prefs::kPrefNtpBackgroundStyle);
  background_style_ =
      static_cast<Astra::AstraNtpBackgroundStyle>(std::max(0, std::min(2, bg_style_int)));

  // Load background URL.
  custom_background_url_ = prefs->GetString(prefs::kPrefNtpCustomBackgroundUrl);

  // Load other settings.
  show_most_visited_ = prefs->GetBoolean(prefs::kPrefNtpShowMostVisited);

  int greeting_style_int = prefs->GetInteger(prefs::kPrefNtpGreetingStyle);
  greeting_style_ =
      static_cast<AstraNtpGreetingStyle>(std::max(0, std::min(2, greeting_style_int)));

  // Load custom shortcuts.
  const base::Value::List& shortcut_list =
      prefs->GetList(prefs::kPrefNtpCustomShortcuts);
  shortcuts_.clear();
  for (const auto& entry : shortcut_list) {
    if (entry.is_dict()) {
      shortcuts_.push_back(ShortcutFromDict(entry.GetDict()));
    }
  }

  // Load quick actions.
  const base::Value::List& action_list =
      prefs->GetList(prefs::kPrefNtpQuickActions);
  quick_actions_.clear();
  // TODO(astra): Load full quick action data (label, icon, etc.) from prefs.
  // Currently we only load action IDs; labels and icons are set by the controller.
  for (size_t i = 0; i < action_list.size(); ++i) {
    const std::string* id = action_list[i].GetIfString();
    if (id) {
      AstraNtpQuickAction action;
      action.id = *id;
      action.order_index = static_cast<int>(i);
      action.is_enabled = true;
      quick_actions_.push_back(action);
    }
  }

  // Load recently closed cache.
  const base::Value::List& rc_list =
      prefs->GetList(prefs::kPrefNtpRecentlyClosed);
  recently_closed_.clear();
  for (const auto& entry : rc_list) {
    if (entry.is_dict()) {
      recently_closed_.push_back(RecentlyClosedFromDict(entry.GetDict()));
    }
  }

  // Notify observers that settings changed.
  NotifyNtpSettingsChanged();
  NotifyShortcutsChanged();
  NotifyQuickActionsChanged();
  NotifyRecentlyClosedChanged();
}

void AstraNewTabModel::SaveToPrefs(PrefService* prefs) const {
  if (!prefs) {
    return;
  }

  // Save visibility settings.
  prefs->SetBoolean(prefs::kPrefNtpShowGreeting, show_greeting_);
  prefs->SetBoolean(prefs::kPrefNtpShowSearchBar, show_search_bar_);
  prefs->SetBoolean(prefs::kPrefNtpShowWorkspaceCards, show_workspace_cards_);
  prefs->SetBoolean(prefs::kPrefNtpShowShortcuts, show_shortcuts_);
  prefs->SetBoolean(prefs::kPrefNtpShowRecentlyClosed, show_recently_closed_);
  prefs->SetBoolean(prefs::kPrefNtpShowQuickActions, show_quick_actions_);

  // Save layout settings.
  prefs->SetInteger(prefs::kPrefNtpShortcutColumns, shortcut_columns_);
  prefs->SetInteger(prefs::kPrefNtpMaxWorkspacesShown, max_workspaces_shown_);
  prefs->SetInteger(prefs::kPrefNtpMaxRecentlyClosedShown,
                    max_recently_closed_shown_);

  // Save layout modes.
  prefs->SetInteger(prefs::kPrefNtpShortcutLayoutMode,
                    static_cast<int>(shortcut_layout_mode_));
  prefs->SetInteger(prefs::kPrefNtpWorkspaceCardStyle,
                    static_cast<int>(workspace_card_style_));
  prefs->SetInteger(prefs::kPrefNtpBackgroundStyle,
                    static_cast<int>(background_style_));

  // Save background URL.
  prefs->SetString(prefs::kPrefNtpCustomBackgroundUrl, custom_background_url_);

  // Save other settings.
  prefs->SetBoolean(prefs::kPrefNtpShowMostVisited, show_most_visited_);
  prefs->SetInteger(prefs::kPrefNtpGreetingStyle,
                    static_cast<int>(greeting_style_));

  // Save custom shortcuts (only custom ones).
  base::Value::List shortcut_list;
  for (const auto& shortcut : shortcuts_) {
    if (shortcut.is_custom) {
      shortcut_list.Append(ShortcutToDict(shortcut));
    }
  }
  prefs->SetList(prefs::kPrefNtpCustomShortcuts, std::move(shortcut_list));

  // Save quick action IDs.
  base::Value::List action_list;
  for (const auto& action : quick_actions_) {
    action_list.Append(action.id);
  }
  prefs->SetList(prefs::kPrefNtpQuickActions, std::move(action_list));

  // Save recently closed cache.
  base::Value::List rc_list;
  for (const auto& tab : recently_closed_) {
    rc_list.Append(RecentlyClosedToDict(tab));
  }
  prefs->SetList(prefs::kPrefNtpRecentlyClosed, std::move(rc_list));
}

// =========================================================================
// Shortcut management
// =========================================================================

const AstraNtpShortcutInfo* AstraNewTabModel::GetShortcutAt(size_t index) const {
  if (index >= shortcuts_.size()) {
    return nullptr;
  }
  return &shortcuts_[index];
}

int AstraNewTabModel::FindShortcutByUrl(const GURL& url) const {
  for (size_t i = 0; i < shortcuts_.size(); ++i) {
    if (shortcuts_[i].url == url) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

size_t AstraNewTabModel::AddCustomShortcut(const std::u16string& title,
                                           const GURL& url) {
  return AddCustomShortcut(title, url, GURL());
}

size_t AstraNewTabModel::AddCustomShortcut(const std::u16string& title,
                                           const GURL& url,
                                           const GURL& icon_url) {
  AstraNtpShortcutInfo info;
  info.title = title;
  info.url = url;
  info.icon_url = icon_url;
  info.is_custom = true;
  info.order_index = static_cast<int>(shortcuts_.size());
  shortcuts_.push_back(info);
  NotifyShortcutsChanged();
  return shortcuts_.size() - 1;
}

bool AstraNewTabModel::RemoveShortcutAt(size_t index) {
  if (index >= shortcuts_.size()) {
    return false;
  }
  shortcuts_.erase(shortcuts_.begin() + index);
  // Recompute order indices.
  for (size_t i = index; i < shortcuts_.size(); ++i) {
    shortcuts_[i].order_index = static_cast<int>(i);
  }
  NotifyShortcutsChanged();
  return true;
}

bool AstraNewTabModel::RemoveShortcutByUrl(const GURL& url) {
  int index = FindShortcutByUrl(url);
  if (index < 0) {
    return false;
  }
  return RemoveShortcutAt(static_cast<size_t>(index));
}

bool AstraNewTabModel::UpdateShortcutAt(size_t index,
                                       const std::u16string& title,
                                       const GURL& url) {
  if (index >= shortcuts_.size()) {
    return false;
  }
  shortcuts_[index].title = title;
  shortcuts_[index].url = url;
  NotifyShortcutsChanged();
  return true;
}

bool AstraNewTabModel::ReorderShortcuts(
    const std::vector<size_t>& ordered_indices) {
  if (ordered_indices.size() != shortcuts_.size()) {
    return false;
  }

  // Validate that ordered_indices is a valid permutation.
  std::set<size_t> seen;
  for (size_t idx : ordered_indices) {
    if (idx >= shortcuts_.size() || seen.count(idx) > 0) {
      return false;
    }
    seen.insert(idx);
  }

  // Build the new order.
  std::vector<AstraNtpShortcutInfo> new_order;
  new_order.reserve(shortcuts_.size());
  for (size_t idx : ordered_indices) {
    new_order.push_back(shortcuts_[idx]);
  }

  shortcuts_ = std::move(new_order);

  // Recompute order indices.
  for (size_t i = 0; i < shortcuts_.size(); ++i) {
    shortcuts_[i].order_index = static_cast<int>(i);
  }

  NotifyShortcutsChanged();
  return true;
}

bool AstraNewTabModel::MoveShortcut(size_t from_index, size_t to_index) {
  if (from_index >= shortcuts_.size() || to_index >= shortcuts_.size()) {
    return false;
  }
  if (from_index == to_index) {
    return true;  // No-op.
  }

  AstraNtpShortcutInfo item = shortcuts_[from_index];
  shortcuts_.erase(shortcuts_.begin() + from_index);
  shortcuts_.insert(shortcuts_.begin() + to_index, item);

  // Recompute order indices.
  for (size_t i = 0; i < shortcuts_.size(); ++i) {
    shortcuts_[i].order_index = static_cast<int>(i);
  }

  NotifyShortcutsChanged();
  return true;
}

size_t AstraNewTabModel::BulkRemoveShortcuts(
    const std::vector<size_t>& indices) {
  // Sort indices in descending order so we can erase without affecting indices.
  std::vector<size_t> sorted = indices;
  std::sort(sorted.rbegin(), sorted.rend());

  size_t removed = 0;
  for (size_t idx : sorted) {
    if (idx < shortcuts_.size()) {
      shortcuts_.erase(shortcuts_.begin() + idx);
      ++removed;
    }
  }

  if (removed > 0) {
    // Recompute order indices.
    for (size_t i = 0; i < shortcuts_.size(); ++i) {
      shortcuts_[i].order_index = static_cast<int>(i);
    }
    NotifyShortcutsChanged();
  }
  return removed;
}

void AstraNewTabModel::SetShortcuts(
    std::vector<AstraNtpShortcutInfo> shortcuts) {
  shortcuts_ = std::move(shortcuts);
  // Recompute order indices.
  for (size_t i = 0; i < shortcuts_.size(); ++i) {
    shortcuts_[i].order_index = static_cast<int>(i);
  }
  NotifyShortcutsChanged();
}

// =========================================================================
// Workspace card management
// =========================================================================

const AstraNtpWorkspaceCardInfo* AstraNewTabModel::GetWorkspaceCardAt(
    size_t index) const {
  if (index >= workspace_cards_.size()) {
    return nullptr;
  }
  return &workspace_cards_[index];
}

int AstraNewTabModel::FindWorkspaceCardById(const std::string& id) const {
  for (size_t i = 0; i < workspace_cards_.size(); ++i) {
    if (workspace_cards_[i].id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

size_t AstraNewTabModel::AddOrUpdateWorkspaceCard(
    const AstraNtpWorkspaceCardInfo& card) {
  int existing = FindWorkspaceCardById(card.id);
  if (existing >= 0) {
    workspace_cards_[static_cast<size_t>(existing)] = card;
    workspace_cards_[static_cast<size_t>(existing)].order_index = existing;
    NotifyWorkspacesChanged();
    return static_cast<size_t>(existing);
  }

  AstraNtpWorkspaceCardInfo new_card = card;
  new_card.order_index = static_cast<int>(workspace_cards_.size());
  workspace_cards_.push_back(new_card);
  NotifyWorkspacesChanged();
  return workspace_cards_.size() - 1;
}

bool AstraNewTabModel::RemoveWorkspaceCard(const std::string& id) {
  int index = FindWorkspaceCardById(id);
  if (index < 0) {
    return false;
  }
  workspace_cards_.erase(workspace_cards_.begin() + index);
  // Recompute order indices.
  for (size_t i = static_cast<size_t>(index); i < workspace_cards_.size(); ++i) {
    workspace_cards_[i].order_index = static_cast<int>(i);
  }
  NotifyWorkspacesChanged();
  return true;
}

bool AstraNewTabModel::ReorderWorkspaceCards(
    const std::vector<size_t>& ordered_indices) {
  if (ordered_indices.size() != workspace_cards_.size()) {
    return false;
  }

  std::set<size_t> seen;
  for (size_t idx : ordered_indices) {
    if (idx >= workspace_cards_.size() || seen.count(idx) > 0) {
      return false;
    }
    seen.insert(idx);
  }

  std::vector<AstraNtpWorkspaceCardInfo> new_order;
  new_order.reserve(workspace_cards_.size());
  for (size_t idx : ordered_indices) {
    new_order.push_back(workspace_cards_[idx]);
  }

  workspace_cards_ = std::move(new_order);
  for (size_t i = 0; i < workspace_cards_.size(); ++i) {
    workspace_cards_[i].order_index = static_cast<int>(i);
  }

  NotifyWorkspacesChanged();
  return true;
}

bool AstraNewTabModel::MoveWorkspaceCard(size_t from_index, size_t to_index) {
  if (from_index >= workspace_cards_.size() || to_index >= workspace_cards_.size()) {
    return false;
  }
  if (from_index == to_index) {
    return true;
  }

  AstraNtpWorkspaceCardInfo item = workspace_cards_[from_index];
  workspace_cards_.erase(workspace_cards_.begin() + from_index);
  workspace_cards_.insert(workspace_cards_.begin() + to_index, item);

  for (size_t i = 0; i < workspace_cards_.size(); ++i) {
    workspace_cards_[i].order_index = static_cast<int>(i);
  }

  NotifyWorkspacesChanged();
  return true;
}

void AstraNewTabModel::SetWorkspaceCards(
    std::vector<AstraNtpWorkspaceCardInfo> cards) {
  workspace_cards_ = std::move(cards);
  for (size_t i = 0; i < workspace_cards_.size(); ++i) {
    workspace_cards_[i].order_index = static_cast<int>(i);
  }
  NotifyWorkspacesChanged();
}

// =========================================================================
// Quick action management
// =========================================================================

const AstraNtpQuickAction* AstraNewTabModel::GetQuickActionAt(
    size_t index) const {
  if (index >= quick_actions_.size()) {
    return nullptr;
  }
  return &quick_actions_[index];
}

int AstraNewTabModel::FindQuickActionById(const std::string& id) const {
  for (size_t i = 0; i < quick_actions_.size(); ++i) {
    if (quick_actions_[i].id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

size_t AstraNewTabModel::AddOrUpdateQuickAction(
    const AstraNtpQuickAction& action) {
  int existing = FindQuickActionById(action.id);
  if (existing >= 0) {
    quick_actions_[static_cast<size_t>(existing)] = action;
    quick_actions_[static_cast<size_t>(existing)].order_index = existing;
    NotifyQuickActionsChanged();
    return static_cast<size_t>(existing);
  }

  AstraNtpQuickAction new_action = action;
  new_action.order_index = static_cast<int>(quick_actions_.size());
  quick_actions_.push_back(new_action);
  NotifyQuickActionsChanged();
  return quick_actions_.size() - 1;
}

bool AstraNewTabModel::RemoveQuickAction(const std::string& id) {
  int index = FindQuickActionById(id);
  if (index < 0) {
    return false;
  }
  quick_actions_.erase(quick_actions_.begin() + index);
  for (size_t i = static_cast<size_t>(index); i < quick_actions_.size(); ++i) {
    quick_actions_[i].order_index = static_cast<int>(i);
  }
  NotifyQuickActionsChanged();
  return true;
}

void AstraNewTabModel::SetQuickActions(
    std::vector<AstraNtpQuickAction> actions) {
  quick_actions_ = std::move(actions);
  for (size_t i = 0; i < quick_actions_.size(); ++i) {
    quick_actions_[i].order_index = static_cast<int>(i);
  }
  NotifyQuickActionsChanged();
}

// =========================================================================
// Recently closed tabs
// =========================================================================

const AstraNtpRecentlyClosedTab* AstraNewTabModel::GetRecentlyClosedAt(
    size_t index) const {
  if (index >= recently_closed_.size()) {
    return nullptr;
  }
  return &recently_closed_[index];
}

void AstraNewTabModel::AddRecentlyClosedTab(
    const AstraNtpRecentlyClosedTab& tab) {
  recently_closed_.insert(recently_closed_.begin(), tab);
  if (recently_closed_.size() >
      static_cast<size_t>(max_recently_closed_shown_)) {
    recently_closed_.pop_back();
  }
  NotifyRecentlyClosedChanged();
}

bool AstraNewTabModel::RemoveRecentlyClosedBySessionId(int session_id) {
  for (auto it = recently_closed_.begin(); it != recently_closed_.end(); ++it) {
    if (it->session_id == session_id) {
      recently_closed_.erase(it);
      NotifyRecentlyClosedChanged();
      return true;
    }
  }
  return false;
}

void AstraNewTabModel::SetRecentlyClosed(
    std::vector<AstraNtpRecentlyClosedTab> tabs) {
  recently_closed_ = std::move(tabs);
  NotifyRecentlyClosedChanged();
}

void AstraNewTabModel::ClearRecentlyClosed() {
  if (!recently_closed_.empty()) {
    recently_closed_.clear();
    NotifyRecentlyClosedChanged();
  }
}

// =========================================================================
// Greeting utility
// =========================================================================

std::u16string AstraNewTabModel::GenerateGreeting(base::Time now) const {
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);

  int hour = exploded.hour;

  switch (greeting_style_) {
    case AstraNtpGreetingStyle::kFormal: {
    std::u16string time_greeting;
    if (hour < 5) {
      time_greeting = u"Good night";
    } else if (hour < 12) {
      time_greeting = u"Good morning";
    } else if (hour < 18) {
      time_greeting = u"Good afternoon";
    } else {
      time_greeting = u"Good evening";
    }
    return time_greeting;
  }
  case AstraNtpGreetingStyle::kCasual:
    return u"Hey there";

  case AstraNtpGreetingStyle::kMinimal: {
    // Format as "HH:MM" (24-hour format).
    // TODO(astra): Use proper time formatting (base::TimeFormat).
    // For now, format a simple numeric time string.
    char buf[32];
    base::snprintf(buf, sizeof(buf), "%02d:%02d", hour, exploded.minute);
    return base::UTF8ToUTF16(buf);
  }
  }
  return u"Hello";
}

std::u16string AstraNewTabModel::GenerateGreeting() const {
  return GenerateGreeting(base::Time::Now());
}

// =========================================================================
// Presentation settings: visibility
// =========================================================================

void AstraNewTabModel::set_show_greeting(bool show) {
  if (show_greeting_ != show) {
    show_greeting_ = show;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_show_search_bar(bool show) {
  if (show_search_bar_ != show) {
    show_search_bar_ = show;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_show_workspace_cards(bool show) {
  if (show_workspace_cards_ != show) {
    show_workspace_cards_ = show;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_show_shortcuts(bool show) {
  if (show_shortcuts_ != show) {
    show_shortcuts_ = show;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_show_recently_closed(bool show) {
  if (show_recently_closed_ != show) {
    show_recently_closed_ = show;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_show_quick_actions(bool show) {
  if (show_quick_actions_ != show) {
    show_quick_actions_ = show;
    NotifyNtpSettingsChanged();
  }
}

// =========================================================================
// Presentation settings: layout
// =========================================================================

void AstraNewTabModel::set_shortcut_columns(int columns) {
  int clamped = ClampInt(columns, kMinShortcutColumns, kMaxShortcutColumns);
  if (shortcut_columns_ != clamped) {
    shortcut_columns_ = clamped;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_max_workspaces_shown(int max) {
  int clamped =
      ClampInt(max, kMinMaxWorkspacesShown, kMaxMaxWorkspacesShown);
  if (max_workspaces_shown_ != clamped) {
    max_workspaces_shown_ = clamped;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_max_recently_closed_shown(int max) {
  int clamped =
      ClampInt(max, kMinMaxRecentlyClosedShown, kMaxMaxRecentlyClosedShown);
  if (max_recently_closed_shown_ != clamped) {
    max_recently_closed_shown_ = clamped;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_shortcut_layout_mode(
    AstraNtpShortcutLayoutMode mode) {
  if (shortcut_layout_mode_ != mode) {
    shortcut_layout_mode_ = mode;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_workspace_card_style(
    AstraNtpWorkspaceCardStyle style) {
  if (workspace_card_style_ != style) {
    workspace_card_style_ = style;
    NotifyNtpSettingsChanged();
  }
}

// =========================================================================
// Presentation settings: background
// =========================================================================

void AstraNewTabModel::set_background_style(AstraNtpBackgroundStyle style) {
  if (background_style_ != style) {
    background_style_ = style;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_custom_background_url(const std::string& url) {
  if (custom_background_url_ != url) {
    custom_background_url_ = url;
    NotifyNtpSettingsChanged();
  }
}

// =========================================================================
// Presentation settings: other
// =========================================================================

void AstraNewTabModel::set_show_most_visited(bool show) {
  if (show_most_visited_ != show) {
    show_most_visited_ = show;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_greeting_style(AstraNtpGreetingStyle style) {
  if (greeting_style_ != style) {
    greeting_style_ = style;
    NotifyNtpSettingsChanged();
  }
}

// =========================================================================
// Theme
// =========================================================================

void AstraNewTabModel::NotifyThemeChanged() {
  for (auto& observer : observers_) {
    observer.OnThemeChanged();
  }
}

// =========================================================================
// Observers
// =========================================================================

void AstraNewTabModel::AddObserver(AstraNewTabModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraNewTabModel::RemoveObserver(AstraNewTabModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraNewTabModel::NotifyShortcutsChanged() {
  for (auto& observer : observers_) {
    observer.OnShortcutsChanged();
  }
}

void AstraNewTabModel::NotifyWorkspacesChanged() {
  for (auto& observer : observers_) {
    observer.OnWorkspacesChanged();
  }
}

void AstraNewTabModel::NotifyQuickActionsChanged() {
  for (auto& observer : observers_) {
    observer.OnQuickActionsChanged();
  }
}

void AstraNewTabModel::NotifyRecentlyClosedChanged() {
  for (auto& observer : observers_) {
    observer.OnRecentlyClosedChanged();
  }
}

void AstraNewTabModel::NotifyNtpSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnNtpSettingsChanged();
  }
}

// static
int AstraNewTabModel::ClampInt(int value, int min_val, int max_val) {
  return std::max(min_val, std::min(max_val, value));
}

// static
SkColor AstraNewTabModel::ParseHexColor(const std::string& hex) {
  if (hex.empty() || hex.size() != 7 || hex[0] != '#') {
    return SK_ColorTRANSPARENT;
  }
  unsigned int color_val = 0;
  if (base::HexStringToUInt(hex.substr(1), &color_val)) {
    return SkColorSetRGB(SkColorGetR(color_val), SkColorGetG(color_val),
                         SkColorGetB(color_val));
  }
  return SK_ColorTRANSPARENT;
}

// static
std::string AstraNewTabModel::ColorToHex(SkColor color) {
  if (color == SK_ColorTRANSPARENT) {
    return std::string();
  }
  char buf[8];
  base::snprintf(buf, sizeof(buf), "#%02X%02X%02X", SkColorGetR(color),
                 SkColorGetG(color), SkColorGetB(color));
  return std::string(buf);
}

// =========================================================================
// Suggested content management
// =========================================================================

const AstraNtpSuggestedContent* AstraNewTabModel::GetSuggestedContentAt(
    size_t index) const {
  if (index >= suggested_content_.size()) {
    return nullptr;
  }
  return &suggested_content_[index];
}

int AstraNewTabModel::FindSuggestedContentById(const std::string& id) const {
  for (size_t i = 0; i < suggested_content_.size(); ++i) {
    if (suggested_content_[i].id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void AstraNewTabModel::AddSuggestedContent(
    const AstraNtpSuggestedContent& item) {
  // Check for duplicate by ID.
  if (FindSuggestedContentById(item.id) >= 0) {
    return;
  }
  suggested_content_.push_back(item);
  // Enforce max items.
  if (suggested_content_.size() > kMaxSuggestedContentItems) {
    suggested_content_.resize(kMaxSuggestedContentItems);
  }
  NotifySuggestedContentChanged();
}

bool AstraNewTabModel::RemoveSuggestedContent(const std::string& id) {
  int index = FindSuggestedContentById(id);
  if (index < 0) {
    return false;
  }
  suggested_content_.erase(suggested_content_.begin() + index);
  NotifySuggestedContentChanged();
  return true;
}

void AstraNewTabModel::SetSuggestedContent(
    std::vector<AstraNtpSuggestedContent> items) {
  suggested_content_ = std::move(items);
  // Enforce max items.
  if (suggested_content_.size() > kMaxSuggestedContentItems) {
    suggested_content_.resize(kMaxSuggestedContentItems);
  }
  NotifySuggestedContentChanged();
}

void AstraNewTabModel::ClearSuggestedContent() {
  if (!suggested_content_.empty()) {
    suggested_content_.clear();
    NotifySuggestedContentChanged();
  }
}

// =========================================================================
// Suggested content categories
// =========================================================================

void AstraNewTabModel::SetEnabledSuggestedCategories(
    std::vector<AstraNtpSuggestedCategory> categories) {
  enabled_suggested_categories_ = std::move(categories);
  NotifySuggestedContentSettingsChanged();
}

void AstraNewTabModel::AddEnabledSuggestedCategory(
    AstraNtpSuggestedCategory category) {
  if (IsSuggestedCategoryEnabled(category)) {
    return;
  }
  enabled_suggested_categories_.push_back(category);
  NotifySuggestedContentSettingsChanged();
}

void AstraNewTabModel::RemoveEnabledSuggestedCategory(
    AstraNtpSuggestedCategory category) {
  auto it = std::find(enabled_suggested_categories_.begin(),
                      enabled_suggested_categories_.end(), category);
  if (it == enabled_suggested_categories_.end()) {
    return;
  }
  enabled_suggested_categories_.erase(it);
  NotifySuggestedContentSettingsChanged();
}

bool AstraNewTabModel::IsSuggestedCategoryEnabled(
    AstraNtpSuggestedCategory category) const {
  return std::find(enabled_suggested_categories_.begin(),
                   enabled_suggested_categories_.end(),
                   category) != enabled_suggested_categories_.end();
}

// =========================================================================
// Theme settings
// =========================================================================

void AstraNewTabModel::set_theme_mode(AstraNtpThemeMode mode) {
  if (theme_mode_ != mode) {
    theme_mode_ = mode;
    NotifyThemeChanged();
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_accent_color(SkColor color) {
  if (accent_color_ != color) {
    accent_color_ = color;
    NotifyAccentColorChanged();
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_gradient_settings(
    const AstraNtpGradientSettings& settings) {
  if (!(gradient_settings_ == settings)) {
    gradient_settings_ = settings;
    NotifyNtpSettingsChanged();
  }
}

// =========================================================================
// Layout density
// =========================================================================

void AstraNewTabModel::set_layout_density(AstraNtpLayoutDensity density) {
  if (layout_density_ != density) {
    layout_density_ = density;
    NotifyLayoutDensityChanged();
    NotifyNtpSettingsChanged();
  }
}

// =========================================================================
// Shortcut display options
// =========================================================================

void AstraNewTabModel::set_shortcut_icon_size(AstraNtpShortcutIconSize size) {
  if (shortcut_icon_size_ != size) {
    shortcut_icon_size_ = size;
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_show_shortcut_titles(bool show) {
  if (show_shortcut_titles_ != show) {
    show_shortcut_titles_ = show;
    NotifyNtpSettingsChanged();
  }
}

// =========================================================================
// Clock / date settings
// =========================================================================

void AstraNewTabModel::set_clock_format(AstraNtpClockFormat format) {
  if (clock_format_ != format) {
    clock_format_ = format;
    NotifyClockFormatChanged();
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_show_seconds(bool show) {
  if (show_seconds_ != show) {
    show_seconds_ = show;
    NotifyClockFormatChanged();
  }
}

void AstraNewTabModel::set_show_date(bool show) {
  if (show_date_ != show) {
    show_date_ = show;
    NotifyClockFormatChanged();
  }
}

std::u16string AstraNewTabModel::FormatClockTime(base::Time now) const {
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);

  int hour = exploded.hour;
  int minute = exploded.minute;
  int second = exploded.second;

  // Determine if we should use 12-hour format.
  bool use_12_hour = false;
  switch (clock_format_) {
    case AstraNtpClockFormat::k12Hour:
      use_12_hour = true;
      break;
    case AstraNtpClockFormat::k24Hour:
      use_12_hour = false;
      break;
    case AstraNtpClockFormat::kSystem:
      // Default to 24-hour for system locale (simplified).
      // TODO(astra): Use actual system locale to determine 12/24 hour format.
      // Chromium pattern: base::TimeFormat::GetHourClockType().
      use_12_hour = false;
      break;
  }

  std::u16string result;
  char buf[32];
  if (use_12_hour) {
    int display_hour = hour % 12;
    if (display_hour == 0) {
      display_hour = 12;
    }
    if (show_seconds_) {
      base::snprintf(buf, sizeof(buf), "%d:%02d:%02d %s",
                     display_hour, minute, second,
                     hour < 12 ? "AM" : "PM");
    } else {
      base::snprintf(buf, sizeof(buf), "%d:%02d %s",
                     display_hour, minute,
                     hour < 12 ? "AM" : "PM");
    }
  } else {
    if (show_seconds_) {
      base::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour, minute, second);
    } else {
      base::snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
    }
  }
  return base::UTF8ToUTF16(buf);
}

std::u16string AstraNewTabModel::FormatDate(base::Time now) const {
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);

  char buf[64];
  base::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                 exploded.year, exploded.month, exploded.day_of_month);
  // TODO(astra): Use proper date formatting (base::TimeFormat).
  return base::UTF8ToUTF16(buf);
}

// =========================================================================
// Search bar settings
// =========================================================================

void AstraNewTabModel::set_search_bar_style(AstraNtpSearchBarStyle style) {
  if (search_bar_style_ != style) {
    search_bar_style_ = style;
    NotifySearchBarStyleChanged();
    NotifyNtpSettingsChanged();
  }
}

void AstraNewTabModel::set_show_search_engine(bool show) {
  if (show_search_engine_ != show) {
    show_search_engine_ = show;
    NotifySearchBarStyleChanged();
  }
}

void AstraNewTabModel::set_search_engine_name(const std::string& name) {
  if (search_engine_name_ != name) {
    search_engine_name_ = name;
    NotifySearchBarStyleChanged();
  }
}

// =========================================================================
// Greeting customization
// =========================================================================

void AstraNewTabModel::set_greeting_name(const std::u16string& name) {
  if (greeting_name_ != name) {
    greeting_name_ = name;
    NotifyGreetingNameChanged();
    NotifyNtpSettingsChanged();
  }
}

// =========================================================================
// Suggested content visibility
// =========================================================================

void AstraNewTabModel::set_show_suggested_content(bool show) {
  if (show_suggested_content_ != show) {
    show_suggested_content_ = show;
    NotifySuggestedContentSettingsChanged();
    NotifyNtpSettingsChanged();
  }
}

// =========================================================================
// Workspace window count helper
// =========================================================================

bool AstraNewTabModel::SetWorkspaceWindowCount(const std::string& id,
                                               int window_count) {
  int index = FindWorkspaceCardById(id);
  if (index < 0) {
    return false;
  }
  if (workspace_cards_[static_cast<size_t>(index)].window_count == window_count) {
    return false;
  }
  workspace_cards_[static_cast<size_t>(index)].window_count = window_count;
  NotifyWorkspacesChanged();
  return true;
}

// =========================================================================
// Import / Export
// =========================================================================

base::Value::Dict AstraNewTabModel::ExportSettings() const {
  base::Value::Dict dict;

  // Visibility settings.
  dict.Set("show_greeting", show_greeting_);
  dict.Set("show_search_bar", show_search_bar_);
  dict.Set("show_workspace_cards", show_workspace_cards_);
  dict.Set("show_shortcuts", show_shortcuts_);
  dict.Set("show_recently_closed", show_recently_closed_);
  dict.Set("show_quick_actions", show_quick_actions_);
  dict.Set("show_suggested_content", show_suggested_content_);

  // Layout settings.
  dict.Set("shortcut_columns", shortcut_columns_);
  dict.Set("max_workspaces_shown", max_workspaces_shown_);
  dict.Set("max_recently_closed_shown", max_recently_closed_shown_);

  // Layout modes.
  dict.Set("shortcut_layout_mode", static_cast<int>(shortcut_layout_mode_));
  dict.Set("workspace_card_style", static_cast<int>(workspace_card_style_));
  dict.Set("background_style", static_cast<int>(background_style_));
  dict.Set("theme_mode", static_cast<int>(theme_mode_));
  dict.Set("layout_density", static_cast<int>(layout_density_));
  dict.Set("greeting_style", static_cast<int>(greeting_style_));
  dict.Set("clock_format", static_cast<int>(clock_format_));
  dict.Set("search_bar_style", static_cast<int>(search_bar_style_));
  dict.Set("shortcut_icon_size", static_cast<int>(shortcut_icon_size_));

  // Other settings.
  dict.Set("custom_background_url", custom_background_url_);
  dict.Set("show_most_visited", show_most_visited_);
  dict.Set("show_shortcut_titles", show_shortcut_titles_);
  dict.Set("show_seconds", show_seconds_);
  dict.Set("show_date", show_date_);
  dict.Set("show_search_engine", show_search_engine_);
  dict.Set("search_engine_name", search_engine_name_);
  dict.Set("greeting_name", base::UTF16ToUTF8(greeting_name_));
  dict.Set("accent_color", ColorToHex(accent_color_));

  // Gradient settings.
  base::Value::Dict gradient_dict;
  gradient_dict.Set("start_color", ColorToHex(gradient_settings_.start_color));
  gradient_dict.Set("end_color", ColorToHex(gradient_settings_.end_color));
  gradient_dict.Set("angle", gradient_settings_.angle);
  dict.Set("gradient_settings", std::move(gradient_dict));

  // Custom shortcuts.
  base::Value::List shortcut_list;
  for (const auto& shortcut : shortcuts_) {
    if (shortcut.is_custom) {
      shortcut_list.Append(ShortcutToDict(shortcut));
    }
  }
  dict.Set("custom_shortcuts", std::move(shortcut_list));

  // Quick action order (just IDs).
  base::Value::List action_list;
  for (const auto& action : quick_actions_) {
    action_list.Append(action.id);
  }
  dict.Set("quick_actions", std::move(action_list));

  return dict;
}

bool AstraNewTabModel::ImportSettings(const base::Value::Dict& settings) {
  bool any_imported = false;

  // Visibility settings.
  if (auto* show_greeting = settings.FindBool("show_greeting")) {
    show_greeting_ = *show_greeting;
    any_imported = true;
  }
  if (auto* show_search_bar = settings.FindBool("show_search_bar")) {
    show_search_bar_ = *show_search_bar;
    any_imported = true;
  }
  if (auto* show_workspace = settings.FindBool("show_workspace_cards")) {
    show_workspace_cards_ = *show_workspace;
    any_imported = true;
  }
  if (auto* show_shortcuts = settings.FindBool("show_shortcuts")) {
    show_shortcuts_ = *show_shortcuts;
    any_imported = true;
  }
  if (auto* show_recent = settings.FindBool("show_recently_closed")) {
    show_recently_closed_ = *show_recent;
    any_imported = true;
  }
  if (auto* show_actions = settings.FindBool("show_quick_actions")) {
    show_quick_actions_ = *show_actions;
    any_imported = true;
  }
  if (auto* show_suggested = settings.FindBool("show_suggested_content")) {
    show_suggested_content_ = *show_suggested;
    any_imported = true;
  }

  // Layout settings.
  if (auto* columns = settings.FindInt("shortcut_columns")) {
    shortcut_columns_ = ClampInt(*columns, kMinShortcutColumns,
                                  kMaxShortcutColumns);
    any_imported = true;
  }
  if (auto* max_ws = settings.FindInt("max_workspaces_shown")) {
    max_workspaces_shown_ = ClampInt(*max_ws, kMinMaxWorkspacesShown,
                                      kMaxMaxWorkspacesShown);
    any_imported = true;
  }
  if (auto* max_rc = settings.FindInt("max_recently_closed_shown")) {
    max_recently_closed_shown_ = ClampInt(*max_rc, kMinMaxRecentlyClosedShown,
                                           kMaxMaxRecentlyClosedShown);
    any_imported = true;
  }

  // Layout modes.
  if (auto* sc_mode = settings.FindInt("shortcut_layout_mode")) {
    shortcut_layout_mode_ = static_cast<AstraNtpShortcutLayoutMode>(
        std::max(0, std::min(1, *sc_mode)));
    any_imported = true;
  }
  if (auto* ws_style = settings.FindInt("workspace_card_style")) {
    workspace_card_style_ = static_cast<AstraNtpWorkspaceCardStyle>(
        std::max(0, std::min(2, *ws_style)));
    any_imported = true;
  }
  if (auto* bg_style = settings.FindInt("background_style")) {
    background_style_ = static_cast<AstraNtpBackgroundStyle>(
        std::max(0, std::min(3, *bg_style)));
    any_imported = true;
  }
  if (auto* theme = settings.FindInt("theme_mode")) {
    theme_mode_ = static_cast<AstraNtpThemeMode>(
        std::max(0, std::min(2, *theme)));
    any_imported = true;
  }
  if (auto* density = settings.FindInt("layout_density")) {
    layout_density_ = static_cast<AstraNtpLayoutDensity>(
        std::max(0, std::min(2, *density)));
    any_imported = true;
  }
  if (auto* g_style = settings.FindInt("greeting_style")) {
    greeting_style_ = static_cast<AstraNtpGreetingStyle>(
        std::max(0, std::min(2, *g_style)));
    any_imported = true;
  }
  if (auto* clock = settings.FindInt("clock_format")) {
    clock_format_ = static_cast<AstraNtpClockFormat>(
        std::max(0, std::min(2, *clock)));
    any_imported = true;
  }
  if (auto* sb_style = settings.FindInt("search_bar_style")) {
    search_bar_style_ = static_cast<AstraNtpSearchBarStyle>(
        std::max(0, std::min(2, *sb_style)));
    any_imported = true;
  }
  if (auto* icon_size = settings.FindInt("shortcut_icon_size")) {
    shortcut_icon_size_ = static_cast<AstraNtpShortcutIconSize>(
        std::max(0, std::min(2, *icon_size)));
    any_imported = true;
  }

  // String settings.
  if (auto* bg_url = settings.FindString("custom_background_url")) {
    custom_background_url_ = *bg_url;
    any_imported = true;
  }
  if (auto* se_name = settings.FindString("search_engine_name")) {
    search_engine_name_ = *se_name;
    any_imported = true;
  }
  if (auto* g_name = settings.FindString("greeting_name")) {
    greeting_name_ = base::UTF8ToUTF16(*g_name);
    any_imported = true;
  }
  if (auto* accent_hex = settings.FindString("accent_color")) {
    accent_color_ = ParseHexColor(*accent_hex);
    any_imported = true;
  }

  // Bool settings.
  if (auto* most_visited = settings.FindBool("show_most_visited")) {
    show_most_visited_ = *most_visited;
    any_imported = true;
  }
  if (auto* show_titles = settings.FindBool("show_shortcut_titles")) {
    show_shortcut_titles_ = *show_titles;
    any_imported = true;
  }
  if (auto* show_sec = settings.FindBool("show_seconds")) {
    show_seconds_ = *show_sec;
    any_imported = true;
  }
  if (auto* show_dt = settings.FindBool("show_date")) {
    show_date_ = *show_dt;
    any_imported = true;
  }
  if (auto* show_se = settings.FindBool("show_search_engine")) {
    show_search_engine_ = *show_se;
    any_imported = true;
  }

  // Gradient settings.
  if (auto* gradient_dict = settings.FindDict("gradient_settings")) {
    if (auto* start = gradient_dict->FindString("start_color")) {
      gradient_settings_.start_color = ParseHexColor(*start);
    }
    if (auto* end = gradient_dict->FindString("end_color")) {
      gradient_settings_.end_color = ParseHexColor(*end);
    }
    if (auto* angle = gradient_dict->FindInt("angle")) {
      gradient_settings_.angle = *angle;
    }
    any_imported = true;
  }

  // Custom shortcuts.
  if (auto* shortcut_list = settings.FindList("custom_shortcuts")) {
    shortcuts_.clear();
    for (const auto& entry : *shortcut_list) {
      if (entry.is_dict()) {
        shortcuts_.push_back(ShortcutFromDict(entry.GetDict()));
      }
    }
    any_imported = true;
  }

  // Quick action IDs.
  if (auto* action_list = settings.FindList("quick_actions")) {
    quick_actions_.clear();
    for (size_t i = 0; i < action_list->size(); ++i) {
      const std::string* id = (*action_list)[i].GetIfString();
      if (id) {
        AstraNtpQuickAction action;
        action.id = *id;
        action.order_index = static_cast<int>(i);
        action.is_enabled = true;
        quick_actions_.push_back(action);
      }
    }
    any_imported = true;
  }

  if (any_imported) {
    NotifyShortcutsChanged();
    NotifyQuickActionsChanged();
    NotifyNtpSettingsChanged();
    NotifyThemeChanged();
    NotifyLayoutDensityChanged();
  }

  return any_imported;
}

// =========================================================================
// Reset to defaults
// =========================================================================

void AstraNewTabModel::ResetSettingsToDefaults() {
  show_greeting_ = true;
  show_search_bar_ = true;
  show_workspace_cards_ = true;
  show_shortcuts_ = true;
  show_recently_closed_ = true;
  show_quick_actions_ = true;
  show_suggested_content_ = kDefaultShowSuggestedContent;

  shortcut_columns_ = kDefaultShortcutColumns;
  max_workspaces_shown_ = kDefaultMaxWorkspacesShown;
  max_recently_closed_shown_ = kDefaultMaxRecentlyClosedShown;

  shortcut_layout_mode_ = AstraNtpShortcutLayoutMode::kGrid;
  workspace_card_style_ = AstraNtpWorkspaceCardStyle::kFull;
  background_style_ = AstraNtpBackgroundStyle::kSimple;
  theme_mode_ = kDefaultThemeMode;
  layout_density_ = kDefaultLayoutDensity;
  greeting_style_ = AstraNtpGreetingStyle::kFormal;
  clock_format_ = kDefaultClockFormat;
  search_bar_style_ = kDefaultSearchBarStyle;
  shortcut_icon_size_ = kDefaultShortcutIconSize;

  custom_background_url_.clear();
  show_most_visited_ = true;
  show_shortcut_titles_ = kDefaultShowShortcutTitles;
  show_seconds_ = kDefaultShowSeconds;
  show_date_ = kDefaultShowDate;
  show_search_engine_ = kDefaultShowSearchEngine;
  search_engine_name_ = "Google";
  greeting_name_.clear();
  accent_color_ = SK_ColorTRANSPARENT;
  gradient_settings_ = AstraNtpGradientSettings();

  NotifyNtpSettingsChanged();
  NotifyThemeChanged();
  NotifyLayoutDensityChanged();
  NotifyClockFormatChanged();
  NotifySearchBarStyleChanged();
}

void AstraNewTabModel::ResetAllToDefaults() {
  ResetSettingsToDefaults();

  shortcuts_.clear();
  quick_actions_.clear();
  recently_closed_.clear();
  suggested_content_.clear();
  enabled_suggested_categories_.clear();

  NotifyShortcutsChanged();
  NotifyWorkspacesChanged();
  NotifyQuickActionsChanged();
  NotifyRecentlyClosedChanged();
  NotifySuggestedContentChanged();
}

// =========================================================================
// Extended notification helpers
// =========================================================================

void AstraNewTabModel::NotifySuggestedContentChanged() {
  for (auto& observer : observers_) {
    observer.OnSuggestedContentChanged();
  }
}

void AstraNewTabModel::NotifyLayoutDensityChanged() {
  for (auto& observer : observers_) {
    observer.OnLayoutDensityChanged();
  }
}

void AstraNewTabModel::NotifyAccentColorChanged() {
  for (auto& observer : observers_) {
    observer.OnAccentColorChanged();
  }
}

void AstraNewTabModel::NotifyClockFormatChanged() {
  for (auto& observer : observers_) {
    observer.OnClockFormatChanged();
  }
}

void AstraNewTabModel::NotifySearchBarStyleChanged() {
  for (auto& observer : observers_) {
    observer.OnSearchBarStyleChanged();
  }
}

void AstraNewTabModel::NotifyGreetingNameChanged() {
  for (auto& observer : observers_) {
    observer.OnGreetingNameChanged();
  }
}

void AstraNewTabModel::NotifySuggestedContentSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnSuggestedContentSettingsChanged();
  }
}

// =========================================================================
// Serialization helpers
// =========================================================================

// static
base::Value::Dict AstraNewTabModel::SuggestedContentToDict(
    const AstraNtpSuggestedContent& item) {
  base::Value::Dict dict;
  dict.Set("id", item.id);
  dict.Set("title", base::UTF16ToUTF8(item.title));
  dict.Set("source", base::UTF16ToUTF8(item.source));
  dict.Set("url", item.url.spec());
  dict.Set("image_url", item.image_url.spec());
  dict.Set("category", static_cast<int>(item.category));
  dict.Set("publish_time",
           static_cast<double>(item.publish_time.ToDeltaSinceWindowsEpoch()
                                   .InMicroseconds()));
  return dict;
}

// static
AstraNtpSuggestedContent AstraNewTabModel::SuggestedContentFromDict(
    const base::Value::Dict& dict) {
  AstraNtpSuggestedContent item;
  const std::string* id_str = dict.FindString("id").value_or(&std::string());
  item.id = *id_str;
  const std::string* title_str =
      dict.FindString("title").value_or(&std::string());
  item.title = base::UTF8ToUTF16(*title_str);
  const std::string* source_str =
      dict.FindString("source").value_or(&std::string());
  item.source = base::UTF8ToUTF16(*source_str);
  const std::string* url_str = dict.FindString("url").value_or(&std::string());
  item.url = GURL(*url_str);
  const std::string* img_str =
      dict.FindString("image_url").value_or(&std::string());
  item.image_url = GURL(*img_str);
  int cat = dict.FindInt("category").value_or(0);
  item.category = static_cast<AstraNtpSuggestedCategory>(
      std::max(0, std::min(5, cat)));
  double pub_time = dict.FindDouble("publish_time").value_or(0.0);
  item.publish_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(static_cast<int64_t>(pub_time)));
  return item;
}

}  // namespace astra
