#ifndef ASTRA_BROWSER_ASTRA_NEW_TAB_PAGE_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_NEW_TAB_PAGE_SERVICE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "third_party/skia/include/core/SkColor.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace astra {

// =========================================================================
// AstraNewTabPageService — full-featured new tab page service
// =========================================================================
//
// Provides data and settings for the Astra-branded new tab page (NTP).
// This is a projection service — it adapts Chromium-owned data (top sites,
// history, most visited) and Astra-owned metadata (workspaces, favorites,
// custom shortcuts, layout options) into a format suitable for the NTP UI.
//
// Chromium subsystems reused:
//   - TopSites (components/history/core/browser/top_sites.h) — for
//     most-visited site shortcuts on the NTP.
//   - HistoryService (components/history/core/browser/history_service.h)
//     — for recently visited sites and suggested content.
//   - PrefService (components/prefs/pref_service.h) — for persisting
//     NTP settings, custom shortcuts, and theme preferences.
//   - ProfileKeyedServiceFactory pattern.
//
// Truth model:
//   - Most-visited shortcut data from Chromium TopSites / HistoryService.
//   - Workspace data from AstraWorkspaceService.
//   - Favorite data from AstraFavoriteService.
//   - Managed shortcuts, layout, theme, workspace card visibility:
//     owned by this service and persisted via PrefService.
//   - This service aggregates / adapts — it stores only Astra-specific
//     NTP metadata, not browsing history or top sites.
//
// TODO(astra): Wire up real TopSites / HistoryService integration.
//   Currently the service returns placeholder data so the NTP UI can be
//   developed and tested independently of the history subsystem.
//   Patch point: Add a TopSitesObserver or HistoryService consumer to get
//   real data asynchronously.
// TODO(astra): Integrate with Chromium NTP WebUI (astra://newtab).
//   Chromium owner: NewTabPageUI (chrome/browser/ui/webui/new_tab_page/)
// =========================================================================

// -- Enums -------------------------------------------------------------------

// Layout preset for the new tab page.
// Controls which sections are visible and their overall arrangement.
enum class AstraNtpLayout {
  kDefault = 0,        // Shortcut grid + workspace cards + suggested content.
  kShortcutsOnly = 1,  // Just the shortcut grid.
  kWorkspacesOnly = 2, // Just workspace cards.
  kMinimal = 3,        // Just the search bar.
  kCustom = 4,         // Custom layout (user-configured section visibility).
};

// Dark mode preference for the NTP.
enum class AstraNtpDarkMode {
  kAuto = 0,   // Follow system / Chromium theme.
  kLight = 1,  // Force light mode.
  kDark = 2,   // Force dark mode.
};

// Legacy layout density mode (kept for backward compatibility).
// Controls spacing and density of NTP sections.
enum class AstraNtpLayoutMode {
  kStandard,   // Default layout with comfortable spacing.
  kCompact,    // Tighter spacing, more content visible.
  kFocused,    // Minimal layout — focus on search and shortcuts only.
};

// Legacy background type (kept for backward compatibility).
enum class AstraNtpBackgroundType {
  kDefault,      // Default Chromium / Astra theme background.
  kSolidColor,   // Solid color background.
  kCustomImage,  // Custom image background (URL or local file).
};

// -- Structs -----------------------------------------------------------------

// A managed shortcut entry for the NTP shortcut grid.
//
// Shortcuts are user-curated (or default) tiles on the new tab page.
// Each has a stable ID and rich metadata including position, use count,
// and optional workspace association.
//
// Shortcuts are persisted to PrefService and owned by this service.
// They are distinct from Chromium's TopSites / most-visited data, which
// is computed from browsing history.
struct AstraShortcut {
  // Unique identifier for this shortcut.
  // Auto-generated on creation; stable for the lifetime of the shortcut.
  std::string shortcut_id;

  // Display name of the shortcut (site name or page title).
  std::string name;

  // The URL the shortcut navigates to.
  GURL url;

  // Custom icon URL (optional).
  // If empty, Chromium's favicon service provides the default icon.
  // TODO(astra): Integrate with Chromium's favicon service for default icons.
  //   Chromium subsystem: components/favicon/core/favicon_service.h
  GURL icon_url;

  // Whether this is one of the built-in default shortcuts.
  // Default shortcuts can be reset but not permanently removed
  // (they reappear on ResetShortcutsToDefaults).
  bool is_default = false;

  // Grid position index (0-based).
  // Determines the order in the shortcut grid.
  int position = 0;

  // Optional: associated Astra workspace ID.
  // If non-empty, clicking the shortcut opens or switches to this workspace.
  std::string astra_workspace_id;

  // When the shortcut was created.
  base::Time created_time;

  // When the shortcut was last used/clicked.
  base::Time last_used;

  // Number of times this shortcut has been activated.
  // Used for sorting and "most used" suggestions.
  int use_count = 0;
};

// Theme and layout settings for the new tab page.
//
// Controls visual presentation, section visibility, and grid dimensions.
// All fields are persisted to PrefService.
struct AstraNtpTheme {
  // Layout preset.
  // Each preset configures section visibility and arrangement.
  AstraNtpLayout layout = AstraNtpLayout::kDefault;

  // Background color (used in solid-color and default modes).
  SkColor background_color = SK_ColorWHITE;

  // Background image URL (optional).
  // If non-empty and custom background is enabled, this image is used as
  // the NTP background.  May be a chrome:// URL, data URL, or remote URL.
  std::string background_image_url;

  // Whether the shortcut grid section is visible.
  bool show_shortcuts = true;

  // Whether workspace cards section is visible.
  bool show_workspace_cards = true;

  // Whether suggested content cards are visible.
  bool show_suggestions = true;

  // Whether the Google / Astra logo is visible at the top.
  bool show_google_logo = true;

  // Whether the search box is visible.
  bool show_search_box = true;

  // Number of columns in the shortcut grid.
  // Default: 4 columns (standard 4x2 = 8 shortcuts).
  int shortcut_columns = 4;

  // Number of rows in the shortcut grid.
  int shortcut_rows = 2;
};

// A suggested content card for the NTP "For you" section.
//
// Suggestions are projected from Chromium history / most-visited data.
// They are not owned by this service — this service only provides a
// projection layer with Astra-specific metadata (dismissed state, score).
//
// TODO(astra): Project real suggestions from Chromium history / most visited.
//   Currently returns placeholder data.
//   Chromium owner: HistoryService + MostVisitedSites
struct AstraSuggestedContent {
  // The URL of the suggested page.
  GURL url;

  // Title of the suggested content / page.
  std::string title;

  // Publisher or source name (e.g., "The New York Times", "YouTube").
  std::string source;

  // Thumbnail / hero image URL.
  GURL thumbnail_url;

  // Content category (e.g., "news", "technology", "sports", "productivity").
  std::string category;

  // Relevance score (0.0 - 1.0).
  // Higher scores indicate more relevant / interesting suggestions.
  // Used for sorting and filtering.
  double score = 0.0;

  // Whether the user has marked this suggestion as read.
  bool is_read = false;

  // When the content was published / first visited.
  base::Time published_time;
};

// -- Legacy structs (kept for backward compatibility) ------------------------
//
// These types predate the full NTP service deepening.  They are kept for
// backward compatibility with existing callers and tests.
// New code should prefer the AstraShortcut / AstraNtpTheme / etc. types above.

// A single shortcut entry shown on the new tab page (legacy type).
struct AstraNtpShortcut {
  // Display title of the shortcut (site name or page title).
  std::u16string title;

  // The URL the shortcut navigates to.
  GURL url;

  // Optional favicon / tile icon URL.
  // TODO(astra): Use Chromium's favicon service for real icons.
  // Chromium subsystem: components/favicon/core/favicon_service.h
  GURL favicon_url;

  // Whether this shortcut is from the "most visited" set (TopSites)
  // or from a user-added custom shortcut.
  bool is_most_visited = true;
};

// A recently visited entry for the NTP "Recently visited" section.
struct AstraNtpRecentVisit {
  std::u16string title;
  GURL url;
  base::Time visit_time;
};

// Workspace summary for the NTP workspace cards section.
struct AstraNtpWorkspaceSummary {
  std::string id;
  std::string name;
  std::string accent_color;
  int tab_count = 0;
  bool is_active = false;
};

// -- Observers ---------------------------------------------------------------

// Observer interface for AstraNewTabPageService (expanded / new API).
//
// UI layers (NTP WebUI, Views NTP bubble) should observe this service
// to update their presentation when NTP state changes.  UI must never
// be the source of truth — this service is.
//
// All methods have empty default implementations so observers only need
// to override the events they care about.
class AstraNewTabPageObserver : public base::CheckedObserver {
 public:
  // Called when a new shortcut is added to the NTP.
  // |shortcut_id| identifies the newly added shortcut.
  virtual void OnShortcutAdded(AstraNewTabPageService* service,
                               const std::string& shortcut_id) {}

  // Called when a shortcut is removed from the NTP.
  // |shortcut_id| identifies the removed shortcut.
  virtual void OnShortcutRemoved(AstraNewTabPageService* service,
                                 const std::string& shortcut_id) {}

  // Called when a shortcut's properties (name, URL, icon) change.
  // |shortcut_id| identifies the changed shortcut.
  virtual void OnShortcutChanged(AstraNewTabPageService* service,
                                 const std::string& shortcut_id) {}

  // Called when the order of shortcuts changes (reorder operation).
  virtual void OnShortcutsReordered(AstraNewTabPageService* service) {}

  // Called when NTP theme or layout settings change.
  // This includes layout preset, background color, section visibility,
  // grid dimensions, and any other theme-related setting.
  virtual void OnNtpThemeChanged(AstraNewTabPageService* service) {}

  // Called when a workspace card's visibility on the NTP changes.
  // |workspace_id| identifies the workspace, |visible| is the new state.
  virtual void OnWorkspaceCardVisibilityChanged(
      AstraNewTabPageService* service,
      const std::string& workspace_id,
      bool visible) {}

  // Called when the suggested content list is updated.
  // This can happen due to a refresh, dismissal, or restore operation.
  virtual void OnSuggestionsChanged(AstraNewTabPageService* service) {}

  // Called when the service is shutting down.
  // Observers should remove themselves and drop any references to the
  // service in this callback.  The service pointer is still valid during
  // the callback but will be destroyed shortly after.
  virtual void OnNewTabPageServiceShutdown(AstraNewTabPageService* service) {}
};

// Legacy observer interface (kept for backward compatibility).
// New code should use AstraNewTabPageObserver instead.
class AstraNewTabPageServiceObserver : public base::CheckedObserver {
 public:
  // Called when custom shortcuts are added, removed, updated, or reordered.
  virtual void OnCustomShortcutsChanged() {}

  // Called when NTP layout options or section visibility changes.
  virtual void OnLayoutChanged() {}

  // Called when the background image, color, or type changes.
  virtual void OnBackgroundChanged() {}
};

// =========================================================================
// AstraNewTabPageService
// =========================================================================

class AstraNewTabPageService final : public KeyedService {
 public:
  // -- Pref keys (public static for factory registration) ------------------
  //
  // These are declared as public static constexpr so the factory and tests
  // can reference them by name.  All prefs live under the "astra.ntp."
  // namespace to avoid collisions with Chromium built-in prefs.

  // List of managed shortcut dictionaries (base::Value::List).
  // Each entry is a dict with keys matching AstraShortcut fields.
  static constexpr const char kPrefShortcuts[] = "astra.ntp.shortcuts";

  // NTP layout preset (integer, see AstraNtpLayout).
  static constexpr const char kPrefNtpLayout[] = "astra.ntp.layout";

  // Background color as a hex string (e.g., "#FFEEDD").
  static constexpr const char kPrefBackgroundColor[] =
      "astra.ntp.background_color";

  // Background image URL (string).
  static constexpr const char kPrefBackgroundImageUrl[] =
      "astra.ntp.background_image_url";

  // Whether the shortcuts section is visible (boolean).
  static constexpr const char kPrefShowShortcuts[] =
      "astra.ntp.show_shortcuts";

  // Whether the workspace cards section is visible (boolean).
  static constexpr const char kPrefShowWorkspaceCards[] =
      "astra.ntp.show_workspace_cards";

  // Whether the suggested content section is visible (boolean).
  static constexpr const char kPrefShowSuggestions[] =
      "astra.ntp.show_suggestions";

  // Whether the Google / Astra logo is visible (boolean).
  static constexpr const char kPrefShowGoogleLogo[] =
      "astra.ntp.show_google_logo";

  // Whether the search box is visible (boolean).
  static constexpr const char kPrefShowSearchBox[] =
      "astra.ntp.show_search_box";

  // Number of shortcut grid columns (integer).
  static constexpr const char kPrefShortcutColumns[] =
      "astra.ntp.shortcut_columns";

  // Number of shortcut grid rows (integer).
  static constexpr const char kPrefShortcutRows[] =
      "astra.ntp.shortcut_rows";

  // Maximum number of workspace cards to show (integer).
  static constexpr const char kPrefMaxWorkspaceCards[] =
      "astra.ntp.max_workspace_cards";

  // Maximum number of suggestion cards to show (integer).
  static constexpr const char kPrefMaxSuggestions[] =
      "astra.ntp.max_suggestions";

  // Whether to show most-visited sites in shortcuts (boolean).
  static constexpr const char kPrefShowMostVisited[] =
      "astra.ntp.show_most_visited";

  // Whether to show recently closed tabs section (boolean).
  static constexpr const char kPrefShowRecentlyClosed[] =
      "astra.ntp.show_recently_closed";

  // Dark mode preference (integer, see AstraNtpDarkMode).
  static constexpr const char kPrefDarkMode[] = "astra.ntp.dark_mode";

  // Whether custom background is enabled (boolean).
  static constexpr const char kPrefCustomBackgroundEnabled[] =
      "astra.ntp.custom_background_enabled";

  // Workspace card visibility map (dict: workspace_id -> boolean).
  // Controls which workspaces appear as cards on the NTP.
  static constexpr const char kPrefWorkspaceCardVisibility[] =
      "astra.ntp.workspace_card_visibility";

  // Workspace card display order (list of workspace_id strings).
  // Determines the order of workspace cards on the NTP.
  static constexpr const char kPrefWorkspaceCardOrder[] =
      "astra.ntp.workspace_card_order";

  // Dismissed suggestion URLs (list of strings).
  // Suggestions matching these URLs are hidden from the suggestions list.
  static constexpr const char kPrefDismissedSuggestions[] =
      "astra.ntp.dismissed_suggestions";

  // Whether the suggestions feature is enabled (boolean).
  // When disabled, GetSuggestedContent returns an empty list.
  static constexpr const char kPrefSuggestionsEnabled[] =
      "astra.ntp.suggestions_enabled";

  // -- Construction / destruction ------------------------------------------

  explicit AstraNewTabPageService(Profile* profile);
  AstraNewTabPageService(const AstraNewTabPageService&) = delete;
  AstraNewTabPageService& operator=(const AstraNewTabPageService&) = delete;
  ~AstraNewTabPageService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observer management (new API) ---------------------------------------

  // Adds/removes an observer for NTP state changes.
  void AddObserver(AstraNewTabPageObserver* observer);
  void RemoveObserver(AstraNewTabPageObserver* observer);

  // -- Observer management (legacy API) ------------------------------------

  // Adds/removes a legacy observer.
  void AddObserver(AstraNewTabPageServiceObserver* observer);
  void RemoveObserver(AstraNewTabPageServiceObserver* observer);

  // -----------------------------------------------------------------------
  // Managed shortcuts (new API)
  // -----------------------------------------------------------------------
  //
  // These methods operate on the AstraShortcut model — a richer shortcut
  // type with stable IDs, positions, use counts, and workspace association.
  // Shortcuts are persisted to PrefService and owned by this service.

  // Returns all shortcuts in position order.
  // Shortcuts are sorted by the |position| field ascending.
  std::vector<AstraShortcut> GetShortcuts() const;

  // Returns the number of shortcuts.
  size_t GetShortcutCount() const;

  // Adds a new shortcut with the given name and URL.
  // |position| specifies the grid position; if -1 (default), the shortcut
  // is appended at the end.
  // Returns the ID of the newly created shortcut.
  // Notifies observers via OnShortcutAdded.
  std::string AddShortcut(const std::string& name,
                          const GURL& url,
                          int position = -1);

  // Removes the shortcut with the given ID.
  // Returns true if the shortcut was found and removed.
  // Notifies observers via OnShortcutRemoved.
  bool RemoveShortcut(const std::string& shortcut_id);

  // Updates the name and URL of the shortcut with the given ID.
  // Returns true if the shortcut was found and updated.
  // Notifies observers via OnShortcutChanged.
  bool UpdateShortcut(const std::string& shortcut_id,
                      const std::string& name,
                      const GURL& url);

  // Reorders shortcuts according to the given list of IDs.
  // |shortcut_ids_in_order| must contain all current shortcut IDs exactly
  // once.  If validation fails, no change is made.
  // Notifies observers via OnShortcutsReordered.
  void ReorderShortcuts(const std::vector<std::string>& shortcut_ids_in_order);

  // Returns the shortcut with the given ID, or nullopt if not found.
  std::optional<AstraShortcut> GetShortcut(
      const std::string& shortcut_id) const;

  // Sets a custom icon URL for the shortcut with the given ID.
  // Returns true if the shortcut was found and the icon was set.
  // Notifies observers via OnShortcutChanged.
  bool SetShortcutIcon(const std::string& shortcut_id,
                       const GURL& icon_url);

  // Resets shortcuts to the default set.
  // Removes all user-added shortcuts and restores the default shortcuts.
  // Notifies observers via OnShortcutsReordered (bulk change).
  void ResetShortcutsToDefaults();

  // Returns the list of default shortcut definitions.
  // These are the shortcuts that appear when ResetShortcutsToDefaults
  // is called or on a fresh profile.
  static std::vector<AstraShortcut> GetDefaultShortcuts();

  // Returns true if the shortcut with the given ID is a default shortcut.
  bool IsDefaultShortcut(const std::string& shortcut_id) const;

  // -----------------------------------------------------------------------
  // Workspace cards on NTP
  // -----------------------------------------------------------------------
  //
  // Workspace cards show the user's workspaces on the new tab page.
  // The actual workspace data comes from AstraWorkspaceService.
  // This service only owns the visibility and ordering metadata.

  // Returns workspace cards configured to show on the NTP.
  // Respects visibility settings, max card count, and custom order.
  // TODO(astra): Integrate with AstraWorkspaceService for real data.
  std::vector<AstraNtpWorkspaceSummary> GetWorkspaceCards() const;

  // Returns the number of visible workspace cards.
  size_t GetVisibleWorkspacesCount() const;

  // Sets whether a workspace card is visible on the NTP.
  // Notifies observers via OnWorkspaceCardVisibilityChanged.
  void ShowWorkspace(const std::string& workspace_id, bool show);

  // Returns whether the workspace card is currently visible.
  bool IsWorkspaceVisible(const std::string& workspace_id) const;

  // Sets the maximum number of workspace cards to display.
  // Notifies observers via OnNtpThemeChanged (layout change).
  void SetMaxWorkspaceCards(int max);

  // Returns the maximum number of workspace cards.
  int GetMaxWorkspaceCards() const;

  // Reorders workspace cards according to the given list of workspace IDs.
  // Updates the workspace card order preference.
  // Notifies observers via OnNtpThemeChanged.
  void ReorderWorkspaceCards(const std::vector<std::string>& workspace_ids);

  // -----------------------------------------------------------------------
  // Suggested content
  // -----------------------------------------------------------------------
  //
  // Suggested content cards are projected from Chromium history / most
  // visited data.  This service only owns the dismissed / hidden state.
  //
  // TODO(astra): Wire to Chromium history / most visited for real data.
  //   Currently returns placeholder data.
  //   Chromium owner: HistoryService + MostVisitedSites

  // Returns up to |max_count| suggested content cards.
  // Filters out dismissed suggestions.
  // Suggestions are sorted by score descending.
  std::vector<AstraSuggestedContent> GetSuggestedContent(int max_count) const;

  // Dismisses / hides a suggestion by URL.
  // Dismissed suggestions no longer appear in GetSuggestedContent.
  // Notifies observers via OnSuggestionsChanged.
  void DismissSuggestion(const GURL& url);

  // Restores all dismissed suggestions.
  // Notifies observers via OnSuggestionsChanged.
  void RestoreDismissedSuggestions();

  // Returns the number of dismissed suggestions.
  size_t GetDismissedSuggestionCount() const;

  // Triggers a refresh of suggested content.
  // In the stub implementation, this regenerates placeholder suggestions.
  // Notifies observers via OnSuggestionsChanged.
  // TODO(astra): Trigger real refresh from HistoryService.
  void RefreshSuggestions();

  // Returns whether the suggestions feature is enabled.
  bool IsSuggestionsEnabled() const;

  // -----------------------------------------------------------------------
  // Layout & theme settings
  // -----------------------------------------------------------------------

  // Returns the full NTP theme struct.
  AstraNtpTheme GetNtpTheme() const;

  // Sets all theme settings from the given struct.
  // Notifies observers via OnNtpThemeChanged.
  void SetNtpTheme(const AstraNtpTheme& theme);

  // Resets all theme settings to their defaults.
  // Notifies observers via OnNtpThemeChanged.
  void ResetNtpTheme();

  // Individual theme getters / setters.

  // NTP layout preset.
  AstraNtpLayout layout() const;
  void set_layout(AstraNtpLayout layout);

  // Background color.
  SkColor background_color() const;
  void set_background_color(SkColor color);

  // Background image URL.
  std::string background_image_url() const;
  void set_background_image_url(const std::string& url);

  // Whether shortcuts section is visible.
  bool show_shortcuts() const;
  void set_show_shortcuts(bool show);

  // Whether workspace cards section is visible.
  bool show_workspace_cards() const;
  void set_show_workspace_cards(bool show);

  // Whether suggestions section is visible.
  bool show_suggestions() const;
  void set_show_suggestions(bool show);

  // Whether the logo is visible.
  bool show_google_logo() const;
  void set_show_google_logo(bool show);

  // Whether the search box is visible.
  bool show_search_box() const;
  void set_show_search_box(bool show);

  // Shortcut grid columns.
  int shortcut_columns() const;
  void set_shortcut_columns(int columns);

  // Shortcut grid rows.
  int shortcut_rows() const;
  void set_shortcut_rows(int rows);

  // -- Additional NTP settings ---------------------------------------------

  // Maximum number of suggestions to show.
  int max_suggestions() const;
  void set_max_suggestions(int max);

  // Whether to show most-visited sites.
  bool show_most_visited() const;
  void set_show_most_visited(bool show);

  // Whether to show recently closed tabs.
  bool show_recently_closed() const;
  void set_show_recently_closed(bool show);

  // Dark mode preference for the NTP.
  AstraNtpDarkMode dark_mode() const;
  void set_dark_mode(AstraNtpDarkMode mode);

  // Whether custom background is enabled.
  bool custom_background_enabled() const;
  void set_custom_background_enabled(bool enabled);

  // -----------------------------------------------------------------------
  // Legacy API (kept for backward compatibility)
  // -----------------------------------------------------------------------

  // -- Shortcut data (legacy) ----------------------------------------------

  // Returns the top |count| most-visited site shortcuts.
  // TODO(astra): Return real data from Chromium's TopSites service.
  std::vector<AstraNtpShortcut> GetTopSites(size_t count) const;

  // Returns the |count| most recently visited pages (from history).
  // TODO(astra): Return real data from Chromium's HistoryService.
  std::vector<AstraNtpRecentVisit> GetRecentlyVisited(size_t count) const;

  // -- Workspace summary (legacy) ------------------------------------------

  // Returns workspace summaries for the NTP workspace cards section.
  // Reads workspace data from AstraWorkspaceService.
  // TODO(astra): Compute real tab counts per workspace from TabStripModel.
  std::vector<AstraNtpWorkspaceSummary> GetWorkspaceSummaries() const;

  // -- Favorite shortcuts (legacy) -----------------------------------------

  // Returns favorite site shortcuts (from Astra favorite folders).
  // TODO(astra): Build favorite shortcuts from AstraFavoriteService.
  std::vector<AstraNtpShortcut> GetFavoriteShortcuts(size_t count) const;

  // -- Custom shortcuts (legacy index-based API) ---------------------------

  size_t AddCustomShortcut(const std::u16string& title, const GURL& url);
  bool RemoveCustomShortcut(size_t index);
  bool UpdateCustomShortcut(size_t index,
                            const std::u16string& title,
                            const GURL& url);
  void ReorderCustomShortcuts(const std::vector<size_t>& ordered_indices);
  size_t GetCustomShortcutCount() const;
  std::vector<AstraNtpShortcut> GetCustomShortcuts(size_t count) const;
  std::vector<AstraNtpShortcut> GetAllShortcuts(size_t count) const;

  // -- Layout options (legacy) ---------------------------------------------

  AstraNtpLayoutMode layout_mode() const;
  void set_layout_mode(AstraNtpLayoutMode mode);
  std::string GetLayoutModeName() const;

  bool show_recently_visited() const;
  void set_show_recently_visited(bool show);

  bool show_favorites() const;
  void set_show_favorites(bool show);

  // -- Background / theme (legacy) -----------------------------------------

  AstraNtpBackgroundType background_type() const;
  void set_background_type(AstraNtpBackgroundType type);
  SkColor GetBackgroundColor() const;
  void SetBackgroundColor(SkColor color);

 private:
  // -- Shortcut helpers ----------------------------------------------------

  // Loads all shortcuts from prefs as a list of dicts.
  base::Value::List LoadShortcutsFromPrefs() const;

  // Saves the given shortcut list to prefs.
  void SaveShortcutsToPrefs(base::Value::List shortcuts);

  // Converts a base::Value::Dict to an AstraShortcut.
  // Returns nullopt if the dict is invalid.
  static std::optional<AstraShortcut> ShortcutFromDict(
      const base::Value::Dict& dict);

  // Converts an AstraShortcut to a base::Value::Dict.
  static base::Value::Dict ShortcutToDict(const AstraShortcut& shortcut);

  // Generates a new unique shortcut ID.
  static std::string GenerateShortcutId();

  // Finds the index of a shortcut by ID in a list of shortcut dicts.
  // Returns -1 if not found.
  static int FindShortcutIndexById(const base::Value::List& shortcuts,
                                   const std::string& shortcut_id);

  // -- Theme / settings helpers --------------------------------------------

  // Notifies new-style observers that the theme changed.
  void NotifyNtpThemeChanged();

  // Notifies new-style observers about a shortcut addition.
  void NotifyShortcutAdded(const std::string& shortcut_id);

  // Notifies new-style observers about a shortcut removal.
  void NotifyShortcutRemoved(const std::string& shortcut_id);

  // Notifies new-style observers about a shortcut change.
  void NotifyShortcutChanged(const std::string& shortcut_id);

  // Notifies new-style observers that shortcuts were reordered.
  void NotifyShortcutsReordered();

  // Notifies new-style observers about workspace card visibility change.
  void NotifyWorkspaceCardVisibilityChanged(
      const std::string& workspace_id,
      bool visible);

  // Notifies new-style observers that suggestions changed.
  void NotifySuggestionsChanged();

  // Notifies new-style observers that the service is shutting down.
  void NotifyShutdown();

  // -- Legacy notification helpers -----------------------------------------

  void NotifyCustomShortcutsChanged();
  void NotifyLayoutChanged();
  void NotifyBackgroundChanged();

  // -- Legacy custom shortcut pref helpers ---------------------------------

  base::Value::List LoadCustomShortcutsFromPrefs() const;
  void SaveCustomShortcutsToPrefs(base::Value::List shortcuts);

  // -- Members -------------------------------------------------------------

  raw_ptr<Profile> profile_;

  // New-style observer list (AstraNewTabPageObserver).
  base::ObserverList<AstraNewTabPageObserver> observers_;

  // Legacy observer list (AstraNewTabPageServiceObserver).
  base::ObserverList<AstraNewTabPageServiceObserver> legacy_observers_;
};

// =========================================================================
// AstraNewTabPageServiceFactory
// =========================================================================
//
// Factory for AstraNewTabPageService.
//
// Incognito behavior: uses kRedirectedToOriginal because the NTP data
// (shortcuts, workspaces, theme) is product-level state shared with
// the main profile.  An incognito new tab still shows the same shortcuts
// and workspaces — only the browsing session is isolated.
// Guest sessions get their own instance (kOwnInstance).
// =========================================================================

class AstraNewTabPageServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraNewTabPageService instance for |profile|.
  static AstraNewTabPageService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraNewTabPageServiceFactory* GetInstance();

  // Registers all NTP-related profile prefs on the given registry.
  // Uses the pref key constants defined in AstraNewTabPageService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraNewTabPageServiceFactory>;

  AstraNewTabPageServiceFactory();
  ~AstraNewTabPageServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_NEW_TAB_PAGE_SERVICE_H_
