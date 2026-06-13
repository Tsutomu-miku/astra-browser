// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/page_info/astra_page_info_bubble.h"

#include <memory>
#include <string>

#include "astra/ui/views/page_info/astra_page_info_model.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/models/simple_combobox_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_view.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Constants.
constexpr int kSecurityIconSize = 20;
constexpr int kIconSize = 16;
constexpr int kRowHeight = 32;
constexpr int kBubbleMinWidth = 320;
constexpr int kBubbleMaxWidth = 380;
constexpr int kContentPadding = 16;
constexpr int kSectionSpacing = 12;
constexpr int kRowSpacing = 8;

// Draw a lock icon (secure).
void DrawLockIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Lock body (bottom rectangle).
  SkPath body;
  body.addRRect(SkRRect::MakeRectXY(
      SkRect::MakeXYWH(cx - s, cy - s * 0.2f, s * 2, s * 1.3f),
      s * 0.15f, s * 0.15f));
  canvas->DrawPath(body, flags);

  // Shackle (top arc).
  SkPath shackle;
  shackle.moveTo(cx - s * 0.6f, cy - s * 0.2f);
  shackle.lineTo(cx - s * 0.6f, cy - s * 0.8f);
  shackle.arcTo(SkRect::MakeXYWH(cx - s * 0.6f, cy - s * 1.3f,
                                  s * 1.2f, s * 1.0f),
                180, 180, false);
  shackle.lineTo(cx + s * 0.6f, cy - s * 0.2f);
  canvas->DrawPath(shackle, flags);

  // Keyhole dot.
  canvas->DrawCircle(gfx::Point(cx, cy + s * 0.25f), s * 0.12f, flags);
  // Keyhole slot.
  SkPath slot;
  slot.moveTo(cx, cy + s * 0.25f);
  slot.lineTo(cx, cy + s * 0.55f);
  canvas->DrawPath(slot, flags);
}

// Draw a warning / danger icon (insecure).
void DrawWarningIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.38f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Triangle outline.
  SkPath triangle;
  triangle.moveTo(cx, cy - s);
  triangle.lineTo(cx + s, cy + s * 0.8f);
  triangle.lineTo(cx - s, cy + s * 0.8f);
  triangle.close();
  canvas->DrawPath(triangle, flags);

  // Exclamation mark.
  SkPath exclaim;
  exclaim.moveTo(cx, cy - s * 0.2f);
  exclaim.lineTo(cx, cy + s * 0.3f);
  canvas->DrawPath(exclaim, flags);
  canvas->DrawCircle(gfx::Point(cx, cy + s * 0.55f), s * 0.08f, flags);
}

// Draw an info / "i" icon.
void DrawInfoIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.38f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Circle outline.
  canvas->DrawCircle(gfx::Point(cx, cy), s, flags);

  // "i" dot.
  canvas->DrawCircle(gfx::Point(cx, cy - s * 0.35f), s * 0.1f, flags);

  // "i" stem.
  SkPath stem;
  stem.moveTo(cx, cy - s * 0.15f);
  stem.lineTo(cx, cy + s * 0.45f);
  canvas->DrawPath(stem, flags);
}

// Draw a camera icon.
void DrawCameraIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Camera body.
  SkPath path;
  path.addRect(SkRect::MakeXYWH(cx - s, cy - s * 0.7f, s * 2, s * 1.4f));
  canvas->DrawPath(path, flags);

  // Lens.
  canvas->DrawCircle(gfx::Point(cx, cy), s * 0.45f, flags);

  // Flash / viewfinder top.
  SkPath top;
  top.moveTo(cx - s * 0.6f, cy - s * 0.7f);
  top.lineTo(cx - s * 0.4f, cy - s * 1.1f);
  top.lineTo(cx + s * 0.4f, cy - s * 1.1f);
  top.lineTo(cx + s * 0.6f, cy - s * 0.7f);
  canvas->DrawPath(top, flags);
}

// Draw a microphone icon.
void DrawMicIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Mic capsule.
  SkRect rect = SkRect::MakeXYWH(cx - s * 0.5f, cy - s, s, s * 1.4f);
  SkPath path;
  path.addOval(rect);
  canvas->DrawPath(path, flags);

  // Stand / base.
  SkPath stand;
  stand.moveTo(cx - s * 0.8f, cy + s);
  stand.arcTo(SkRect::MakeXYWH(cx - s * 0.8f, cy + s * 0.3f, s * 1.6f, s * 1.4f),
              180, 180, false);
  canvas->DrawPath(stand, flags);

  // Bottom bar.
  SkPath bar;
  bar.moveTo(cx, cy + s);
  bar.lineTo(cx, cy + s * 1.5f);
  canvas->DrawPath(bar, flags);
}

// Draw a location / map pin icon.
void DrawLocationIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Pin shape (teardrop).
  SkPath path;
  path.moveTo(cx, cy - s);
  path.arcTo(SkRect::MakeXYWH(cx - s, cy - s, s * 2, s * 2), 270, -180, false);
  path.lineTo(cx, cy + s);
  path.close();
  canvas->DrawPath(path, flags);

  // Inner dot.
  canvas->DrawCircle(gfx::Point(cx, cy - s * 0.1f), s * 0.25f, flags);
}

// Draw a bell / notifications icon.
void DrawBellIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Bell body.
  SkPath path;
  path.moveTo(cx - s * 0.8f, cy + s * 0.3f);
  path.arcTo(SkRect::MakeXYWH(cx - s, cy - s * 0.3f, s * 2, s * 1.2f),
             180, 180, false);
  path.lineTo(cx + s * 0.8f, cy + s * 0.3f);
  path.quadTo(cx + s * 0.6f, cy + s * 0.6f, cx + s * 0.5f, cy + s * 0.7f);
  path.lineTo(cx - s * 0.5f, cy + s * 0.7f);
  path.quadTo(cx - s * 0.6f, cy + s * 0.6f, cx - s * 0.8f, cy + s * 0.3f);
  canvas->DrawPath(path, flags);

  // Bell bottom / clapper.
  canvas->DrawCircle(gfx::Point(cx, cy + s * 0.95f), s * 0.15f, flags);

  // Top mount.
  canvas->DrawRect(gfx::Rect(cx - s * 0.3f, cy - s * 0.5f, s * 0.6f, s * 0.2f),
                   flags);
}

// Draw a clipboard icon.
void DrawClipboardIcon(gfx::Canvas* canvas,
                       const gfx::Rect& bounds,
                       SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Clipboard body.
  SkPath body;
  body.addRRect(SkRRect::MakeRectXY(
      SkRect::MakeXYWH(cx - s * 0.8f, cy - s * 0.7f, s * 1.6f, s * 1.7f),
      s * 0.1f, s * 0.1f));
  canvas->DrawPath(body, flags);

  // Clip.
  SkPath clip;
  clip.addRRect(SkRRect::MakeRectXY(
      SkRect::MakeXYWH(cx - s * 0.45f, cy - s * 0.95f, s * 0.9f, s * 0.45f),
      s * 0.08f, s * 0.08f));
  canvas->DrawPath(clip, flags);

  // Content lines.
  SkPath line1;
  line1.moveTo(cx - s * 0.5f, cy - s * 0.2f);
  line1.lineTo(cx + s * 0.5f, cy - s * 0.2f);
  canvas->DrawPath(line1, flags);

  SkPath line2;
  line2.moveTo(cx - s * 0.5f, cy + s * 0.1f);
  line2.lineTo(cx + s * 0.5f, cy + s * 0.1f);
  canvas->DrawPath(line2, flags);

  SkPath line3;
  line3.moveTo(cx - s * 0.5f, cy + s * 0.4f);
  line3.lineTo(cx + s * 0.2f, cy + s * 0.4f);
  canvas->DrawPath(line3, flags);
}

// Draw a popup / window icon.
void DrawPopupIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Main window.
  SkPath main;
  main.addRect(SkRect::MakeXYWH(cx - s, cy - s * 0.3f, s * 1.7f, s * 1.3f));
  canvas->DrawPath(main, flags);

  // Title bar dots.
  canvas->DrawCircle(gfx::Point(cx - s * 0.75f, cy - s * 0.05f),
                     s * 0.08f, flags);
  canvas->DrawCircle(gfx::Point(cx - s * 0.5f, cy - s * 0.05f),
                     s * 0.08f, flags);
  canvas->DrawCircle(gfx::Point(cx - s * 0.25f, cy - s * 0.05f),
                     s * 0.08f, flags);

  // Popup window (smaller, top-right).
  SkPath popup;
  popup.addRect(SkRect::MakeXYWH(cx + s * 0.1f, cy - s, s * 0.8f, s * 0.7f));
  canvas->DrawPath(popup, flags);

  // Arrow from popup to main.
  SkPath arrow;
  arrow.moveTo(cx + s * 0.1f, cy - s * 0.3f);
  arrow.lineTo(cx - s * 0.1f, cy - s * 0.1f);
  canvas->DrawPath(arrow, flags);
}

// Draw an image / picture icon.
void DrawImageIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Frame.
  SkPath frame;
  frame.addRect(SkRect::MakeXYWH(cx - s, cy - s * 0.8f, s * 2, s * 1.6f));
  canvas->DrawPath(frame, flags);

  // Mountain.
  SkPath mountain;
  mountain.moveTo(cx - s * 0.7f, cy + s * 0.5f);
  mountain.lineTo(cx - s * 0.2f, cy - s * 0.1f);
  mountain.lineTo(cx + s * 0.2f, cy + s * 0.3f);
  mountain.lineTo(cx + s * 0.7f, cy - s * 0.3f);
  mountain.lineTo(cx + s * 0.7f, cy + s * 0.5f);
  mountain.lineTo(cx - s * 0.7f, cy + s * 0.5f);
  canvas->DrawPath(mountain, flags);

  // Sun/circle.
  canvas->DrawCircle(gfx::Point(cx + s * 0.35f, cy - s * 0.4f),
                     s * 0.18f, flags);
}

// Draw a JavaScript / code icon.
void DrawJsIcon(gfx::Canvas* canvas,
                const gfx::Rect& bounds,
                SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Opening angle bracket.
  SkPath left;
  left.moveTo(cx - s * 0.2f, cy - s * 0.6f);
  left.lineTo(cx - s * 0.7f, cy);
  left.lineTo(cx - s * 0.2f, cy + s * 0.6f);
  canvas->DrawPath(left, flags);

  // Closing angle bracket.
  SkPath right;
  right.moveTo(cx + s * 0.2f, cy - s * 0.6f);
  right.lineTo(cx + s * 0.7f, cy);
  right.lineTo(cx + s * 0.2f, cy + s * 0.6f);
  canvas->DrawPath(right, flags);

  // Slash.
  SkPath slash;
  slash.moveTo(cx + s * 0.6f, cy - s * 0.6f);
  slash.lineTo(cx - s * 0.1f, cy + s * 0.6f);
  canvas->DrawPath(slash, flags);
}

// Draw a sound / speaker icon.
void DrawSoundIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Speaker body.
  SkPath speaker;
  speaker.moveTo(cx - s * 0.7f, cy - s * 0.3f);
  speaker.lineTo(cx - s * 0.2f, cy - s * 0.3f);
  speaker.lineTo(cx + s * 0.4f, cy - s * 0.6f);
  speaker.lineTo(cx + s * 0.4f, cy + s * 0.6f);
  speaker.lineTo(cx - s * 0.2f, cy + s * 0.3f);
  speaker.lineTo(cx - s * 0.7f, cy + s * 0.3f);
  speaker.close();
  canvas->DrawPath(speaker, flags);

  // Sound waves.
  canvas->DrawArc(
      gfx::RectF(cx + s * 0.3f, cy - s * 0.5f, s * 0.5f, s),
      -60, 120, false, flags);
  canvas->DrawArc(
      gfx::RectF(cx + s * 0.45f, cy - s * 0.65f, s * 0.5f, s * 1.3f),
      -60, 120, false, flags);
}

// Draw a fullscreen icon.
void DrawFullscreenIcon(gfx::Canvas* canvas,
                        const gfx::Rect& bounds,
                        SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Outer frame.
  SkPath frame;
  frame.addRect(SkRect::MakeXYWH(cx - s, cy - s * 0.8f, s * 2, s * 1.6f));
  canvas->DrawPath(frame, flags);

  // Top-left arrow.
  SkPath tl_arrow;
  tl_arrow.moveTo(cx - s * 0.2f, cy - s * 0.6f);
  tl_arrow.lineTo(cx - s * 0.6f, cy - s * 0.6f);
  tl_arrow.lineTo(cx - s * 0.6f, cy - s * 0.2f);
  canvas->DrawPath(tl_arrow, flags);

  // Top-right arrow.
  SkPath tr_arrow;
  tr_arrow.moveTo(cx + s * 0.2f, cy - s * 0.6f);
  tr_arrow.lineTo(cx + s * 0.6f, cy - s * 0.6f);
  tr_arrow.lineTo(cx + s * 0.6f, cy - s * 0.2f);
  canvas->DrawPath(tr_arrow, flags);

  // Bottom-left arrow.
  SkPath bl_arrow;
  bl_arrow.moveTo(cx - s * 0.2f, cy + s * 0.6f);
  bl_arrow.lineTo(cx - s * 0.6f, cy + s * 0.6f);
  bl_arrow.lineTo(cx - s * 0.6f, cy + s * 0.2f);
  canvas->DrawPath(bl_arrow, flags);

  // Bottom-right arrow.
  SkPath br_arrow;
  br_arrow.moveTo(cx + s * 0.2f, cy + s * 0.6f);
  br_arrow.lineTo(cx + s * 0.6f, cy + s * 0.6f);
  br_arrow.lineTo(cx + s * 0.6f, cy + s * 0.2f);
  canvas->DrawPath(br_arrow, flags);
}

// Draw a cookie icon.
void DrawCookieIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.38f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Cookie body (circle with a bite taken out).
  SkPath cookie;
  cookie.moveTo(cx + s * 0.3f, cy - s * 0.9f);
  cookie.arcTo(SkRect::MakeXYWH(cx - s, cy - s, s * 2, s * 2),
               -70, 270, false);
  cookie.arcTo(SkRect::MakeXYWH(cx + s * 0.4f, cy - s * 0.6f, s * 0.5f, s * 0.5f),
               110, -100, false);
  cookie.close();
  canvas->DrawPath(cookie, flags);

  // Chocolate chips.
  canvas->DrawCircle(gfx::Point(cx - s * 0.3f, cy - s * 0.3f),
                     s * 0.1f, flags);
  canvas->DrawCircle(gfx::Point(cx + s * 0.15f, cy + s * 0.2f),
                     s * 0.12f, flags);
  canvas->DrawCircle(gfx::Point(cx - s * 0.45f, cy + s * 0.35f),
                     s * 0.09f, flags);
  canvas->DrawCircle(gfx::Point(cx + s * 0.4f, cy - s * 0.2f),
                     s * 0.08f, flags);
}

// Draw a settings / gear icon.
void DrawSettingsIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Gear teeth (8 points).
  SkPath gear;
  for (int i = 0; i < 8; ++i) {
    float angle = (i * 360.0f / 8.0f) * 3.14159f / 180.0f;
    float outer_x = cx + sinf(angle) * s;
    float outer_y = cy - cosf(angle) * s;
    float inner_angle = ((i + 0.5f) * 360.0f / 8.0f) * 3.14159f / 180.0f;
    float inner_x = cx + sinf(inner_angle) * s * 0.7f;
    float inner_y = cy - cosf(inner_angle) * s * 0.7f;
    if (i == 0) {
      gear.moveTo(outer_x, outer_y);
    } else {
      gear.lineTo(outer_x, outer_y);
    }
    gear.lineTo(inner_x, inner_y);
  }
  gear.close();
  canvas->DrawPath(gear, flags);

  // Center hole.
  canvas->DrawCircle(gfx::Point(cx, cy), s * 0.3f, flags);
}

// Draw a certificate / badge icon.
void DrawCertificateIcon(gfx::Canvas* canvas,
                         const gfx::Rect& bounds,
                         SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Certificate body (top rectangle, bottom ribbon).
  SkPath body;
  body.addRect(SkRect::MakeXYWH(cx - s * 0.7f, cy - s * 0.7f,
                                s * 1.4f, s * 1.1f));
  canvas->DrawPath(body, flags);

  // Ribbon folds.
  SkPath ribbon_left;
  ribbon_left.moveTo(cx - s * 0.7f, cy + s * 0.4f);
  ribbon_left.lineTo(cx - s * 0.5f, cy + s);
  ribbon_left.lineTo(cx - s * 0.2f, cy + s * 0.5f);
  canvas->DrawPath(ribbon_left, flags);

  SkPath ribbon_right;
  ribbon_right.moveTo(cx + s * 0.7f, cy + s * 0.4f);
  ribbon_right.lineTo(cx + s * 0.5f, cy + s);
  ribbon_right.lineTo(cx + s * 0.2f, cy + s * 0.5f);
  canvas->DrawPath(ribbon_right, flags);

  // Seal / checkmark.
  canvas->DrawCircle(gfx::Point(cx, cy + s * 0.1f), s * 0.3f, flags);
  SkPath check;
  check.moveTo(cx - s * 0.15f, cy + s * 0.1f);
  check.lineTo(cx - s * 0.02f, cy + s * 0.22f);
  check.lineTo(cx + s * 0.18f, cy - s * 0.02f);
  canvas->DrawPath(check, flags);
}

// Draw a shield / security icon.
void DrawShieldIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(cx, cy - s);
  path.lineTo(cx + s * 0.8f, cy - s * 0.5f);
  path.lineTo(cx + s * 0.8f, cy + s * 0.3f);
  path.lineTo(cx, cy + s);
  path.lineTo(cx - s * 0.8f, cy + s * 0.3f);
  path.lineTo(cx - s * 0.8f, cy - s * 0.5f);
  path.close();
  canvas->DrawPath(path, flags);

  // Checkmark inside.
  SkPath check;
  check.moveTo(cx - s * 0.35f, cy);
  check.lineTo(cx - s * 0.1f, cy + s * 0.25f);
  check.lineTo(cx + s * 0.4f, cy - s * 0.25f);
  canvas->DrawPath(check, flags);
}

// Draw a chevron/arrow icon.
void DrawChevronIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color,
                     bool pointing_right) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.25f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  if (pointing_right) {
    path.moveTo(cx - s * 0.5f, cy - s);
    path.lineTo(cx + s * 0.5f, cy);
    path.lineTo(cx - s * 0.5f, cy + s);
  } else {
    path.moveTo(cx + s * 0.5f, cy - s);
    path.lineTo(cx - s * 0.5f, cy);
    path.lineTo(cx + s * 0.5f, cy + s);
  }
  canvas->DrawPath(path, flags);
}

// Draw a section title with proper styling.
std::unique_ptr<views::Label> CreateSectionTitle(const std::u16string& text) {
  auto label = std::make_unique<views::Label>(text);
  label->SetFontList(views::style::GetFont(
      views::style::CONTEXT_DIALOG_TITLE, views::style::STYLE_PRIMARY));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  return label;
}

}  // namespace

// ===========================================================================
// AstraPageInfoBubble
// ===========================================================================

AstraPageInfoBubble::AstraPageInfoBubble(views::View* anchor_view,
                                         AstraPageInfoModel* model)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_LEFT) {
  SetModel(model);
  BuildUI();
}

AstraPageInfoBubble::~AstraPageInfoBubble() {
  if (scoped_observation_.IsObserving()) {
    scoped_observation_.RemoveObserver();
  }
}

void AstraPageInfoBubble::SetModel(AstraPageInfoModel* model) {
  if (model_ == model) {
    return;
  }

  if (scoped_observation_.IsObserving()) {
    scoped_observation_.RemoveObserver();
  }

  model_ = model;

  if (model_) {
    scoped_observation_.Observe(model_);
    RefreshFromModel();
  }
}

void AstraPageInfoBubble::RefreshFromModel() {
  if (!model_) {
    return;
  }

  RefreshSecurityDisplay();
  RefreshCookiesDisplay();

  // Refresh all permission rows.
  for (auto& [type, combobox] : permission_comboboxes_) {
    RefreshPermissionRow(type);
  }

  if (GetWidget()) {
    SizeToContents();
  }
}

std::u16string AstraPageInfoBubble::GetWindowTitle() const {
  if (!model_) {
    return std::u16string();
  }
  return model_->GetSiteInfo().display_name;
}

void AstraPageInfoBubble::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor bg_color = color_provider->GetColor(ui::kColorDialogBackground);
  SetBackground(views::CreateSolidBackground(bg_color));

  // Refresh all icons with the new theme colors.
  RefreshFromModel();

  SchedulePaint();
}

void AstraPageInfoBubble::OnPermissionChanged(AstraPageInfoModel* model,
                                              AstraPagePermissionType type) {
  DCHECK_EQ(model, model_);
  RefreshPermissionRow(type);
}

void AstraPageInfoBubble::OnCookiesChanged(AstraPageInfoModel* model) {
  DCHECK_EQ(model, model_);
  RefreshCookiesDisplay();
}

void AstraPageInfoBubble::OnSecurityStatusChanged(AstraPageInfoModel* model) {
  DCHECK_EQ(model, model_);
  RefreshSecurityDisplay();
}

void AstraPageInfoBubble::OnPageInfoModelShutdown(AstraPageInfoModel* model) {
  DCHECK_EQ(model, model_);
  scoped_observation_.RemoveObserver();
  model_ = nullptr;
}

views::Combobox* AstraPageInfoBubble::GetPermissionCombobox(
    AstraPagePermissionType type) {
  for (auto& [t, cb] : permission_comboboxes_) {
    if (t == type) {
      return cb;
    }
  }
  return nullptr;
}

// ===========================================================================
// UI construction
// ===========================================================================

void AstraPageInfoBubble::BuildUI() {
  set_margins(gfx::Insets(0));
  SetArrow(views::BubbleBorder::Arrow::TOP_LEFT);
  set_close_on_deactivate(true);
  set_close_on_esc(true);

  auto* layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);

  BuildHeader();
  BuildSiteInfoSection();
  BuildPermissionsSection();
  BuildSecuritySection();

  set_min_size(gfx::Size(kBubbleMinWidth, 0));
  set_max_size(gfx::Size(kBubbleMaxWidth, 0));
}

void AstraPageInfoBubble::BuildHeader() {
  auto header = std::make_unique<views::View>();
  auto* header_layout =
      header->SetLayoutManager(std::make_unique<views::FlexLayout>());
  header_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  header_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  header_layout->SetInteriorMargin(
      gfx::Insets::VH(kContentPadding, kContentPadding));

  // Security icon.
  auto icon = std::make_unique<views::ImageView>();
  icon->SetImageSize(gfx::Size(kSecurityIconSize, kSecurityIconSize));
  security_icon_ = header->AddChildView(std::move(icon));
  security_icon_->SetProperty(views::kMarginsKey,
                              gfx::Insets::VH(0, 0, 0, 12));

  // Site name + connection status column.
  auto* title_col = header->AddChildView(std::make_unique<views::View>());
  auto* title_layout =
      title_col->SetLayoutManager(std::make_unique<views::FlexLayout>());
  title_layout->SetOrientation(views::LayoutOrientation::kVertical);
  header_layout->SetFlexForView(title_col, 1);

  auto site = std::make_unique<views::Label>();
  site->SetFontList(views::style::GetFont(
      views::style::CONTEXT_DIALOG_TITLE, views::style::STYLE_PRIMARY));
  site->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  site->SetAutoColorReadabilityEnabled(false);
  site_label_ = title_col->AddChildView(std::move(site));

  auto connection = std::make_unique<views::Label>();
  connection->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  connection->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  connection->SetAutoColorReadabilityEnabled(false);
  connection_label_ = title_col->AddChildView(std::move(connection));

  AddChildView(std::move(header));
}

void AstraPageInfoBubble::BuildSiteInfoSection() {
  auto section = std::make_unique<views::View>();
  auto* layout =
      section->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetInteriorMargin(
      gfx::Insets::VH(0, kContentPadding, kSectionSpacing, kContentPadding));

  // Section title.
  auto title = CreateSectionTitle(u"Site information");
  section->AddChildView(std::move(title));

  // Separator.
  auto separator = std::make_unique<views::Separator>();
  separator->SetProperty(views::kMarginsKey,
                         gfx::Insets::VH(4, 0, 8, 0));
  section->AddChildView(std::move(separator));

  // Cookies row (button).
  auto cookies_row = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&AstraPageInfoBubble::OnCookiesClicked,
                          base::Unretained(this)),
      u"");
  cookies_row->SetMinSize(gfx::Size(0, kRowHeight));
  cookies_row->SetStyle(ui::ButtonStyle::kText);
  cookies_button_ = section->AddChildView(std::move(cookies_row));

  // Site settings row (button).
  auto settings_row = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&AstraPageInfoBubble::OnSiteSettingsClicked,
                          base::Unretained(this)),
      u"");
  settings_row->SetMinSize(gfx::Size(0, kRowHeight));
  settings_row->SetStyle(ui::ButtonStyle::kText);
  site_settings_button_ = section->AddChildView(std::move(settings_row));

  site_info_section_ = AddChildView(std::move(section));
}

void AstraPageInfoBubble::BuildPermissionsSection() {
  auto section = std::make_unique<views::View>();
  auto* layout =
      section->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetInteriorMargin(
      gfx::Insets::VH(0, kContentPadding, kSectionSpacing, kContentPadding));

  // Section title.
  auto title = CreateSectionTitle(u"Permissions");
  section->AddChildView(std::move(title));

  // Separator.
  auto separator = std::make_unique<views::Separator>();
  separator->SetProperty(views::kMarginsKey,
                         gfx::Insets::VH(4, 0, 8, 0));
  section->AddChildView(std::move(separator));

  // Permission rows container.
  auto container = std::make_unique<views::View>();
  auto* container_layout =
      container->SetLayoutManager(std::make_unique<views::FlexLayout>());
  container_layout->SetOrientation(views::LayoutOrientation::kVertical);
  permission_rows_container_ = section->AddChildView(std::move(container));

  if (model_) {
    const auto& permissions = model_->GetPermissions();
    for (const auto& entry : permissions) {
      if (!entry.show_in_page_info) {
        continue;
      }

      auto* row = permission_rows_container_->AddChildView(
          std::make_unique<views::View>());
      auto* row_layout =
          row->SetLayoutManager(std::make_unique<views::FlexLayout>());
      row_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
      row_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
      row_layout->SetProperty(views::kMarginsKey,
                              gfx::Insets::VH(2, 0));

      // Permission icon.
      auto icon = std::make_unique<views::ImageView>();
      icon->SetImageSize(gfx::Size(kIconSize, kIconSize));
      icon->SetProperty(views::kMarginsKey,
                        gfx::Insets::VH(0, 0, 0, 8));
      row->AddChildView(std::move(icon));

      // Permission name label.
      auto label = std::make_unique<views::Label>(entry.name);
      label->SetFontList(views::style::GetFont(
          views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY));
      label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      label->SetAutoColorReadabilityEnabled(false);
      row->AddChildView(std::move(label));
      row_layout->SetFlexForView(label, 1);

      // Permission setting combobox.
      std::vector<ui::SimpleComboboxModel::Item> items = {
          ui::SimpleComboboxModel::Item(u"Allow"),
          ui::SimpleComboboxModel::Item(u"Block"),
          ui::SimpleComboboxModel::Item(u"Ask"),
      };
      auto combobox = std::make_unique<views::Combobox>(
          std::make_unique<ui::SimpleComboboxModel>(std::move(items)));
      combobox->SetProperty(views::kMarginsKey,
                            gfx::Insets::VH(0, 0, 0, 0));
      combobox->SetCallback(base::BindRepeating(
          &AstraPageInfoBubble::OnPermissionChanged,
          base::Unretained(this), entry.type));

      auto* cb_raw = combobox.get();
      permission_comboboxes_.push_back({entry.type, cb_raw});
      row->AddChildView(std::move(combobox));
    }
  }

  permissions_section_ = AddChildView(std::move(section));
}

void AstraPageInfoBubble::BuildSecuritySection() {
  auto section = std::make_unique<views::View>();
  auto* layout =
      section->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetInteriorMargin(
      gfx::Insets::VH(0, kContentPadding, kContentPadding, kContentPadding));

  // Section title.
  auto title = CreateSectionTitle(u"Security");
  section->AddChildView(std::move(title));

  // Separator.
  auto separator = std::make_unique<views::Separator>();
  separator->SetProperty(views::kMarginsKey,
                         gfx::Insets::VH(4, 0, 8, 0));
  section->AddChildView(std::move(separator));

  // Certificate row (button).
  auto cert_row = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&AstraPageInfoBubble::OnCertificateClicked,
                          base::Unretained(this)),
      u"");
  cert_row->SetMinSize(gfx::Size(0, kRowHeight));
  cert_row->SetStyle(ui::ButtonStyle::kText);
  section->AddChildView(std::move(cert_row));

  // Connection details row (button).
  auto conn_row = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&AstraPageInfoBubble::OnConnectionDetailsClicked,
                          base::Unretained(this)),
      u"");
  conn_row->SetMinSize(gfx::Size(0, kRowHeight));
  conn_row->SetStyle(ui::ButtonStyle::kText);
  section->AddChildView(std::move(conn_row));

  security_section_ = AddChildView(std::move(section));
}

// ===========================================================================
// Refresh methods
// ===========================================================================

void AstraPageInfoBubble::RefreshSecurityDisplay() {
  if (!model_ || !security_icon_ || !site_label_ || !connection_label_) {
    return;
  }

  const auto& site_info = model_->GetSiteInfo();

  SkColor status_color =
      AstraPageInfoModel::GetSecurityStatusColor(site_info.security_status);

  // Update site label.
  site_label_->SetText(site_info.display_name);

  // Update connection label.
  connection_label_->SetText(site_info.security_summary);

  // Draw security icon.
  SkBitmap bitmap;
  bitmap.allocN32Pixels(kSecurityIconSize, kSecurityIconSize);
  bitmap.eraseColor(SK_ColorTRANSPARENT);
  gfx::Canvas canvas(gfx::Size(kSecurityIconSize, kSecurityIconSize), 1.0f,
                     false);
  DrawSecurityIcon(&canvas, gfx::Rect(0, 0, kSecurityIconSize, kSecurityIconSize),
                   site_info.security_status, status_color);
  security_icon_->SetImage(
      gfx::ImageSkia::CreateFrom1xBitmap(canvas.GetBitmap()));
}

void AstraPageInfoBubble::RefreshCookiesDisplay() {
  if (!model_ || !cookies_button_) {
    return;
  }

  const auto& cookie_info = model_->GetCookies();
  std::u16string label =
      u"Cookies and site data: " +
      base::NumberToString16(cookie_info.cookies_in_use) +
      u" in use";
  cookies_button_->SetText(label);
}

void AstraPageInfoBubble::RefreshPermissionRow(AstraPagePermissionType type) {
  if (!model_) {
    return;
  }

  const auto* entry = model_->GetPermission(type);
  auto* combobox = GetPermissionCombobox(type);
  if (!entry || !combobox) {
    return;
  }

  // Set combobox to match the current setting.
  int index = 0;
  switch (entry->setting) {
    case AstraPermissionSetting::kAllow:
      index = 0;
      break;
    case AstraPermissionSetting::kBlock:
      index = 1;
      break;
    case AstraPermissionSetting::kAsk:
      index = 2;
      break;
  }
  combobox->SetSelectedIndex(index);
}

// ===========================================================================
// Icon drawing
// ===========================================================================

void AstraPageInfoBubble::DrawSecurityIcon(gfx::Canvas* canvas,
                                           const gfx::Rect& bounds,
                                           AstraSecurityStatus status,
                                           SkColor color) {
  switch (status) {
    case AstraSecurityStatus::kSecure:
      DrawLockIcon(canvas, bounds, color);
      break;
    case AstraSecurityStatus::kInsecure:
    case AstraSecurityStatus::kWarning:
      DrawWarningIcon(canvas, bounds, color);
      break;
    case AstraSecurityStatus::kNeutral:
    case AstraSecurityStatus::kUnknown:
      DrawInfoIcon(canvas, bounds, color);
      break;
  }
}

void AstraPageInfoBubble::DrawPermissionIcon(gfx::Canvas* canvas,
                                             const gfx::Rect& bounds,
                                             AstraPagePermissionType type,
                                             SkColor color) {
  switch (type) {
    case AstraPagePermissionType::kCamera:
      DrawCameraIcon(canvas, bounds, color);
      break;
    case AstraPagePermissionType::kMicrophone:
      DrawMicIcon(canvas, bounds, color);
      break;
    case AstraPagePermissionType::kGeolocation:
      DrawLocationIcon(canvas, bounds, color);
      break;
    case AstraPagePermissionType::kNotifications:
      DrawBellIcon(canvas, bounds, color);
      break;
    case AstraPagePermissionType::kClipboard:
      DrawClipboardIcon(canvas, bounds, color);
      break;
    case AstraPagePermissionType::kPopups:
      DrawPopupIcon(canvas, bounds, color);
      break;
    case AstraPagePermissionType::kImages:
      DrawImageIcon(canvas, bounds, color);
      break;
    case AstraPagePermissionType::kJavaScript:
      DrawJsIcon(canvas, bounds, color);
      break;
    case AstraPagePermissionType::kSound:
      DrawSoundIcon(canvas, bounds, color);
      break;
    case AstraPagePermissionType::kFullscreen:
      DrawFullscreenIcon(canvas, bounds, color);
      break;
  }
}

void AstraPageInfoBubble::DrawIconByName(gfx::Canvas* canvas,
                                         const gfx::Rect& bounds,
                                         const std::string& name,
                                         SkColor color) {
  if (name == "lock") {
    DrawLockIcon(canvas, bounds, color);
  } else if (name == "warning") {
    DrawWarningIcon(canvas, bounds, color);
  } else if (name == "info") {
    DrawInfoIcon(canvas, bounds, color);
  } else if (name == "cookie") {
    DrawCookieIcon(canvas, bounds, color);
  } else if (name == "settings") {
    DrawSettingsIcon(canvas, bounds, color);
  } else if (name == "certificate") {
    DrawCertificateIcon(canvas, bounds, color);
  } else if (name == "shield") {
    DrawShieldIcon(canvas, bounds, color);
  } else if (name == "chevron") {
    DrawChevronIcon(canvas, bounds, color, true);
  }
}

// ===========================================================================
// Event handlers
// ===========================================================================

void AstraPageInfoBubble::OnPermissionChanged(AstraPagePermissionType type) {
  auto* combobox = GetPermissionCombobox(type);
  if (!combobox || !model_) {
    return;
  }

  int index = combobox->GetSelectedIndex();
  AstraPermissionSetting setting;
  switch (index) {
    case 0:
      setting = AstraPermissionSetting::kAllow;
      break;
    case 1:
      setting = AstraPermissionSetting::kBlock;
      break;
    case 2:
    default:
      setting = AstraPermissionSetting::kAsk;
      break;
  }

  model_->SetPermission(type, setting);
}

void AstraPageInfoBubble::OnCookiesClicked() {
  if (model_) {
    model_->OpenCookiesDialog();
  }
}

void AstraPageInfoBubble::OnSiteSettingsClicked() {
  if (model_) {
    model_->OpenSiteSettings();
  }
}

void AstraPageInfoBubble::OnCertificateClicked() {
  if (model_) {
    model_->OpenCertificateViewer();
  }
}

void AstraPageInfoBubble::OnConnectionDetailsClicked() {
  // TODO(astra): Show connection details dialog.
}

}  // namespace astra
