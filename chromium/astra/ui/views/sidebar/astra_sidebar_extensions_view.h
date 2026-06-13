#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_EXTENSIONS_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_EXTENSIONS_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "astra/browser/astra_extension_helper.h"
#include "astra/ui/views/sidebar/astra_extension_icon_view.h"
#include "astra/ui/views/sidebar/astra_extension_popup_view.h"
#include "ui/views/view.h"

namespace views {
class Label;
class Textfield;
class View;
}  // namespace views

class Profile;

namespace astra {

// Sort order for extension icons in the sidebar.
enum class AstraExtensionSortBy {
  kManual,       // Manual order (drag-and-drop)
  kName,         // Alphabetical by name
  kInstallDate,  // Most recently installed first
  kLastUsed,     // Most recently used first
};

// Delegate interface for AstraSidebarExtensionsView events.
// Implemented by the controller to handle extension actions.
//
// Chromium owner: ExtensionsToolbarContainer delegate
//   (chrome/browser/ui/views/toolbar/extensions_toolbar_container.h)
class AstraSidebarExtensionsDelegate {
 public:
  virtual ~AstraSidebarExtensionsDelegate() = default;

  // Called when an extension icon is left-clicked.
  virtual void OnExtensionClicked(const std::string& extension_id) = 0;

  // Called when an extension icon is middle-clicked.
  virtual void OnExtensionMiddleClicked(const std::string& extension_id) = 0;

  // Called when an extension icon is right-clicked.
  virtual void OnExtensionRightClicked(const std::string& extension_id,
                                       const gfx::Point& point) = 0;

  // Called when an extension is pinned or unpinned.
  virtual void OnExtensionPinned(const std::string& extension_id,
                                 bool pinned) = 0;

  // Called when extensions are reordered via drag-and-drop.
  virtual void OnExtensionReordered(int from_index, int to_index) = 0;

  // Called when "Manage extensions" is requested.
  virtual void OnManageExtensionsRequested() = 0;

  // Called when an extension popup is shown.
  virtual void OnExtensionPopupShown(const std::string& extension_id) = 0;

  // Called when an extension popup is closed.
  virtual void OnExtensionPopupClosed(const std::string& extension_id) = 0;

  // Called when enabling an extension is requested.
  virtual void OnEnableExtensionRequested(const std::string& extension_id) = 0;

  // Called when disabling an extension is requested.
  virtual void OnDisableExtensionRequested(const std::string& extension_id) = 0;

  // Called when removing an extension is requested.
  virtual void OnRemoveExtensionRequested(const std::string& extension_id) = 0;

  // Called when extension options are requested.
  virtual void OnExtensionOptionsRequested(const std::string& extension_id) = 0;
};

// Sidebar section that displays extension browser action icons.
//
// This is a presentation-only view: it projects Chromium extension state
// (from AstraExtensionHelper, which wraps ExtensionRegistry) into a grid
// of extension icons. It never stores extension truth state — the
// ExtensionRegistry is the single source of truth.
//
// Layout:
//   +------------------------------+
//   | [>] Extensions          [+] |  <- Header (collapsible)
//   +------------------------------+
//   | [pinned ext icons]           |  <- Pinned section (top)
//   +------------------------------+
//   | [search box]                 |  <- Search (optional)
//   +------------------------------+
//   | [all extension icons]        |  <- All extensions grid (scrollable)
//   +------------------------------+
//   | Manage extensions...         |  <- Footer link
//   +------------------------------+
//
// Implements AstraExtensionHelperObserver to receive live updates when
// extensions are installed, uninstalled, enabled, or disabled.
//
// Implements AstraExtensionIconDelegate to handle click and context-menu
// actions from individual extension icon views.
//
// Implements AstraExtensionPopupDelegate to handle popup lifecycle events.
//
// Chromium owner: ExtensionRegistry (extensions/browser/extension_registry.h)
// Chromium owner: ToolbarActionsModel
//   (chrome/browser/ui/toolbar/toolbar_actions_model.h)
// Chromium owner: ExtensionsToolbarContainer
//   (chrome/browser/ui/views/toolbar/extensions_toolbar_container.h)
//
// TODO(astra): Proper ExtensionRegistry observer integration.
//   Currently the extension list is read once at construction time.
//   To get live updates, AstraExtensionHelper needs to implement
//   extensions::ExtensionRegistryObserver and forward events to us.
//   Chromium owner: ExtensionRegistryObserver
//     (extensions/browser/extension_registry.h)
//   Patch point: None needed — ExtensionRegistryObserver is a public
//     observer interface that any KeyedService can implement.
class AstraSidebarExtensionsView
    : public views::View,
      public AstraExtensionHelperObserver,
      public AstraExtensionIconDelegate,
      public AstraExtensionPopupDelegate {
 public:
  // |profile| is used to look up the AstraExtensionHelper. Not owned.
  explicit AstraSidebarExtensionsView(Profile* profile);
  AstraSidebarExtensionsView(const AstraSidebarExtensionsView&) = delete;
  AstraSidebarExtensionsView& operator=(const AstraSidebarExtensionsView&) =
      delete;
  ~AstraSidebarExtensionsView() override;

  // -- Delegate ----------------------------------------------------------

  void SetDelegate(AstraSidebarExtensionsDelegate* delegate);
  AstraSidebarExtensionsDelegate* GetDelegate() const;

  // -- Extension list management ----------------------------------------

  // Set the full list of extensions. Rebuilds the entire grid.
  void SetExtensions(const std::vector<AstraExtensionInfo>& extensions);

  // Get the total number of extensions shown.
  int GetExtensionCount() const;

  // Get the extension info at the given index (across all extensions,
  // not just visible ones). Returns default info if index is out of bounds.
  AstraExtensionInfo GetExtensionAt(int index) const;

  // Add a single extension.
  void AddExtension(const AstraExtensionInfo& info);

  // Remove an extension by ID.
  void RemoveExtension(const std::string& extension_id);

  // Update an existing extension's info.
  void UpdateExtension(const AstraExtensionInfo& info);

  // Returns true if the extension with the given ID is present.
  bool HasExtension(const std::string& extension_id) const;

  // -- Selection ---------------------------------------------------------

  // Set the selected/active extension (e.g., the one whose popup is open).
  void SetSelectedExtension(const std::string& extension_id);
  std::string GetSelectedExtensionId() const;
  void ClearSelection();

  // -- Pinned extensions -------------------------------------------------

  // Set the list of pinned extension IDs.
  void SetPinnedExtensions(const std::vector<std::string>& extension_ids);
  std::vector<std::string> GetPinnedExtensions() const;

  // Returns true if the extension is pinned.
  bool IsExtensionPinned(const std::string& extension_id) const;

  // Pin an extension (moves it to the pinned section).
  void PinExtension(const std::string& extension_id);

  // Unpin an extension (moves it to the all extensions section).
  void UnpinExtension(const std::string& extension_id);

  // -- Reordering --------------------------------------------------------

  // Move an extension from one position to another (within the same section).
  void MoveExtension(int from_index, int to_index);

  // -- Grid layout -------------------------------------------------------

  // Set the number of extensions per row in the grid.
  void SetExtensionsPerRow(int count);
  int GetExtensionsPerRow() const;

  // Set the icon size in pixels.
  void SetIconSize(int size_px);
  int GetIconSize() const;

  // Set the spacing between icons in pixels.
  void SetSpacing(int spacing_px);
  int GetSpacing() const;

  // -- Sections ----------------------------------------------------------

  // Set whether the pinned extensions section is shown.
  void SetShowPinnedSection(bool show);
  bool GetShowPinnedSection() const;

  // Set whether the "all extensions" section is shown.
  void SetShowAllExtensionsSection(bool show);
  bool GetShowAllExtensionsSection() const;

  // Set whether disabled extensions are shown in the grid.
  void SetShowDisabledExtensions(bool show);
  bool GetShowDisabledExtensions() const;

  // -- Sorting -----------------------------------------------------------

  // Set the sort order for the extensions grid.
  void SetSortExtensionsBy(AstraExtensionSortBy sort_by);
  AstraExtensionSortBy GetSortExtensionsBy() const;

  // -- Counts ------------------------------------------------------------

  // Get the number of pinned extensions.
  int GetPinnedExtensionCount() const;

  // Get the number of enabled extensions.
  int GetEnabledExtensionCount() const;

  // Get the number of disabled extensions.
  int GetDisabledExtensionCount() const;

  // -- Search ------------------------------------------------------------

  // Filter visible extensions by name (case-insensitive substring match).
  void SearchExtensions(const std::u16string& query);
  int GetSearchResultsCount() const;

  // -- Icon view access --------------------------------------------------

  // Get the icon view for a given extension ID, or nullptr if not found.
  AstraExtensionIconView* GetExtensionIconView(
      const std::string& extension_id) const;

  // -- Popup management --------------------------------------------------

  // Show the popup for an extension, anchored to its icon view.
  void ShowExtensionPopup(const std::string& extension_id,
                          views::View* anchor);

  // Hide the currently showing popup, if any.
  void HideExtensionPopup();

  // Returns true if a popup is currently visible.
  bool IsPopupVisible() const;

  // Returns the extension ID of the currently showing popup, or empty string.
  std::string GetCurrentPopupExtensionId() const;

  // -- Badge / notifications --------------------------------------------

  // Set whether to show the extensions badge (total notification count).
  void SetShowExtensionsBadge(bool show);
  bool GetShowExtensionsBadge() const;

  // Get the total notification count across all extensions.
  int GetExtensionNotificationCount() const;

  // -- Refresh / rebuild -------------------------------------------------

  // Refresh the extensions list from the underlying helper.
  // Full rebuild — used for initial sync and when incremental updates
  // are not available.
  void RefreshFromHelper();

  // -- Collapse / expand ------------------------------------------------

  // Set section visibility and update layout accordingly.
  void SetSectionVisible(bool visible);

  // Toggle the section collapsed/expanded state.
  void ToggleExpanded();
  void SetExpanded(bool expanded);
  bool expanded() const { return expanded_; }

  // -- AstraExtensionHelperObserver --------------------------------------

  void OnExtensionsChanged() override;
  void OnExtensionIconChanged(const std::string& extension_id) override;

  // -- AstraExtensionIconDelegate ---------------------------------------

  void OnExtensionClicked(const std::string& extension_id) override;
  void OnExtensionMiddleClicked(const std::string& extension_id) override;
  void OnExtensionRightClicked(const std::string& extension_id,
                               const gfx::Point& point) override;
  void OnExtensionPopupShown(const std::string& extension_id) override;
  void OnExtensionPopupClosed(const std::string& extension_id) override;
  void OnPinExtension(const std::string& extension_id, bool pinned) override;
  void OnManageExtensionRequested(const std::string& extension_id) override;
  void OnRemoveExtensionRequested(const std::string& extension_id) override;
  void OnDisableExtensionRequested(const std::string& extension_id) override;
  void OnExtensionOptionsRequested(const std::string& extension_id) override;
  // Legacy delegate methods (kept for backward compatibility).
  void OnExtensionIconClicked(const std::string& extension_id,
                              views::View* anchor_view) override;
  void OnExtensionIconContextMenu(const std::string& extension_id,
                                  const gfx::Point& point) override;

  // -- AstraExtensionPopupDelegate --------------------------------------

  void OnExtensionPopupClosed(const std::string& extension_id) override;
  void OnExtensionPopupShown(const std::string& extension_id) override;

  // -- views::View -------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Find the icon view for a given extension ID, or nullptr if not found.
  AstraExtensionIconView* FindIconView(const std::string& extension_id) const;

  // Create an icon view for an extension.
  std::unique_ptr<AstraExtensionIconView> CreateIconView(
      const AstraExtensionInfo& info);

  // Rebuild the icons grid from the current extension list.
  void RebuildIcons();

  // Sort the extension list according to the current sort setting.
  void SortExtensionList();

  // Update the visibility of the section based on whether there are
  // any extensions with browser actions.
  void UpdateSectionVisibility();

  // Open (or close) the extension popup for |extension_id|.
  // If the popup is already showing for this extension, close it.
  // If another popup is showing, close it and open this one.
  void ToggleExtensionPopup(const std::string& extension_id,
                            views::View* anchor_view);

  // Close the currently showing popup, if any.
  void CloseActivePopup();

  // Show the extension context menu at the given screen point.
  // TODO(astra): Implement real extension context menu from Chromium.
  //   Chromium owner: ExtensionContextMenuModel
  //     (chrome/browser/extensions/extension_context_menu_model.h)
  //   Patch point: None needed — we can use the model directly.
  void ShowExtensionContextMenu(const std::string& extension_id,
                                const gfx::Point& screen_point);

  // Get the list of extensions in display order (pinned first, then all
  // others, respecting sort order and filters).
  std::vector<AstraExtensionInfo> GetDisplayedExtensions() const;

  // Check if an extension should be visible given current filters.
  bool ShouldShowExtension(const AstraExtensionInfo& info) const;

  // The profile this view is associated with. Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // The extension helper we observe and project. Not owned.
  raw_ptr<AstraExtensionHelper> extension_helper_ = nullptr;

  // Observation of the extension helper for reactive updates.
  base::ScopedObservation<AstraExtensionHelper, AstraExtensionHelperObserver>
      extension_helper_observation_{this};

  // Delegate for extension actions. Not owned.
  raw_ptr<AstraSidebarExtensionsDelegate> delegate_ = nullptr;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> header_row_ = nullptr;
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::View> pinned_section_ = nullptr;
  raw_ptr<views::View> pinned_icons_container_ = nullptr;
  raw_ptr<views::View> all_section_ = nullptr;
  raw_ptr<views::View> all_icons_container_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::Label> manage_link_ = nullptr;

  // The currently showing extension popup, if any. Not owned — the
  // popup view is owned by its widget (BubbleDialogDelegate pattern).
  raw_ptr<AstraExtensionPopupView> active_popup_ = nullptr;

  // Extension ID of the currently showing popup. Empty if no popup.
  std::string active_popup_extension_id_;

  // The selected/active extension ID.
  std::string selected_extension_id_;

  // Full list of extension infos (all extensions, unsorted).
  std::vector<AstraExtensionInfo> extensions_;

  // List of pinned extension IDs (in display order).
  std::vector<std::string> pinned_ids_;

  // Current search query (empty = no filter).
  std::u16string search_query_;

  // Whether the section is expanded (icons visible) or collapsed.
  bool expanded_ = true;

  // Whether to show the pinned section.
  bool show_pinned_section_ = true;

  // Whether to show the all extensions section.
  bool show_all_section_ = true;

  // Whether to show disabled extensions.
  bool show_disabled_extensions_ = true;

  // Whether to show the search box.
  bool show_search_ = false;

  // Whether to show the manage extensions link.
  bool show_manage_link_ = true;

  // Whether to show the extension badge (total notifications).
  bool show_extensions_badge_ = false;

  // Number of columns in the icon grid.
  int extensions_per_row_ = 4;

  // Icon size in pixels.
  int icon_size_ = 24;

  // Spacing between icons in pixels.
  int icon_spacing_ = 4;

  // Sort order for the extensions grid.
  AstraExtensionSortBy sort_by_ = AstraExtensionSortBy::kName;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_EXTENSIONS_VIEW_H_
