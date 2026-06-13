#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DRAG_TYPES_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DRAG_TYPES_H_

#include <string>

namespace astra {

// Identifies which sidebar section an item belongs to or is being dropped into.
// Used by drag-and-drop to validate drops and determine what action to take.
enum class AstraSidebarSectionId {
  kFavorites,   // Favorites section (top-level favorites and folders)
  kPinnedTabs,  // Pinned tabs section
  kOpenTabs,    // Open tabs in the current workspace
  kReadingList, // Reading list section (read-later items)
  kTabGroups,   // Tab groups section
  kWorkspaces,  // Workspace list (in switcher or workspace section)
  kBookmarks,   // Chrome bookmarks section (tree view)
  kUnknown,
};

// The type of item being dragged. Determines what drop targets are valid.
enum class AstraSidebarDragItemType {
  kTab,         // A tab item (can be from favorites, pinned, or open tabs)
  kWorkspace,   // A workspace item (for reordering workspaces)
  kFolder,      // A favorite folder (for reordering or nesting folders)
  kBookmark,    // A bookmark node (URL or folder) from the bookmarks section
  kReadingList, // A reading list entry (URL from the reading list section)
};

// Data carried during a sidebar drag-and-drop operation.
//
// This struct is lightweight and contains only identifiers — the actual
// tab/workspace/folder state is always read from services at drop time.
// This ensures the drop operates on current state, not stale drag-start state.
//
// TODO(astra): Consider using Chromium's ui::OSExchangeData for cross-widget
// and cross-process drag-and-drop support (e.g., dragging tabs between
// browser windows). For now, intra-sidebar drag uses an in-process struct.
// Chromium DnD subsystem: ui/base/dragdrop/os_exchange_data.h
// Chromium Views drag: views/widget/native_widget.h (StartDragForViewFrom...),
//   views/view.h (OnDragStarted, OnDragDone)
struct AstraSidebarDragData {
  // What kind of item is being dragged.
  AstraSidebarDragItemType item_type = AstraSidebarDragItemType::kTab;

  // Which section the drag started from.
  AstraSidebarSectionId source_section = AstraSidebarSectionId::kUnknown;

  // Workspace the dragged item belongs to (for tabs and folders).
  std::string source_workspace_id;

  // -- Tab drag fields (used when item_type == kTab) --

  // Index in TabStripModel at drag start time. May be stale at drop time
  // if tabs were added/removed during the drag; the drop handler should
  // re-lookup by WebContents or other stable identifier.
  // TODO(astra): Use a stable tab identifier (e.g., WebContents* or a
  // unique tab ID) instead of an index. Index-based lookup breaks if
  // other tabs are added/removed during a drag.
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  int tab_index = -1;

  // For favorite tabs: the folder the tab is currently in.
  // "root" means the top-level favorites bar.
  std::string favorite_folder_id;

  // -- Workspace drag fields (used when item_type == kWorkspace) --

  std::string workspace_id;

  // -- Folder drag fields (used when item_type == kFolder) --

  std::string folder_id;
};

// Result of a drop validation. Returned by drop targets to indicate
// whether a drop at the current position is valid and what action it
// would perform.
struct AstraSidebarDropResult {
  // True if the drop is valid at the current position.
  bool is_valid = false;

  // Where in the target section the item would be inserted.
  // -1 means append to the end, or drop onto a container (e.g., folder).
  int insert_index = -1;

  // For folder drops: id of the folder being dropped into.
  // Empty for non-folder drops.
  std::string target_folder_id;

  // For workspace drops: id of the target workspace.
  std::string target_workspace_id;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DRAG_TYPES_H_
