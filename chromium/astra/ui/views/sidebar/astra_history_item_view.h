#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_HISTORY_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_HISTORY_ITEM_VIEW_H_

#include <string>

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

// Delegate interface for AstraHistoryItemView actions.
class AstraHistoryItemDelegate {
 public:
  virtual ~AstraHistoryItemDelegate() = default;

  // Called when the history item is clicked.
  virtual void OnHistoryItemClicked(const GURL& url) = 0;

  // Called when the "Remove from history" action is triggered.
  virtual void OnHistoryItemRemoved(const GURL& url) = 0;
};

// A single history entry row in the sidebar history section.
//
// Shows:
//   - Favicon placeholder (left side)
//   - Page title (primary text)
//   - URL / domain (secondary text, smaller)
//   - Relative time indicator (e.g. "2h ago") on hover or secondary line
//
// This is a pure presentation view. Data is projected from Chromium's
// HistoryService by the parent AstraSidebarHistoryView. The item view
// does not store or cache history state — it only renders what it is given.
//
// Clicking the item navigates to the URL.
//
// TODO(astra): Add a "Remove from history" action button that appears on
//   hover, similar to Chrome's history page.
//   Chromium owner: history::HistoryService::RemoveURLs
//     (components/history/core/browser/history_service.h)
class AstraHistoryItemView : public AstraSidebarItemView {
 public:
  AstraHistoryItemView(const std::u16string& title,
                       const GURL& url,
                       base::Time visit_time);
  AstraHistoryItemView(const AstraHistoryItemView&) = delete;
  AstraHistoryItemView& operator=(const AstraHistoryItemView&) = delete;
  ~AstraHistoryItemView() override;

  // -- History info -------------------------------------------------------

  // Set all history info at once.
  void SetHistoryInfo(const GURL& url,
                      const std::u16string& title,
                      base::Time visit_time);

  // Get the URL of the history entry.
  const GURL& GetUrl() const { return url_; }

  // Get the last visit time.
  base::Time GetVisitTime() const { return visit_time_; }

  // -- Visit count --------------------------------------------------------

  // Set the total visit count for this URL.
  void SetVisitCount(int count);
  int GetVisitCount() const { return visit_count_; }

  // Show or hide the visit count display.
  void ShowVisitCount(bool show);

  // -- Time grouping ------------------------------------------------------

  // Set whether this entry is from today (special styling).
  void SetIsToday(bool is_today);
  bool IsToday() const { return is_today_; }

  // Set the time group label (e.g. "Today", "Yesterday", "Last week").
  void SetTimeGroup(const std::u16string& group_name);
  const std::u16string& GetTimeGroup() const { return time_group_; }

  // -- Typed visit --------------------------------------------------------

  // Set whether this URL was typed directly by the user.
  void SetTypedVisit(bool typed);
  bool IsTypedVisit() const { return is_typed_visit_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraHistoryItemDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

 protected:
  // AstraSidebarItemView overrides.
  void BuildLayout() override;
  void OnItemClicked() override;

 private:
  // Compute and return the relative time string (e.g. "2h ago", "Yesterday").
  // TODO(astra): Use Chromium's time formatting utilities.
  std::u16string GetRelativeTimeText() const;

  // Extract a human-readable domain from the URL for secondary display.
  std::u16string GetDomainText() const;

  // Update the secondary label from URL and time info.
  void UpdateSecondaryText();

  // Update the tooltip text.
  void UpdateTooltipText();

  // Data projected from HistoryService.
  GURL url_;
  base::Time visit_time_;
  int visit_count_ = 1;
  bool is_today_ = false;
  std::u16string time_group_;
  bool is_typed_visit_ = false;
  bool show_visit_count_ = false;

  // Action delegate. Not owned.
  raw_ptr<AstraHistoryItemDelegate> delegate_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_HISTORY_ITEM_VIEW_H_
