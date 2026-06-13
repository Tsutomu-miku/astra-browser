#ifndef ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_VIEW_H_
#define ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"

class Browser;
class Profile;

namespace views {
class ScrollView;
class GridLayout;
class ColumnSet;
}  // namespace views

namespace astra {

class AstraNtpShortcutView;
class AstraNtpWorkspaceCard;
class AstraNewTabModel;

// =========================================================================
// AstraNewTabView — the main Astra-branded new tab page view
// =========================================================================
//
// A Views-based new tab page alternative for Astra.  Shows:
//   - Greeting / time of day
//   - Search bar (omnibox-like quick search)
//   - Workspace quick access (cards + new workspace action)
//   - Most visited sites (shortcut grid)
//   - Recently closed tabs (horizontal scroll)
//   - Quick actions row (new workspace, screenshot, focus mode, etc.)
//
// This is a presentation-only view.  It reads data from the model
// (AstraNewTabModel) and delegates user actions to the controller.
// It never stores state — all data is read from the model and projected
// into the view hierarchy.
//
// Layout: vertical stack of sections, centered horizontally.
// Sections adapt to the available width (responsive).
//
// New features:
//   - Settings gear button (top-right)
//   - Customize menu with display options
//   - Drag-and-drop reordering of shortcuts
//   - Keyboard navigation between sections
//   - Accessibility improvements
//   - Responsive layout (column count adapts to width)
//
// Chromium owner: NewTabPageUI
//   (chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h)
//
// TODO(astra): Replace Views-based NTP with proper WebUI NTP (chrome://astra-newtab).
// Patch point: chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.cc
// =========================================================================

class AstraNewTabView : public views::View {
 public:
  // Quick action identifiers.
  static constexpr char kActionNewWorkspace[] = "new_workspace";
  static constexpr char kActionScreenshot[] = "screenshot";
  static constexpr char kActionFocusMode[] = "focus_mode";
  static constexpr char kActionHistory[] = "history";
  static constexpr char kActionDownloads[] = "downloads";
  static constexpr char kActionBookmarks[] = "bookmarks";

  // Delegate interface for NTP actions.
  // Implemented by the controller / bubble that owns this view.
  class Delegate {
   public:
    // Called when a shortcut tile is clicked.
    virtual void OnNavigateToURL(const GURL& url) = 0;

    // Called when a workspace card is clicked.
    virtual void OnOpenWorkspace(const std::string& workspace_id) = 0;

    // Called when the "New workspace" action is triggered.
    virtual void OnNewWorkspace() = 0;

    // Called when the "Open workspace overview" action is triggered.
    virtual void OnShowAllWorkspaces() = 0;

    // Called when a quick action button is pressed.
    virtual void OnQuickAction(const std::string& action_id) = 0;

    // Called when a recently closed tab is clicked.
    virtual void OnRestoreRecentlyClosed(int session_id) = 0;

    // Called when a shortcut is requested to be removed.
    virtual void OnRemoveShortcut(const GURL& url) = 0;

    // Called to show the context menu for a shortcut.
    virtual void OnShowShortcutContextMenu(const GURL& url,
                                           const gfx::Point& screen_point) = 0;

    // Called to show the context menu for a workspace card.
    virtual void OnShowWorkspaceContextMenu(
        const std::string& workspace_id,
        const gfx::Point& screen_point) = 0;

    // Called when a shortcut is reordered via drag-and-drop.
    virtual void OnShortcutReordered(size_t from_index, size_t to_index) = 0;

    // Called when the settings gear button is pressed.
    virtual void OnSettingsGearPressed() = 0;

    // Called to toggle greeting visibility.
    virtual void OnToggleGreeting() = 0;

    // Called to toggle shortcut visibility.
    virtual void OnToggleShortcuts() = 0;

    // Called to change shortcut columns.
    virtual void OnShortcutColumnsChanged(int columns) = 0;

    // Called to change shortcut layout mode.
    virtual void OnShortcutLayoutModeChanged(int mode) = 0;

    // Called to change background style.
    virtual void OnBackgroundStyleChanged(int style) = 0;

   protected:
    ~Delegate() = default;
  };

  explicit AstraNewTabView(Browser* browser);
  AstraNewTabView(const AstraNewTabView&) = delete;
  AstraNewTabView& operator=(const AstraNewTabView&) = delete;
  ~AstraNewTabView() override;

  // Sets the delegate for NTP actions.
  void SetDelegate(Delegate* delegate);

  // Sets the model (used to read data for the view).
  void SetModel(AstraNewTabModel* model);

  // Refreshes all NTP content from the model.
  void RefreshContent();

  // Updates the view based on model settings changes.
  void UpdateFromSettings();

  // -- Section visibility controls (from controller) --

  void SetGreetingVisible(bool visible);
  void SetSearchBarVisible(bool visible);
  void SetWorkspaceCardsVisible(bool visible);
  void SetShortcutsVisible(bool visible);
  void SetRecentlyClosedVisible(bool visible);
  void SetQuickActionsVisible(bool visible);
  void SetSuggestedContentVisible(bool visible);

  // -- Drag and drop --

  // Called when a shortcut drag starts.
  void OnShortcutDragStarted(AstraNtpShortcutView* dragged_view);

  // Called during drag to show drop indicator.
  void OnShortcutDragMoved(const gfx::Point& screen_point);

  // Called when drag ends (reorder).
  void OnShortcutDragEnded(const gfx::Point& screen_point);

  // -- Animation --

  // Play entrance animations for all sections (staggered).
  void PlayEntranceAnimations();

  // Skip animations (for testing).
  void SkipAnimationsForTesting();

  // -- views::View --

  void OnThemeChanged() override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the view hierarchy.  Called once from constructor.
  void BuildLayout();

  // Update greeting based on time of day.
  void UpdateGreeting();

  // Update clock/time display.
  void UpdateClock();

  // Populate workspace cards section.
  void UpdateWorkspaceSection();

  // Populate shortcuts section.
  void UpdateShortcutsSection();

  // Populate recently closed section.
  void UpdateRecentlyClosedSection();

  // Populate quick actions section.
  void UpdateQuickActionsSection();

  // Populate suggested content section.
  void UpdateSuggestedContentSection();

  // Recompute layout based on current width (responsive adaptation).
  void UpdateResponsiveLayout();

  // Helper: create a section header label.
  std::unique_ptr<views::Label> CreateSectionLabel(
      const std::u16string& text);

  // Helper: create a quick action button.
  std::unique_ptr<views::View> CreateQuickActionButton(
      const std::string& action_id,
      const std::u16string& label,
      char16_t icon_char);

  // Helper: create a suggested content card.
  std::unique_ptr<views::View> CreateSuggestedContentCard(
      const AstraNtpSuggestedContent& item);

  // Callback from shortcut click.
  void OnShortcutClicked(const GURL& url);

  // Callback from shortcut remove.
  void OnShortcutRemove(const GURL& url);

  // Callback from shortcut context menu.
  void OnShortcutContextMenu(const GURL& url, const gfx::Point& screen_point);

  // Callback from shortcut drag start.
  void OnShortcutDragStart(AstraNtpShortcutView* view);

  // Callback from workspace card click.
  void OnWorkspaceCardClicked(const std::string& workspace_id);

  // Callback from workspace card menu.
  void OnWorkspaceCardMenu(const std::string& workspace_id,
                           const gfx::Point& screen_point);

  // Callback from quick action button.
  void OnQuickActionButton(const std::string& action_id);

  // Callback from recently closed item click.
  void OnRecentlyClosedClicked(int session_id);

  // Callback from suggested content click.
  void OnSuggestedContentClicked(const GURL& url);

  // Callback from settings gear button.
  void OnSettingsGearPressed();

  // Callback from profile button.
  void OnProfileButtonPressed();

  // Gets the current number of shortcut columns based on width.
  int GetCurrentShortcutColumns() const;

  // Rebuilds the shortcut grid with the given number of columns.
  void RebuildShortcutGrid(int columns);

  // Timer callback for clock updates.
  void OnClockTick();

  raw_ptr<Browser> browser_;
  raw_ptr<Profile> profile_;
  raw_ptr<Delegate> delegate_ = nullptr;
  raw_ptr<AstraNewTabModel> model_ = nullptr;

  // Top bar buttons.
  raw_ptr<views::ImageButton> settings_gear_button_ = nullptr;
  raw_ptr<views::ImageButton> profile_button_ = nullptr;

  // Section containers (owned by view hierarchy).
  raw_ptr<views::View> top_bar_ = nullptr;
  raw_ptr<views::View> greeting_section_ = nullptr;
  raw_ptr<views::Label> greeting_label_ = nullptr;
  raw_ptr<views::Label> clock_label_ = nullptr;
  raw_ptr<views::Label> date_label_ = nullptr;
  raw_ptr<views::View> workspace_section_ = nullptr;
  raw_ptr<views::View> shortcut_section_ = nullptr;
  raw_ptr<views::View> shortcut_grid_ = nullptr;
  raw_ptr<views::View> recent_section_ = nullptr;
  raw_ptr<views::View> quick_actions_section_ = nullptr;
  raw_ptr<views::View> suggested_content_section_ = nullptr;
  raw_ptr<views::View> footer_section_ = nullptr;

  // Container for horizontally scrollable recently closed items.
  raw_ptr<views::ScrollView> recent_scroll_view_ = nullptr;
  raw_ptr<views::View> recent_items_row_ = nullptr;

  // Shortcut views (owned by shortcut_grid_).
  std::vector<raw_ptr<AstraNtpShortcutView>> shortcut_views_;

  // Workspace card views (owned by workspace_section_).
  std::vector<raw_ptr<AstraNtpWorkspaceCard>> workspace_card_views_;

  // Quick action buttons (owned by quick_actions_section_).
  std::vector<raw_ptr<views::View>> quick_action_buttons_;

  // Suggested content views.
  std::vector<raw_ptr<views::View>> suggested_content_views_;

  // Current content width for responsive layout.
  int current_content_width_ = 0;

  // Current number of shortcut columns.
  int current_shortcut_columns_ = 4;

  // Drag state.
  raw_ptr<AstraNtpShortcutView> dragged_shortcut_ = nullptr;
  int drag_drop_index_ = -1;

  // Animation state.
  bool animations_skipped_ = false;
  bool entrance_animations_played_ = false;

  base::WeakPtrFactory<AstraNewTabView> weak_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_VIEW_H_
