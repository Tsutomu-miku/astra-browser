// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_STATUS_BAR_ASTRA_STATUS_BAR_VIEW_H_
#define ASTRA_UI_VIEWS_STATUS_BAR_ASTRA_STATUS_BAR_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

namespace astra {

// Security level for the current page.
enum class AstraSecurityLevel {
  kNone,
  kSecure,
  kSecureWithWarning,
  kDangerous,
  kInternalPage,
};

// =========================================================================
// AstraStatusBarView — status bar at the bottom of the browser
// =========================================================================
//
// A compact status bar that appears at the bottom of the browser window.
// Shows URL hover status, security information, zoom level, and other
// page status indicators.
//
// Layout (left to right):
//   1. Security indicator (lock icon + label)
//   2. URL / status text (shows hovered link URL or page status)
//   3. Zoom level indicator (right side)
//   4. Downloads count badge (right side)
//
// Chromium pattern: StatusBubble / StatusBubbleViews
//   (chrome/browser/ui/views/status_bubble_views.h)
//
// TODO(astra): Integrate with BrowserView via a patch.
//   Chromium owner: StatusBubble (chrome/browser/ui/status_bubble.h)
//   Patch point: BrowserView::InitViews() — replace or augment status bubble
// =========================================================================

class AstraStatusBarView : public views::View {
 public:
  // Delegate for status bar actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the zoom indicator is clicked.
    virtual void OnZoomClicked() = 0;

    // Called when the security indicator is clicked.
    virtual void OnSecurityClicked() = 0;

    // Called when the downloads indicator is clicked.
    virtual void OnDownloadsClicked() = 0;
  };

  AstraStatusBarView();
  ~AstraStatusBarView() override;

  AstraStatusBarView(const AstraStatusBarView&) = delete;
  AstraStatusBarView& operator=(const AstraStatusBarView&) = delete;

  // -- Status text ----------------------------------------------------------

  // Set the status text (e.g. hovered link URL).
  void SetStatusText(const std::u16string& text);

  // Clear the status text (returns to showing current page URL).
  void ClearStatusText();

  // Set the current page URL (shown when no status text is set).
  void SetPageURL(const GURL& url);

  // -- Security -------------------------------------------------------------

  void SetSecurityLevel(AstraSecurityLevel level);
  AstraSecurityLevel security_level() const { return security_level_; }

  // -- Zoom -----------------------------------------------------------------

  void SetZoomLevel(double zoom_level);
  double zoom_level() const { return zoom_level_; }

  // -- Downloads ------------------------------------------------------------

  void SetDownloadCount(int count);
  int download_count() const { return download_count_; }

  // -- Visibility -----------------------------------------------------------

  void SetVisible(bool visible);

  // -- Delegate -------------------------------------------------------------

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // -- views::View ----------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build child views.
  void BuildLayout();

  // Update colors from the color provider.
  void UpdateColors();

  // Update security icon based on security level.
  void UpdateSecurityIcon();

  // Update zoom label.
  void UpdateZoomLabel();

  // Update status label text.
  void UpdateStatusLabel();

  // Update downloads badge visibility and text.
  void UpdateDownloadsBadge();

  // Button handlers.
  void OnSecurityClicked();
  void OnZoomClicked();
  void OnDownloadsClicked();

  // Delegate.
  raw_ptr<Delegate> delegate_ = nullptr;

  // State.
  std::u16string status_text_;
  GURL page_url_;
  AstraSecurityLevel security_level_ = AstraSecurityLevel::kNone;
  double zoom_level_ = 1.0;
  int download_count_ = 0;
  bool has_status_text_ = false;

  // Child views.
  raw_ptr<views::View> security_container_ = nullptr;
  raw_ptr<views::ImageView> security_icon_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;
  raw_ptr<views::View> right_container_ = nullptr;
  raw_ptr<views::Label> zoom_label_ = nullptr;
  raw_ptr<views::View> downloads_container_ = nullptr;
  raw_ptr<views::ImageView> downloads_icon_ = nullptr;
  raw_ptr<views::Label> downloads_badge_ = nullptr;

  // -- Constants ------------------------------------------------------------

  static constexpr int kStatusBarHeight = 24;
  static constexpr int kHorizontalPadding = 8;
  static constexpr int kIconSize = 16;
  static constexpr int kIconTextSpacing = 6;
  static constexpr int kRightSectionSpacing = 12;
  static constexpr int kBadgeSize = 16;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_STATUS_BAR_ASTRA_STATUS_BAR_VIEW_H_
