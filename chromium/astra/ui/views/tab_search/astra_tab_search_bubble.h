#ifndef ASTRA_UI_VIEWS_TAB_SEARCH_ASTRA_TAB_SEARCH_BUBBLE_H_
#define ASTRA_UI_VIEWS_TAB_SEARCH_ASTRA_TAB_SEARCH_BUBBLE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/widget/widget_observer.h"

#include "astra/ui/views/tab_search/astra_tab_search_item_view.h"
#include "astra/ui/views/tab_search/astra_tab_search_model.h"

namespace views {
class Label;
class ScrollView;
class Textfield;
}  // namespace views

class Browser;
class TabStripModel;

namespace astra {

// =========================================================================
// Tab search bubble — quick tab switcher UI
// =========================================================================
//
// AstraTabSearchBubble provides a Chromium bubble dialog for searching and
// switching between open tabs, recently closed tabs, and bookmarks.  Users
// type to filter results by title or URL, use arrow keys to navigate, and
// press Enter to activate the selected result.
//
// This is a projection layer only — all data comes from Chromium
// subsystems, projected through AstraTabSearchModel.
//
// Chromium subsystems reused:
//   - TabStripModel — source of truth for open tab data.
//   - TabRestoreService — recently closed tabs.
//   - BookmarkModel — bookmarks.
//   - views::BubbleDialogDelegateView — bubble UI framework.
//   - views::Textfield — search input field.
//   - views::ScrollView — scrollable results container.
//
// Chromium owner / reference implementation:
//   TabSearchBubbleHost / TabSearchButton in
//   chrome/browser/ui/views/tab_search/tab_search_bubble_host.h
//
// TODO(astra): Integrate with Chromium's TabSearchBubbleHost if we want
//   to reuse Chrome's tab search UI instead of building an Astra-specific
//   version.  The current approach gives us full control over styling and
//   behavior but duplicates some logic.  Evaluate tradeoffs.
//   Chromium owner: TabSearchBubbleHost (chrome/browser/ui/views/tab_search)
// =========================================================================

class AstraTabSearchBubble : public views::BubbleDialogDelegateView,
                             public views::TextfieldController,
                             public AstraTabSearchObserver {
 public:
  // Delegate interface for events from the tab search bubble.
  // Implemented by browser-level code (e.g. AstraBrowserView) to handle
  // user actions without the views layer depending on browser internals.
  //
  // All methods have default empty implementations so subclasses only
  // need to override the methods they care about.
  class Delegate {
   public:
    // Called when a tab is activated (user presses Enter or clicks).
    virtual void OnTabActivated(content::WebContents* web_contents) {}

    // Called when a tab is closed via the close button.
    virtual void OnTabClosed(content::WebContents* web_contents) {}

    // Called when the bubble is about to close (widget destruction).
    virtual void OnBubbleClosed() {}

    // Called when the search text changes.
    virtual void OnSearchTextChanged(const std::u16string& new_text) {}

    // Called when the selected result changes.
    virtual void OnSelectionChanged(size_t index) {}

    // Called when the result count changes.
    virtual void OnResultCountChanged(size_t count) {}

    // Called when the bubble is opened (shown).
    virtual void OnBubbleOpened() {}

    // Called when a bulk close is requested for all matching tabs.
    virtual void OnBulkCloseRequested(size_t count) {}

    // Called when a tab is requested to be closed from search.
    virtual void OnTabCloseRequested(content::WebContents* web_contents) {}

    // Called when the search mode changes.
    virtual void OnSearchModeChanged(AstraTabSearchMode mode) {}

   protected:
    ~Delegate() = default;
  };

  // Legacy search result struct — kept for backward compatibility.
  // Prefer using AstraTabSearchItem from the model.
  struct SearchResult {
    AstraTabSearchItemView::Group group =
        AstraTabSearchItemView::Group::kOpenTabs;
    std::u16string title;
    std::u16string host;
    int tab_index = -1;
    std::string identifier;
    int score = 0;
  };

  // Creates and shows the tab search bubble anchored to |anchor_view|.
  // Returns the bubble widget (owned by the widget system).
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   Browser* browser,
                                   Delegate* delegate);

  ~AstraTabSearchBubble() override;

  AstraTabSearchBubble(const AstraTabSearchBubble&) = delete;
  AstraTabSearchBubble& operator=(const AstraTabSearchBubble&) = delete;

  // -- Bubble visibility ---------------------------------------------------

  // Show the bubble anchored to the given native view and rect.
  void Show(gfx::NativeView parent, const gfx::Rect& anchor_rect);

  // Hide (close) the bubble.
  void Hide();

  // Whether the bubble is currently visible.
  bool IsVisible() const;

  // Get the inner content view of the bubble.
  views::View* GetBubbleView();

  // -- Query management ----------------------------------------------------

  // Set the search query text programmatically.
  void SetQuery(const std::u16string& query);

  // Get the current search query text.
  std::u16string GetQuery() const;

  // -- Selection navigation ------------------------------------------------

  // Move selection to the next item.
  void SelectNext();

  // Move selection to the previous item.
  void SelectPrevious();

  // Move selection to the first item.
  void SelectFirst();

  // Move selection to the last item.
  void SelectLast();

  // Get the index of the currently selected item.
  size_t GetSelectedIndex() const { return selected_index_; }

  // Activate (switch to) the currently selected tab.
  void ActivateSelected();

  // -- Search mode ---------------------------------------------------------

  // Set the current search mode (all tabs, current workspace, etc.).
  void SetSearchMode(AstraTabSearchMode mode);

  // Get the current search mode.
  AstraTabSearchMode GetSearchMode() const;

  // -- Bubble sizing -------------------------------------------------------

  // Set the preferred width of the bubble.
  void SetBubbleWidth(int width);

  // Set the preferred height of the bubble.
  void SetBubbleHeight(int height);

  // -- Access to underlying model ------------------------------------------

  AstraTabSearchModel* model() { return &model_; }
  const AstraTabSearchModel* model() const { return &model_; }

  // -- views::BubbleDialogDelegateView -------------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;

  // Gives focus to the search field.
  void RequestSearchFocus();

  // -- views::TextfieldController ------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // -- AstraTabSearchObserver ----------------------------------------------

  void OnTabListChanged(AstraTabSearchModel* model) override;
  void OnSearchResultsChanged(AstraTabSearchModel* model) override;
  void OnSearchModeChanged(AstraTabSearchModel* model,
                           AstraTabSearchMode mode) override;
  void OnTabSearchModelShutdown(AstraTabSearchModel* model) override;

 private:
  AstraTabSearchBubble(views::View* anchor_view,
                       Browser* browser,
                       Delegate* delegate);

  // Build the bubble contents.
  void Init() override;

  // Build the child views and layout.
  void BuildLayout();

  // -----------------------------------------------------------------------
  // Results UI
  // -----------------------------------------------------------------------

  // Update the results list based on the current search query and mode.
  void UpdateResults();

  // Add a group header label to the results container.
  void AddGroupHeader(AstraTabSearchItemView::Group group, size_t count);

  // Add a result item to the results container.
  AstraTabSearchItemView* AddResultItem(const AstraTabSearchItem& item,
                                        int display_index);

  // Move the selection by |delta| items (positive = down, negative = up).
  // Clamps to valid range.
  void MoveSelection(int delta);

  // Move selection to the first item of the next/previous group.
  void MoveToNextGroup();
  void MoveToPreviousGroup();

  // Activate the currently selected result and close the bubble.
  void ActivateSelectedResult();

  // Activate the result at the given index.
  void ActivateResultAt(size_t index);

  // Close the tab at the given result index and refresh results.
  void CloseTabAtResultIndex(size_t result_index);

  // Get the number of selectable result items.
  size_t GetResultCount() const;

  // Update the visual selection state of all result items.
  void UpdateSelectionVisual();

  // Update the result count label text.
  void UpdateResultCountLabel();

  // Ensure the selected item is visible in the scroll view.
  void ScrollSelectedIntoView();

  // Find the result item view at the given index.
  // Returns nullptr if the index is out of range.
  AstraTabSearchItemView* GetItemViewAt(size_t index) const;

  // Get the group of the result at the given index.
  AstraTabSearchItemView::Group GetGroupAt(size_t index) const;

  // -----------------------------------------------------------------------
  // Helpers
  // -----------------------------------------------------------------------

  // Get the TabStripModel from the browser.
  TabStripModel* GetTabStripModel() const;

  // Get a human-readable group label for headers.
  static std::u16string GetGroupLabel(AstraTabSearchItemView::Group group,
                                      size_t count);

  // Refresh the model's tab list from the TabStripModel.
  void RefreshModelFromTabStrip();

  raw_ptr<Browser> browser_;
  raw_ptr<Delegate> delegate_ = nullptr;

  // The tab search model (owns search state and results).
  AstraTabSearchModel model_;

  // Current search results (cached from model for UI rendering).
  std::vector<AstraTabSearchItem> results_;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::Label> result_count_label_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> results_container_ = nullptr;

  // Currently selected result index (0-based, into results_).
  size_t selected_index_ = 0;

  // Bubble dimensions.
  int bubble_width_ = 360;
  int bubble_max_height_ = 480;

  // Maximum number of results to show.
  static constexpr size_t kMaxResults = 15;

  base::WeakPtrFactory<AstraTabSearchBubble> weak_ptr_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_SEARCH_ASTRA_TAB_SEARCH_BUBBLE_H_
