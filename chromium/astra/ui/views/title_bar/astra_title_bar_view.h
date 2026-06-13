// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TITLE_BAR_ASTRA_TITLE_BAR_VIEW_H_
#define ASTRA_UI_VIEWS_TITLE_BAR_ASTRA_TITLE_BAR_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

namespace astra {

// Window control button types.
enum class AstraWindowControlType {
  kMinimize,
  kMaximize,
  kRestore,  // Shown when window is maximized
  kClose,
};

// Delegate for AstraTitleBarView.
class AstraTitleBarDelegate {
 public:
  virtual ~AstraTitleBarDelegate() = default;

  // Called when a window control is clicked.
  virtual void OnWindowControlClicked(AstraWindowControlType type) = 0;

  // Called when the title bar is double-clicked (toggle maximize).
  virtual void OnTitleBarDoubleClicked() {}

  // Called when the app icon is clicked.
  virtual void OnAppIconClicked() {}

  // Called when the workspace indicator is clicked.
  virtual void OnWorkspaceClicked() {}

  // Returns whether the window is currently maximized.
  virtual bool IsWindowMaximized() const { return false; }

  // Returns the current workspace name.
  virtual std::u16string GetWorkspaceName() const { return std::u16string(); }

  // Returns the current workspace accent color.
  virtual SkColor GetWorkspaceColor() const { return SK_ColorBLUE; }

  // Returns the page title for the active tab.
  virtual std::u16string GetActiveTabTitle() const { return u"Astra Browser"; }
};

// The Astra title bar view.
//
// Replaces or augments Chromium's native title bar with an Astra-styled
// title bar. Contains:
//   - App icon (left)
//   - Workspace indicator (left, after icon)
//   - Window title / page title (center-left)
//   - Window controls (right) — minimize, maximize/restore, close
//
// On macOS, traffic light buttons are on the left and this view
// provides spacing for them.
//
// Chromium owner: BrowserTitlebar / BrowserNonClientFrameView
//   (chrome/browser/ui/views/frame/browser_titlebar.h)
//   (chrome/browser/ui/views/frame/browser_non_client_frame_view.h)
//
// TODO(astra): Integrate with BrowserNonClientFrameView via a patch to
// chrome/browser/ui/views/frame/browser_non_client_frame_view.h to
// replace or decorate the standard title bar with Astra's version.
// TODO(astra): Handle platform differences (Mac traffic lights vs
// Windows/Linux window controls on right side).
class AstraTitleBarView : public views::View {
 public:
  METADATA_HEADER(AstraTitleBarView);

  AstraTitleBarView();
  explicit AstraTitleBarView(AstraTitleBarDelegate* delegate);
  AstraTitleBarView(const AstraTitleBarView&) = delete;
  AstraTitleBarView& operator=(const AstraTitleBarView&) = delete;
  ~AstraTitleBarView() override;

  // -- Delegate ------------------------------------------------------------

  void SetDelegate(AstraTitleBarDelegate* delegate) { delegate_ = delegate; }
  AstraTitleBarDelegate* delegate() const { return delegate_; }

  // -- Title ---------------------------------------------------------------

  void SetTitle(const std::u16string& title);
  const std::u16string& GetTitle() const;

  // -- App icon ------------------------------------------------------------

  void SetAppIcon(const gfx::ImageSkia& icon);
  void SetAppIconVisible(bool visible);
  bool IsAppIconVisible() const;

  // -- Workspace indicator -------------------------------------------------

  void SetWorkspaceName(const std::u16string& name);
  void SetWorkspaceColor(SkColor color);
  void SetWorkspaceVisible(bool visible);
  bool IsWorkspaceVisible() const;

  // -- Window controls -----------------------------------------------------

  void SetWindowControlsVisible(bool visible);
  bool AreWindowControlsVisible() const;

  // Update maximize/restore button state.
  void UpdateMaximizeButton(bool maximized);

  // Get individual control buttons (for testing).
  views::ImageButton* minimize_button() { return minimize_button_; }
  views::ImageButton* maximize_button() { return maximize_button_; }
  views::ImageButton* close_button() { return close_button_; }

  // -- Height --------------------------------------------------------------

  void SetTitleBarHeight(int height);
  int GetTitleBarHeight() const { return title_bar_height_; }

  // -- Background ----------------------------------------------------------

  void SetBackgroundColor(SkColor color);
  SkColor GetBackgroundColor() const { return background_color_; }

  // -- Theme ---------------------------------------------------------------

  void SetDarkMode(bool dark_mode);
  bool IsDarkMode() const { return dark_mode_; }

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDoubleClick(const ui::MouseEvent& event) override;

 private:
  // Build the child views.
  void Build();

  // Handle window control clicks.
  void OnMinimizeClicked();
  void OnMaximizeClicked();
  void OnCloseClicked();

  // Handle app icon click.
  void OnAppIconClicked();

  // Handle workspace indicator click.
  void OnWorkspaceClicked();

  // Update control button icons.
  void UpdateControlIcons();

  // Update the title from delegate.
  void UpdateTitleFromDelegate();

  raw_ptr<AstraTitleBarDelegate> delegate_ = nullptr;

  std::u16string title_;
  SkColor background_color_ = SK_ColorWHITE;
  SkColor workspace_color_ = SK_ColorBLUE;
  int title_bar_height_ = 32;
  bool dark_mode_ = false;
  bool app_icon_visible_ = false;
  bool workspace_visible_ = true;
  bool window_controls_visible_ = true;

  // Child views.
  raw_ptr<views::ImageView> app_icon_ = nullptr;
  raw_ptr<views::View> workspace_indicator_ = nullptr;
  raw_ptr<views::Label> workspace_label_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::ImageButton> minimize_button_ = nullptr;
  raw_ptr<views::ImageButton> maximize_button_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;

  // Constants.
  static constexpr int kAppIconSize = 16;
  static constexpr int kAppIconPadding = 8;
  static constexpr int kWorkspaceDotSize = 8;
  static constexpr int kWorkspacePadding = 8;
  static constexpr int kControlButtonSize = 36;  // Click target
  static constexpr int kControlIconSize = 12;
  static constexpr int kControlButtonSpacing = 0;
  static constexpr int kTitleLeftPadding = 12;
  static constexpr int kTitleRightPadding = 12;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TITLE_BAR_ASTRA_TITLE_BAR_VIEW_H_
