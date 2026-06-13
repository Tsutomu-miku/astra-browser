#ifndef ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_OVERVIEW_VIEW_H_
#define ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_OVERVIEW_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "astra/browser/astra_workspace_service.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {
class Label;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

class AstraWorkspaceCardView;

// View mode for workspace overview.
enum class AstraWorkspaceOverviewViewMode {
  kGrid,   // Grid of cards (default)
  kList,   // Compact list view
  kCompact,// Ultra-compact list view (icons only)
};

// Layout mode for workspace overview (alias-friendly naming).
// Same semantics as AstraWorkspaceOverviewViewMode — provided for
// API consistency with SetLayout/GetLayout naming.
using AstraOverviewLayout = AstraWorkspaceOverviewViewMode;

// Card size for workspace overview.
enum class AstraWorkspaceOverviewCardSize {
  kSmall,
  kMedium,  // default
  kLarge,
};

// Observer interface for the workspace overview view.
//
// The controller implements this to receive user action notifications from
// the view.  The view never calls the controller directly — it notifies
// through this observer interface, and the controller reacts.
//
// This follows Chromium's observer pattern (e.g. TabStripModelObserver,
// BookmarkModelObserver) and keeps the view decoupled from the controller.
//
// All methods have default empty implementations so observers can override
// only the methods they care about.
class AstraWorkspaceOverviewViewObserver : public base::CheckedObserver {
 public:
  // -- Workspace interaction -----------------------------------------------

  // Called when the user clicks a workspace card (single click = switch).
  virtual void OnWorkspaceClicked(const std::string& workspace_id) {}

  // Called when the user double-clicks a workspace card (rename).
  virtual void OnWorkspaceRenameRequested(const std::string& workspace_id) {}

  // Called when the user requests the menu for a workspace (⋮ button).
  virtual void OnWorkspaceMenuRequested(
      const std::string& workspace_id,
      const gfx::Point& screen_point) {}

  // Called when the user requests deletion of a workspace.
  virtual void OnWorkspaceDeleteRequested(const std::string& workspace_id) {}

  // Called when the user clicks the "New workspace" card/button.
  virtual void OnNewWorkspaceRequested() {}

  // Called when a workspace is selected (keyboard navigation or click).
  // This is different from clicked/activated — selection is just highlight.
  virtual void OnWorkspaceSelected(const std::string& workspace_id) {}

  // Called when a workspace is activated (double-click, Enter key).
  virtual void OnWorkspaceActivated(const std::string& workspace_id) {}

  // Called when a workspace is created (e.g. via quick add).
  virtual void OnWorkspaceCreated(const std::string& workspace_id) {}

  // Called when a workspace is deleted.
  virtual void OnWorkspaceDeleted(const std::string& workspace_id) {}

  // Called when a workspace is renamed.
  virtual void OnWorkspaceRenamed(const std::string& workspace_id,
                                  const std::string& new_name) {}

  // -- Import / export ----------------------------------------------------

  // Called when the user clicks the export button.
  virtual void OnExportRequested() {}

  // Called when the user clicks the import button.
  virtual void OnImportRequested() {}

  // -- Search / filter ----------------------------------------------------

  // Called when the search query changes.
  virtual void OnSearchQueryChanged(const std::u16string& query) {}

  // -- Overview lifecycle -------------------------------------------------

  // Called when the overview is opening.
  virtual void OnOverviewOpening() {}

  // Called when the overview is closing (user pressed Escape, clicked outside,
  // etc.).  The controller uses this to update its is_visible_ state.
  virtual void OnOverviewClosing() {}

  // Called when the overview is fully shown (after animation).
  virtual void OnOverviewShown() {}

  // Called when the overview is fully hidden (after animation).
  virtual void OnOverviewHidden() {}

  // -- Reordering ---------------------------------------------------------

  // Called when workspaces are reordered via drag and drop.
  virtual void OnWorkspacesReordered(
      const std::vector<std::string>& ordered_ids) {}

  // -- View mode / presentation ------------------------------------------

  // Called when the view mode changes (grid <-> list).
  virtual void OnViewModeChanged(AstraWorkspaceOverviewViewMode mode) {}

  // Called when card size changes.
  virtual void OnCardSizeChanged(AstraWorkspaceOverviewCardSize size) {}

  // Called when "show statistics" toggle changes.
  virtual void OnShowStatisticsChanged(bool show) {}

  // -- Bulk operations ---------------------------------------------------

  // Called when "hibernate all" is requested.
  virtual void OnHibernateAllRequested() {}

  // Called when "delete all non-default" is requested.
  virtual void OnDeleteAllNonDefaultRequested() {}

  // -- Settings -----------------------------------------------------------

  // Called when the user opens overview settings.
  virtual void OnOverviewSettingsRequested() {}

 protected:
  ~AstraWorkspaceOverviewViewObserver() override = default;
};

// The full-window workspace overview overlay.
//
// This is a presentation-only view.  It shows all workspaces as cards in a
// responsive multi-column grid with a "New workspace" card at the end.
// It reads workspace data from AstraWorkspaceService (pushed in by the
// controller) and dispatches user actions through observer notifications.
//
// Architecture:
//   - The controller (AstraWorkspaceOverviewController) owns this view
//     and is responsible for showing/hiding the widget and pushing data.
//   - This view never reads from models directly — all data comes from the
//     controller via UpdateWorkspaces() and related setters.
//   - User actions (click workspace, new workspace, rename, delete) are
//     dispatched via observers that the controller wires to the service.
//
// Layout structure:
//   +------------------------------------------------+
//   |  [Search bar]          [Import] [Export]      |  <- header row
//   |                                                |
//   |  All Workspaces (N)                           |  <- section header
//   |                                                |
//   |  +------+ +------+ +------+ +------+          |
//   |  | card | | card | | card | | card |          |  <- card grid
//   |  +------+ +------+ +------+ +------+          |     (scrollable)
//   |  +------+ +------+                            |
//   |  | card | | +New |                            |
//   |  +------+ +------+                            |
//   +------------------------------------------------+
//
// TODO(astra): The overview could be implemented as a views::Widget with
//   WidgetDelegate, or as a child view overlay in BrowserView.  For the
//   skeleton we use a Widget-based approach so it can be shown/hidden
//   independently of the BrowserView layout.
// Chromium pattern: similar to Task Manager or Bookmark Manager widgets,
//   but modal-less and overlay-style.
class AstraWorkspaceOverviewView : public views::WidgetDelegateView,
                                   public views::TextfieldController {
 public:
  AstraWorkspaceOverviewView();
  AstraWorkspaceOverviewView(const AstraWorkspaceOverviewView&) = delete;
  AstraWorkspaceOverviewView& operator=(const AstraWorkspaceOverviewView&) = delete;
  ~AstraWorkspaceOverviewView() override;

  // -- Observer management ------------------------------------------------

  void AddObserver(AstraWorkspaceOverviewViewObserver* observer);
  void RemoveObserver(AstraWorkspaceOverviewViewObserver* observer);

  // -- Data update (called by controller) ---------------------------------

  // Rebuild all workspace cards from the given workspace list.
  void UpdateWorkspaces(const std::vector<AstraWorkspace>& workspaces,
                        const std::string& active_workspace_id,
                        const std::vector<int>& tab_counts,
                        const std::vector<int>& window_counts);

  // Alias for UpdateWorkspaces — SetWorkspaces naming for API consistency.
  void SetWorkspaces(const std::vector<AstraWorkspace>& workspaces,
                     const std::string& active_workspace_id,
                     const std::vector<int>& tab_counts,
                     const std::vector<int>& window_counts);

  // Updates the workspace count label in the section header.
  void UpdateWorkspaceCount(int count);

  // Sets the current search query (e.g. to clear it).
  void SetSearchQuery(const std::u16string& query);

  // -- Presentation settings (called by controller) ------------------------

  // Sets the view mode (grid or list).
  void SetViewMode(AstraWorkspaceOverviewViewMode mode);

  // Returns the current view mode.
  AstraWorkspaceOverviewViewMode view_mode() const { return view_mode_; }

  // Sets the card size (small/medium/large).
  void SetCardSize(AstraWorkspaceOverviewCardSize size);

  // Returns the current card size.
  AstraWorkspaceOverviewCardSize card_size() const { return card_size_; }

  // Sets whether to show statistics on cards.
  void SetShowStatistics(bool show);

  // Returns whether statistics are shown.
  bool show_statistics() const { return show_statistics_; }

  // -- Selection / focus helpers ------------------------------------------

  // Selects the card at the given index and scrolls it into view.
  void SelectWorkspaceAt(int index);

  // Returns the ID of the currently selected workspace, or empty string
  // if none is selected.
  std::string GetSelectedWorkspaceId() const;

  // Returns the index of the currently selected workspace card, or -1.
  int selected_index() const { return selected_index_; }

  // Returns the total number of workspace cards (excluding the new button).
  int GetWorkspaceCardCount() const;

  // Returns the card view at the given |index|, or nullptr if out of range.
  AstraWorkspaceCardView* GetWorkspaceCardAt(int index) const;

  // Clears the current selection (sets selected_index_ to -1).
  void ClearSelection();

  // Selects the first workspace card (Home key equivalent).
  void SelectFirstWorkspace();

  // Selects the last workspace card (End key equivalent).
  void SelectLastWorkspace();

  // -- Search --------------------------------------------------------------

  // Returns the current search query text.
  std::u16string GetSearchQuery() const;

  // Shows or hides the search bar.
  void ShowSearch(bool show);

  // Returns whether the search bar is currently visible.
  bool IsSearchVisible() const;

  // Returns the search textfield view.
  views::Textfield* GetSearchBox() const { return search_field_; }

  // -- New workspace button ------------------------------------------------

  // Shows or hides the "New workspace" button/card.
  void ShowNewWorkspaceButton(bool show);

  // Returns whether the new workspace button is visible.
  bool IsNewWorkspaceButtonVisible() const;

  // Returns the new workspace button/card view.
  views::View* GetNewWorkspaceButton() const { return new_workspace_card_; }

  // -- Layout --------------------------------------------------------------

  // Sets the overview layout (grid/list/compact).
  // This is an alias for SetViewMode with a different naming convention.
  void SetLayout(AstraOverviewLayout layout);

  // Returns the current overview layout.
  // This is an alias for view_mode() with a different naming convention.
  AstraOverviewLayout GetLayout() const { return view_mode_; }

  // -- views::WidgetDelegateView ------------------------------------------

  void WindowClosing() override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void Layout() override;

  // -- views::TextfieldController -----------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

 private:
  // Build the child views and layout.  Called once from constructor.
  void BuildLayout();

  // (Re)build all workspace cards from current filtered data.
  void RebuildCards();

  // Creates a new "New Workspace" card with a + icon.
  std::unique_ptr<views::View> CreateNewWorkspaceCard();

  // Finds the card view for the given workspace id, or nullptr if not found.
  AstraWorkspaceCardView* FindCardForWorkspace(const std::string& workspace_id);

  // Filters workspaces by the current search query and returns the
  // resulting list.
  std::vector<AstraWorkspace> FilterWorkspaces() const;

  // Keyboard navigation helpers.
  void SelectNextWorkspace();
  void SelectPreviousWorkspace();
  void SelectWorkspaceAbove();
  void SelectWorkspaceBelow();
  void ActivateSelectedWorkspace();
  void DeleteSelectedWorkspace();

  // Calculates how many columns fit in the available width.
  int CalculateColumnCount(int available_width) const;

  // Lays out cards in grid mode.
  void LayoutGridMode();

  // Lays out cards in list mode.
  void LayoutListMode();

  // Scrolls the selected card into view.
  void ScrollSelectedCardIntoView();

  // Notifies observers that a workspace was selected.
  void NotifyWorkspaceSelected(const std::string& workspace_id);

  // Notifies observers that the view mode changed.
  void NotifyViewModeChanged();

  // Notifies observers that card size changed.
  void NotifyCardSizeChanged();

  // Notifies observers that show statistics changed.
  void NotifyShowStatisticsChanged();

  // Gets the card width based on current card size setting.
  int GetCardWidth() const;

  // Gets the card height based on current card size setting.
  int GetCardHeight() const;

  // Current data (pushed in by controller).
  std::vector<AstraWorkspace> workspaces_;
  std::string active_workspace_id_;
  std::vector<int> tab_counts_;
  std::vector<int> window_counts_;

  // Current search query (for filtering).
  std::u16string search_query_;

  // Index of the currently selected card (for keyboard navigation).
  // -1 means no selection.
  int selected_index_ = -1;

  // Presentation settings.
  AstraWorkspaceOverviewViewMode view_mode_ =
      AstraWorkspaceOverviewViewMode::kGrid;
  AstraWorkspaceOverviewCardSize card_size_ =
      AstraWorkspaceOverviewCardSize::kMedium;
  bool show_statistics_ = true;

  // Observers of this view (typically the controller).
  base::ObserverList<AstraWorkspaceOverviewViewObserver> observers_;

  // UI child views (owned by view hierarchy).

  // Top header row: search bar + action buttons.
  raw_ptr<views::View> header_row_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::LabelButton> import_button_ = nullptr;
  raw_ptr<views::LabelButton> export_button_ = nullptr;
  raw_ptr<views::LabelButton> view_mode_button_ = nullptr;
  raw_ptr<views::LabelButton> settings_button_ = nullptr;

  // Section header: title + count.
  raw_ptr<views::View> section_header_ = nullptr;
  raw_ptr<views::Label> section_title_ = nullptr;
  raw_ptr<views::Label> section_count_ = nullptr;

  // Scrollable cards area.
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> cards_container_ = nullptr;

  // The "New workspace" button/card (inside cards_container_).
  raw_ptr<views::View> new_workspace_card_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_OVERVIEW_VIEW_H_
