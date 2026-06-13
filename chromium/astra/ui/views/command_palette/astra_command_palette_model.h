#ifndef ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_MODEL_H_
#define ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_MODEL_H_

#include <set>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/strings/string_piece.h"
#include "ui/gfx/range/range.h"
#include "url/gurl.h"

class PrefService;

namespace astra {

// =========================================================================
// Command palette model — merged Chrome + Astra command index
// =========================================================================
//
// AstraCommandPaletteModel maintains a searchable index of all commands
// available through the command palette.  It owns the command data,
// search/ranking logic, selection state, and recently-used tracking.
//
// Command sources:
//   1. Chrome standard commands — IDs from chrome/app/chrome_command_ids.h
//      (IDC_NEW_TAB, IDC_RELOAD, etc.).  Dispatched through Chrome's
//      BrowserCommandController.
//   2. Astra-specific commands — IDs from astra/browser/astra_command_delegate.h
//      (kAstraCommandToggleSidebar, etc.).  Dispatched through
//      AstraCommandDelegate.
//   3. Workspace commands — dynamically generated per workspace
//      (switch to workspace N, create workspace, etc.).
//   4. Recently used commands — a history of commands the user has
//      executed, boosted in search ranking.
//
// Categories (used for grouping and weighting):
//   - kTabs:       tab management (new, close, navigate, pin, mute, etc.)
//   - kNavigation: page navigation (back, forward, reload, home, etc.)
//   - kWorkspaces: workspace management (switch, new, rename, etc.)
//   - kView:       view controls (zoom, fullscreen, sidebar, split view)
//   - kTools:      tools and pages (find, downloads, history, bookmarks,
//                  devtools, passwords, extensions)
//   - kSettings:   settings and preferences
//   - kHelp:       help and about
//
// Architecture boundary:
//   Chromium owns: command execution (BrowserCommandController,
//                  AstraCommandDelegate), accelerator table.
//   Astra owns:    merged command list for search/display,
//                  fuzzy matching logic, recent command history,
//                  selection state.
//
// The model is purely a data/search model — it does not execute commands.
// Command execution is delegated to the view/bubble layer via the
// ExecuteCommand method, which notifies observers.
//
// TODO(astra): Dynamically enumerate Chrome commands instead of hardcoding
// a curated list.  Chrome has ~1000+ command IDs; we should either query
// the command controller for all supported commands or read them from the
// accelerator table at runtime.  Patch point:
// chrome/browser/ui/browser_command_controller.h — add a method that
// returns the set of command IDs the controller knows about, similar to
// how CommandUpdater exposes its command map.
// =========================================================================

// Command type — high-level classification of what kind of command this is.
// Used for type-based filtering and icon selection.  Different from
// AstraCommandCategory which is used for grouping in the results list.
enum class AstraCommandType {
  kAction,      // Generic action command (e.g. new tab, close window)
  kNavigation,  // Navigation command (e.g. back, forward, go to URL)
  kWorkspace,   // Workspace-related command
  kSetting,     // Settings / preferences command
  kBookmark,    // Bookmark command
  kHistory,     // History command
  kTab,         // Tab management command
  kExtension,   // Extension command
};

// Command category — used for grouping, filtering, and search weighting.
// Categories are listed in priority order for empty-query display.
enum class AstraCommandCategory {
  kTabs,
  kNavigation,
  kWorkspaces,
  kBookmarks,
  kHistory,
  kActions,
  kView,
  kTools,
  kSettings,
  kHelp,
};

// Returns a human-readable group label for a category.
// Used as a section header in the command palette results list.
const char16_t* GetCategoryLabel(AstraCommandCategory category);

// Returns the icon name for a category.
// Used for category filter chips and section headers.
const char* GetCategoryIconName(AstraCommandCategory category);

// Returns the human-readable name for a command type.
const char16_t* GetCommandTypeName(AstraCommandType type);

// Returns the total number of categories.
constexpr size_t GetCategoryCount() {
  return static_cast<size_t>(AstraCommandCategory::kHelp) + 1;
}

// =========================================================================
// AstraCommandItem — a single command entry in the palette's search index.
// =========================================================================
//
// Contains all metadata needed to display, search, rank, and execute a
// command.  This is the primary data structure passed between the model
// and views layers.
//
// Fields marked with "(search)" are used by the search/scoring algorithm.
// Fields marked with "(display)" are used by the item view for rendering.
// Fields marked with "(execution)" are used by command execution logic.
// =========================================================================
struct AstraCommandItem {
  // Command ID — either a Chrome IDC_* constant or an Astra kAstraCommand*
  // constant.  (execution)
  int command_id;

  // Human-readable title shown in the results list.  (display, search)
  std::u16string title;

  // Short description shown below the title (secondary text).
  // (display, search)
  std::u16string description;

  // Keyboard shortcut string (e.g. "⌘T" or "Ctrl+T").  (display)
  std::u16string shortcut_text;

  // High-level command type for classification and filtering.
  // (display, search)
  AstraCommandType type;

  // Category for grouping and search weighting.
  // (display, search)
  AstraCommandCategory category;

  // Icon identifier (string key for the icon system).  (display)
  // TODO(astra): Replace with a proper vector icon / gfx::ImageSkia.
  // Chromium component: ui/gfx/vector_icon_types.h
  std::string icon_name;

  // Relevance score from the most recent search.  Higher = better match.
  // Set by SearchCommands / UpdateResults.  (search)
  double relevance_score = 0.0;

  // For navigation commands: the target URL to navigate to.  (execution)
  GURL target_url;

  // For workspace commands: the workspace ID.  (execution)
  std::string workspace_id;

  // Whether this command has been recently used.  (search, display)
  bool is_recent = false;

  // How many times this command has been used.  (search, ranking)
  int use_count = 0;

  // True if this is an Astra-specific command (ID >= 60000).
  // False if it is a standard Chrome command.  (execution)
  bool is_astra = false;

  // True if this is a dynamically-generated workspace command
  // (e.g. "Switch to Workspace 3").  (execution)
  bool is_dynamic_workspace = false;
};

// =========================================================================
// AstraCommandPaletteObserver — observer interface for the model.
// =========================================================================
//
// Views observe the model to stay in sync with search results, command
// list changes, command execution, and model shutdown.  The model
// broadcasts notifications whenever its state changes so views can update
// themselves.
//
// All observer methods have empty default implementations.  Subclasses
// only override the methods they care about.
//
// Extends base::CheckedObserver for safety — observers must be removed
// before destruction, and the observer list verifies this.
// =========================================================================
class AstraCommandPaletteObserver : public base::CheckedObserver {
 public:
  // Called when the set of available commands changes (commands added,
  // removed, or the index is rebuilt).  Views should rebuild their
  // full command list caches.
  virtual void OnCommandListChanged(AstraCommandPaletteModel* model) {}

  // Called when search results change (new query, ranking update, etc.).
  // Views should rebuild their result lists.
  virtual void OnSearchResultsChanged(AstraCommandPaletteModel* model) {}

  // Called after a command has been executed.
  virtual void OnCommandExecuted(AstraCommandPaletteModel* model,
                                 int command_id) {}

  // Called when the model is shutting down.  Observers should drop their
  // reference to the model when this is called.
  virtual void OnCommandPaletteModelShutdown(AstraCommandPaletteModel* model) {}

 protected:
  ~AstraCommandPaletteObserver() override = default;
};

// A group of results belonging to the same category.
struct AstraCommandPaletteResultGroup {
  AstraCommandCategory category;
  std::vector<AstraCommandItem> items;
};

// =========================================================================
// Legacy observer interface (kept for backward compatibility).
// =========================================================================
//
// The existing observer interface used by the view layer.  Maintained for
// backward compatibility with existing code.  New code should prefer
// AstraCommandPaletteObserver.
//
// TODO(astra): Migrate all observers to AstraCommandPaletteObserver and
// remove this legacy interface.
// =========================================================================
class AstraCommandPaletteModelObserver : public base::CheckedObserver {
 public:
  // Called when the set of results changes (new query, commands added,
  // recently used updated, etc.).  Views should rebuild their result lists.
  virtual void OnModelChanged() {}

  // Called when the selected index changes.  Views should update the
  // visual selection highlight.
  virtual void OnSelectionChanged() {}

  // Called when a command should be executed (e.g. user pressed Enter on
  // a selected item).  The delegate layer handles actual execution.
  virtual void OnCommandExecutionRequested(int command_id, bool is_astra) {}

  // Called when the search text changes.  Fires before OnModelChanged
  // so observers can react specifically to text input vs. other model changes.
  virtual void OnSearchTextChanged(const std::u16string& new_text) {}

  // Called when the command palette is opened or closed.
  virtual void OnPaletteOpened() {}
  virtual void OnPaletteClosed() {}

  // Called after a command has been successfully executed.
  // (OnCommandExecutionRequested fires *before* execution; this fires after.)
  virtual void OnCommandExecuted(int command_id, bool is_astra) {}

 protected:
  ~AstraCommandPaletteModelObserver() override = default;
};

// =========================================================================
// Model class
// =========================================================================

class AstraCommandPaletteModel {
 public:
  AstraCommandPaletteModel();
  ~AstraCommandPaletteModel();

  AstraCommandPaletteModel(const AstraCommandPaletteModel&) = delete;
  AstraCommandPaletteModel& operator=(const AstraCommandPaletteModel&) = delete;

  // -- Query / search ----------------------------------------------------

  // Sets the current search query and triggers re-ranking of results.
  // Notifies observers with OnModelChanged and OnSearchResultsChanged.
  void SetQuery(const std::u16string& query);

  // Returns the current search query.
  const std::u16string& query() const { return query_; }

  // Returns the filtered and ranked list of commands matching the current
  // query.  For an empty query, returns recently used + recommended commands.
  const std::vector<AstraCommandItem>& GetResults() const { return results_; }

  // Returns results grouped by category.  Categories are ordered by the
  // highest-scoring item in each category (most relevant categories first).
  // Within each group, items are sorted by relevance.
  const std::vector<AstraCommandPaletteResultGroup>& GetResultGroups() const {
    return result_groups_;
  }

  // Returns the total number of results across all groups.
  size_t GetResultCount() const { return results_.size(); }

  // -- Full command index ------------------------------------------------

  // Returns all commands in the index (unfiltered, unsorted by relevance).
  const std::vector<AstraCommandItem>& GetCommands() const { return commands_; }

  // Returns the total number of commands in the index.
  size_t GetCommandCount() const { return commands_.size(); }

  // -- Search API --------------------------------------------------------

  // Searches the command index for commands matching |query| and returns
  // the filtered and scored results.  Results are sorted by relevance
  // score (highest first).  Uses the current settings for max results,
  // fuzzy search, etc.
  std::vector<AstraCommandItem> SearchCommands(
      const std::u16string& query) const;

  // -- Recent commands ---------------------------------------------------

  // Returns the list of recently used commands (most recent first), up to
  // |max_count| entries.  If |max_count| is 0, uses max_recent_commands
  // from settings.
  std::vector<AstraCommandItem> GetRecentCommands(int max_count) const;

  // Records that a command was used, updating use_count and recency.
  // This affects ranking for future searches.
  void RecordCommandUse(int command_id);

  // Clears the recently used commands list.
  void ClearRecentCommands();

  // -- Default commands --------------------------------------------------

  // Returns the default commands shown when there is no query.
  // This includes recently used commands and recommended commands.
  std::vector<AstraCommandItem> GetDefaultCommands() const;

  // -- Command access ----------------------------------------------------

  // Returns the command at |index| in the current results list.
  // Returns nullptr if the index is out of range.
  const AstraCommandItem* GetCommandAt(int index) const;

  // -- Command execution -------------------------------------------------

  // Executes the command at |index| in the current results list.
  // Records the command as used.  Notifies observers.
  void ExecuteCommand(int index);

  // -- Commands by type --------------------------------------------------

  // Returns all commands of the given type.
  std::vector<AstraCommandItem> GetCommandsByType(AstraCommandType type) const;

  // -- Dynamic commands --------------------------------------------------

  // Adds a dynamic command to the index.  Notifies observers with
  // OnCommandListChanged.
  void AddCommand(const AstraCommandItem& command);

  // Removes a command from the index by command_id.  Returns true if a
  // command was removed.  Notifies observers with OnCommandListChanged
  // if a command was removed.
  bool RemoveCommand(int command_id);

  // -- Ranking -----------------------------------------------------------

  // Recalculates ranking based on usage data.  Call this after bulk
  // updates to use counts or when ranking settings change.
  void UpdateRanking();

  // -- Selection ---------------------------------------------------------

  // Returns the index of the currently selected result, or -1 if no
  // results.
  int GetSelectedIndex() const { return selected_index_; }

  // Returns the selected command item, or nullptr if no selection.
  const AstraCommandItem* GetSelectedItem() const;

  // Sets the selected index.  Clamps to valid range.  Notifies observers
  // with OnSelectionChanged.
  void SetSelectedIndex(int index);

  // Moves the selection by |delta| items (positive = down, negative = up).
  // Clamps to valid range.  Wraps around at edges.
  void MoveSelection(int delta);

  // Moves selection to the first item in the next category group.
  // Wraps around at the last group to the first group.
  void SelectNextGroup();

  // Moves selection to the first item in the previous category group.
  // Wraps around at the first group to the last group.
  void SelectPrevGroup();

  // -- Workspace commands ------------------------------------------------

  // Rebuilds the dynamic workspace commands based on |workspace_count|.
  // This should be called when workspaces are added or removed.
  // Notifies observers with OnModelChanged.
  void UpdateWorkspaceCommands(size_t workspace_count);

  // -- Category filter ---------------------------------------------------

  // Sets the active category filter.  Only commands in the specified
  // categories will appear in results.  An empty set means "all categories".
  // Notifies observers with OnModelChanged.
  void SetCategoryFilter(const std::set<AstraCommandCategory>& categories);

  // Returns the current category filter set.  Empty means no filter (all).
  const std::set<AstraCommandCategory>& category_filter() const {
    return category_filter_;
  }

  // Returns true if |category| passes the current filter.
  bool IsCategoryVisible(AstraCommandCategory category) const;

  // Clears the category filter (show all categories).
  void ClearCategoryFilter();

  // -- Settings ----------------------------------------------------------
  //
  // Presentation and behavior settings for the command palette.  These
  // are stored on the model as UI state — they are not persisted via
  // PrefService (that is handled separately by LoadFromPrefs/SaveToPrefs).

  // Maximum number of search results returned.
  // Default: 20.
  size_t max_search_results() const { return max_search_results_; }
  void set_max_search_results(size_t max);

  // Maximum number of recent commands shown.
  // Default: 10.
  size_t max_recent_commands() const { return max_recent_commands_; }
  void set_max_recent_commands(size_t max);

  // Whether to show command descriptions in results.
  // Default: true.
  bool show_descriptions() const { return show_descriptions_; }
  void set_show_descriptions(bool show);

  // Whether to show keyboard shortcut hints in results.
  // Default: true.
  bool show_shortcuts() const { return show_shortcuts_; }
  void set_show_shortcuts(bool show);

  // Whether to enable fuzzy search (matching characters in order but
  // not necessarily contiguous).  Default: false.
  bool enable_fuzzy_search() const { return enable_fuzzy_search_; }
  void set_enable_fuzzy_search(bool enable);

  // Whether to search in command IDs in addition to titles and
  // descriptions.  Default: false.
  bool search_in_command_ids() const { return search_in_command_ids_; }
  void set_search_in_command_ids(bool enable);

  // Sort mode: true = sort by relevance score, false = sort by usage count.
  // Default: true (sort by relevance).
  bool sort_by_relevance() const { return sort_by_relevance_; }
  void set_sort_by_relevance(bool sort_by_relevance);

  // Whether to automatically execute when there is exactly one search
  // result.  Default: false.
  bool auto_execute_single_result() const { return auto_execute_single_result_; }
  void set_auto_execute_single_result(bool auto_execute);

  // -- Legacy presentation settings (kept for backward compatibility) ---

  // Maximum number of commands visible in the results list.
  // Default: kMaxResults (20).
  size_t max_visible_commands() const { return max_search_results_; }
  void set_max_visible_commands(size_t max);

  // Whether to show the "recent commands" section when the query is empty.
  // Default: true.
  bool show_recent_section() const { return show_recent_section_; }
  void set_show_recent_section(bool show);

  // -- Bulk operations ---------------------------------------------------

  // Executes all currently visible (filtered) commands.  Use with caution —
  // typically only useful for bulk actions like "close all tabs" or batch
  // operations.  Notifies OnCommandExecutionRequested for each command.
  // Returns the number of commands executed.
  size_t ExecuteAllVisible();

  // Executes the first |count| commands in the current results list.
  // Returns the number actually executed.
  size_t ExecuteFirstN(size_t count);

  // -- Palette lifecycle -------------------------------------------------

  // Notifies observers that the palette has been opened.
  void NotifyPaletteOpened();

  // Notifies observers that the palette has been closed.
  void NotifyPaletteClosed();

  // Notifies observers that a command has finished executing.
  void NotifyCommandExecuted(int command_id, bool is_astra);

  // -- Persistence -------------------------------------------------------

  // Loads presentation settings and recent commands from PrefService.
  // Call this after construction when a profile is available.
  void LoadFromPrefs(PrefService* prefs);

  // Saves presentation settings and recent commands to PrefService.
  // Call this before destruction or when settings change.
  void SaveToPrefs(PrefService* prefs) const;

  // -- Observers ---------------------------------------------------------

  // Add/remove new-style observers (AstraCommandPaletteObserver).
  void AddObserver(AstraCommandPaletteObserver* observer);
  void RemoveObserver(AstraCommandPaletteObserver* observer);

  // Add/remove legacy observers (AstraCommandPaletteModelObserver).
  void AddObserver(AstraCommandPaletteModelObserver* observer);
  void RemoveObserver(AstraCommandPaletteModelObserver* observer);

  // -- Constants ---------------------------------------------------------

  // Maximum number of results returned by GetResults.
  static constexpr size_t kMaxResults = 20;

  // Maximum number of recently used commands tracked.
  static constexpr size_t kMaxRecentlyUsed = 10;

  // Maximum number of workspace switch commands generated.
  static constexpr size_t kMaxWorkspaceCommands = 10;

  // -- Scoring helpers (public for testing) -----------------------------

  // Computes a relevance score for |query| against |item|.
  // Returns a score >= 0 if there is a match, negative otherwise.
  // Public for testing.
  double ComputeRelevanceScore(const std::u16string& query,
                               const AstraCommandItem& item) const;

  // Returns true if |query| fuzzy matches |text| (all characters of query
  // appear in order in text).  Public for testing.
  static bool FuzzyMatch(const std::u16string& query,
                         const std::u16string& text);

  // Returns the match ranges for |query| in |text|.  Each range is a
  // contiguous matching substring.  Used for highlighting matched text.
  // Public for testing.
  static std::vector<gfx::Range> GetMatchRanges(
      const std::u16string& query,
      const std::u16string& text);

  // Returns true if |query| matches as an acronym of |text|.
  // For example: "nt" matches "New Tab" by taking first letter of each word.
  // Public for testing.
  static bool IsAcronymMatch(const std::u16string& query,
                             const std::u16string& text);

  // Returns true if |query| matches on word boundaries in |text|.
  // For example: "new tab" matches "New Tab" as word boundary match.
  // Public for testing.
  static bool IsWordBoundaryMatch(const std::u16string& query,
                                  const std::u16string& text);

 private:
  // Builds the full command index from all sources (Chrome, Astra,
  // workspace).  Called at construction and when workspace count changes.
  void BuildCommandIndex();

  // Recomputes the filtered/ranked results list based on the current query.
  void UpdateResults();

  // Rebuilds result_groups_ from the current results_.
  void BuildResultGroups();

  // Returns a category weight multiplier for ranking.
  // Higher values make commands in that category rank higher.
  double GetCategoryWeight(AstraCommandCategory category) const;

  // Returns the index of |command_id| in commands_, or -1 if not found.
  int FindCommandIndex(int command_id) const;

  // Returns the index of |command_id| in recently_used_ids_, or -1 if not found.
  int FindRecentIndex(int command_id) const;

  // Returns the group index of the group containing |result_index|.
  // Returns -1 if the index is invalid.
  int FindGroupIndexForResult(int result_index) const;

  // Returns the flat result index of the first item in |group_index|.
  // Returns -1 if the group index is invalid.
  int GetFirstResultInGroup(int group_index) const;

  // -- Static command sources (Chrome + Astra) ---------------------------

  // Populates the Chrome command entries.
  static void BuildChromeCommands(std::vector<AstraCommandItem>& out);

  // Populates the Astra command entries.
  static void BuildAstraCommands(std::vector<AstraCommandItem>& out);

  // -- Data --------------------------------------------------------------

  // The full command index (all sources merged).
  std::vector<AstraCommandItem> commands_;

  // Current search query.
  std::u16string query_;

  // Filtered and ranked results for the current query.
  std::vector<AstraCommandItem> results_;

  // Results grouped by category.  Categories are ordered by the highest-
  // scoring item in each category.  Within each group, items are
  // sorted by relevance score (highest first).
  std::vector<AstraCommandPaletteResultGroup> result_groups_;

  // Index of the currently selected result, or -1 if no results.
  int selected_index_ = -1;

  // Recently used command IDs, most recent first.
  std::vector<int> recently_used_ids_;

  // Current workspace count (for dynamic workspace commands).
  size_t workspace_count_ = 0;

  // -- Category filter ---------------------------------------------------

  // Set of categories to show.  Empty means "show all categories".
  std::set<AstraCommandCategory> category_filter_;

  // -- Settings ----------------------------------------------------------

  // Maximum number of search results.
  size_t max_search_results_ = kMaxResults;

  // Maximum number of recent commands.
  size_t max_recent_commands_ = kMaxRecentlyUsed;

  // Whether to show descriptions in result items.
  bool show_descriptions_ = true;

  // Whether to show keyboard shortcuts in result items.
  bool show_shortcuts_ = true;

  // Whether to show the recent commands section on empty query.
  bool show_recent_section_ = true;

  // Whether fuzzy search is enabled.
  bool enable_fuzzy_search_ = false;

  // Whether to search in command IDs.
  bool search_in_command_ids_ = false;

  // Whether to sort by relevance vs. usage count.
  bool sort_by_relevance_ = true;

  // Whether to auto-execute single-result queries.
  bool auto_execute_single_result_ = false;

  // -- Observers ---------------------------------------------------------

  // New-style observers.
  base::ObserverList<AstraCommandPaletteObserver> observers_;

  // Legacy observers.
  base::ObserverList<AstraCommandPaletteModelObserver> legacy_observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_MODEL_H_
