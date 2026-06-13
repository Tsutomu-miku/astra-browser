#ifndef ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_MODEL_H_
#define ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "url/gurl.h"

class PrefService;

namespace astra {

// =========================================================================
// AstraNewTabModel — data model for the Astra new tab page
// =========================================================================
//
// Central state model for the Astra new tab page.  Owns all NTP data:
// shortcuts, workspace cards, quick actions, recently closed tabs, and
// presentation settings.  Persists settings and custom shortcuts via
// PrefService.  Notifies observers of changes.
//
// This is the single source of truth for NTP state.  Views never store
// state — they read from the model and send user actions to the
// controller, which updates the model.
//
// Chromium subsystems reused:
//   - PrefService (persistence via profile prefs)
//   - base::ObserverList (observer pattern)
//
// Astra-owned data:
//   - Custom shortcuts (user-added shortcuts)
//   - Presentation settings (visibility, layout modes, background)
//   - Quick action configuration
//   - Recently closed cache (projected from TabRestoreService)
//
// Data projected from Chromium / Astra services:
//   - Most visited sites (TopSites — projected by controller)
//   - Workspace summaries (AstraWorkspaceService — projected by controller)
//   - Recently closed tabs (TabRestoreService — projected by controller)
//
// TODO(astra): The model currently stores "most visited" shortcuts as
//   data pushed by the controller.  In a full Chromium build, the model
//   should observe TopSites directly via TopSitesObserver.
// =========================================================================

// Shortcut layout mode.
enum class AstraNtpShortcutLayoutMode {
  kGrid = 0,  // Grid of icon+title tiles.
  kList = 1,  // Horizontal list of compact items.
};

// Workspace card style.
enum class AstraNtpWorkspaceCardStyle {
  kCompact = 0,  // Small card with icon + name only.
  kFull = 1,     // Full card with accent bar, name, tab count, menu.
};

// Background style.
enum class AstraNtpBackgroundStyle {
  kSimple = 0,    // Solid color / theme background.
  kGradient = 1,  // Gradient background.
  kImage = 2,     // Custom image background.
};

// Greeting style.
enum class AstraNtpGreetingStyle {
  kFormal = 0,   // "Good morning"
  kCasual = 1,   // "Hey there"
  kMinimal = 2,  // Just the time
};

// A single shortcut entry shown on the new tab page.
struct AstraNtpShortcutInfo {
  std::u16string title;
  GURL url;
  GURL icon_url;
  bool is_custom = false;     // True if user-added (not most-visited).
  int order_index = 0;        // Sort position.

  bool operator==(const AstraNtpShortcutInfo& other) const {
    return title == other.title && url == other.url &&
           icon_url == other.icon_url && is_custom == other.is_custom &&
           order_index == other.order_index;
  }
};

// A workspace card entry.
struct AstraNtpWorkspaceCardInfo {
  std::string id;
  std::u16string name;
  std::string accent_color_hex;
  SkColor accent_color = SK_ColorTRANSPARENT;
  int tab_count = 0;
  bool is_active = false;
  int order_index = 0;

  bool operator==(const AstraNtpWorkspaceCardInfo& other) const {
    return id == other.id && name == other.name &&
           accent_color_hex == other.accent_color_hex &&
           accent_color == other.accent_color &&
           tab_count == other.tab_count && is_active == other.is_active &&
           order_index == other.order_index;
  }
};

// A quick action entry.
struct AstraNtpQuickAction {
  std::string id;
  std::u16string label;
  std::u16string icon;
  bool is_enabled = true;
  int order_index = 0;

  bool operator==(const AstraNtpQuickAction& other) const {
    return id == other.id && label == other.label && icon == other.icon &&
           is_enabled == other.is_enabled && order_index == other.order_index;
  }
};

// A recently closed tab entry.
struct AstraNtpRecentlyClosedTab {
  std::u16string title;
  GURL url;
  GURL favicon_url;
  base::Time close_time;
  int session_id = -1;  // Session ID from TabRestoreService, -1 if unknown.

  bool operator==(const AstraNtpRecentlyClosedTab& other) const {
    return title == other.title && url == other.url &&
           favicon_url == other.favicon_url &&
           close_time == other.close_time && session_id == other.session_id;
  }
};

// =========================================================================
// Observer interface
// =========================================================================
//
// Views observe the model to stay in sync with state changes.
// All observer methods have empty default implementations.
// Subclasses only override the methods they care about.
// =========================================================================

class AstraNewTabModelObserver : public base::CheckedObserver {
 public:
  // Called when shortcuts are added, removed, reordered, or updated.
  virtual void OnShortcutsChanged() {}

  // Called when workspace cards are added, removed, or updated.
  virtual void OnWorkspacesChanged() {}

  // Called when quick actions change (added, removed, reordered).
  virtual void OnQuickActionsChanged() {}

  // Called when recently closed tabs change.
  virtual void OnRecentlyClosedChanged() {}

  // Called when NTP presentation settings change.
  virtual void OnNtpSettingsChanged() {}

  // Called when the theme changes (colors, dark/light mode).
  virtual void OnThemeChanged() {}

 protected:
  ~AstraNewTabModelObserver() override = default;
};

// =========================================================================
// Model class
// =========================================================================

class AstraNewTabModel {
 public:
  AstraNewTabModel();
  explicit AstraNewTabModel(PrefService* prefs);
  ~AstraNewTabModel();

  AstraNewTabModel(const AstraNewTabModel&) = delete;
  AstraNewTabModel& operator=(const AstraNewTabModel&) = delete;

  // -- Persistence ---------------------------------------------------------

  // Loads all settings and custom shortcuts from PrefService.
  // Call this after construction when a profile is available.
  void LoadFromPrefs(PrefService* prefs);

  // Saves presentation settings and custom shortcuts to PrefService.
  // Call this before destruction or when settings change.
  void SaveToPrefs(PrefService* prefs) const;

  // -- Shortcut management -------------------------------------------------

  // Returns all shortcuts (custom + most-visited, combined).
  const std::vector<AstraNtpShortcutInfo>& GetShortcuts() const {
    return shortcuts_;
  }

  // Returns the number of shortcuts.
  size_t GetShortcutCount() const { return shortcuts_.size(); }

  // Returns the shortcut at |index|, or nullptr if out of bounds.
  const AstraNtpShortcutInfo* GetShortcutAt(size_t index) const;

  // Finds a shortcut by URL.  Returns index or -1 if not found.
  int FindShortcutByUrl(const GURL& url) const;

  // Adds a custom shortcut.  Returns the index of the new shortcut.
  // Notifies observers with OnShortcutsChanged.
  size_t AddCustomShortcut(const std::u16string& title, const GURL& url);

  // Adds a custom shortcut with icon URL.
  size_t AddCustomShortcut(const std::u16string& title,
                           const GURL& url,
                           const GURL& icon_url);

  // Removes the shortcut at |index|.  Returns true if successful.
  // Notifies observers with OnShortcutsChanged.
  bool RemoveShortcutAt(size_t index);

  // Removes the shortcut with the given URL.  Returns true if found and removed.
  bool RemoveShortcutByUrl(const GURL& url);

  // Updates the title and URL of the shortcut at |index|.
  // Returns true if the index was valid.
  bool UpdateShortcutAt(size_t index,
                        const std::u16string& title,
                        const GURL& url);

  // Reorders shortcuts according to |ordered_indices|.
  // |ordered_indices| must be a permutation of [0, count).
  // Returns true on success.
  bool ReorderShortcuts(const std::vector<size_t>& ordered_indices);

  // Moves a shortcut from |from_index| to |to_index|.
  // Returns true if both indices are valid.
  bool MoveShortcut(size_t from_index, size_t to_index);

  // Bulk remove: removes all shortcuts at the given indices.
  // Returns the number of shortcuts removed.
  size_t BulkRemoveShortcuts(const std::vector<size_t>& indices);

  // Replaces the full list of shortcuts.  Used by the controller to push
  // most-visited data from TopSites or to load from prefs.
  // Notifies observers with OnShortcutsChanged.
  void SetShortcuts(std::vector<AstraNtpShortcutInfo> shortcuts);

  // -- Workspace card management ------------------------------------------

  const std::vector<AstraNtpWorkspaceCardInfo>& GetWorkspaceCards() const {
    return workspace_cards_;
  }

  size_t GetWorkspaceCardCount() const { return workspace_cards_.size(); }

  const AstraNtpWorkspaceCardInfo* GetWorkspaceCardAt(size_t index) const;

  // Finds a workspace card by ID.  Returns index or -1 if not found.
  int FindWorkspaceCardById(const std::string& id) const;

  // Adds or updates a workspace card.  If a card with the same ID exists,
  // it is updated.  Otherwise, a new card is appended.
  // Returns the index of the card.
  size_t AddOrUpdateWorkspaceCard(const AstraNtpWorkspaceCardInfo& card);

  // Removes the workspace card with |id|.  Returns true if found and removed.
  bool RemoveWorkspaceCard(const std::string& id);

  // Reorders workspace cards.
  bool ReorderWorkspaceCards(const std::vector<size_t>& ordered_indices);

  // Moves a workspace card from |from_index| to |to_index|.
  bool MoveWorkspaceCard(size_t from_index, size_t to_index);

  // Replaces all workspace cards.  Used by the controller to push data
  // from AstraWorkspaceService.
  void SetWorkspaceCards(std::vector<AstraNtpWorkspaceCardInfo> cards);

  // -- Quick action management ---------------------------------------------

  const std::vector<AstraNtpQuickAction>& GetQuickActions() const {
    return quick_actions_;
  }

  size_t GetQuickActionCount() const { return quick_actions_.size(); }

  const AstraNtpQuickAction* GetQuickActionAt(size_t index) const;

  // Finds a quick action by ID.  Returns index or -1 if not found.
  int FindQuickActionById(const std::string& id) const;

  // Adds or updates a quick action.
  size_t AddOrUpdateQuickAction(const AstraNtpQuickAction& action);

  // Removes a quick action by ID.
  bool RemoveQuickAction(const std::string& id);

  // Replaces all quick actions.
  void SetQuickActions(std::vector<AstraNtpQuickAction> actions);

  // -- Recently closed tabs ------------------------------------------------

  const std::vector<AstraNtpRecentlyClosedTab>& GetRecentlyClosed() const {
    return recently_closed_;
  }

  size_t GetRecentlyClosedCount() const { return recently_closed_.size(); }

  const AstraNtpRecentlyClosedTab* GetRecentlyClosedAt(size_t index) const;

  // Adds a recently closed tab to the front (most recent).
  // If the list exceeds max_recently_closed_shown_, the oldest is dropped.
  void AddRecentlyClosedTab(const AstraNtpRecentlyClosedTab& tab);

  // Removes a recently closed tab by session ID.
  bool RemoveRecentlyClosedBySessionId(int session_id);

  // Replaces all recently closed tabs.
  void SetRecentlyClosed(std::vector<AstraNtpRecentlyClosedTab> tabs);

  // Clears all recently closed tabs.
  void ClearRecentlyClosed();

  // -- Greeting utility ----------------------------------------------------

  // Generates a greeting string based on the time of day and greeting style.
  // Uses |now| for the current time (injected for testability).
  std::u16string GenerateGreeting(base::Time now) const;

  // Convenience overload that uses base::Time::Now().
  std::u16string GenerateGreeting() const;

  // -- Presentation settings: visibility -----------------------------------

  bool show_greeting() const { return show_greeting_; }
  void set_show_greeting(bool show);

  bool show_search_bar() const { return show_search_bar_; }
  void set_show_search_bar(bool show);

  bool show_workspace_cards() const { return show_workspace_cards_; }
  void set_show_workspace_cards(bool show);

  bool show_shortcuts() const { return show_shortcuts_; }
  void set_show_shortcuts(bool show);

  bool show_recently_closed() const { return show_recently_closed_; }
  void set_show_recently_closed(bool show);

  bool show_quick_actions() const { return show_quick_actions_; }
  void set_show_quick_actions(bool show);

  // -- Presentation settings: layout ---------------------------------------

  int shortcut_columns() const { return shortcut_columns_; }
  void set_shortcut_columns(int columns);

  int max_workspaces_shown() const { return max_workspaces_shown_; }
  void set_max_workspaces_shown(int max);

  int max_recently_closed_shown() const { return max_recently_closed_shown_; }
  void set_max_recently_closed_shown(int max);

  AstraNtpShortcutLayoutMode shortcut_layout_mode() const {
    return shortcut_layout_mode_;
  }
  void set_shortcut_layout_mode(AstraNtpShortcutLayoutMode mode);

  AstraNtpWorkspaceCardStyle workspace_card_style() const {
    return workspace_card_style_;
  }
  void set_workspace_card_style(AstraNtpWorkspaceCardStyle style);

  // -- Presentation settings: background -----------------------------------

  AstraNtpBackgroundStyle background_style() const { return background_style_; }
  void set_background_style(AstraNtpBackgroundStyle style);

  const std::string& custom_background_url() const {
    return custom_background_url_;
  }
  void set_custom_background_url(const std::string& url);

  // -- Presentation settings: other ----------------------------------------

  bool show_most_visited() const { return show_most_visited_; }
  void set_show_most_visited(bool show);

  AstraNtpGreetingStyle greeting_style() const { return greeting_style_; }
  void set_greeting_style(AstraNtpGreetingStyle style);

  // -- Theme ---------------------------------------------------------------

  // Notifies observers that the theme changed.
  void NotifyThemeChanged();

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraNewTabModelObserver* observer);
  void RemoveObserver(AstraNewTabModelObserver* observer);

  // -- Constants -----------------------------------------------------------

  // Clamp limits for integer settings.
  static constexpr int kMinShortcutColumns = 3;
  static constexpr int kMaxShortcutColumns = 8;
  static constexpr int kMinMaxWorkspacesShown = 3;
  static constexpr int kMaxMaxWorkspacesShown = 10;
  static constexpr int kMinMaxRecentlyClosedShown = 3;
  static constexpr int kMaxMaxRecentlyClosedShown = 10;

  // Default values.
  static constexpr int kDefaultShortcutColumns = 4;
  static constexpr int kDefaultMaxWorkspacesShown = 5;
  static constexpr int kDefaultMaxRecentlyClosedShown = 8;

 private:
  // Notification helpers.
  void NotifyShortcutsChanged();
  void NotifyWorkspacesChanged();
  void NotifyQuickActionsChanged();
  void NotifyRecentlyClosedChanged();
  void NotifyNtpSettingsChanged();

  // Helper to clamp a value between min and max.
  static int ClampInt(int value, int min_val, int max_val);

  // Helper to parse a hex color string into SkColor.
  static SkColor ParseHexColor(const std::string& hex);

  // Helper to convert SkColor to hex string.
  static std::string ColorToHex(SkColor color);

  // -- Data ----------------------------------------------------------------

  // Shortcuts (custom + most-visited combined).
  std::vector<AstraNtpShortcutInfo> shortcuts_;

  // Workspace cards.
  std::vector<AstraNtpWorkspaceCardInfo> workspace_cards_;

  // Quick actions.
  std::vector<AstraNtpQuickAction> quick_actions_;

  // Recently closed tabs.
  std::vector<AstraNtpRecentlyClosedTab> recently_closed_;

  // -- Presentation settings -----------------------------------------------

  bool show_greeting_ = true;
  bool show_search_bar_ = true;
  bool show_workspace_cards_ = true;
  bool show_shortcuts_ = true;
  bool show_recently_closed_ = true;
  bool show_quick_actions_ = true;

  int shortcut_columns_ = kDefaultShortcutColumns;
  int max_workspaces_shown_ = kDefaultMaxWorkspacesShown;
  int max_recently_closed_shown_ = kDefaultMaxRecentlyClosedShown;

  AstraNtpShortcutLayoutMode shortcut_layout_mode_ =
      AstraNtpShortcutLayoutMode::kGrid;
  AstraNtpWorkspaceCardStyle workspace_card_style_ =
      AstraNtpWorkspaceCardStyle::kFull;
  AstraNtpBackgroundStyle background_style_ =
      AstraNtpBackgroundStyle::kSimple;

  std::string custom_background_url_;

  bool show_most_visited_ = true;
  AstraNtpGreetingStyle greeting_style_ = AstraNtpGreetingStyle::kFormal;

  // Observers.
  base::ObserverList<AstraNewTabModelObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_MODEL_H_
