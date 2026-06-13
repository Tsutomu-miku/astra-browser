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

}  // namespace astra
