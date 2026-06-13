#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_RECENTLY_CLOSED_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_RECENTLY_CLOSED_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/view.h"

class Profile;

namespace views {
class Label;
class LabelButton;
}  // namespace views

namespace astra {

class AstraRecentlyClosedItemView;
struct AstraRecentlyClosedTab;

// A sidebar section that shows recently closed tabs, projected from
// Chromium's TabRestoreService.
//
// This is a presentation-only view. It never stores or mutates session
// state — it only queries and displays Chromium's TabRestoreService data.
// TabRestoreService is the single source of truth for recently closed tabs.
//
// Layout (top to bottom):
//   - Section header ("Recently closed") — click to collapse/expand
//   - List of recently closed tab items (most recent first)
//   - "Restore all" footer link
//
// The view observes TabRestoreService via TabRestoreServiceObserver and
// refreshes its projection whenever tabs are closed or restored.
//
// Chromium owner: sessions::TabRestoreService
//   (chrome/browser/sessions/tab_restore_service.h)
// Chromium observer: TabRestoreServiceObserver
//   (chrome/browser/sessions/tab_restore_service_observer.h)
//
// TODO(astra): Implement TabRestoreServiceObserver for reactive updates.
// Currently, the view refreshes on construction and can be manually
// refreshed via Refresh(). Real-time observer support requires wiring
// into TabRestoreService's observer list.
// Chromium observer: TabRestoreServiceObserver
//   (chrome/browser/sessions/tab_restore_service_observer.h)
class AstraSidebarRecentlyClosedView : public views::View {
 public:
  // Delegate interface for actions that need browser-level context,
  // such as restoring a tab in the current window. The parent sidebar
  // view implements this so the recently closed section doesn't need
  // direct access to Browser.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Restore a specific recently closed tab by its entry id.
    // |entry_id| is the TabRestoreService entry id.
    // The tab should be restored in the current browser window.
    virtual void RestoreRecentlyClosedTab(int entry_id) = 0;

    // Restore all recently closed tabs.
    virtual void RestoreAllRecentlyClosedTabs() = 0;
  };

  explicit AstraSidebarRecentlyClosedView(Profile* profile);
  AstraSidebarRecentlyClosedView(const AstraSidebarRecentlyClosedView&) = delete;
  AstraSidebarRecentlyClosedView& operator=(const AstraSidebarRecentlyClosedView&) = delete;
  ~AstraSidebarRecentlyClosedView() override;

  // Set the delegate for navigation/actions. Not owned.
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // Refresh the recently closed list from TabRestoreService.
  // Reads current entries and rebuilds the item views.
  void Refresh();

  // Set the maximum number of recently closed items to show.
  void set_max_items(size_t max) { max_items_ = max; }
  size_t max_items() const { return max_items_; }

  // Set whether the section is expanded/collapsed.
  // When collapsed, only the header is visible.
  void SetExpanded(bool expanded);
  bool IsExpanded() const { return is_expanded_; }

  // Returns true if there are any recently closed items displayed.
  bool HasItems() const;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Clear all recently closed items from the items container.
  void ClearItems();

  // Populate the items container with recently closed tabs.
  // Reads from AstraRecentTabsHelper (which reads from TabRestoreService).
  void PopulateItems(const std::vector<AstraRecentlyClosedTab>& tabs);

  // Callback for the header click (toggle expanded state).
  void OnHeaderClicked();

  // Callback for the "Restore all" link.
  void OnRestoreAllClicked();

  // Callback when a recently closed item is clicked.
  // |entry_id| is the TabRestoreService entry id to restore.
  void OnItemClicked(int entry_id);

  raw_ptr<Profile> profile_;
  raw_ptr<Delegate> delegate_ = nullptr;

  // Maximum number of recently closed items to display.
  size_t max_items_ = 8;

  // Whether the section is expanded (items visible) or collapsed.
  bool is_expanded_ = true;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::LabelButton> header_button_ = nullptr;
  raw_ptr<views::View> items_container_ = nullptr;
  raw_ptr<views::Label> empty_state_label_ = nullptr;
  raw_ptr<views::LabelButton> restore_all_link_ = nullptr;

  // Weak pointer factory for async operations.
  // When this view is destroyed, any in-flight callbacks are silently
  // dropped instead of accessing freed memory.
  base::WeakPtrFactory<AstraSidebarRecentlyClosedView> weak_ptr_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_RECENTLY_CLOSED_VIEW_H_
