// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/toolbar/astra_toolbar_view.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Helper to draw a back arrow icon.
void DrawBackArrow(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 0.45;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(cx + size, cy - size * 0.7f);
  path.lineTo(cx - size * 0.3f, cy);
  path.lineTo(cx + size, cy + size * 0.7f);

  canvas->DrawPath(path, flags);
}

// Helper to draw a forward arrow icon.
void DrawForwardArrow(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 0.45;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(cx - size, cy - size * 0.7f);
  path.lineTo(cx + size * 0.3f, cy);
  path.lineTo(cx - size, cy + size * 0.7f);

  canvas->DrawPath(path, flags);
}

// Helper to draw a reload icon.
void DrawReloadIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 0.4;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Draw a partial circle.
  SkPath path;
  SkRect rect = SkRect::MakeXYWH(cx - size, cy - size, size * 2, size * 2);
  path.addArc(rect, -45, 270);
  canvas->DrawPath(path, flags);

  // Draw arrowhead.
  SkPath arrow;
  arrow.moveTo(cx + size * 0.3f, cy - size - size * 0.1f);
  arrow.lineTo(cx + size * 0.7f, cy - size * 0.3f);
  arrow.lineTo(cx + size * 0.1f, cy - size * 0.3f);
  canvas->DrawPath(arrow, flags);
}

// Helper to draw a home icon.
void DrawHomeIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 0.4;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // House roof.
  SkPath path;
  path.moveTo(cx - size, cy);
  path.lineTo(cx, cy - size);
  path.lineTo(cx + size, cy);
  canvas->DrawPath(path, flags);

  // House body.
  canvas->DrawRect(
      gfx::Rect(cx - size * 0.7f, cy, size * 1.4f, size), flags);
}

// Helper to draw a menu (three dots) icon.
void DrawMenuIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float dot_radius = 2.5f;
  float spacing = 7;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  // Three dots vertically.
  canvas->DrawCircle(gfx::Point(cx, cy - spacing), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx, cy + spacing), dot_radius, flags);
}

}  // namespace

// ===========================================================================
// AstraToolbarView
// ===========================================================================

AstraToolbarView::AstraToolbarView() {
  Build();
}

AstraToolbarView::AstraToolbarView(AstraToolbarDelegate* delegate)
    : delegate_(delegate) {
  Build();
}

AstraToolbarView::~AstraToolbarView() = default;

void AstraToolbarView::SetOmniboxText(const std::u16string& text) {
  if (omnibox_) {
    omnibox_->SetText(text);
  }
}

std::u16string AstraToolbarView::GetOmniboxText() const {
  return omnibox_ ? omnibox_->GetText() : std::u16string();
}

void AstraToolbarView::SetOmniboxPlaceholder(const std::u16string& placeholder) {
  if (omnibox_) {
    omnibox_->SetPlaceholderText(placeholder);
  }
}

void AstraToolbarView::SetUrl(const std::u16string& url) {
  SetOmniboxText(url);
}

std::u16string AstraToolbarView::GetUrl() const {
  return GetOmniboxText();
}

void AstraToolbarView::SetSecure(bool secure) {
  if (is_secure_ == secure) {
    return;
  }
  is_secure_ = secure;
  UpdateOmniboxAppearance();
}

void AstraToolbarView::SetLoading(bool loading) {
  if (is_loading_ == loading) {
    return;
  }
  is_loading_ = loading;
  if (reload_button_) {
    reload_button_->SetTooltipText(loading ? u"Stop" : u"Reload");
    reload_button_->SetAccessibleName(loading ? u"Stop loading" : u"Reload page");
    reload_button_->SchedulePaint();
  }
}

void AstraToolbarView::UpdateNavigationButtonStates() {
  if (!delegate_) {
    return;
  }

  if (back_button_) {
    back_button_->SetEnabled(delegate_->CanGoBack());
  }
  if (forward_button_) {
    forward_button_->SetEnabled(delegate_->CanGoForward());
  }
}

void AstraToolbarView::SetToolbarHeight(int height) {
  if (toolbar_height_ == height) {
    return;
  }
  toolbar_height_ = height;
  InvalidateLayout();
}

gfx::Size AstraToolbarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(0, toolbar_height_);
}

void AstraToolbarView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int y = bounds.y();
  int height = bounds.height();
  int center_y = y + height / 2;

  int current_x = bounds.x() + kSidePadding;

  // Back button.
  if (back_button_ && back_button_->GetVisible()) {
    back_button_->SetBounds(current_x,
                            y + (height - kButtonSize) / 2,
                            kButtonSize, kButtonSize);
    current_x += kButtonSize + kButtonSpacing;
  }

  // Forward button.
  if (forward_button_ && forward_button_->GetVisible()) {
    forward_button_->SetBounds(current_x,
                               y + (height - kButtonSize) / 2,
                               kButtonSize, kButtonSize);
    current_x += kButtonSize + kButtonSpacing;
  }

  // Reload button.
  if (reload_button_ && reload_button_->GetVisible()) {
    reload_button_->SetBounds(current_x,
                              y + (height - kButtonSize) / 2,
                              kButtonSize, kButtonSize);
    current_x += kButtonSize + kButtonSpacing;
  }

  // Home button.
  if (home_button_ && home_button_->GetVisible()) {
    home_button_->SetBounds(current_x,
                            y + (height - kButtonSize) / 2,
                            kButtonSize, kButtonSize);
    current_x += kButtonSize + kButtonSpacing;
  }

  // Right side: app menu button.
  int right_x = bounds.right() - kSidePadding - kAppMenuButtonSize;
  if (app_menu_button_ && app_menu_button_->GetVisible()) {
    app_menu_button_->SetBounds(right_x,
                                y + (height - kAppMenuButtonSize) / 2,
                                kAppMenuButtonSize, kAppMenuButtonSize);
    right_x -= kButtonSpacing;
  }

  // Page actions container (before app menu).
  int page_actions_width = 0;
  if (page_actions_container_ && page_actions_container_->GetVisible()) {
    page_actions_width = page_actions_container_->GetPreferredSize().width();
    right_x -= page_actions_width + kButtonSpacing;
    page_actions_container_->SetBounds(right_x, y, page_actions_width, height);
  }

  // Omnibox in the middle.
  int omnibox_x = current_x + kButtonSpacing;
  int omnibox_width = right_x - omnibox_x - kButtonSpacing;
  if (omnibox_width < kMinOmniboxWidth) {
    omnibox_width = kMinOmniboxWidth;
  }

  if (omnibox_container_) {
    omnibox_container_->SetBounds(omnibox_x,
                                  y + (height - kOmniboxHeight) / 2,
                                  omnibox_width, kOmniboxHeight);
  }
}

void AstraToolbarView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateButtonVisuals();
  UpdateOmniboxAppearance();
}

void AstraToolbarView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  // Background.
  cc::PaintFlags flags;
  flags.setColor(SK_ColorWHITE);
  canvas->DrawRect(bounds, flags);

  // Bottom border.
  flags.setColor(SkColorSetA(SK_ColorBLACK, 0x0D));
  canvas->DrawRect(gfx::Rect(bounds.x(), bounds.bottom() - 1,
                             bounds.width(), 1), flags);

  // Draw custom icons on top of buttons (since we use ImageButton but
  // draw icons programmatically for this scaffold).
  SkColor icon_color = SK_ColorBLACK;
  // TODO(astra): Use theme colors properly.

  // Draw back button icon.
  if (back_button_ && back_button_->GetVisible()) {
    DrawBackArrow(canvas, back_button_->GetContentsBounds(),
                  back_button_->GetEnabled() ? icon_color
                                            : SkColorSetA(icon_color, 0x4D));
  }

  // Draw forward button icon.
  if (forward_button_ && forward_button_->GetVisible()) {
    DrawForwardArrow(canvas, forward_button_->GetContentsBounds(),
                     forward_button_->GetEnabled() ? icon_color
                                                   : SkColorSetA(icon_color, 0x4D));
  }

  // Draw reload button icon.
  if (reload_button_ && reload_button_->GetVisible()) {
    DrawReloadIcon(canvas, reload_button_->GetContentsBounds(), icon_color);
  }

  // Draw home button icon.
  if (home_button_ && home_button_->GetVisible()) {
    DrawHomeIcon(canvas, home_button_->GetContentsBounds(), icon_color);
  }

  // Draw app menu button icon.
  if (app_menu_button_ && app_menu_button_->GetVisible()) {
    DrawMenuIcon(canvas, app_menu_button_->GetContentsBounds(), icon_color);
  }
}

void AstraToolbarView::Build() {
  // Back button.
  auto back_button = std::make_unique<views::ImageButton>();
  back_button->SetTooltipText(u"Back");
  back_button->SetAccessibleName(u"Back");
  back_button->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  back_button->SetCallback(base::BindRepeating(
      &AstraToolbarView::OnBackClicked, base::Unretained(this)));
  back_button_ = AddChildView(std::move(back_button));

  // Forward button.
  auto forward_button = std::make_unique<views::ImageButton>();
  forward_button->SetTooltipText(u"Forward");
  forward_button->SetAccessibleName(u"Forward");
  forward_button->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  forward_button->SetCallback(base::BindRepeating(
      &AstraToolbarView::OnForwardClicked, base::Unretained(this)));
  forward_button_ = AddChildView(std::move(forward_button));

  // Reload button.
  auto reload_button = std::make_unique<views::ImageButton>();
  reload_button->SetTooltipText(u"Reload");
  reload_button->SetAccessibleName(u"Reload page");
  reload_button->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  reload_button->SetCallback(base::BindRepeating(
      &AstraToolbarView::OnReloadClicked, base::Unretained(this)));
  reload_button_ = AddChildView(std::move(reload_button));

  // Home button.
  auto home_button = std::make_unique<views::ImageButton>();
  home_button->SetTooltipText(u"Home");
  home_button->SetAccessibleName(u"Go to home page");
  home_button->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  home_button->SetCallback(base::BindRepeating(
      &AstraToolbarView::OnHomeClicked, base::Unretained(this)));
  home_button_ = AddChildView(std::move(home_button));

  // Omnibox container.
  auto omnibox_container = std::make_unique<views::View>();
  omnibox_container->SetBorder(views::CreateRoundedRectBorder(
      1, kOmniboxCornerRadius, SkColorSetA(SK_ColorBLACK, 0x1A)));
  omnibox_container_ = AddChildView(std::move(omnibox_container));

  // Omnibox text field.
  auto omnibox = std::make_unique<views::Textfield>();
  omnibox->SetPlaceholderText(u"Search Google or type a URL");
  omnibox->SetAccessibleName(u"Address bar");
  omnibox_ = omnibox_container->AddChildView(std::move(omnibox));

  // Security icon inside omnibox.
  auto security_icon = std::make_unique<views::ImageView>();
  security_icon->SetImageSize(gfx::Size(16, 16));
  security_icon->SetTooltipText(u"Secure");
  security_icon_ = omnibox_container->AddChildView(std::move(security_icon));

  // Page actions container.
  auto page_actions_container = std::make_unique<views::View>();
  page_actions_container->SetVisible(true);
  page_actions_container_ = AddChildView(std::move(page_actions_container));

  // App menu button.
  auto app_menu_button = std::make_unique<views::ImageButton>();
  app_menu_button->SetTooltipText(u"Customize and control Astra");
  app_menu_button->SetAccessibleName(u"App menu");
  app_menu_button->SetMinSize(gfx::Size(kAppMenuButtonSize, kAppMenuButtonSize));
  app_menu_button->SetCallback(base::BindRepeating(
      &AstraToolbarView::OnAppMenuClicked, base::Unretained(this)));
  app_menu_button_ = AddChildView(std::move(app_menu_button));

  UpdateNavigationButtonStates();
  UpdateOmniboxAppearance();
}

void AstraToolbarView::OnBackClicked() {
  if (delegate_) {
    delegate_->OnBack();
  }
}

void AstraToolbarView::OnForwardClicked() {
  if (delegate_) {
    delegate_->OnForward();
  }
}

void AstraToolbarView::OnReloadClicked() {
  if (delegate_) {
    if (is_loading_) {
      // TODO(astra): Stop loading.
    } else {
      delegate_->OnReload();
    }
  }
}

void AstraToolbarView::OnHomeClicked() {
  if (delegate_) {
    delegate_->OnHome();
  }
}

void AstraToolbarView::OnAppMenuClicked() {
  if (delegate_) {
    delegate_->OnAppMenu();
  }
}

void AstraToolbarView::OnOmniboxContentsChanged() {
  if (delegate_ && omnibox_) {
    delegate_->OnOmniboxChanged(omnibox_->GetText());
  }
}

void AstraToolbarView::UpdateButtonVisuals() {
  // Update button states from theme.
  // In a real implementation, we'd use vector icons with color IDs.
  SchedulePaint();
}

void AstraToolbarView::UpdateOmniboxAppearance() {
  if (!omnibox_container_) {
    return;
  }

  // Update border color based on security state.
  SkColor border_color = is_secure_
                              ? SkColorSetA(SK_ColorBLACK, 0x1A)
                              : SkColorSetRGB(0xEA, 0x43, 0x35);  // Red for insecure

  omnibox_container_->SetBorder(views::CreateRoundedRectBorder(
      1, kOmniboxCornerRadius, border_color));

  SchedulePaint();
}

}  // namespace astra
