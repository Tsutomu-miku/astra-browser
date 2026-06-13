// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_WORKSPACES_H_
#define ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_WORKSPACES_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

namespace views {
class Label;
class ScrollView;
class Separator;
}  // namespace views

namespace astra {

class AstraWorkspaceMenuItemView;
class AstraWorkspaceService;

// Display mode for workspace items in the menu.
enum class AstraWorkspaceDisplayMode {
  kIconsOnly,       // Only color dots.
  kNamesOnly,       // Only names and tab counts.
  kIconsAndNames,   // Both color dots and names (default).
};

// =========================================================================
// Workspace section for the Chrome profile menu
// =========================================================================
//
// AstraProfileMenuWorkspaces is a view that shows workspace switching
// controls in the profile menu area.  It is designed to be embedded as
// a section inside Chromium's ProfileMenuView, or shown as a standalone
// bubble anchored to the avatar toolbar button.
//
// Layout:
//   +-------------------------------+
//   |  Workspaces                   |  <- section header
//   +-------------------------------+
//   |  [dot] Workspace 1  5 tabs ✓ |  <- workspace list (scrollable)
//   |  [dot] Workspace 2  3 tabs   |
//   |  ...                          |
//   +-------------------------------+
//   |  + New workspace              |  <- create new action
//   |  Manage workspaces            |  <- opens workspace settings
//   +-------------------------------+
//
// Deepened features:
//   - Multiple display modes (icons only, names only, icons+names)
//   - Reorderable workspace items (with drag handles)
//   - Configurable max visible workspaces
//   - Enhanced keyboard navigation
//   - Full accessibility support
//   - Hover states and visual feedback
//
// This view follows the visual style of Chromium's profile menu items
// (heights, paddings, typography) so it blends in when embedded.
//
// Chromium owner: ProfileMenuView
//   (chrome/browser/ui/views/profiles/profile_menu_view.h)
// Patch point: ProfileMenuView::BuildBody() or BuildSyncInfo() —
//   insert this view as an additional section in the profile menu.
//
// State:
//   - Truth source: AstraWorkspaceService (ProfileKeyedService)
//   - This view reads from the service and projects the list into UI.
//   - Workspace switching delegates to the service (via delegate).
//   - UI code never stores workspace state.
//
// Keyboard navigation:
//   - Up/Down arrows: move focus between workspace items
//   - Enter/Space: activate focused workspace
//   - Home/End: jump to first/last workspace
//   - Alt+Up/Alt+Down: reorder focused workspace
// =========================================================================

class AstraProfileMenuWorkspaces : public views::View {
 public:
  // Delegate interface for workspace actions.  The controller (e.g.
  // AstraProfileMenuController) implements this to handle user actions
  // like switching workspaces or creating a new one.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the user clicks on a workspace item to switch to it.
    virtual void OnWorkspaceSelected(const std::string& workspace_id) = 0;

    // Called when the user clicks "New workspace".
    virtual void OnNewWorkspace() = 0;

    // Called when the user clicks "Manage workspaces".
    virtual void OnManageWorkspaces() = 0;

    // Called when the user reorders a workspace.
    // |workspace_id| is the workspace being moved.
    // |direction| is -1 (up) or +1 (down).
    virtual void OnWorkspaceReordered(const std::string& workspace_id,
                                      int direction) {}
  };

  explicit AstraProfileMenuWorkspaces(AstraWorkspaceService* workspace_service,
                                      Delegate* delegate);
  ~AstraProfileMenuWorkspaces() override;

  AstraProfileMenuWorkspaces(const AstraProfileMenuWorkspaces&) = delete;
  AstraProfileMenuWorkspaces& operator=(const AstraProfileMenuWorkspaces&) =
      delete;

  // Refreshes the workspace list from the underlying service.
  // Call this when workspace state changes (added, removed, renamed,
  // reordered, or active workspace switched).
  void UpdateFromService();

  // Sets the maximum height of the workspace list before scrolling kicks in.
  void SetMaxListHeight(int max_height);

  // -- Display mode -------------------------------------------------------

  // Sets the display mode for workspace items.
  void SetDisplayMode(AstraWorkspaceDisplayMode mode);
  AstraWorkspaceDisplayMode display_mode() const { return display_mode_; }

  // -- Reorder handles ----------------------------------------------------

  // Sets whether reorder handles are shown on workspace items.
  void SetReorderHandlesVisible(bool visible);
  bool reorder_handles_visible() const { return reorder_handles_visible_; }

  // -- Keyboard navigation helpers ----------------------------------------

  // Moves focus to the next/previous workspace item.
  // Returns true if focus moved, false if at the boundary.
  bool MoveFocusDown();
  bool MoveFocusUp();

  // Activates the currently focused workspace item.
  // Returns true if an item was activated.
  bool ActivateFocusedItem();

  // Moves the focused workspace up or down in the list (reorder).
  // Returns true if the reorder was performed.
  bool ReorderFocusedItem(int direction);

  // -- "Show more" / expand all -------------------------------------------

  // Sets whether the "Show more" / "Show less" button is visible.
  void SetShowMoreButtonVisible(bool visible);
  bool show_more_button_visible() const { return show_more_button_visible_; }

  // Toggles between showing all workspaces and showing only max_visible.
  void ToggleShowMore();
  bool is_showing_all() const { return is_showing_all_; }

  // -- Empty state --------------------------------------------------------

  // Sets whether the empty state view is shown (no workspaces beyond default).
  void SetEmptyStateVisible(bool visible);
  bool empty_state_visible() const { return empty_state_visible_; }

  // -- Context menu -------------------------------------------------------

  // Shows a context menu for the workspace at |index|.
  // Returns true if a context menu was shown.
  bool ShowContextMenuForWorkspace(size_t index, const gfx::Point& point);

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void ViewHierarchyChanged(
      const views::ViewHierarchyChangedDetails& details) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Rebuilds all child views (header, list, action rows).
  // Called from UpdateFromService() and the constructor.
  void RebuildWorkspaceList();

  // Creates a single workspace item view.
  std::unique_ptr<AstraWorkspaceMenuItemView> CreateWorkspaceItem(
      const std::string& workspace_id,
      const std::string& workspace_name,
      const std::string& accent_color,
      int tab_count,
      bool is_active);

  // Handles a click on a workspace item.
  void OnWorkspaceItemClicked(const std::string& workspace_id);

  // Handles a reorder request from a workspace item.
  void OnWorkspaceItemReordered(const std::string& workspace_id,
                                int direction);

  // Handles a click on the "New workspace" button.
  void OnNewWorkspaceClicked();

  // Handles a click on the "Manage workspaces" button.
  void OnManageWorkspacesClicked();

  // Returns the index of the currently focused workspace item, or -1
  // if no item is focused.
  int GetFocusedItemIndex() const;

  // Returns the number of workspace items in the list.
  int GetItemCount() const;

  // Handles a click on the "Show more" / "Show less" button.
  void OnShowMoreClicked();

  // Updates the empty state visibility based on workspace count.
  void UpdateEmptyState();

  // Updates the "Show more" button visibility and label.
  void UpdateShowMoreButton();

  // Updates which workspace items are visible based on max_visible and is_showing_all_.
  void UpdateItemVisibility();

  raw_ptr<AstraWorkspaceService> workspace_service_;
  raw_ptr<Delegate> delegate_;

  // Display mode for workspace items.
  AstraWorkspaceDisplayMode display_mode_ =
      AstraWorkspaceDisplayMode::kIconsAndNames;

  // Whether reorder handles are shown.
  bool reorder_handles_visible_ = false;

  // Section header label.
  raw_ptr<views::Label> section_header_ = nullptr;

  // Separator below the header.
  raw_ptr<views::Separator> header_separator_ = nullptr;

  // Scroll view containing the workspace list.
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;

  // Container view for the workspace list items (inside the scroll view).
  raw_ptr<views::View> list_container_ = nullptr;

  // Separator above action buttons.
  raw_ptr<views::Separator> action_separator_ = nullptr;

  // "New workspace" action button.
  raw_ptr<views::LabelButton> new_workspace_button_ = nullptr;

  // "Manage workspaces" action button.
  raw_ptr<views::LabelButton> manage_workspaces_button_ = nullptr;

  // "Show more" / "Show less" button.
  raw_ptr<views::LabelButton> show_more_button_ = nullptr;

  // Empty state view (shown when there are no workspaces beyond default).
  raw_ptr<views::View> empty_state_view_ = nullptr;
  raw_ptr<views::Label> empty_state_label_ = nullptr;

  // Max height of the scrollable workspace list, in pixels.
  // 0 means no limit (use preferred size).
  int max_list_height_ = 0;

  // Whether we're showing all workspaces or just the max visible.
  bool is_showing_all_ = false;
  bool show_more_button_visible_ = true;

  // Whether empty state is shown.
  bool empty_state_visible_ = false;

  // Cached list of workspace item views for keyboard navigation.
  std::vector<raw_ptr<AstraWorkspaceMenuItemView>> workspace_items_;

  base::WeakPtrFactory<AstraProfileMenuWorkspaces> weak_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PROFILES_ASTRA_PROFILE_MENU_WORKSPACES_H_
