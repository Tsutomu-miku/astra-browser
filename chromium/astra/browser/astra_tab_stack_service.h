#ifndef ASTRA_BROWSER_ASTRA_TAB_STACK_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_TAB_STACK_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class PrefService;
class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace astra {

// =========================================================================
// AstraTabStackId
// =========================================================================
//
// Opaque identifier for a named tab stack.  Uses std::string for easy
// serialization in prefs and compatibility with WebContentsUserData
// string fields.
//
// Generated from base::UnguessableToken for uniqueness across sessions
// and devices.
//
// Chromium analog: tab_groups::TabGroupId (chrome/browser/ui/tabs/tab_group.h)
//   TabGroupId uses base::Token internally; we use std::string for simpler
//   pref serialization and to match the pattern of workspace_id.
// =========================================================================

using AstraTabStackId = std::string;

// =========================================================================
// AstraTabStackInfo
// =========================================================================
//
// Full metadata for a named tab stack — an Astra feature for grouping tabs
// into named, colored, ordered stacks (like Vivaldi tab stacks or Arc
// tab orphans).
//
// Each stack has:
//   - stack_id: unique opaque identifier (from base::UnguessableToken)
//   - name: user-visible display name
//   - color: accent color (hex string, e.g. "#5B8FF9")
//   - is_collapsed: whether the stack is collapsed in the sidebar
//   - is_pinned: whether the stack is pinned to the top of the list
//   - note: user-added note / description for the stack
//   - tab_indices: ordered list of tab indices in this stack
//   - order_index: sort position in the stack list
//   - created_time: when the stack was created
//   - last_accessed: when the stack was last accessed / viewed
//   - tab_count: number of tabs in the stack (derived from tab_indices)
//
// Truth model:
//   - Stack metadata is owned by AstraTabStackService and persisted via
//     PrefService (profile-scoped).
//   - Tab membership order is stored as tab_indices within the stack.
//   - Tab ownership, order, and lifecycle are entirely Chromium's
//     (TabStripModel, WebContents).  tab_indices here refer to positions
//     within TabStripModel and are kept in sync via TabStripModelObserver.
//
// Chromium analog: TabGroup (chrome/browser/ui/tabs/tab_group.h)
//   Tab groups are flat and color-based; stacks are richer (named, ordered,
//   sidebar-projected, with notes and pinning).
//
// TODO(astra): Evaluate integrating with Chromium's TabGroup model.
//   Chromium owner: tab_groups (chrome/browser/ui/tabs/tab_group.h)
//   Patch point: TabGroupModel — extend or wrap with Astra metadata.
// =========================================================================

struct AstraTabStackInfo {
  // Unique identifier for the stack.
  AstraTabStackId stack_id;

  // User-visible display name.
  std::string name;

  // Accent color as hex string, e.g. "#5B8FF9".
  std::string color;

  // Whether the stack is collapsed (child tabs hidden in sidebar).
  bool is_collapsed = false;

  // Whether the stack is pinned to the top of the stack list.
  bool is_pinned = false;

  // User note / description for this stack.
  std::string note;

  // Ordered list of tab indices (positions in TabStripModel) for tabs
  // in this stack.  The order determines presentation in the sidebar.
  // Indices are kept in sync with TabStripModel via the service's
  // TabStripModelObserver.
  //
  // TODO(astra): Replace int indices with stable tab identifiers.
  //   Indices are fragile — they change when tabs move in the strip.
  //   Consider using WebContents* or tab unique IDs instead.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  std::vector<int> tab_indices;

  // Sort position in the stack list (0-based).
  int order_index = 0;

  // When the stack was created.
  base::Time created_time;

  // When the stack was last accessed / viewed.
  base::Time last_accessed;

  // Number of tabs in the stack (derived from tab_indices size).
  size_t tab_count = 0;

  // Returns true if the stack has no tabs.
  bool IsEmpty() const { return tab_indices.empty(); }

  // Returns true if the tab at |tab_index| is in this stack.
  bool HasTab(int tab_index) const {
    for (int idx : tab_indices) {
      if (idx == tab_index) {
        return true;
      }
    }
    return false;
  }
};

// =========================================================================
// AstraTabStackObserver
// =========================================================================
//
// Observer interface for UI layers (sidebar, tab strip overlays) to react
// to tab stack changes.  UI must never be the source of truth —
// AstraTabStackService is.
//
// All methods have empty default implementations so observers only need
// to override the events they care about.
//
// Chromium owner: TabGroupModelObserver (analogous observer for tab groups)
//   (chrome/browser/ui/tabs/tab_group_model_observer.h)
// =========================================================================

class AstraTabStackObserver : public base::CheckedObserver {
 public:
  // Called after a new stack is created.
  virtual void OnStackCreated(AstraTabStackService* service,
                              const std::string& stack_id) {}

  // Called after a stack is deleted.
  // Tabs that were in the stack have already been unstacked.
  virtual void OnStackDeleted(AstraTabStackService* service,
                              const std::string& stack_id) {}

  // Called after stack metadata changes (name, color, note, pinned, etc.).
  virtual void OnStackChanged(AstraTabStackService* service,
                              const std::string& stack_id) {}

  // Called after a stack's collapsed state changes.
  virtual void OnStackCollapsedChanged(AstraTabStackService* service,
                                       const std::string& stack_id,
                                       bool collapsed) {}

  // Called after a tab is added to a stack.
  virtual void OnTabAddedToStack(AstraTabStackService* service,
                                 int tab_index,
                                 const std::string& stack_id) {}

  // Called after a tab is removed from a stack.
  virtual void OnTabRemovedFromStack(AstraTabStackService* service,
                                     int tab_index,
                                     const std::string& stack_id) {}

  // Called after the stack list is reordered.
  virtual void OnStacksReordered(AstraTabStackService* service) {}

  // Called when the service is shutting down.
  // Observers should remove themselves in response.
  virtual void OnTabStackServiceShutdown(AstraTabStackService* service) {}

 protected:
  ~AstraTabStackObserver() override = default;
};

// =========================================================================
// AstraTabStackService
// =========================================================================
//
// Profile-scoped keyed service that manages named tab stacks.
//
// Tab stacks are an Astra feature: tabs can be grouped into named, colored,
// ordered stacks that appear as collapsible sections in the sidebar.  This
// is similar to Vivaldi's tab stacks or Arc's tab orphans.
//
// Truth model:
//   - Stack definitions (name, color, order, collapsed state, note, pinned,
//     timestamps) are owned by this service and persisted via PrefService
//     (profile prefs).
//   - Stack membership (which tab belongs to which stack) is stored as
//     ordered tab_indices within each stack's metadata.
//   - Tab ownership, order, and lifecycle are entirely Chromium's
//     (TabStripModel, WebContents).
//   - This service orchestrates stack operations and provides a clean API
//     for UI layers.
//
// Why a service instead of just WebContentsUserData:
//   - Stack operations involve both stack metadata and per-tab membership,
//     and need a coordinator to maintain consistency.
//   - Tab removal cleanup needs to observe TabStripModel across windows.
//   - Observer pattern for UI updates belongs at the service layer.
//   - Persistence through PrefService requires a profile-scoped service.
//
// Chromium subsystems reused:
//   - TabStripModel (tab ownership, order, lifecycle).
//   - TabStripModelObserver (for tab removal cleanup).
//   - ProfileKeyedServiceFactory pattern.
//   - PrefService for stack metadata persistence.
//   - base::UnguessableToken for unique stack IDs.
//
// Chromium analog: TabGroupModel + TabGroup (chrome/browser/ui/tabs/)
//   Tab groups are flat (label/color based); stacks are named, ordered,
//   and have richer sidebar projection (notes, pinned, timestamps).
// =========================================================================

class AstraTabStackService final : public KeyedService {
 public:
  // -- Pref keys (public so factory can register them) --------------------
  //
  // These are the pref keys for stack-related user settings.
  // Stack data itself is stored under kPrefTabStacks (see astra_prefs.h).
  //
  // Format follows Chromium pref conventions: dot-separated path under
  // the "astra" namespace.

  // Default color for newly created stacks (string, hex color).
  // Default: "#5B8FF9" (Astra blue).
  static constexpr const char kPrefStackDefaultColor[] =
      "astra.tab_stacks.default_color";

  // Whether inactive stacks auto-collapse after a period of time (bool).
  // Default: false — stacks stay expanded until manually collapsed.
  static constexpr const char kPrefStackAutoCollapseInactive[] =
      "astra.tab_stacks.auto_collapse_inactive";

  // Time in seconds after which inactive stacks auto-collapse (int).
  // Only applies if auto_collapse_inactive is true.
  // Default: 300 seconds (5 minutes).
  static constexpr const char kPrefStackAutoCollapseAfterSeconds[] =
      "astra.tab_stacks.auto_collapse_after_seconds";

  // Whether to show a count badge on stack headers (bool).
  // The badge shows the number of stacks or items.
  // Default: true — count badges provide useful context.
  static constexpr const char kPrefStackShowCountBadge[] =
      "astra.tab_stacks.show_count_badge";

  // Whether to show the tab count inside each stack (bool).
  // When true, the stack header shows the number of tabs.
  // Default: true — tab count is useful information.
  static constexpr const char kPrefStackShowTabCount[] =
      "astra.tab_stacks.show_tab_count";

  // Behavior when creating a stack from existing tabs (int enum).
  // 0 = Tabs remain in their current order (preserve tab strip order).
  // 1 = Tabs move to the front of the tab strip.
  // Default: 0 — preserve order.
  static constexpr const char kPrefStackCreationBehavior[] =
      "astra.tab_stacks.creation_behavior";

  // Whether to auto-stack related tabs (e.g. tabs from the same domain) (bool).
  // When true, newly opened tabs that are related to an existing stack
  // are automatically added to that stack.
  // Default: false — users control stacking explicitly.
  static constexpr const char kPrefStackAutoStackRelated[] =
      "astra.tab_stacks.auto_stack_related";

  // Minimum number of tabs per stack before the stack is considered
  // "valid" or shown in certain views (int).
  // 0 = no minimum / unlimited.
  // Default: 0 — stacks can have any number of tabs.
  static constexpr const char kPrefStackMinimumTabsPerStack[] =
      "astra.tab_stacks.minimum_tabs_per_stack";

  // -- Stack creation behavior enum values --------------------------------

  // Tabs remain in their current tab strip order when stacked.
  static constexpr int kStackCreationBehaviorPreserveOrder = 0;

  // Tabs move to the front of the tab strip when stacked.
  static constexpr int kStackCreationBehaviorMoveToFront = 1;

  // -- Default values -----------------------------------------------------

  static constexpr const char kDefaultStackColor[] = "#5B8FF9";
  static constexpr bool kDefaultAutoCollapseInactive = false;
  static constexpr int kDefaultAutoCollapseAfterSeconds = 300;  // 5 minutes
  static constexpr bool kDefaultShowCountBadge = true;
  static constexpr bool kDefaultShowTabCount = true;
  static constexpr int kDefaultStackCreationBehavior =
      kStackCreationBehaviorPreserveOrder;
  static constexpr bool kDefaultAutoStackRelated = false;
  static constexpr int kDefaultMinimumTabsPerStack = 0;

  // =======================================================================
  // Construction / destruction
  // =======================================================================

  explicit AstraTabStackService(Profile* profile);
  AstraTabStackService(const AstraTabStackService&) = delete;
  AstraTabStackService& operator=(const AstraTabStackService&) = delete;
  ~AstraTabStackService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraTabStackObserver* observer);
  void RemoveObserver(AstraTabStackObserver* observer);

  // =======================================================================
  // Stack management
  // =======================================================================

  // Creates a new empty stack with the given name and optional color.
  // Returns the ID of the new stack.
  //
  // Fires OnStackCreated.
  AstraTabStackId CreateStack(const std::string& name,
                              const std::string& color = std::string());

  // Deletes a stack.  All tabs in the stack become unstacked (removed
  // from the stack).  The tabs themselves are not closed.
  // Returns true if the stack existed and was deleted.
  //
  // Fires OnTabRemovedFromStack for each tab, then OnStackDeleted.
  bool DeleteStack(const std::string& stack_id);

  // Renames a stack.  Returns true if the stack existed.
  //
  // Fires OnStackChanged if the name changed.
  bool RenameStack(const std::string& stack_id,
                   const std::string& new_name);

  // Changes the color of a stack.  Returns true if the stack existed.
  //
  // Fires OnStackChanged if the color changed.
  bool SetStackColor(const std::string& stack_id,
                     const std::string& color);

  // Returns the stack with the given ID, or nullptr if not found.
  // The returned pointer is valid only until the next mutation.
  const AstraTabStackInfo* GetStack(const std::string& stack_id) const;

  // Returns all stacks, sorted by order_index (pinned first, then
  // by order_index within each group).
  std::vector<AstraTabStackInfo> GetAllStacks() const;

  // Returns the number of stacks.
  size_t GetStackCount() const;

  // Returns true if a stack with the given ID exists.
  bool DoesStackExist(const std::string& stack_id) const;

  // =======================================================================
  // Tab-to-stack membership
  // =======================================================================

  // Adds a tab to a stack.  If the tab is already in another stack,
  // it is first removed from that stack.
  // Returns true on success (stack exists and tab was added).
  //
  // Fires OnTabRemovedFromStack (if previously in another stack)
  // and OnTabAddedToStack.
  bool AddTabToStack(int tab_index, const std::string& stack_id);

  // Removes a tab from a specific stack.
  // Returns false if the tab is not in |stack_id|.
  //
  // Fires OnTabRemovedFromStack.
  bool RemoveTabFromStack(int tab_index, const std::string& stack_id);

  // Removes a tab from all stacks it belongs to.
  // Returns true if the tab was in any stack.
  //
  // Fires OnTabRemovedFromStack for each stack the tab was in.
  bool RemoveTabFromAllStacks(int tab_index);

  // Returns the tab indices in the given stack, in stack order.
  // Returns an empty vector if the stack doesn't exist or has no tabs.
  std::vector<int> GetTabsInStack(const std::string& stack_id) const;

  // Returns the number of tabs in the given stack.
  // Returns 0 if the stack doesn't exist.
  size_t GetTabCountInStack(const std::string& stack_id) const;

  // Returns the stack ID that |tab_index| belongs to, or an empty
  // string if the tab is not in any stack.
  std::string GetStackForTab(int tab_index) const;

  // Returns true if the tab at |tab_index| is in |stack_id|.
  bool IsTabInStack(int tab_index, const std::string& stack_id) const;

  // Moves a tab within its current stack to a new position.
  // |new_position| is the 0-based position within the stack's tab list.
  // The tab must already be in a stack.
  // Returns true on success.
  bool MoveTabInStack(int tab_index, int new_position);

  // Moves multiple tabs to a stack.  Tabs are added in the order given.
  // If a tab is already in another stack, it is removed first.
  // Returns the number of tabs successfully added.
  //
  // Fires OnTabRemovedFromStack and OnTabAddedToStack for each tab.
  size_t MoveTabsToStack(const std::vector<int>& tab_indices,
                         const std::string& stack_id);

  // =======================================================================
  // Stack operations
  // =======================================================================

  // Collapses a stack (tabs hidden in the tab strip / sidebar).
  // Has no effect if the stack is already collapsed.
  //
  // Fires OnStackCollapsedChanged if the state changed.
  void CollapseStack(const std::string& stack_id);

  // Expands a stack.  Child tabs become visible.
  // Has no effect if the stack is already expanded.
  //
  // Fires OnStackCollapsedChanged if the state changed.
  void ExpandStack(const std::string& stack_id);

  // Toggles the collapsed state of a stack.
  // Returns the new collapsed state.
  bool ToggleStackCollapsed(const std::string& stack_id);

  // Returns true if the stack is collapsed.  Returns false if the stack
  // doesn't exist.
  bool IsStackCollapsed(const std::string& stack_id) const;

  // Closes all tabs in a stack.  The stack itself is preserved (becomes
  // empty).  Use UndoCloseStack to restore.
  // Returns the number of tabs that were closed.
  //
  // TODO(astra): Implement actual tab closing via TabStripModel.
  //   For now, this removes tabs from the stack (metadata only).
  //   Chromium owner: TabStripModel::CloseWebContentsAt()
  //   Patch point: chrome/browser/ui/tabs/tab_strip_model.cc
  size_t CloseStack(const std::string& stack_id);

  // Undoes a previous CloseStack operation — restores the tabs that were
  // in the stack.  Only works for the most recently closed stack.
  // Returns true if the stack had closed tabs and they were restored.
  //
  // TODO(astra): Implement proper undo with tab restoration.
  //   Chromium owner: TabRestoreService (chrome/browser/sessions/tab_restore_service.h)
  bool UndoCloseStack(const std::string& stack_id);

  // =======================================================================
  // Stack metadata
  // =======================================================================

  // Sets the user note for a stack.
  // Returns true if the stack existed.
  bool SetStackNote(const std::string& stack_id, const std::string& note);

  // Returns the user note for a stack.
  // Returns empty string if the stack doesn't exist.
  std::string GetStackNote(const std::string& stack_id) const;

  // Sets whether a stack is pinned.
  // Pinned stacks appear at the top of the stack list.
  // Returns true if the stack existed.
  bool SetStackPinned(const std::string& stack_id, bool pinned);

  // Returns true if the stack is pinned.
  // Returns false if the stack doesn't exist.
  bool IsStackPinned(const std::string& stack_id) const;

  // Returns the last accessed time for a stack.
  // Returns base::Time() if the stack doesn't exist.
  base::Time GetStackLastAccessed(const std::string& stack_id) const;

  // Sets the order index of a stack.
  // Returns true if the stack existed.
  bool SetStackOrderIndex(const std::string& stack_id, int order_index);

  // Returns the order index of a stack.
  // Returns -1 if the stack doesn't exist.
  int GetStackOrderIndex(const std::string& stack_id) const;

  // =======================================================================
  // Bulk operations
  // =======================================================================

  // Closes all stacks — all tabs remain open and become unstacked.
  // Returns the number of stacks that were closed (deleted).
  //
  // Fires OnStackDeleted for each stack.
  size_t CloseAllStacks();

  // Returns all stacks that have the given color.
  // Color matching is case-insensitive.
  std::vector<AstraTabStackInfo> GetStacksByColor(
      const std::string& color) const;

  // Returns the most recently accessed stacks, up to |max_count|.
  // Stacks are sorted by last_accessed descending.
  std::vector<AstraTabStackInfo> GetRecentlyAccessedStacks(
      int max_count) const;

  // Searches stacks by name and note (case-insensitive substring match).
  // Returns all stacks that match the query.
  // An empty query returns all stacks.
  std::vector<AstraTabStackInfo> SearchStacks(
      const std::string& query) const;

  // Merges two stacks.  All tabs from |source_stack_id| are moved to
  // |target_stack_id|, and the source stack is deleted.
  // Returns true if both stacks existed and the merge succeeded.
  //
  // Fires OnTabRemovedFromStack for each tab leaving source,
  // OnTabAddedToStack for each tab joining target,
  // and OnStackDeleted for the source stack.
  bool MergeStacks(const std::string& source_stack_id,
                   const std::string& target_stack_id);

  // Duplicates a stack.  Creates a new stack with the same name (with
  // " copy" suffix), same color, same tabs (copied), and same settings.
  // Returns the ID of the new stack, or empty string on failure.
  //
  // Fires OnStackCreated for the new stack.
  //
  // TODO(astra): For full tab duplication, duplicate WebContents too.
  //   Currently duplicates metadata only (stack structure, not actual tabs).
  //   Chromium owner: TabStripModel::DuplicateTabAt()
  AstraTabStackId DuplicateStack(const std::string& stack_id);

  // =======================================================================
  // Settings
  // =======================================================================

  // -- Default color ------------------------------------------------------

  std::string GetDefaultStackColor() const;
  void SetDefaultStackColor(const std::string& color);

  // -- Auto-collapse inactive stacks -------------------------------------

  bool GetAutoCollapseInactive() const;
  void SetAutoCollapseInactive(bool enabled);

  int GetAutoCollapseAfterSeconds() const;
  void SetAutoCollapseAfterSeconds(int seconds);

  // -- Display settings ---------------------------------------------------

  bool GetShowStackCountBadge() const;
  void SetShowStackCountBadge(bool show);

  bool GetShowStackTabCount() const;
  void SetShowStackTabCount(bool show);

  // -- Stack creation behavior -------------------------------------------

  int GetStackCreationBehavior() const;
  void SetStackCreationBehavior(int behavior);

  // -- Auto-stack related tabs -------------------------------------------

  bool GetAutoStackRelated() const;
  void SetAutoStackRelated(bool enabled);

  // -- Minimum tabs per stack --------------------------------------------

  int GetMinimumTabsPerStack() const;
  void SetMinimumTabsPerStack(int minimum);

  // =======================================================================
  // Stack utility
  // =======================================================================

  // Returns the preset color palette for stack colors.  These are the
  // recommended colors shown in the stack color picker UI.
  //
  // Chromium analog: TabGroupColorId enum (chrome/browser/ui/tabs/tab_group.h)
  //   Tab groups use a fixed color enum; stacks support arbitrary hex colors
  //   but provide a curated palette for the UI.
  static std::vector<std::string> GetStackColorPalette();

  // Finds a stack by name (case-insensitive).  Returns nullptr if not found.
  const AstraTabStackInfo* FindStackByName(const std::string& name) const;

  // Collapses all stacks.  Returns the number of stacks whose state changed.
  //
  // Fires OnStackCollapsedChanged for each stack that changed.
  size_t CollapseAllStacks();

  // Expands all stacks.  Returns the number of stacks whose state changed.
  //
  // Fires OnStackCollapsedChanged for each stack that changed.
  size_t ExpandAllStacks();

  // Registers profile prefs for tab stack settings.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  // Loads stack metadata from the profile's PrefService.
  void LoadFromPrefs();

  // Persists current stack metadata to the profile's PrefService.
  void SaveToPrefs();

  // Finds a stack by ID.  Returns nullptr if not found.
  AstraTabStackInfo* FindStack(const std::string& stack_id);
  const AstraTabStackInfo* FindStack(const std::string& stack_id) const;

  // Generates a new unique stack ID using base::UnguessableToken.
  static AstraTabStackId GenerateStackId();

  // Updates the last_accessed timestamp for a stack.
  void TouchStack(AstraTabStackInfo* stack);

  // Recomputes order_index for all stacks based on their current order.
  void RecomputeOrderIndices();

  // Gets the PrefService from the profile.  Returns nullptr if no profile.
  PrefService* GetPrefs() const;

  raw_ptr<Profile> profile_;
  base::ObserverList<AstraTabStackObserver> observers_;

  // All stacks, keyed by stack_id for O(1) lookup.
  // Use GetAllStacks() for sorted iteration.
  std::map<std::string, AstraTabStackInfo> stacks_;

  // Closed stacks for undo support.
  // Maps stack_id -> the stack state before it was closed.
  std::map<std::string, AstraTabStackInfo> closed_stacks_undo_;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_TAB_STACK_SERVICE_H_
