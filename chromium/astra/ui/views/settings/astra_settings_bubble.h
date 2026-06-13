#ifndef ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_BUBBLE_H_
#define ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_BUBBLE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list_types.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

#include "astra/ui/views/settings/astra_settings_model.h"

class Browser;

namespace views {
class ScrollView;
}  // namespace views

namespace astra {

class AstraSettingsPageView;
class AstraSettingsSearchBox;
class AstraSettingsModel;

// =========================================================================
// Astra settings bubble — floating dialog wrapper
// =========================================================================
//
// AstraSettingsBubble wraps the Astra settings page content view in a
// Chromium bubble dialog.  It is shown from the sidebar, command palette,
// or toolbar settings button.
//
// The bubble is:
//   - Modeless (doesn't block the rest of the UI).
//   - Anchorable to a toolbar button or sidebar settings button.
//   - Auto-dismissed when focus is lost (click outside, switch windows).
//   - Styled with rounded corners and shadow via BubbleDialogDelegateView.
//   - Has a search box at the top for filtering settings.
//   - Has a scrollable content area for all settings sections.
//   - Supports navigation between sections, search results, and subpages.
//
// Settings are auto-applied — toggles and sliders write directly to
// PrefService when changed.  There is no "Apply" or "OK" button; the
// bubble only has a close button and is dismissed by clicking outside.
//
// Navigation model:
//   - Main page: shows all sections as collapsed overview
//   - Section detail: shows one section expanded with all its settings
//   - Search results: shows all sections matching a search query
//   - Subpages: named subpages (e.g. search engine settings)
//
// Chromium subsystem reused: views::BubbleDialogDelegateView
// (ui/views/bubble/bubble_dialog_delegate_view.h).
//
// Chromium pattern reference:
//   chrome/browser/ui/views/bookmarks/bookmark_bubble_view.h
//   chrome/browser/ui/views/download/download_bubble/bubble_contents_view.h
//   chrome/browser/ui/views/profiles/profile_menu_view.h
//
// TODO(astra): Consider integrating into Chrome's main settings page
// (chrome://settings) instead of using a standalone bubble.  The bubble
// approach is used because Astra is an overlay that cannot directly
// modify Chrome WebUI resources.
// Patch point: chrome/browser/resources/settings/ for WebUI integration,
// or chrome/browser/ui/views/settings/ for native Views settings.
// =========================================================================

class AstraSettingsBubble : public views::BubbleDialogDelegateView,
                            public AstraSettingsObserver {
 public:
  // Delegate interface for bubble events.
  //
  // All methods have default empty implementations so that subclasses can
  // override only the methods they care about.
  //
  // Implemented by the owner (e.g. AstraBrowserView) to be notified of
  // settings bubble lifecycle and navigation events.
  class Delegate {
   public:
    // Called when the settings bubble is opened/shown.
    virtual void OnSettingsBubbleOpened() {}

    // Called when the settings bubble is closed.
    virtual void OnSettingsBubbleClosed() {}

    // Called when a setting value changes.
    virtual void OnSettingChanged(const std::string& key) {}

    // Called when the search query changes.
    virtual void OnSettingsSearchQueryChanged(const std::u16string& query) {}

    // Called when the navigation stack changes (section entered, back, etc.).
    virtual void OnSettingsNavigationChanged() {}

   protected:
    virtual ~Delegate() = default;
  };

  // -- Static show/hide API -----------------------------------------------

  // Creates and shows the settings bubble anchored to |anchor_view|.
  // Returns the bubble widget (owned by the native widget system).
  // The caller can listen for closure via the Delegate.
  //
  // If the bubble is already open, the existing widget is activated
  // instead of creating a new one.
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   Browser* browser,
                                   Delegate* delegate);

  // Hides (closes) the settings bubble if it is currently shown.
  static void HideBubble();

  // Returns true if the settings bubble is currently visible.
  static bool IsBubbleVisible();

  // Returns the current bubble widget, or null if not shown.
  static views::Widget* GetBubbleWidget();

  // Returns the current bubble instance, or null if not shown.
  static AstraSettingsBubble* GetBubble();

  ~AstraSettingsBubble() override;

  AstraSettingsBubble(const AstraSettingsBubble&) = delete;
  AstraSettingsBubble& operator=(const AstraSettingsBubble&) = delete;

  // -- Navigation ---------------------------------------------------------

  // Show the main page with all sections in overview.
  void ShowMainPage();

  // Show a specific section's settings (section detail page).
  void ShowSection(const std::string& section_id);

  // Show search results for the given query.
  void ShowSearchResults(const std::u16string& query);

  // Navigate back in the navigation stack.
  void NavigateBack();

  // Returns true if we can navigate back (navigation stack > 1 entry).
  bool CanNavigateBack() const;

  // -- Size control -------------------------------------------------------

  // Set the preferred width of the bubble.
  void SetBubbleWidth(int width);

  // Get the current bubble width.
  int GetBubbleWidth() const;

  // Set the maximum height of the bubble.
  void SetBubbleMaxHeight(int height);

  // Get the current maximum bubble height.
  int GetBubbleMaxHeight() const;

  // -- views::BubbleDialogDelegateView ------------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  gfx::Size CalculatePreferredSize() const override;

  // -- AstraSettingsObserver ----------------------------------------------

  void OnSettingChanged(AstraSettingsModel* model,
                        const std::string& key) override;
  void OnSettingsReset(AstraSettingsModel* model) override;
  void OnSettingsSearchResultsChanged(AstraSettingsModel* model) override;
  void OnSettingsModelShutdown(AstraSettingsModel* model) override;

  // -- Accessors ----------------------------------------------------------

  AstraSettingsPageView* settings_page_view() { return settings_page_view_; }
  AstraSettingsSearchBox* search_box() { return search_box_; }
  AstraSettingsModel* settings_model() { return settings_model_.get(); }

  // Returns the delegate, or null if none.
  Delegate* delegate() { return delegate_; }

 private:
  AstraSettingsBubble(views::View* anchor_view,
                      Browser* browser,
                      Delegate* delegate);

  // Build the bubble contents.
  void Init() override;

  // Build the header area with search box and breadcrumbs.
  void BuildHeader();

  // Build the scrollable content area with the settings page.
  void BuildContent();

  // Handler for search text changes.
  void OnSearchTextChanged(const std::u16string& query);

  // Singleton tracking — the current bubble instance (owned by widget).
  // Only one settings bubble can be shown at a time.
  static AstraSettingsBubble* instance_;
  static views::Widget* instance_widget_;

  raw_ptr<Browser> browser_;
  raw_ptr<Delegate> delegate_ = nullptr;

  // Settings model (owned by the bubble, shared with views).
  std::unique_ptr<AstraSettingsModel> settings_model_;

  // Search box at the top of the bubble (owned by the view hierarchy).
  raw_ptr<AstraSettingsSearchBox> search_box_ = nullptr;

  // Scroll view wrapping the settings page (owned by the view hierarchy).
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;

  // The settings page content view (owned by the view hierarchy via
  // AddChildView in Init()).
  raw_ptr<AstraSettingsPageView> settings_page_view_ = nullptr;

  // Bubble sizing.
  int bubble_width_ = 440;
  int bubble_max_height_ = 640;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_BUBBLE_H_
