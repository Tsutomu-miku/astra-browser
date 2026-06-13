// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TOOLBAR_ASTRA_TOOLBAR_VIEW_H_
#define ASTRA_UI_VIEWS_TOOLBAR_ASTRA_TOOLBAR_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class Label;
class Textfield;
}  // namespace views

namespace astra {

class AstraPageActionsView;
class AstraOmniboxDecorationView;

// Delegate for toolbar interactions.
class AstraToolbarDelegate {
 public:
  virtual ~AstraToolbarDelegate() = default;

  // Navigation commands.
  virtual void OnBack() = 0;
  virtual void OnForward() = 0;
  virtual void OnReload() = 0;
  virtual void OnHome() = 0;

  // Omnibox.
  virtual void OnOmniboxFocused() {}
  virtual void OnOmniboxChanged(const std::u16string& text) {}
  virtual void OnOmniboxCommitted(const std::u16string& text) {}

  // Sidebar toggle.
  virtual void OnSidebarToggle() {}

  // Menu / app menu.
  virtual void OnAppMenu() {}

  // Tab strip.
  virtual void OnNewTab() {}
  virtual void OnTabSwitch(int tab_index) {}

  // Returns whether back navigation is available.
  virtual bool CanGoBack() const { return false; }

  // Returns whether forward navigation is available.
  virtual bool CanGoForward() const { return false; }

  // Returns whether the page is loading.
  virtual bool IsLoading() const { return false; }

  // Returns the current URL.
  virtual std::u16string GetCurrentUrl() const { return std::u16string(); }

  // Returns the current tab's title.
  virtual std::u16string GetTabTitle() const { return std::u16string(); }
};

// The main browser toolbar view.
//
// Contains the standard browser toolbar controls:
//   - Back / Forward / Reload / Home buttons
//   - Omnibox / location bar
//   - Page actions (bookmark star, extensions, etc.)
//   - App menu button
//
// This is the top-level toolbar container. Individual sub-components
// (omnibox, page actions, etc.) are separate views.
//
// Chromium owner: ToolbarView
//   (chrome/browser/ui/views/toolbar/toolbar_view.h)
//
// TODO(astra): Integrate with Chromium's ToolbarView via a patch to
// chrome/browser/ui/views/toolbar/toolbar_view.h to replace or
// augment the standard toolbar with Astra's version.
// TODO(astra): Use Chromium's real OmniboxView instead of a simple
// textfield placeholder.
class AstraToolbarView : public views::View {
 public:
  METADATA_HEADER(AstraToolbarView);

  AstraToolbarView();
  explicit AstraToolbarView(AstraToolbarDelegate* delegate);
  AstraToolbarView(const AstraToolbarView&) = delete;
  AstraToolbarView& operator=(const AstraToolbarView&) = delete;
  ~AstraToolbarView() override;

  // -- Delegate -------------------------------------------------------------

  void SetDelegate(AstraToolbarDelegate* delegate) { delegate_ = delegate; }
  AstraToolbarDelegate* delegate() const { return delegate_; }

  // -- Buttons --------------------------------------------------------------

  views::ImageButton* back_button() { return back_button_; }
  views::ImageButton* forward_button() { return forward_button_; }
  views::ImageButton* reload_button() { return reload_button_; }
  views::ImageButton* home_button() { return home_button_; }
  views::ImageButton* app_menu_button() { return app_menu_button_; }

  // -- Omnibox --------------------------------------------------------------

  views::Textfield* omnibox() { return omnibox_; }

  void SetOmniboxText(const std::u16string& text);
  std::u16string GetOmniboxText() const;

  void SetOmniboxPlaceholder(const std::u16string& placeholder);

  // -- URL ------------------------------------------------------------------

  void SetUrl(const std::u16string& url);
  std::u16string GetUrl() const;

  // -- Security state -------------------------------------------------------

  void SetSecure(bool secure);
  bool IsSecure() const { return is_secure_; }

  // -- Loading state --------------------------------------------------------

  void SetLoading(bool loading);
  bool IsLoading() const { return is_loading_; }

  // -- Button states --------------------------------------------------------

  void UpdateNavigationButtonStates();

  // -- Toolbar height -------------------------------------------------------

  void SetToolbarHeight(int height);
  int GetToolbarHeight() const { return toolbar_height_; }

  // -- Page actions ---------------------------------------------------------

  // Get the page actions container (if available).
  // TODO(astra): Wire up real page actions view.
  views::View* page_actions_for_test() { return page_actions_container_; }

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  // Build the toolbar UI.
  void Build();

  // Button click handlers.
  void OnBackClicked();
  void OnForwardClicked();
  void OnReloadClicked();
  void OnHomeClicked();
  void OnAppMenuClicked();
  void OnOmniboxContentsChanged();

  // Update button visual states.
  void UpdateButtonVisuals();

  // Update the omnibox appearance based on security state.
  void UpdateOmniboxAppearance();

  raw_ptr<AstraToolbarDelegate> delegate_ = nullptr;

  // Child views.
  raw_ptr<views::ImageButton> back_button_ = nullptr;
  raw_ptr<views::ImageButton> forward_button_ = nullptr;
  raw_ptr<views::ImageButton> reload_button_ = nullptr;
  raw_ptr<views::ImageButton> home_button_ = nullptr;
  raw_ptr<views::View> omnibox_container_ = nullptr;
  raw_ptr<views::Textfield> omnibox_ = nullptr;
  raw_ptr<views::ImageView> security_icon_ = nullptr;
  raw_ptr<views::View> page_actions_container_ = nullptr;
  raw_ptr<views::ImageButton> app_menu_button_ = nullptr;

  // State.
  bool is_secure_ = true;
  bool is_loading_ = false;
  int toolbar_height_ = 48;

  // Constants.
  static constexpr int kButtonSize = 32;
  static constexpr int kButtonIconSize = 20;
  static constexpr int kButtonSpacing = 4;
  static constexpr int kSidePadding = 8;
  static constexpr int kOmniboxHeight = 36;
  static constexpr int kOmniboxCornerRadius = 8;
  static constexpr int kMinOmniboxWidth = 200;
  static constexpr int kAppMenuButtonSize = 32;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TOOLBAR_ASTRA_TOOLBAR_VIEW_H_
