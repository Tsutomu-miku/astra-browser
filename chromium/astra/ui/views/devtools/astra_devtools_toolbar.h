#ifndef ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_TOOLBAR_H_
#define ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_TOOLBAR_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/view.h"
#include "astra/ui/views/devtools/astra_devtools_model.h"

namespace content {
class WebContents;
}  // namespace content

namespace astra {

// =========================================================================
// AstraDevToolsToolbar — Astra-branded toolbar extension for DevTools
// =========================================================================
//
// A horizontal toolbar strip that sits inside the DevTools window, adding
// Astra-specific shortcut buttons, panel tabs, and search alongside
// Chromium's native DevTools toolbar.
//
// This is a pure projection / command surface — it does not own any state.
// All state comes from the AstraDevToolsModel.  Button clicks dispatch
// through the delegate interface.
//
// Features (deepened):
//   - Astra tab/button (opens Astra panel drawer)
//   - Panel selector buttons (one per visible Astra panel)
//   - Dock state button (cycles dock positions)
//   - Close button (closes DevTools)
//   - Menu button (more actions)
//   - Search box for filtering panel content
//   - Settings button (opens settings drawer)
//   - Back/forward navigation buttons
//   - Theming support (light/dark)
//
// Design principles:
//   - Minimal footprint — matches Chromium DevTools visual style.
//   - No truth — reads all state from AstraDevToolsModel.
//   - Commands only — dispatches through the delegate interface.
//
// Chromium subsystems reused:
//   - DevToolsWindow — hosts the toolbar in its title/toolbar area.
//   - views::LabelButton — button widgets.
//   - views::Textfield — search box.
//   - views::BoxLayout — horizontal layout.
//
// Chromium patch point:
//   TODO(astra): Inject this toolbar into the DevToolsWindow's title bar
//     or main toolbar area. Two possible approaches:
//     1. Patch chrome/browser/devtools/devtools_window.cc to create and
//        insert an AstraDevToolsToolbar into the DevTools window's
//        toolbar container.
//     2. Use DevToolsUIBindings to add a custom panel button via the
//        DevTools extension API (chrome.devtools.panels), but that would
//        be WebUI-based, not native Views.
//   Chromium owner: DevToolsWindow
//     (chrome/browser/devtools/devtools_window.h)
//   Chromium owner: DevToolsUIBindings
//     (chrome/browser/devtools/devtools_ui_bindings.h)
// =========================================================================

class AstraDevToolsToolbar : public views::View {
 public:
  // Delegate interface for toolbar actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when a panel tab is clicked.
    virtual void OnPanelTabClicked(const std::string& panel_id) = 0;

    // Called when the settings button is clicked.
    virtual void OnSettingsClicked() = 0;

    // Called when the detach button is clicked.
    virtual void OnDetachClicked() = 0;

    // Called when the menu button is clicked.
    virtual void OnMenuClicked() = 0;

    // Called when back navigation is requested.
    virtual void OnBackClicked() = 0;

    // Called when forward navigation is requested.
    virtual void OnForwardClicked() = 0;

    // Called when the search text changes.
    virtual void OnSearchTextChanged(const std::u16string& text) = 0;

    // Called when focus mode is toggled.
    virtual void OnFocusModeToggled() = 0;

    // Called when the close button is clicked.
    virtual void OnCloseClicked() = 0;

    // Called when the Astra tab button is clicked.
    virtual void OnAstraTabClicked() = 0;

    // Called when the dock button is clicked.
    virtual void OnDockClicked() = 0;
  };

  explicit AstraDevToolsToolbar(Delegate* delegate);
  ~AstraDevToolsToolbar() override;

  AstraDevToolsToolbar(const AstraDevToolsToolbar&) = delete;
  AstraDevToolsToolbar& operator=(const AstraDevToolsToolbar&) = delete;

  // -- Model integration ---------------------------------------------------

  // Sets the model to read panel and setting state from.
  // The model must outlive this toolbar.
  void SetModel(AstraDevToolsModel* model);

  // Returns the current model, or null if none is set.
  AstraDevToolsModel* GetModel() const { return model_; }

  // Refreshes all UI from the model state.
  void UpdateFromModel();

  // -- Dock state ----------------------------------------------------------

  // Sets the displayed dock state.
  void SetDockState(AstraDevToolsDockState state);

  // Returns the current dock state displayed by the toolbar.
  AstraDevToolsDockState GetDockState() const { return dock_state_; }

  // -- Active panel --------------------------------------------------------

  // Sets the active panel by ID and updates the UI.
  void SetActivePanel(const std::string& panel_id);

  // Returns the active panel ID.
  std::string GetActivePanel() const { return active_panel_id_; }

  // -- Panel button visibility --------------------------------------------

  // Shows or hides a specific panel button by ID.
  void ShowPanelButton(const std::string& panel_id, bool show);

  // Returns true if the panel button is currently visible.
  bool IsPanelButtonVisible(const std::string& panel_id) const;

  // -- Astra tab visibility ------------------------------------------------

  // Sets whether the "Astra" tab/button is visible.
  void SetAstraTabVisible(bool visible);

  // Returns whether the Astra tab is visible.
  bool IsAstraTabVisible() const;

  // -- Panel button access -------------------------------------------------

  // Returns the number of panel buttons currently shown.
  size_t GetPanelButtonCount() const { return panel_tabs_.size(); }

  // Returns the panel button at the given index, or null if out of bounds.
  views::LabelButton* GetPanelButtonAt(int index) const;

  // -- Toolbar visibility --------------------------------------------------

  // Sets whether the entire toolbar is visible.
  void SetToolbarVisible(bool visible);

  // Returns whether the toolbar is visible.
  bool IsToolbarVisible() const { return GetVisible(); }

  // -- Dock/close button visibility ---------------------------------------

  // Sets whether the dock button is visible.
  void SetDockButtonVisible(bool visible);

  // Sets whether the close button is visible.
  void SetCloseButtonVisible(bool visible);

  // -- WebContents ---------------------------------------------------------

  // Sets the inspected WebContents.  The toolbar reads Astra metadata
  // (focus mode state, workspace) from this tab's services.
  void SetInspectedWebContents(content::WebContents* web_contents);

  // -- Theme ---------------------------------------------------------------

  // Sets the theme for the toolbar (dark = true, light = false).
  void SetTheme(bool dark_theme);

  // Returns the current search text.
  std::u16string search_text() const {
    return search_box_ ? search_box_->GetText() : std::u16string();
  }

  // -- Accessors for testing -----------------------------------------------

  views::LabelButton* back_button_for_testing() { return back_button_; }
  views::LabelButton* forward_button_for_testing() { return forward_button_; }
  views::Textfield* search_box_for_testing() { return search_box_; }
  views::LabelButton* settings_button_for_testing() { return settings_button_; }
  views::LabelButton* detach_button_for_testing() { return detach_button_; }
  views::LabelButton* menu_button_for_testing() { return menu_button_; }
  views::LabelButton* focus_mode_button_for_testing() {
    return focus_mode_button_;
  }
  views::LabelButton* astra_tab_button_for_testing() {
    return astra_tab_button_;
  }
  views::LabelButton* dock_button_for_testing() { return dock_button_; }
  views::LabelButton* close_button_for_testing() { return close_button_; }
  views::View* panel_tabs_container_for_testing() {
    return panel_tabs_container_;
  }
  size_t panel_tab_count_for_testing() const;

 private:
  // Creates the toolbar UI structure.
  void BuildToolbar();

  // Rebuilds panel tabs from the model.
  void RebuildPanelTabs();

  // Updates the active panel tab appearance.
  void UpdateActivePanelTab();

  // Updates the dock button appearance based on current dock state.
  void UpdateDockButton();

  // Button click handlers.
  void OnBackButtonPressed();
  void OnForwardButtonPressed();
  void OnSettingsButtonPressed();
  void OnDetachButtonPressed();
  void OnMenuButtonPressed();
  void OnFocusModeButtonPressed();
  void OnPanelTabPressed(const std::string& panel_id);
  void OnSearchTextChanged();
  void OnCloseButtonPressed();
  void OnAstraTabPressed();
  void OnDockButtonPressed();

  // Applies the current theme to all toolbar elements.
  void ApplyTheme();

  // Delegate for action dispatch.  Not owned.
  raw_ptr<Delegate> delegate_;

  // The model providing state.  Not owned.
  raw_ptr<AstraDevToolsModel> model_ = nullptr;

  // The WebContents currently being inspected.  Not owned.
  raw_ptr<content::WebContents> inspected_contents_ = nullptr;

  // Whether dark theme is active.
  bool dark_theme_ = true;

  // Current dock state displayed in the toolbar.
  AstraDevToolsDockState dock_state_ = AstraDevToolsDockState::kDockedBottom;

  // Currently active panel ID.
  std::string active_panel_id_;

  // Container for panel tab buttons.  Owned by views hierarchy.
  raw_ptr<views::View> panel_tabs_container_ = nullptr;

  // Navigation buttons.  Owned by views hierarchy.
  raw_ptr<views::LabelButton> back_button_ = nullptr;
  raw_ptr<views::LabelButton> forward_button_ = nullptr;

  // Astra tab button.  Owned by views hierarchy.
  raw_ptr<views::LabelButton> astra_tab_button_ = nullptr;

  // Search box.  Owned by views hierarchy.
  raw_ptr<views::Textfield> search_box_ = nullptr;

  // Action buttons.  Owned by views hierarchy.
  raw_ptr<views::LabelButton> focus_mode_button_ = nullptr;
  raw_ptr<views::LabelButton> settings_button_ = nullptr;
  raw_ptr<views::LabelButton> detach_button_ = nullptr;
  raw_ptr<views::LabelButton> dock_button_ = nullptr;
  raw_ptr<views::LabelButton> menu_button_ = nullptr;
  raw_ptr<views::LabelButton> close_button_ = nullptr;

  // Panel tab buttons.  Owned by panel_tabs_container_'s children.
  std::vector<raw_ptr<views::LabelButton>> panel_tabs_;

  // Map from panel ID to its tab button index in panel_tabs_.
  // Used for fast lookup by panel ID.
  std::vector<std::string> panel_tab_ids_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_TOOLBAR_H_
