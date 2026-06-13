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
class View;
}  // namespace views

class Profile;

namespace astra {

// Sidebar section that displays extension browser action icons.
//
// This is a presentation-only view: it projects Chromium extension state
// (from AstraExtensionHelper, which wraps ExtensionRegistry) into a grid
// of extension icons. It never stores extension truth state — the
// ExtensionRegistry is the single source of truth.
//
// Layout:
//   - Header with "Extensions" label
//   - Grid of extension icons (browser actions)
//   - Each icon is clickable to open the extension popup
//   - Right-clicking an icon shows the extension context menu
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
// Chromium owner: ExtensionAction (extensions/browser/extension_action.h)
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
class AstraSidebarExtensionsView : public views::View,
                                   public AstraExtensionHelperObserver,
                                   public AstraExtensionIconDelegate,
                                   public AstraExtensionPopupDelegate {
 public:
  // |profile| is used to look up the AstraExtensionHelper. Not owned.
  explicit AstraSidebarExtensionsView(Profile* profile);
  AstraSidebarExtensionsView(const AstraSidebarExtensionsView&) = delete;
  AstraSidebarExtensionsView& operator=(const AstraSidebarExtensionsView&) = delete;
  ~AstraSidebarExtensionsView() override;

  // Refresh the extensions list from the underlying helper.
  // Full rebuild — used for initial sync and when incremental updates
  // are not available.
  void RefreshFromHelper();

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

  void OnExtensionIconClicked(const std::string& extension_id,
                              views::View* anchor_view) override;
  void OnExtensionIconContextMenu(const std::string& extension_id,
                                  const gfx::Point& point) override;

  // -- AstraExtensionPopupDelegate --------------------------------------

  void OnExtensionPopupClosed(const std::string& extension_id) override;

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

  // Rebuild the icons grid from the current extension helper state.
  void RebuildIcons();

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

  // The profile this view is associated with. Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // The extension helper we observe and project. Not owned.
  raw_ptr<AstraExtensionHelper> extension_helper_ = nullptr;

  // Observation of the extension helper for reactive updates.
  base::ScopedObservation<AstraExtensionHelper, AstraExtensionHelperObserver>
      extension_helper_observation_{this};

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> header_row_ = nullptr;
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::View> icons_container_ = nullptr;

  // The currently showing extension popup, if any. Not owned — the
  // popup view is owned by its widget (BubbleDialogDelegate pattern).
  raw_ptr<AstraExtensionPopupView> active_popup_ = nullptr;

  // Extension ID of the currently showing popup. Empty if no popup.
  std::string active_popup_extension_id_;

  // Whether the section is expanded (icons visible) or collapsed.
  bool expanded_ = true;

  // Number of columns in the icon grid.
  // TODO(astra): Make this responsive based on sidebar width.
  static constexpr int kIconGridColumns = 4;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_EXTENSIONS_VIEW_H_
