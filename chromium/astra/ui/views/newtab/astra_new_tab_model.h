#ifndef ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_MODEL_H_
#define ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "base/values.h"
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

// Theme mode (light/dark/system).
enum class AstraNtpThemeMode {
  kSystem = 0,   // Follow system theme.
  kLight = 1,    // Force light mode.
  kDark = 2,     // Force dark mode.
};

// Shortcut layout mode.
enum class AstraNtpShortcutLayoutMode {
  kGrid = 0,  // Grid of icon+title tiles.
  kList = 1,  // Horizontal list of compact items.
};

// Shortcut icon size.
enum class AstraNtpShortcutIconSize {
  kSmall = 0,   // Small icon (32px).
  kMedium = 1,  // Medium icon (48px, default).
  kLarge = 2,   // Large icon (64px).
};

// Workspace card style.
enum class AstraNtpWorkspaceCardStyle {
  kCompact = 0,  // Small card with icon + name only.
  kFull = 1,     // Full card with accent bar, name, tab count, menu.
  kGrid = 2,     // Grid-style card (icon + name centered).
};

// Background style.
enum class AstraNtpBackgroundStyle {
  kSimple = 0,    // Solid color / theme background.
  kGradient = 1,  // Gradient background.
  kImage = 2,     // Custom image background.
  kDaily = 3,     // Daily refresh (Bing image of the day).
};

// Layout density.
enum class AstraNtpLayoutDensity {
  kComfortable = 0,  // Spacious padding, large gaps.
  kCozy = 1,         // Balanced spacing (default).
  kCompact = 2,      // Tight padding, small gaps.
};

// Greeting style.
enum class AstraNtpGreetingStyle {
  kFormal = 0,   // "Good morning"
  kCasual = 1,   // "Hey there"
  kMinimal = 2,  // Just the time
};

// Clock format.
enum class AstraNtpClockFormat {
  k12Hour = 0,    // 12-hour format (e.g., 2:30 PM).
  k24Hour = 1,    // 24-hour format (e.g., 14:30).
  kSystem = 2,    // Follow system locale.
};

// Search bar style.
enum class AstraNtpSearchBarStyle {
  kBoxed = 0,      // Boxed with border (default).
  kMinimal = 1,    // Minimal style, just an underline.
  kCentered = 2,   // Centered with icon on left.
};

// Suggested content category.
enum class AstraNtpSuggestedCategory {
  kNews = 0,
  kEntertainment = 1,
  kSports = 2,
  kTechnology = 3,
  kScience = 4,
  kHealth = 5,
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
  int window_count = 0;
  bool is_active = false;
  int order_index = 0;
  base::Time last_active_time;

  bool operator==(const AstraNtpWorkspaceCardInfo& other) const {
    return id == other.id && name == other.name &&
           accent_color_hex == other.accent_color_hex &&
           accent_color == other.accent_color &&
           tab_count == other.tab_count &&
           window_count == other.window_count &&
           is_active == other.is_active &&
           order_index == other.order_index &&
           last_active_time == other.last_active_time;
  }
};

// A quick action entry.
struct AstraNtpQuickAction {
  std::string id;
  std::u16string label;
  std::u16string icon;
  bool is_enabled = true;
  int order_index = 0;
  std::u16string description;  // Tooltip / longer description.

  bool operator==(const AstraNtpQuickAction& other) const {
    return id == other.id && label == other.label && icon == other.icon &&
           is_enabled == other.is_enabled && order_index == other.order_index &&
           description == other.description;
  }
};

// A suggested content item.
struct AstraNtpSuggestedContent {
  std::string id;
  std::u16string title;
  std::u16string source;
  GURL url;
  GURL image_url;
  AstraNtpSuggestedCategory category = AstraNtpSuggestedCategory::kNews;
  base::Time publish_time;

  bool operator==(const AstraNtpSuggestedContent& other) const {
    return id == other.id && title == other.title && source == other.source &&
           url == other.url && image_url == other.image_url &&
           category == other.category && publish_time == other.publish_time;
  }
};

// Gradient background settings.
struct AstraNtpGradientSettings {
  SkColor start_color = SK_ColorTRANSPARENT;
  SkColor end_color = SK_ColorTRANSPARENT;
  int angle = 135;  // degrees

  bool operator==(const AstraNtpGradientSettings& other) const {
    return start_color == other.start_color && end_color == other.end_color &&
           angle == other.angle;
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

  // Called when suggested content changes.
  virtual void OnSuggestedContentChanged() {}

  // Called when NTP presentation settings change.
  virtual void OnNtpSettingsChanged() {}

  // Called when the theme changes (colors, dark/light mode).
  virtual void OnThemeChanged() {}

  // Called when the layout density changes.
  virtual void OnLayoutDensityChanged() {}

  // Called when the accent color changes.
  virtual void OnAccentColorChanged() {}

  // Called when clock format settings change.
  virtual void OnClockFormatChanged() {}

  // Called when search bar style changes.
  virtual void OnSearchBarStyleChanged() {}

  // Called when the greeting name changes.
  virtual void OnGreetingNameChanged() {}

  // Called when suggested content settings change.
  virtual void OnSuggestedContentSettingsChanged() {}

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

  // -- Theme ---------------------------------------------------------------

  AstraNtpThemeMode theme_mode() const { return theme_mode_; }
  void set_theme_mode(AstraNtpThemeMode mode);

  // Accent color used for UI highlights.
  SkColor accent_color() const { return accent_color_; }
  void set_accent_color(SkColor color);

  // Gradient background settings.
  const AstraNtpGradientSettings& gradient_settings() const {
    return gradient_settings_;
  }
  void set_gradient_settings(const AstraNtpGradientSettings& settings);

  // -- Layout density ------------------------------------------------------

  AstraNtpLayoutDensity layout_density() const { return layout_density_; }
  void set_layout_density(AstraNtpLayoutDensity density);

  // -- Shortcut display options --------------------------------------------

  AstraNtpShortcutIconSize shortcut_icon_size() const {
    return shortcut_icon_size_;
  }
  void set_shortcut_icon_size(AstraNtpShortcutIconSize size);

  bool show_shortcut_titles() const { return show_shortcut_titles_; }
  void set_show_shortcut_titles(bool show);

  // -- Clock / date settings -----------------------------------------------

  AstraNtpClockFormat clock_format() const { return clock_format_; }
  void set_clock_format(AstraNtpClockFormat format);

  bool show_seconds() const { return show_seconds_; }
  void set_show_seconds(bool show);

  bool show_date() const { return show_date_; }
  void set_show_date(bool show);

  // Formats the current time as a string based on clock settings.
  std::u16string FormatClockTime(base::Time now) const;

  // Formats the current date as a string.
  std::u16string FormatDate(base::Time now) const;

  // -- Search bar settings -------------------------------------------------

  AstraNtpSearchBarStyle search_bar_style() const {
    return search_bar_style_;
  }
  void set_search_bar_style(AstraNtpSearchBarStyle style);

  bool show_search_engine() const { return show_search_engine_; }
  void set_show_search_engine(bool show);

  const std::string& search_engine_name() const { return search_engine_name_; }
  void set_search_engine_name(const std::string& name);

  // -- Greeting customization ----------------------------------------------

  const std::u16string& greeting_name() const { return greeting_name_; }
  void set_greeting_name(const std::u16string& name);

  // -- Suggested content ---------------------------------------------------

  bool show_suggested_content() const { return show_suggested_content_; }
  void set_show_suggested_content(bool show);

  const std::vector<AstraNtpSuggestedContent>& GetSuggestedContent() const {
    return suggested_content_;
  }

  size_t GetSuggestedContentCount() const { return suggested_content_.size(); }

  const AstraNtpSuggestedContent* GetSuggestedContentAt(size_t index) const;

  int FindSuggestedContentById(const std::string& id) const;

  void AddSuggestedContent(const AstraNtpSuggestedContent& item);
  bool RemoveSuggestedContent(const std::string& id);
  void SetSuggestedContent(std::vector<AstraNtpSuggestedContent> items);
  void ClearSuggestedContent();

  // Enabled categories for suggested content.
  const std::vector<AstraNtpSuggestedCategory>&
  GetEnabledSuggestedCategories() const {
    return enabled_suggested_categories_;
  }
  void SetEnabledSuggestedCategories(
      std::vector<AstraNtpSuggestedCategory> categories);
  void AddEnabledSuggestedCategory(AstraNtpSuggestedCategory category);
  void RemoveEnabledSuggestedCategory(AstraNtpSuggestedCategory category);
  bool IsSuggestedCategoryEnabled(AstraNtpSuggestedCategory category) const;

  // -- Import / Export -----------------------------------------------------

  // Exports all NTP settings as a base::Value::Dict (for JSON serialization).
  // Includes presentation settings, custom shortcuts, quick action order,
  // and appearance settings.  Does not include service-projected data
  // (most visited, workspaces, recently closed).
  base::Value::Dict ExportSettings() const;

  // Imports NTP settings from a base::Value::Dict.
  // Returns true if import succeeded (at least some settings were valid).
  // Notifies observers of changes.
  bool ImportSettings(const base::Value::Dict& settings);

  // -- Reset to defaults ---------------------------------------------------

  // Resets all presentation settings to their default values.
  // Does not clear custom shortcuts or quick action order.
  // Notifies observers of changes.
  void ResetSettingsToDefaults();

  // Resets everything: settings + custom shortcuts + quick actions.
  void ResetAllToDefaults();

  // -- Workspace card window count support ---------------------------------

  // Window count is stored in workspace card info struct.
  // Helper: sets window count for a workspace card by ID.
  bool SetWorkspaceWindowCount(const std::string& id, int window_count);

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

  // Default theme and appearance.
  static constexpr AstraNtpThemeMode kDefaultThemeMode =
      AstraNtpThemeMode::kSystem;
  static constexpr AstraNtpLayoutDensity kDefaultLayoutDensity =
      AstraNtpLayoutDensity::kCozy;
  static constexpr AstraNtpShortcutIconSize kDefaultShortcutIconSize =
      AstraNtpShortcutIconSize::kMedium;
  static constexpr bool kDefaultShowShortcutTitles = true;
  static constexpr AstraNtpClockFormat kDefaultClockFormat =
      AstraNtpClockFormat::kSystem;
  static constexpr bool kDefaultShowSeconds = false;
  static constexpr bool kDefaultShowDate = true;
  static constexpr AstraNtpSearchBarStyle kDefaultSearchBarStyle =
      AstraNtpSearchBarStyle::kBoxed;
  static constexpr bool kDefaultShowSearchEngine = true;
  static constexpr bool kDefaultShowSuggestedContent = false;

  // Max suggested content items.
  static constexpr size_t kMaxSuggestedContentItems = 12;

 private:
  // Notification helpers.
  void NotifyShortcutsChanged();
  void NotifyWorkspacesChanged();
  void NotifyQuickActionsChanged();
  void NotifyRecentlyClosedChanged();
  void NotifySuggestedContentChanged();
  void NotifyNtpSettingsChanged();
  void NotifyLayoutDensityChanged();
  void NotifyAccentColorChanged();
  void NotifyClockFormatChanged();
  void NotifySearchBarStyleChanged();
  void NotifyGreetingNameChanged();
  void NotifySuggestedContentSettingsChanged();

  // Helper to clamp a value between min and max.
  static int ClampInt(int value, int min_val, int max_val);

  // Helper to parse a hex color string into SkColor.
  static SkColor ParseHexColor(const std::string& hex);

  // Helper to convert SkColor to hex string.
  static std::string ColorToHex(SkColor color);

  // Helper to serialize a suggested content item to dict.
  static base::Value::Dict SuggestedContentToDict(
      const AstraNtpSuggestedContent& item);

  // Helper to parse a suggested content item from dict.
  static AstraNtpSuggestedContent SuggestedContentFromDict(
      const base::Value::Dict& dict);

  // -- Data ----------------------------------------------------------------

  // Shortcuts (custom + most-visited combined).
  std::vector<AstraNtpShortcutInfo> shortcuts_;

  // Workspace cards.
  std::vector<AstraNtpWorkspaceCardInfo> workspace_cards_;

  // Quick actions.
  std::vector<AstraNtpQuickAction> quick_actions_;

  // Recently closed tabs.
  std::vector<AstraNtpRecentlyClosedTab> recently_closed_;

  // Suggested content.
  std::vector<AstraNtpSuggestedContent> suggested_content_;

  // Enabled suggested content categories.
  std::vector<AstraNtpSuggestedCategory> enabled_suggested_categories_;

  // -- Presentation settings -----------------------------------------------

  bool show_greeting_ = true;
  bool show_search_bar_ = true;
  bool show_workspace_cards_ = true;
  bool show_shortcuts_ = true;
  bool show_recently_closed_ = true;
  bool show_quick_actions_ = true;
  bool show_suggested_content_ = false;

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

  // Theme settings.
  AstraNtpThemeMode theme_mode_ = AstraNtpThemeMode::kSystem;
  SkColor accent_color_ = SK_ColorTRANSPARENT;
  AstraNtpGradientSettings gradient_settings_;

  // Layout density.
  AstraNtpLayoutDensity layout_density_ = AstraNtpLayoutDensity::kCozy;

  // Shortcut display options.
  AstraNtpShortcutIconSize shortcut_icon_size_ =
      AstraNtpShortcutIconSize::kMedium;
  bool show_shortcut_titles_ = true;

  // Clock settings.
  AstraNtpClockFormat clock_format_ = AstraNtpClockFormat::kSystem;
  bool show_seconds_ = false;
  bool show_date_ = true;

  // Search bar settings.
  AstraNtpSearchBarStyle search_bar_style_ =
      AstraNtpSearchBarStyle::kBoxed;
  bool show_search_engine_ = true;
  std::string search_engine_name_ = "Google";

  // Greeting name (empty = use default greeting).
  std::u16string greeting_name_;

  // Observers.
  base::ObserverList<AstraNewTabModelObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_MODEL_H_
