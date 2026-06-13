#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_RECENTLY_CLOSED_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_RECENTLY_CLOSED_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/view.h"
#include "url/gurl.h"

class Profile;

namespace views {
class Label;
class LabelButton;
class Textfield;
}  // namespace views

namespace astra {

class AstraRecentlyClosedItemView;

// Type of recently closed item.
enum class AstraRecentlyClosedType {
  kTab,    // A single closed tab.
  kWindow, // A closed window (with multiple tabs).
};

// Data structure describing a recently closed item (tab or window).
//
// This is a projection struct — the truth source is Chromium's
// TabRestoreService. The sidebar projects restore service data into
// these structs for display.
//
// Chromium owner: sessions::TabRestoreService
//   (chrome/browser/sessions/tab_restore_service.h)
struct AstraRecentlyClosedItem {
  // Stable identifier for the item. Maps to TabRestoreService entry ID.
  std::string id;

  // Display title (tab title or window title).
  std::u16string title;

  // URL (for tabs; for windows, this may be empty or the first tab's URL).
  GURL url;

  // Type of item: tab or window.
  AstraRecentlyClosedType type = AstraRecentlyClosedType::kTab;

  // Time the item was closed.
  base::Time close_time;

  // Number of tabs (for windows only; 1 for single tabs).
  int tab_count = 1;

  // Favicon image.
  gfx::ImageSkia favicon;

  // Whether a favicon is available.
  bool has_favicon = false;

  // Session ID for grouping items by session.
  int session_id = 0;

  // Whether this item was from an incognito window.
  bool is_incognito = false;

  // Window bounds (for windows, used for restoration).
  gfx::Rect window_bounds;
};

// Delegate interface for AstraSidebarRecentlyClosedView user actions.
//
// The delegate is implemented by the parent sidebar view or controller.
// All user-initiated actions flow through this delegate so the view
// remains a pure projection.
//
// Chromium owner: TabRestoreService (chrome/browser/sessions/tab_restore_service.h)
class AstraSidebarRecentlyClosedDelegate {
 public:
  virtual ~AstraSidebarRecentlyClosedDelegate() = default;

  // Called when an item is clicked (restore action).
  virtual void OnItemClicked(const std::string& item_id) = 0;

  // Called when an item is middle-clicked.
  virtual void OnItemMiddleClicked(const std::string& item_id) = 0;

  // Called when an item is right-clicked (context menu).
  // |point| is in screen coordinates.
  virtual void OnItemRightClicked(const std::string& item_id,
                                  const gfx::Point& point) = 0;

  // Called to restore a specific tab.
  virtual void OnRestoreTab(const std::string& item_id) = 0;

  // Called to restore a specific window.
  virtual void OnRestoreWindow(const std::string& item_id) = 0;

  // Called when "restore all" is requested.
  virtual void OnRestoreAllRequested() = 0;

  // Called when an item should be removed from the recently closed list.
  virtual void OnRemoveItem(const std::string& item_id) = 0;

  // Called when "clear all" is requested.
  virtual void OnClearAllRequested() = 0;

  // Called when the search query changes.
  virtual void OnSearch(const std::u16string& query) = 0;
};

// A sidebar section that shows recently closed tabs and windows.
//
// Data is projected from Chromium's TabRestoreService. This is a
// presentation-only view — it never stores or mutates session state.
//
// Layout (top to bottom):
//   - Section header (click to collapse/expand)
//   - Optional search box
//   - List of recently closed items (most recent first)
//   - Optional "Restore all" / "Clear all" footer
//
// Chromium owner: sessions::TabRestoreService
//   (chrome/browser/sessions/tab_restore_service.h)
// Chromium observer: TabRestoreServiceObserver
//   (chrome/browser/sessions/tab_restore_service_observer.h)
//
// TODO(astra): Wire to TabRestoreService for recently closed data.
//   Currently the view can operate with AstraRecentlyClosedItem data only.
//   The Profile-based constructor provides TabRestoreService integration.
class AstraSidebarRecentlyClosedView : public views::View {
 public:
  // Default constructor for tests. Does not connect to TabRestoreService.
  AstraSidebarRecentlyClosedView();

  // Construct with a Profile for TabRestoreService integration.
  explicit AstraSidebarRecentlyClosedView(Profile* profile);

  AstraSidebarRecentlyClosedView(const AstraSidebarRecentlyClosedView&) = delete;
  AstraSidebarRecentlyClosedView& operator=(const AstraSidebarRecentlyClosedView&) = delete;
  ~AstraSidebarRecentlyClosedView() override;

  // -- Delegate ------------------------------------------------------------

  void set_delegate(AstraSidebarRecentlyClosedDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraSidebarRecentlyClosedDelegate* delegate() const { return delegate_; }

  // -- Item management -----------------------------------------------------

  // Set all items at once (full rebuild).
  void SetRecentlyClosed(const std::vector<AstraRecentlyClosedItem>& items);

  // Get the number of items displayed.
  int GetItemCount() const;

  // Get item info at the given index.
  AstraRecentlyClosedItem GetItemAt(int index) const;

  // Add an item to the front (most recent position).
  void AddItem(const AstraRecentlyClosedItem& item);

  // Remove the item at the given index.
  void RemoveItem(int index);

  // Clear all items.
  void ClearAll();

  // -- Selection -----------------------------------------------------------

  // Set the selected item by index. -1 clears selection.
  void SetSelectedItem(int index);

  // Get the index of the selected item. Returns -1 if none selected.
  int GetSelectedIndex() const;

  // Clear the current selection.
  void ClearSelection();

  // -- Restore operations --------------------------------------------------

  // Restore (reopen) a closed tab at the given index.
  void RestoreTab(int index);

  // Restore (reopen) a closed window at the given index.
  void RestoreWindow(int index);

  // Restore all recently closed items.
  void RestoreAll();

  // -- Display options -----------------------------------------------------

  // Set the maximum number of items to show.
  void SetMaxItems(int max);
  int GetMaxItems() const;

  // Set whether closed windows are shown.
  void SetShowWindows(bool show);
  bool GetShowWindows() const;

  // Set whether closed tabs are shown.
  void SetShowTabs(bool show);
  bool GetShowTabs() const;

  // Set whether favicons are shown.
  void SetShowFavicons(bool show);
  bool GetShowFavicons() const;

  // Set whether the close time is shown.
  void SetShowTime(bool show);
  bool GetShowTime() const;

  // Set whether the tab count is shown for window items.
  void SetShowTabCount(bool show);
  bool GetShowTabCount() const;

  // Set whether items are grouped by session.
  void SetGroupBySession(bool group);
  bool GetGroupBySession() const;

  // -- Counts --------------------------------------------------------------

  // Get the total number of closed tabs (across all items, including windows).
  int GetTabCount() const;

  // Get the number of closed window items.
  int GetWindowCount() const;

  // -- Search --------------------------------------------------------------

  // Search recently closed items by title/URL.
  void SearchRecentlyClosed(const std::u16string& query);

  // Get the number of search results.
  int GetSearchResultsCount() const;

  // Set whether the search box is shown.
  void SetShowSearch(bool show);
  bool GetShowSearch() const;

  // -- Footer buttons ------------------------------------------------------

  // Set whether the "restore all" button is shown.
  void SetShowRestoreAllButton(bool show);
  bool GetShowRestoreAllButton() const;

  // Set whether the "clear all" button is shown.
  void SetShowClearAllButton(bool show);
  bool GetShowClearAllButton() const;

  // -- View access ---------------------------------------------------------

  // Get the item view at the given index.
  AstraRecentlyClosedItemView* GetItemViewAt(int index);

  // -- Section expansion ---------------------------------------------------

  // Set whether the section is expanded/collapsed.
  void SetExpanded(bool expanded);
  bool IsExpanded() const { return is_expanded_; }

  // Returns true if there are any items displayed.
  bool HasItems() const;

  // -- Refresh (TabRestoreService integration) -----------------------------

  // Refresh the recently closed list from TabRestoreService.
  void Refresh();

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Clear all items from the items container.
  void ClearItems();

  // Rebuild item views from current data.
  void RebuildItems();

  // Filter items based on current display options and search.
  std::vector<AstraRecentlyClosedItem> GetFilteredItems() const;

  // Apply display options to all item views.
  void ApplyDisplayOptions();

  // Update footer button visibility.
  void UpdateFooterVisibility();

  // Update empty state visibility.
  void UpdateEmptyStateVisibility();

  // Handlers for user actions.
  void OnHeaderClicked();
  void OnRestoreAllClicked();
  void OnClearAllClicked();
  void OnItemClicked(int index);
  void OnSearchTextChanged();

  // The profile for TabRestoreService integration. Null for test instances.
  raw_ptr<Profile> profile_ = nullptr;

  // Action delegate. Not owned.
  raw_ptr<AstraSidebarRecentlyClosedDelegate> delegate_ = nullptr;

  // Full item list (unfiltered).
  std::vector<AstraRecentlyClosedItem> items_;

  // Search query (empty means no search filter).
  std::u16string search_query_;

  // Selected item index. -1 means no selection.
  int selected_index_ = -1;

  // Maximum number of items to display.
  int max_items_ = 8;

  // Whether the section is expanded (items visible) or collapsed.
  bool is_expanded_ = true;

  // Display options.
  bool show_windows_ = true;
  bool show_tabs_ = true;
  bool show_favicons_ = true;
  bool show_time_ = true;
  bool show_tab_count_ = true;
  bool group_by_session_ = false;

  // UI element visibility options.
  bool show_search_ = false;
  bool show_restore_all_button_ = true;
  bool show_clear_all_button_ = false;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::LabelButton> header_button_ = nullptr;
  raw_ptr<views::View> search_container_ = nullptr;
  raw_ptr<views::Textfield> search_textfield_ = nullptr;
  raw_ptr<views::View> items_container_ = nullptr;
  raw_ptr<views::Label> empty_state_label_ = nullptr;
  raw_ptr<views::View> footer_container_ = nullptr;
  raw_ptr<views::LabelButton> restore_all_link_ = nullptr;
  raw_ptr<views::LabelButton> clear_all_link_ = nullptr;

  // Cached item views, parallel to the filtered items displayed.
  std::vector<raw_ptr<AstraRecentlyClosedItemView, VectorExperimental>> item_views_;

  // Weak pointer factory for async operations.
  base::WeakPtrFactory<AstraSidebarRecentlyClosedView> weak_ptr_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_RECENTLY_CLOSED_VIEW_H_
