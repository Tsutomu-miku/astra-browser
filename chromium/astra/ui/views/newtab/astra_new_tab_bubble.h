#ifndef ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_BUBBLE_H_
#define ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_BUBBLE_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "url/gurl.h"

#include "astra/ui/views/newtab/astra_new_tab_controller.h"
#include "astra/ui/views/newtab/astra_new_tab_view.h"

namespace views {
class ScrollView;
class Textfield;
}  // namespace views

class Browser;

namespace astra {

// =========================================================================
// AstraNewTabBubble — bubble/widget that shows the Astra new tab page
// =========================================================================
//
// A bubble-style widget that displays the Astra-branded new tab page.
// This provides a Views-based alternative to Chrome's WebUI NTP.
//
// The bubble owns the NTP controller and model, and bridges between:
//   - The view delegate (user actions from the NTP view)
//   - The controller delegate (actions that need browser-level handling)
//   - The outer delegate (integrating with the browser / sidebar)
//
// Architecture:
//   AstraNewTabBubble (owns controller)
//     └─ AstraNewTabController (owns model, mediates view/model)
//         ├─ AstraNewTabModel (state, persistence via PrefService)
//         └─ AstraNewTabView (presentation)
//
// The bubble can be shown:
//   - As an overlay / expansion of the sidebar
//   - As a standalone widget that replaces the NTP content area
//   - From a command / keyboard shortcut
//
// The bubble is modeless and closes on deactivation by default.
// It provides a search/omnibox-like input at the top for quick web searches
// and navigations, and a scrollable content area with NTP sections below.
//
// TODO(astra): Proper integration as the actual new tab page.
// Currently this is shown as a bubble overlay.  The proper integration
// would replace the WebContents of a new tab with this Views widget,
// or redirect chrome://newtab to a custom WebUI page.
//
// Chromium owner: NewTabPageUI
//   (chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h)
// Patch point: chrome/browser/new_tab_page/new_tab_page_url_handler.cc
//   — redirect chrome://newtab to a custom Astra page.
//
// Alternative integration path: The NTP could be shown as a full-width
// content view that replaces the web contents area when a new tab is
// created, rather than as a floating bubble.
//
// TODO(astra): Animated entrance/exit.
// Chromium pattern: views::BubbleDialogDelegateView with show/hide animations,
//   or custom layer animation (ui/compositor/layer_animation.h).
// =========================================================================

class AstraNewTabBubble : public views::BubbleDialogDelegateView,
                          public views::TextfieldController,
                          public AstraNewTabController::Delegate {
 public:
  // Size modes for the NTP bubble.
  enum class SizeMode {
    kStandard,   // Fixed standard size
    kLarge,      // Larger fixed size
    kFullWindow  // Fills the browser window content area
  };

  // Delegate interface for NTP actions that need browser-level handling.
  class Delegate {
   public:
    // Called when the NTP bubble is closing.
    virtual void OnNewTabBubbleClosed() = 0;

    // Called when the user submits the search/omnibox input.
    // |text| is the raw search query or URL.
    virtual void OnNewTabSearchSubmitted(const std::u16string& text) = 0;

    // Called when the user navigates to a URL from a shortcut or tile.
    virtual void OnNewTabNavigateToURL(const GURL& url) = 0;

    // Called to open / switch to a workspace.
    virtual void OnNewTabOpenWorkspace(const std::string& workspace_id) = 0;

    // Called to create a new workspace.
    virtual void OnNewTabNewWorkspace() = 0;

    // Called to show the full workspace overview.
    virtual void OnNewTabShowAllWorkspaces() = 0;

    // Called to trigger a quick action (e.g. screenshot, focus mode).
    // |action_id| identifies which quick action was triggered.
    virtual void OnNewTabQuickAction(const std::string& action_id) = 0;

    // Called to restore a recently closed tab.
    virtual void OnNewTabRestoreRecentlyClosed(int session_id) = 0;

    // Called to show the shortcut context menu.
    virtual void OnNewTabShortcutContextMenu(
        const GURL& url,
        const gfx::Point& screen_point) = 0;

    // Called to show the workspace context menu.
    virtual void OnNewTabWorkspaceContextMenu(
        const std::string& workspace_id,
        const gfx::Point& screen_point) = 0;

   protected:
    ~Delegate() = default;
  };

  // Creates and shows the new tab page bubble anchored to |anchor_view|.
  // Returns the bubble widget (owned by the native widget system).
  //
  // TODO(astra): Determine the correct anchor and presentation for the NTP.
  // Options:
  //   1. Full-width overlay replacing the content area.
  //   2. Sidebar expansion (like Arc's sidebar).
  //   3. Floating card centered in the browser window.
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   Browser* browser,
                                   Delegate* delegate,
                                   SizeMode size_mode = SizeMode::kStandard);

  ~AstraNewTabBubble() override;

  AstraNewTabBubble(const AstraNewTabBubble&) = delete;
  AstraNewTabBubble& operator=(const AstraNewTabBubble&) = delete;

  // -- views::BubbleDialogDelegateView ------------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetShown(views::Widget* widget) override;

  // -- views::TextfieldController ----------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // Accessor for the content view.
  AstraNewTabView* new_tab_view() { return new_tab_view_; }

  // Accessor for the search textfield (for focus management).
  views::Textfield* search_field() { return search_field_; }

  // Request focus on the search field.
  void RequestSearchFocus();

  // Accessor for the controller.
  AstraNewTabController* controller() { return controller_.get(); }

  // -- AstraNewTabController::Delegate ------------------------------------
  // Bridges controller actions to the bubble's outer delegate.

  void OnNavigateToURL(const GURL& url) override;
  void OnOpenWorkspace(const std::string& workspace_id) override;
  void OnNewWorkspace() override;
  void OnShowAllWorkspaces() override;
  void OnQuickAction(const std::string& action_id) override;
  void OnRestoreRecentlyClosed(int session_id) override;
  void OnShowShortcutContextMenu(const GURL& url,
                                 const gfx::Point& screen_point) override;
  void OnShowWorkspaceContextMenu(
      const std::string& workspace_id,
      const gfx::Point& screen_point) override;
  void OnSettingsGearPressed() override;

 private:
  AstraNewTabBubble(views::View* anchor_view,
                    Browser* browser,
                    Delegate* delegate,
                    SizeMode size_mode);

  // Build the bubble contents.
  void Init() override;

  // Build the child views and layout (search bar + scroll view + content).
  void BuildLayout();

  // Play entrance animation (stub).
  // TODO(astra): Implement fade-in + scale entrance animation.
  // Chromium pattern: Use ui::Layer with LayerAnimator for opacity + transform.
  // Reference: chrome/browser/ui/views/bubble/bubble_dialog_delegate_view.cc
  void PlayEntranceAnimation();

  raw_ptr<Browser> browser_;
  raw_ptr<Delegate> delegate_ = nullptr;
  SizeMode size_mode_;

  // NTP controller (owns the model, mediates between model and view).
  std::unique_ptr<AstraNewTabController> controller_;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<AstraNewTabView> new_tab_view_ = nullptr;

  base::WeakPtrFactory<AstraNewTabBubble> weak_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_NEWTAB_ASTRA_NEW_TAB_BUBBLE_H_
