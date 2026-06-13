#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_RECENTLY_CLOSED_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_RECENTLY_CLOSED_ITEM_VIEW_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

namespace astra {

// Delegate interface for AstraRecentlyClosedItemView actions.
class AstraRecentlyClosedItemDelegate {
 public:
  virtual ~AstraRecentlyClosedItemDelegate() = default;

  // Called when the item is clicked (primary action: restore the tab).
  // |entry_id| is the TabRestoreService entry id to restore.
  virtual void OnRecentlyClosedItemClicked(int entry_id) = 0;

  // Called when the item is middle-clicked.
  virtual void OnRecentlyClosedItemMiddleClicked(int entry_id) = 0;
};

// A single recently-closed tab row in the sidebar "Recently closed" section.
//
// Shows:
//   - Favicon placeholder (left side)
//   - Tab title (primary text)
//   - URL / domain + relative time (secondary text)
//
// This is a pure presentation view. Data is projected from Chromium's
// TabRestoreService by the parent AstraSidebarRecentlyClosedView.
//
// Chromium owner: sessions::TabRestoreService
//   (chrome/browser/sessions/tab_restore_service.h)
//
// TODO(astra): Add a "Remove from recently closed" action button that
//   appears on hover.
//   Chromium method: TabRestoreService::RemoveEntryById
class AstraRecentlyClosedItemView : public AstraSidebarItemView {
 public:
  AstraRecentlyClosedItemView(const std::u16string& title,
                              const GURL& url,
                              base::Time close_time,
                              int entry_id);
  AstraRecentlyClosedItemView(const AstraRecentlyClosedItemView&) = delete;
  AstraRecentlyClosedItemView& operator=(
      const AstraRecentlyClosedItemView&) = delete;
  ~AstraRecentlyClosedItemView() override;

  // -- Recently closed info -----------------------------------------------

  // Set all recently closed info at once.
  void SetRecentlyClosedInfo(const GURL& url,
                             const std::u16string& title,
                             base::Time close_time);

  // Get the URL.
  const GURL& GetUrl() const { return url_; }

  // Get the close time.
  base::Time GetCloseTime() const { return close_time_; }

  // -- Tab count (for windows) --------------------------------------------

  // Set the number of tabs (for recently closed windows).
  void SetTabCount(int count);
  int GetTabCount() const { return tab_count_; }

  // -- Window vs tab ------------------------------------------------------

  // Whether this represents a window (multiple tabs) vs. a single tab.
  bool IsWindow() const { return is_window_; }
  void SetIsWindow(bool is_window);

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraRecentlyClosedItemDelegate* delegate) {
    delegate_ = delegate;
  }

  // Get the string ID of this item.
  const std::string& GetId() const { return item_id_; }

  // Set the string ID of this item.
  void SetId(const std::string& id) { item_id_ = id; }

  // Set the callback for click handling.
  using ClickCallback = base::RepeatingClosure;
  void set_callback(ClickCallback callback) { click_callback_ = std::move(callback); }

  int entry_id() const { return entry_id_; }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

 protected:
  // AstraSidebarItemView overrides.
  void BuildLayout() override;
  void OnItemClicked() override;

 private:
  // Compute and return the relative time string.
  // TODO(astra): Use Chromium's time formatting utilities.
  std::u16string GetRelativeTimeText() const;

  // Extract a human-readable domain from the URL.
  std::u16string GetDomainText() const;

  // Build the secondary text: domain + relative time.
  std::u16string GetSecondaryText() const;

  // Update the secondary label from current state.
  void UpdateSecondaryText();

  // Update the tooltip.
  void UpdateTooltipText();

  // Handle middle-click.
  void HandleMiddleClick();

  // Data projected from TabRestoreService.
  std::string item_id_;
  GURL url_;
  base::Time close_time_;
  int entry_id_;
  int tab_count_ = 1;
  bool is_window_ = false;

  // Action delegate. Not owned.
  raw_ptr<AstraRecentlyClosedItemDelegate> delegate_ = nullptr;

  // Simple click callback (alternate to delegate).
  ClickCallback click_callback_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_RECENTLY_CLOSED_ITEM_VIEW_H_
