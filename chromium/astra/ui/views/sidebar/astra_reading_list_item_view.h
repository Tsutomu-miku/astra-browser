#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_READING_LIST_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_READING_LIST_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

namespace views {
class ImageButton;
}  // namespace views

namespace astra {

// Delegate interface for AstraReadingListItemView actions.
class AstraReadingListItemDelegate {
 public:
  virtual ~AstraReadingListItemDelegate() = default;

  // Called when the user clicks the reading list item (primary action).
  virtual void OnReadingListItemClicked(const GURL& url) = 0;

  // Called when the user toggles the read/unread state.
  virtual void OnReadingListToggleRead(const GURL& url) = 0;

  // Called when the user clicks the remove button.
  virtual void OnReadingListRemove(const GURL& url) = 0;
};

// A single reading list item row in the sidebar reading list section.
// Shows a read/unread indicator, title, source domain, and (on hover)
// action buttons.
//
// This is a pure presentation view — it does not own reading list state.
// Data is projected from Chromium's ReadingListModel by the parent
// AstraSidebarReadingListView.
//
// TODO(astra): Replace placeholder icons with real vector icons from
//   chrome/browser/ui/vector_icons/.
//   Chromium owner: ReadLaterIcon (chrome/app/vector_icons/)
class AstraReadingListItemView : public AstraSidebarItemView {
 public:
  AstraReadingListItemView(const GURL& url,
                           const std::u16string& title,
                           const std::u16string& domain,
                           bool is_read);
  AstraReadingListItemView(const AstraReadingListItemView&) = delete;
  AstraReadingListItemView& operator=(const AstraReadingListItemView&) = delete;
  ~AstraReadingListItemView() override;

  // -- Reading list item info ---------------------------------------------

  // Set all reading list item info at once.
  void SetReadingListItem(const GURL& url,
                          const std::u16string& title,
                          bool is_read);

  // Get the URL of the reading list entry.
  const GURL& GetUrl() const { return url_; }

  // -- Read state ---------------------------------------------------------

  // Set whether the item has been read.
  void SetRead(bool read);
  bool IsRead() const { return is_read_; }

  // -- Estimated read time ------------------------------------------------

  // Set estimated reading time.
  void SetEstimatedReadTime(base::TimeDelta read_time);
  base::TimeDelta GetEstimatedReadTime() const { return estimated_read_time_; }

  // -- Word count ---------------------------------------------------------

  // Set the word count of the article.
  void SetWordCount(int count);
  int GetWordCount() const { return word_count_; }

  // -- Domain -------------------------------------------------------------

  // Set the source domain display text.
  void SetDomain(const std::u16string& domain);
  const std::u16string& GetDomain() const { return domain_; }

  // -- Read indicator -----------------------------------------------------

  // Show or hide the read/unread indicator.
  void ShowReadIndicator(bool show);

  // -- Distilled version --------------------------------------------------

  // Set whether the item has a distilled (reader mode) version.
  void SetHasDistilledVersion(bool has_distilled);
  bool HasDistilledVersion() const { return has_distilled_version_; }

  // -- Favorite -----------------------------------------------------------

  // Set whether the item is marked as favorite/starred.
  void SetIsFavorite(bool favorite);
  bool IsFavorite() const { return is_favorite_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraReadingListItemDelegate* delegate) {
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
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  // Update the read indicator appearance.
  void UpdateReadIndicator();

  // Update action button visibility based on hover state.
  void UpdateActionButtonsVisibility();

  // Update the secondary text (domain + read time / word count).
  void UpdateSecondaryText();

  // Button action handlers.
  void OnToggleReadButtonPressed();
  void OnRemoveButtonPressed();

  // The URL of the reading list entry.
  GURL url_;

  // Current projected state.
  bool is_read_ = false;
  base::TimeDelta estimated_read_time_;
  int word_count_ = 0;
  std::u16string domain_;
  bool show_read_indicator_ = true;
  bool has_distilled_version_ = false;
  bool is_favorite_ = false;

  // Action delegate. Not owned.
  raw_ptr<AstraReadingListItemDelegate> delegate_ = nullptr;

  // Child views.
  raw_ptr<views::ImageButton> toggle_read_button_ = nullptr;
  raw_ptr<views::ImageButton> remove_button_ = nullptr;

  // Hover state for showing/hiding action buttons.
  bool is_hovered_internal_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_READING_LIST_ITEM_VIEW_H_
