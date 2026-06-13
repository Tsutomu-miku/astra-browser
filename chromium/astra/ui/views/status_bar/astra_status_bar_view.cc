// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/status_bar/astra_status_bar_view.h"

#include <algorithm>
#include <cmath>

#include "base/i18n/number_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_elider.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Security level colors.
SkColor GetSecurityColor(AstraSecurityLevel level) {
  switch (level) {
    case AstraSecurityLevel::kSecure:
      return SkColorSetRGB(0x2E, 0x7D, 0x32);
    case AstraSecurityLevel::kSecureWithWarning:
      return SkColorSetRGB(0xFF, 0xA0, 0x00);
    case AstraSecurityLevel::kDangerous:
      return SkColorSetRGB(0xD3, 0x2F, 0x2F);
    case AstraSecurityLevel::kInternalPage:
      return SkColorSetRGB(0x5C, 0x6B, 0xC0);
    case AstraSecurityLevel::kNone:
    default:
      return SK_ColorGRAY;
  }
}

// Draw a lock icon for security indicator.
void DrawSecurityIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      AstraSecurityLevel level) {
  if (bounds.IsEmpty())
    return;

  SkColor color = GetSecurityColor(level);
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = bounds.width() * 3 / 4;
  int h = bounds.height() * 3 / 4;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  // Lock body (rectangle).
  int body_w = w;
  int body_h = h * 3 / 5;
  int body_y = cy + h / 10;
  canvas->DrawRoundRect(
      gfx::Rect(cx - body_w / 2, body_y, body_w, body_h), 2, flags);

  // Lock shackle (arc).
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);

  int shackle_w = w * 2 / 3;
  int shackle_h = h * 2 / 5;
  int shackle_y = body_y - shackle_h / 2;

  SkPath shackle;
  shackle.moveTo(cx - shackle_w / 2, body_y);
  shackle.arcTo(cx - shackle_w / 2, shackle_y - shackle_h / 2,
                cx + shackle_w / 2, shackle_y + shackle_h / 2,
                180, 180);
  shackle.lineTo(cx + shackle_w / 2, body_y);
  canvas->DrawPath(shackle, flags);

  // If dangerous, draw an X over the lock.
  if (level == AstraSecurityLevel::kDangerous) {
    flags.setStrokeWidth(1.5f);
    flags.setColor(SK_ColorWHITE);
    canvas->DrawLine(gfx::Point(cx - w / 4, body_y + body_h / 4),
                     gfx::Point(cx + w / 4, body_y + body_h * 3 / 4),
                     flags);
    canvas->DrawLine(gfx::Point(cx + w / 4, body_y + body_h / 4),
                     gfx::Point(cx - w / 4, body_y + body_h * 3 / 4),
                     flags);
  }
}

// Draw a download icon.
void DrawDownloadIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color) {
  if (bounds.IsEmpty())
    return;

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 3 / 4;
  int half = size / 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  // Down arrow.
  canvas->DrawLine(gfx::Point(cx, cy - half), gfx::Point(cx, cy + half / 2),
                   flags);
  canvas->DrawLine(gfx::Point(cx - half / 2, cy), gfx::Point(cx, cy + half / 2),
                   flags);
  canvas->DrawLine(gfx::Point(cx + half / 2, cy), gfx::Point(cx, cy + half / 2),
                   flags);
  // Horizontal line at bottom.
  canvas->DrawLine(gfx::Point(cx - half / 2, cy + half / 2),
                   gfx::Point(cx + half / 2, cy + half / 2), flags);
}

}  // namespace

// =========================================================================
// AstraStatusBarView
// =========================================================================

AstraStatusBarView::AstraStatusBarView() {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  BuildLayout();
}

AstraStatusBarView::~AstraStatusBarView() = default;

// -- Status text ------------------------------------------------------------

void AstraStatusBarView::SetStatusText(const std::u16string& text) {
  status_text_ = text;
  has_status_text_ = !text.empty();
  UpdateStatusLabel();
}

void AstraStatusBarView::ClearStatusText() {
  has_status_text_ = false;
  UpdateStatusLabel();
}

void AstraStatusBarView::SetPageURL(const GURL& url) {
  page_url_ = url;
  if (!has_status_text_)
    UpdateStatusLabel();
}

// -- Security ---------------------------------------------------------------

void AstraStatusBarView::SetSecurityLevel(AstraSecurityLevel level) {
  security_level_ = level;
  UpdateSecurityIcon();
}

// -- Zoom -------------------------------------------------------------------

void AstraStatusBarView::SetZoomLevel(double zoom_level) {
  zoom_level_ = zoom_level;
  UpdateZoomLabel();
}

// -- Downloads --------------------------------------------------------------

void AstraStatusBarView::SetDownloadCount(int count) {
  download_count_ = std::max(0, count);
  UpdateDownloadsBadge();
}

// -- Visibility -------------------------------------------------------------

void AstraStatusBarView::SetVisible(bool visible) {
  views::View::SetVisible(visible);
}

// -- views::View ------------------------------------------------------------

gfx::Size AstraStatusBarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(0, kStatusBarHeight);
}

void AstraStatusBarView::Layout() {
  if (!security_container_ || !status_label_ || !right_container_)
    return;

  int x = kHorizontalPadding;
  int y = 0;
  int height = kStatusBarHeight;

  // Security section.
  int sec_width = kIconSize + kIconTextSpacing;
  security_container_->SetBounds(x, y, sec_width, height);
  security_icon_->SetBounds(0, (height - kIconSize) / 2, kIconSize, kIconSize);
  x += sec_width;

  // Right section (zoom + downloads).
  int right_width = 0;
  if (right_container_ && right_container_->GetVisible()) {
    right_width = right_container_->GetPreferredSize().width();
    int right_x = width() - kHorizontalPadding - right_width;
    right_container_->SetBounds(right_x, y, right_width, height);
  }

  // Status label fills remaining space.
  int status_x = x;
  int status_width = width() - x - kHorizontalPadding - right_width -
                     kRightSectionSpacing;
  status_label_->SetBounds(status_x, 0, status_width, height);
}

void AstraStatusBarView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

void AstraStatusBarView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kStatus;
  node_data->SetName(u"Status bar");
}

// -- Private methods --------------------------------------------------------

void AstraStatusBarView::BuildLayout() {
  // Security container.
  auto sec_container = std::make_unique<views::View>();
  sec_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kIconTextSpacing));
  security_container_ = AddChildView(std::move(sec_container));

  // Security icon.
  auto security_icon = std::make_unique<views::ImageView>();
  security_icon->SetImageSize(gfx::Size(kIconSize, kIconSize));
  security_icon_ = security_container_->AddChildView(std::move(security_icon));

  // Status label.
  auto status_label = std::make_unique<views::Label>();
  status_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label->SetAutoColorReadabilityEnabled(false);
  status_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  status_label_ = AddChildView(std::move(status_label));

  // Right container (zoom + downloads).
  auto right_container = std::make_unique<views::View>();
  right_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kRightSectionSpacing));
  right_container_ = AddChildView(std::move(right_container));

  // Zoom label.
  auto zoom_label = std::make_unique<views::Label>(u"100%");
  zoom_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  zoom_label->SetAutoColorReadabilityEnabled(false);
  zoom_label_ = right_container_->AddChildView(std::move(zoom_label));

  // Downloads container.
  auto downloads_container = std::make_unique<views::View>();
  downloads_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kIconTextSpacing));
  downloads_container_ = right_container_->AddChildView(
      std::move(downloads_container));

  // Downloads icon.
  auto downloads_icon = std::make_unique<views::ImageView>();
  downloads_icon->SetImageSize(gfx::Size(kIconSize, kIconSize));
  downloads_icon_ = downloads_container_->AddChildView(
      std::move(downloads_icon));

  // Downloads badge.
  auto downloads_badge = std::make_unique<views::Label>(u"0");
  downloads_badge->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  downloads_badge->SetAutoColorReadabilityEnabled(false);
  downloads_badge_ = downloads_container_->AddChildView(
      std::move(downloads_badge));

  UpdateSecurityIcon();
  UpdateZoomLabel();
  UpdateDownloadsBadge();
  UpdateColors();
}

void AstraStatusBarView::UpdateColors() {
  if (!GetWidget())
    return;

  SkColor text_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_LabelEnabledColor);
  SkColor secondary_color =
      GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_LabelSecondaryColor);
  SkColor bg_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_WindowBackground);

  SetBackground(views::CreateSolidBackground(bg_color));

  if (status_label_)
    status_label_->SetEnabledColor(secondary_color);
  if (zoom_label_)
    zoom_label_->SetEnabledColor(secondary_color);
  if (downloads_badge_)
    downloads_badge_->SetEnabledColor(text_color);

  // Redraw icons with text color.
  UpdateSecurityIcon();
  if (downloads_icon_) {
    gfx::Canvas canvas(gfx::Size(kIconSize, kIconSize), /*image_scale=*/1.0f,
                       false);
    DrawDownloadIcon(&canvas, gfx::Rect(0, 0, kIconSize, kIconSize),
                     text_color);
    downloads_icon_->SetImage(
        gfx::ImageSkia(canvas.GetBitmap(), gfx::Size(kIconSize, kIconSize)));
  }
}

void AstraStatusBarView::UpdateSecurityIcon() {
  if (!security_icon_)
    return;

  gfx::Canvas canvas(gfx::Size(kIconSize, kIconSize), /*image_scale=*/1.0f,
                     false);
  DrawSecurityIcon(&canvas, gfx::Rect(0, 0, kIconSize, kIconSize),
                   security_level_);
  security_icon_->SetImage(
      gfx::ImageSkia(canvas.GetBitmap(), gfx::Size(kIconSize, kIconSize)));
}

void AstraStatusBarView::UpdateZoomLabel() {
  if (!zoom_label_)
    return;

  int zoom_percent = static_cast<int>(std::round(zoom_level_ * 100.0));
  zoom_label_->SetText(base::NumberToString16(zoom_percent) + u"%");
}

void AstraStatusBarView::UpdateStatusLabel() {
  if (!status_label_)
    return;

  if (has_status_text_) {
    status_label_->SetText(status_text_);
  } else if (!page_url_.is_empty()) {
    status_label_->SetText(base::UTF8ToUTF16(page_url_.spec()));
  } else {
    status_label_->SetText(std::u16string());
  }
}

void AstraStatusBarView::UpdateDownloadsBadge() {
  if (!downloads_badge_ || !downloads_container_)
    return;

  bool show_downloads = download_count_ > 0;
  downloads_container_->SetVisible(show_downloads);

  if (show_downloads) {
    downloads_badge_->SetText(base::NumberToString16(download_count_));
  }

  Layout();
}

// -- Button handlers --------------------------------------------------------

void AstraStatusBarView::OnSecurityClicked() {
  if (delegate_)
    delegate_->OnSecurityClicked();
}

void AstraStatusBarView::OnZoomClicked() {
  if (delegate_)
    delegate_->OnZoomClicked();
}

void AstraStatusBarView::OnDownloadsClicked() {
  if (delegate_)
    delegate_->OnDownloadsClicked();
}

}  // namespace astra
