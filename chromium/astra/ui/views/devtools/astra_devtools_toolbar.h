#ifndef ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_TOOLBAR_H_
#define ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_TOOLBAR_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/view.h"

namespace content {
class WebContents;
}  // namespace content

namespace astra {

class AstraDevToolsModel;

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
// Features:
//   - Panel selection tabs (one tab per visible Astra panel)
//   - Back/forward navigation buttons
//   - Search box for filtering panel content
//   - Settings button (opens settings drawer)
//   - Detach button (undocks DevTools)
//   - Menu button (more actions)
//   - Theming support (light/dark)
//   - Keyboard shortcuts for panel switching
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
//
// For the overlay skeleton, this view is a standalone Views widget that
// can be shown/hidden programmatically. Real integration requires the
// DevToolsWindow patch listed above.
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
  };

  explicit AstraDevToolsToolbar(Delegate* delegate);
  ~AstraDevToolsToolbar() override;

  AstraDevToolsToolbar(const AstraDevToolsToolbar&) = delete;
  AstraDevToolsToolbar& operator=(const AstraDevToolsToolbar&) = delete;

  // Sets the model to read panel and setting state from.
  // The model must outlive this toolbar.
  void SetModel(AstraDevToolsModel* model);

  // Refreshes all UI from the model state.
  void UpdateFromModel();

  // Sets the inspected WebContents.  The toolbar reads Astra metadata
  // (focus mode state, workspace) from this tab's services.
  void SetInspectedWebContents(content::WebContents* web_contents);

  // Sets the theme for the toolbar.
  void SetTheme(bool dark_theme);

  // Returns the current search text.
  std::u16string search_text() const { return search_box_ ? search_box_->GetText() : std::u16string(); }

  // Accessors for testing.
  views::LabelButton* back_button_for_testing() { return back_button_; }
  views::LabelButton* forward_button_for_testing() { return forward_button_; }
  views::Textfield* search_box_for_testing() { return search_box_; }
  views::LabelButton* settings_button_for_testing() { return settings_button_; }
  views::LabelButton* detach_button_for_testing() { return detach_button_; }
  views::LabelButton* menu_button_for_testing() { return menu_button_; }
  views::LabelButton* focus_mode_button_for_testing() { return focus_mode_button_; }
  views::View* panel_tabs_container_for_testing() { return panel_tabs_container_; }
  size_t panel_tab_count_for_testing() const;

 private:
  // Creates the toolbar UI structure.
  void BuildToolbar();

  // Rebuilds panel tabs from the model.
  void RebuildPanelTabs();

  // Updates the active panel tab appearance.
  void UpdateActivePanelTab();

  // Button click handlers.
  void OnBackButtonPressed();
  void OnForwardButtonPressed();
  void OnSettingsButtonPressed();
  void OnDetachButtonPressed();
  void OnMenuButtonPressed();
  void OnFocusModeButtonPressed();
  void OnPanelTabPressed(const std::string& panel_id);

  // Search text changed handler.
  void OnSearchTextChanged();

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

  // Container for panel tab buttons.
  raw_ptr<views::View> panel_tabs_container_ = nullptr;

  // Navigation buttons.
  raw_ptr<views::LabelButton> back_button_ = nullptr;
  raw_ptr<views::LabelButton> forward_button_ = nullptr;

  // Search box.
  raw_ptr<views::Textfield> search_box_ = nullptr;

  // Action buttons.
  raw_ptr<views::LabelButton> focus_mode_button_ = nullptr;
  raw_ptr<views::LabelButton> settings_button_ = nullptr;
  raw_ptr<views::LabelButton> detach_button_ = nullptr;
  raw_ptr<views::LabelButton> menu_button_ = nullptr;

  // Panel tab buttons, mapped by panel ID.
  // Stored as raw_ptrs since they're owned by the views hierarchy.
  std::vector<raw_ptr<views::LabelButton>> panel_tabs_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DEVTOOLS_ASTRA_DEVTOOLS_TOOLBAR_H_
