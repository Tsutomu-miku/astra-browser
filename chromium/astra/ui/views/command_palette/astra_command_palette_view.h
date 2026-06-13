#ifndef ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_VIEW_H_
#define ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"

#include "astra/ui/views/command_palette/astra_command_palette_model.h"

namespace views {
class Label;
class ScrollView;
class Textfield;
}  // namespace views

class Browser;

namespace astra {

class AstraCommandPaletteItemView;

// =========================================================================
// Command palette view — quick command launcher UI
// =========================================================================
//
// AstraCommandPaletteView is the main content view of the command palette.
// It contains a search textfield at the top, a scrollable list of
// command results in the middle, and a status bar at the bottom.
// As the user types, the list is filtered in real time.
//
// Layout:
//   +-------------------------------+
//   |  🔍  Type a command...        |  <- search field
//   +-------------------------------+
//   |  [+]  New Tab          ⌘T    |  <- scrollable results
//   |  [↻]  Reload           ⌘R    |
//   |  ...                          |
//   +-------------------------------+
//   |  24 results · ↑↓ navigate     |  <- status bar
//   +-------------------------------+
//
// Features:
//   - Real-time fuzzy matching on command names and descriptions.
//   - Keyboard navigation: Up/Down arrows to select, Enter to execute,
//     Escape to close.
//   - Configurable max results; scroll if more are available.
//   - Recently used commands are boosted in ranking.
//   - Status bar showing result count and navigation hints.
//
// The view observes the model and updates when results or selection change.
// It does not own command state — it is purely a search and dispatch UI.
//
// Command execution flow:
//   1. User presses Enter or clicks an item.
//   2. View calls model_->ExecuteCommand(index).
//   3. Model notifies observers via OnCommandExecuted.
//   4. View (as observer) forwards to its delegate.
//
// Chromium subsystems reused:
//   - views::Textfield for the search input.
//   - views::ScrollView for the results list.
//   - BrowserCommandController for Chrome command execution.
//
// Implements views::TextfieldController to receive real-time text change
// notifications from the search field, driving the results filter.
//
// Implements AstraCommandPaletteObserver to react to model changes
// (new-style observer interface).
//
// Implements AstraCommandPaletteModelObserver for backward compatibility
// with the legacy observer interface.
//
// TODO(astra): Add highlight styling for matched text in result items.
// Currently items display the full name and description but do not
// visually indicate which characters matched the query.
// Patch point / Chromium component: views::Label can render rich text
// with ranges; we can use StyledLabel or a custom label with text
// styles for matched ranges.
// TODO(astra): use views::StyledLabel from ui/views/controls/styled_label.
// =========================================================================

class AstraCommandPaletteView
    : public views::View,
      public views::TextfieldController,
      public AstraCommandPaletteObserver,
      public AstraCommandPaletteModelObserver {
 public:
  // Interface for delegates that handle command execution and closing.
  // All methods have empty default implementations — subclasses only
  // override the methods they care about.
  class Delegate {
   public:
    // Called when the user selects and executes a command.
    // |command_id| is the Chrome or Astra command ID.
    // |is_astra| is true if this is an Astra command (ID >= 60000).
    virtual void OnCommandPaletteExecute(int command_id, bool is_astra) {}

    // Called when the palette should be closed (Escape key, focus loss, etc.).
    virtual void OnCommandPaletteClose() {}

    // Called when the palette search text changes.
    virtual void OnCommandPaletteSearchTextChanged(
        const std::u16string& text) {}

    // Called when the selected command index changes.
    virtual void OnCommandPaletteSelectionChanged(int selected_index) {}

    // Called when the palette is opened.
    virtual void OnCommandPaletteOpened() {}

    // Called when the palette is closed.
    virtual void OnCommandPaletteClosed() {}

   protected:
    virtual ~Delegate() = default;
  };

  explicit AstraCommandPaletteView(Browser* browser);
  ~AstraCommandPaletteView() override;

  AstraCommandPaletteView(const AstraCommandPaletteView&) = delete;
  AstraCommandPaletteView& operator=(const AstraCommandPaletteView&) = delete;

  // -- Delegate ----------------------------------------------------------

  // Sets the delegate that receives command execution and close events.
  void SetDelegate(Delegate* delegate);

  // -- Model -------------------------------------------------------------

  // Sets the model (not owned).  The view observes the model and updates
  // when results change.  If model is null, the view shows an empty state.
  void SetModel(AstraCommandPaletteModel* model);

  // Returns the current model.
  AstraCommandPaletteModel* GetModel() const { return model_; }

  // Access the model (for external setup, e.g. updating workspace count).
  // Legacy name — prefer GetModel().
  AstraCommandPaletteModel* model() { return model_; }
  const AstraCommandPaletteModel* model() const { return model_; }

  // -- Search ------------------------------------------------------------

  // Gives focus to the search textfield.  Called when the palette opens.
  void RequestSearchFocus();

  // Sets the search query and triggers a search.
  void SetQuery(const std::u16string& query);

  // Returns the current search query.
  std::u16string GetQuery() const;

  // Clears the search text and refreshes results.
  void ClearSearch();

  // -- Selection ---------------------------------------------------------

  // Returns the index of the currently selected item, or -1 if none.
  int GetSelectedIndex() const;

  // Moves selection to the next item.  Wraps around at the end.
  void SelectNext();

  // Moves selection to the previous item.  Wraps around at the start.
  void SelectPrevious();

  // Selects the first item in the list.
  void SelectFirst();

  // Selects the last item in the list.
  void SelectLast();

  // -- Execution ---------------------------------------------------------

  // Executes the currently selected command.
  void ExecuteSelected();

  // -- Results -----------------------------------------------------------

  // Returns the number of currently displayed result items.
  size_t GetResultCount() const;

  // Returns the item view at the given index, or nullptr if out of range.
  AstraCommandPaletteItemView* GetResultViewAt(int index) const;

  // Scrolls the result at |index| into view.
  void ScrollToIndex(int index);

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;

  // -- AstraCommandPaletteObserver ---------------------------------------

  void OnCommandListChanged(AstraCommandPaletteModel* model) override;
  void OnSearchResultsChanged(AstraCommandPaletteModel* model) override;
  void OnCommandExecuted(AstraCommandPaletteModel* model,
                         int command_id) override;
  void OnCommandPaletteModelShutdown(AstraCommandPaletteModel* model) override;

  // -- AstraCommandPaletteModelObserver (legacy) -------------------------

  void OnModelChanged() override;
  void OnSelectionChanged() override;
  void OnCommandExecutionRequested(int command_id, bool is_astra) override;
  void OnSearchTextChanged(const std::u16string& new_text) override;

 private:
  // Build the child views and layout.  Called once from constructor.
  void BuildLayout();

  // Rebuild the results list from the current model state.
  void RebuildResultsList();

  // Update the visual selection state of all result items.
  void UpdateSelectionVisual();

  // Update the status bar text (result count, navigation hints).
  void UpdateStatusBar();

  // Scroll the selected item into view.
  void ScrollSelectedIntoView();

  // Get the number of currently displayed result items.
  // Internal helper used by both public API and internal code.
  size_t result_count() const { return item_views_.size(); }

  // -- views::TextfieldController ----------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

 private:
  // The browser context, used for command execution.
  raw_ptr<Browser> browser_;

  // The command model that owns the searchable command index.
  // Not owned when SetModel() is used; owned internally by default.
  raw_ptr<AstraCommandPaletteModel> model_ = nullptr;
  bool model_owned_ = true;

  // Delegate for command execution and close events.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::View> divider_top_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> results_container_ = nullptr;
  raw_ptr<views::View> divider_bottom_ = nullptr;
  raw_ptr<views::Label> status_bar_label_ = nullptr;

  // Cached item views for direct access by index.
  // Parallel to results_container_->children() but typed for convenience.
  std::vector<raw_ptr<AstraCommandPaletteItemView>> item_views_;

  base::WeakPtrFactory<AstraCommandPaletteView> weak_ptr_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_VIEW_H_
