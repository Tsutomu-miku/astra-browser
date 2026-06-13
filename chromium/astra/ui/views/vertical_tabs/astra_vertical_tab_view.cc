// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/vertical_tabs/astra_vertical_tab_view.h"

#include <memory>
#include <string>

#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kTabHeight = 36;
constexpr int kFaviconSize = 16;
constexpr int kCloseButtonSize = 16;
constexpr int kTabPaddingH = 8;
constexpr int kTabSpacing = 2;

// Draw close (x) icon.
void DrawCloseIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int size = std::min(bounds.width(), bounds.height()) / 2 - 1;

  SkPath path;
  path.moveTo(cx - size, cy - size);
  path.lineTo(cx + size, cy + size);
  path.moveTo(cx + size, cy - size);
  path.lineTo(cx - size, cy + size);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.2f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw a speaker/audio icon.
void DrawAudioIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int left = bounds.x() + 2;
  int cy = bounds.CenterPoint().y();
  int w = bounds.width() - 4;

  SkPath path;
  // Speaker body.
  path.moveTo(left, cy - 3);
  path.lineTo(left + 3, cy - 3);
  path.lineTo(left + 5, cy - 5);
  path.lineTo(left + 5, cy + 5);
  path.lineTo(left + 3, cy + 3);
  path.lineTo(left, cy + 3);
  path.close();
  // Sound wave.
  path.moveTo(left + 7, cy - 3);
  path.quadTo(left + 9, cy, left + 7, cy + 3);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.0f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw loading/spinner icon (simple animation placeholder).
void DrawLoadingIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int r = std::min(bounds.width(), bounds.height()) / 2 - 2;

  SkPath path;
  // Partial arc for "loading" feel.
  path.addArc(SkRect::MakeLTRB(cx - r, cy - r, cx + r, cy + r),
              45, 270);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw pin icon.
void DrawPinIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) - 4;

  SkPath path;
  // Pin head.
  path.addCircle(cx, cy - w * 0.1f, w * 0.3f);
  // Pin body.
  path.moveTo(cx, cy + w * 0.1f);
  path.lineTo(cx, cy + w * 0.45f);
  // Pin point.
  path.moveTo(cx - w * 0.15f, cy + w * 0.3f);
  path.lineTo(cx, cy + w * 0.5f);
  path.lineTo(cx + w * 0.15f, cy + w * 0.3f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.0f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

}  // namespace

// ===========================================================================
// AstraVerticalTabView
// ===========================================================================

AstraVerticalTabView::AstraVerticalTabView(
    const std::string& tab_id,
    const std::u16string& title,
    const gfx::ImageSkia& favicon,
    bool is_active,
    bool is_pinned,
    bool is_audible,
    Delegate* delegate)
    : tab_id_(tab_id),
      title_(title),
      favicon_(favicon),
      is_active_(is_active),
      is_pinned_(is_pinned),
      is_audible_(is_audible),
      delegate_(delegate) {
  BuildUI();

  SetCallback(base::BindRepeating(
      [](AstraVerticalTabView* tab, const ui::Event& event) {
        if (tab->delegate_) {
          tab->delegate_->OnTabClicked(tab->tab_id_);
        }
      },
      base::Unretained(this)));
}

AstraVerticalTabView::~AstraVerticalTabView() = default;

void AstraVerticalTabView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, kTabPaddingH), 8));
  SetPreferredSize(gfx::Size(0, kTabHeight));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  // Favicon.
  favicon_view_ = AddChildView(std::make_unique<views::ImageView>());
  favicon_view_->SetPreferredSize(gfx::Size(kFaviconSize, kFaviconSize));
  if (!favicon_.isNull()) {
    favicon_view_->SetImage(favicon_);
  } else {
    favicon_view_->SetImage(
        gfx::CreateVectorIcon(
            base::BindRepeating(
                &AstraVerticalTabView::DrawFaviconFallback,
                base::Unretained(this)),
            gfx::Size(kFaviconSize, kFaviconSize)));
  }

  // Title.
  title_label_ = AddChildView(std::make_unique<views::Label>(title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Close button (right-aligned, shown on hover or if active).
  close_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(
          [](AstraVerticalTabView* tab, const ui::Event& event) {
            if (tab->delegate_) {
              tab->delegate_->OnTabClosed(tab->tab_id_);
            }
          },
          base::Unretained(this))));
  close_button_->SetPreferredSize(
      gfx::Size(kCloseButtonSize, kCloseButtonSize));
  close_button_->SetTooltipText(u"Close tab");

  UpdateColors();
  UpdateVisibility();
}

void AstraVerticalTabView::SetTitle(const std::u16string& title) {
  if (title_ == title) return;
  title_ = title;
  title_label_->SetText(title);
}

void AstraVerticalTabView::SetFavicon(const gfx::ImageSkia& favicon) {
  favicon_ = favicon;
  if (favicon_.isNull()) {
    favicon_view_->SetImage(
        gfx::CreateVectorIcon(
            base::BindRepeating(
                &AstraVerticalTabView::DrawFaviconFallback,
                base::Unretained(this)),
            gfx::Size(kFaviconSize, kFaviconSize)));
  } else {
    favicon_view_->SetImage(favicon_);
  }
}

void AstraVerticalTabView::SetActive(bool active) {
  if (is_active_ == active) return;
  is_active_ = active;
  UpdateColors();
  SchedulePaint();
}

void AstraVerticalTabView::SetPinned(bool pinned) {
  if (is_pinned_ == pinned) return;
  is_pinned_ = pinned;
  UpdateVisibility();
  SchedulePaint();
}

void AstraVerticalTabView::SetAudible(bool audible) {
  if (is_audible_ == audible) return;
  is_audible_ = audible;
  SchedulePaint();
}

void AstraVerticalTabView::SetLoading(bool loading) {
  if (is_loading_ == loading) return;
  is_loading_ = loading;
  SchedulePaint();
}

void AstraVerticalTabView::UpdateColors() {
  const auto* cp = GetColorProvider();
  if (!cp) return;

  SkColor fg = is_active_
      ? cp->GetColor(ui::kColorTabForegroundActiveFrameActive)
      : cp->GetColor(ui::kColorTabForegroundInactiveFrameActive);
  SkColor bg = is_active_
      ? cp->GetColor(ui::kColorTabBackgroundActiveFrameActive)
      : SK_ColorTRANSPARENT;

  title_label_->SetEnabledColor(fg);
  SetBackground(views::CreateRoundedRectBackground(bg, 6));

  // Update close button icon.
  close_button_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(
          base::BindRepeating(&DrawCloseIcon),
          gfx::Size(kCloseButtonSize, kCloseButtonSize),
          fg));
  close_button_->SetImage(
      views::Button::STATE_HOVERED,
      gfx::CreateVectorIcon(
          base::BindRepeating(&DrawCloseIcon),
          gfx::Size(kCloseButtonSize, kCloseButtonSize),
          cp->GetColor(ui::kColorCloseButtonForegroundHovered)));
}

void AstraVerticalTabView::UpdateVisibility() {
  // Pinned tabs show less info.
  if (is_pinned_) {
    title_label_->SetVisible(false);
    close_button_->SetVisible(false);
  } else {
    title_label_->SetVisible(true);
    close_button_->SetVisible(true);
  }
}

void AstraVerticalTabView::OnThemeChanged() {
  views::Button::OnThemeChanged();
  UpdateColors();
}

gfx::Size AstraVerticalTabView::CalculatePreferredSize() const {
  if (is_pinned_) {
    return gfx::Size(kTabHeight, kTabHeight);
  }
  return gfx::Size(0, kTabHeight);
}

void AstraVerticalTabView::DrawFaviconFallback(gfx::Canvas* canvas,
                                                const gfx::Rect& bounds) {
  SkColor color = GetColorProvider()
      ? GetColorProvider()->GetColor(ui::kColorIcon) : SK_ColorGRAY;

  // Draw a simple globe/globe as fallback.
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int r = std::min(bounds.width(), bounds.height()) / 2 - 1;

  SkPath path;
  path.addCircle(cx, cy, r);
  path.moveTo(cx - r, cy);
  path.lineTo(cx + r, cy);
  path.moveTo(cx, cy - r);
  path.lineTo(cx, cy + r);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.0f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

}  // namespace astra
