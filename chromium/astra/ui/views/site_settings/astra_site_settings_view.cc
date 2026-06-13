// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/site_settings/astra_site_settings_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/base/l10n/l10n_util.h"
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
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_view.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

// Constants.
constexpr int kSidebarWidth = 200;
constexpr int kHeaderHeight = 56;
constexpr int kIconSize = 20;
constexpr int kSiteRowHeight = 72;
constexpr int kSiteRowIconSize = 32;
constexpr int kSidebarItemHeight = 36;
constexpr int kContentPadding = 16;

// Draw a globe icon.
void DrawGlobeIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int r = std::min(bounds.width(), bounds.height()) / 2 - 1;

  SkPath path;
  path.addCircle(cx, cy, r);
  path.moveTo(cx, cy - r);
  path.lineTo(cx, cy + r);
  path.moveTo(cx - r * 0.7f, cy - r * 0.7f);
  path.lineTo(cx - r * 0.7f, cy + r * 0.7f);
  path.moveTo(cx + r * 0.7f, cy - r * 0.7f);
  path.lineTo(cx + r * 0.7f, cy + r * 0.7f);
  path.moveTo(cx - r, cy);
  path.lineTo(cx + r, cy);
  path.moveTo(cx - r * 0.85f, cy - r * 0.5f);
  path.lineTo(cx + r * 0.85f, cy - r * 0.5f);
  path.moveTo(cx - r * 0.85f, cy + r * 0.5f);
  path.lineTo(cx + r * 0.85f, cy + r * 0.5f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw a clock icon.
void DrawClockIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int r = std::min(bounds.width(), bounds.height()) / 2 - 1;

  SkPath path;
  path.addCircle(cx, cy, r);
  path.moveTo(cx, cy);
  path.lineTo(cx, cy - r * 0.5f);
  path.moveTo(cx, cy);
  path.lineTo(cx + r * 0.4f, cy - r * 0.3f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw a shield/security icon.
void DrawShieldIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.CenterPoint().x();
  int w = std::min(bounds.width(), bounds.height()) - 2;
  int top = bounds.y() + 1;

  SkPath path;
  path.moveTo(cx, top);
  path.lineTo(cx + w / 2, top + w * 0.25f);
  path.lineTo(cx + w / 2, top + w * 0.6f);
  path.quadTo(cx + w / 2, top + w * 0.85f, cx, top + w);
  path.quadTo(cx - w / 2, top + w * 0.85f, cx - w / 2, top + w * 0.6f);
  path.lineTo(cx - w / 2, top + w * 0.25f);
  path.close();

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw a cookie icon.
void DrawCookieIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int r = std::min(bounds.width(), bounds.height()) / 2 - 1;

  SkPath path;
  path.addArc(SkRect::MakeLTRB(cx - r, cy - r, cx + r, cy + r),
              -45, 315);
  path.close();

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw location pin icon.
void DrawLocationIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color) {
  int cx = bounds.CenterPoint().x();
  int w = std::min(bounds.width(), bounds.height()) - 4;
  int top = bounds.y() + 2;

  SkPath path;
  path.moveTo(cx, top);
  path.quadTo(cx + w / 2, top, cx + w / 2, top + w / 2);
  path.quadTo(cx + w / 2, top + w * 0.75f, cx, top + w);
  path.quadTo(cx - w / 2, top + w * 0.75f, cx - w / 2, top + w / 2);
  path.quadTo(cx - w / 2, top, cx, top);
  path.close();

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw camera icon.
void DrawCameraIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int w = std::min(bounds.width(), bounds.height()) - 4;
  int left = bounds.x() + (bounds.width() - w) / 2;
  int top = bounds.y() + (bounds.height() - w) / 2 + 2;
  int h = w * 0.7f;

  SkPath path;
  path.addRect(left, top + h * 0.2f, left + w, top + h);
  path.moveTo(left + w * 0.25f, top + h * 0.2f);
  path.lineTo(left + w * 0.4f, top);
  path.lineTo(left + w * 0.6f, top);
  path.lineTo(left + w * 0.75f, top + h * 0.2f);
  path.addCircle(left + w / 2, top + h * 0.6f, w * 0.2f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw microphone icon.
void DrawMicIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int cx = bounds.CenterPoint().x();
  int w = std::min(bounds.width(), bounds.height()) - 4;
  int top = bounds.y() + 2;

  SkPath path;
  path.addOval(SkRect::MakeLTRB(cx - w * 0.25f, top,
                                 cx + w * 0.25f, top + w * 0.6f));
  path.moveTo(cx - w * 0.4f, top + w * 0.85f);
  path.lineTo(cx + w * 0.4f, top + w * 0.85f);
  path.moveTo(cx, top + w * 0.6f);
  path.lineTo(cx, top + w * 0.85f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw bell/notifications icon.
void DrawBellIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) - 4;

  SkPath path;
  path.moveTo(cx - w * 0.35f, cy + w * 0.2f);
  path.quadTo(cx - w * 0.35f, cy - w * 0.2f, cx - w * 0.15f, cy - w * 0.35f);
  path.lineTo(cx + w * 0.15f, cy - w * 0.35f);
  path.quadTo(cx + w * 0.35f, cy - w * 0.2f, cx + w * 0.35f, cy + w * 0.2f);
  path.lineTo(cx + w * 0.45f, cy + w * 0.35f);
  path.lineTo(cx - w * 0.45f, cy + w * 0.35f);
  path.close();

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw JS/script icon.
void DrawJsIcon(gfx::Canvas* canvas,
                const gfx::Rect& bounds,
                SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) / 2 - 2;

  SkPath path;
  path.moveTo(cx - w * 0.5f, cy - w);
  path.quadTo(cx - w * 1.2f, cy - w, cx - w * 1.2f, cy);
  path.quadTo(cx - w * 1.2f, cy + w, cx - w * 0.5f, cy + w);
  path.moveTo(cx + w * 0.5f, cy - w);
  path.quadTo(cx + w * 1.2f, cy - w, cx + w * 1.2f, cy);
  path.quadTo(cx + w * 1.2f, cy + w, cx + w * 0.5f, cy + w);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw image/picture icon.
void DrawImageIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int left = bounds.x() + 2;
  int top = bounds.y() + 2;
  int w = bounds.width() - 4;
  int h = bounds.height() - 4;

  SkPath path;
  path.addRect(left, top, left + w, top + h);
  path.moveTo(left + w * 0.2f, top + h * 0.7f);
  path.lineTo(left + w * 0.4f, top + h * 0.4f);
  path.lineTo(left + w * 0.6f, top + h * 0.6f);
  path.lineTo(left + w * 0.8f, top + h * 0.3f);
  path.lineTo(left + w * 0.8f, top + h * 0.8f);
  path.lineTo(left + w * 0.2f, top + h * 0.8f);
  path.close();

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw popup/window icon.
void DrawPopupIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int left = bounds.x() + 2;
  int top = bounds.y() + 3;
  int w = bounds.width() - 4;
  int h = bounds.height() - 6;

  SkPath path;
  path.addRect(left, top, left + w, top + h);
  path.moveTo(left, top + h * 0.25f);
  path.lineTo(left + w, top + h * 0.25f);
  path.addCircle(left + w * 0.1f, top + h * 0.12f, 1.5f);
  path.addCircle(left + w * 0.2f, top + h * 0.12f, 1.5f);
  path.addCircle(left + w * 0.3f, top + h * 0.12f, 1.5f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.2f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw sound/speaker icon.
void DrawSoundIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int left = bounds.x() + 1;
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) - 2;

  SkPath path;
  path.moveTo(left + w * 0.1f, cy - w * 0.25f);
  path.lineTo(left + w * 0.35f, cy - w * 0.25f);
  path.lineTo(left + w * 0.55f, cy - w * 0.4f);
  path.lineTo(left + w * 0.55f, cy + w * 0.4f);
  path.lineTo(left + w * 0.35f, cy + w * 0.25f);
  path.lineTo(left + w * 0.1f, cy + w * 0.25f);
  path.close();
  path.moveTo(left + w * 0.65f, cy - w * 0.25f);
  path.quadTo(left + w * 0.85f, cy, left + w * 0.65f, cy + w * 0.25f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw ad/megaphone icon.
void DrawAdIcon(gfx::Canvas* canvas,
                const gfx::Rect& bounds,
                SkColor color) {
  int left = bounds.x() + 2;
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) - 4;

  SkPath path;
  path.moveTo(left, cy - w * 0.3f);
  path.lineTo(left + w * 0.6f, cy - w * 0.4f);
  path.lineTo(left + w * 0.6f, cy + w * 0.4f);
  path.lineTo(left, cy + w * 0.3f);
  path.close();
  path.moveTo(left + w * 0.15f, cy + w * 0.3f);
  path.lineTo(left + w * 0.15f, cy + w * 0.45f);
  path.quadTo(left + w * 0.3f, cy + w * 0.5f, left + w * 0.4f, cy + w * 0.45f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw sync/refresh icon.
void DrawSyncIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int r = std::min(bounds.width(), bounds.height()) / 2 - 2;

  SkPath path;
  path.addArc(SkRect::MakeLTRB(cx - r, cy - r, cx + r, cy + r),
              -30, 240);
  path.moveTo(cx + r * 0.8f, cy - r * 0.5f);
  path.lineTo(cx + r * 0.6f, cy - r * 0.7f);
  path.lineTo(cx + r * 0.95f, cy - r * 0.8f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw download icon.
void DrawDownloadIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) - 4;

  SkPath path;
  path.moveTo(cx, cy - w * 0.4f);
  path.lineTo(cx, cy + w * 0.3f);
  path.moveTo(cx - w * 0.35f, cy);
  path.lineTo(cx, cy + w * 0.4f);
  path.lineTo(cx + w * 0.35f, cy);
  path.moveTo(cx - w * 0.4f, cy + w * 0.4f);
  path.lineTo(cx - w * 0.4f, cy + w * 0.45f);
  path.lineTo(cx + w * 0.4f, cy + w * 0.45f);
  path.lineTo(cx + w * 0.4f, cy + w * 0.4f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw USB icon.
void DrawUsbIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int cx = bounds.CenterPoint().x();
  int top = bounds.y() + 2;
  int w = std::min(bounds.width(), bounds.height()) - 4;

  SkPath path;
  path.moveTo(cx, top);
  path.lineTo(cx, top + w - 4);
  path.moveTo(cx - w * 0.35f, top + w * 0.3f);
  path.lineTo(cx - w * 0.35f, top);
  path.moveTo(cx + w * 0.35f, top + w * 0.3f);
  path.lineTo(cx + w * 0.35f, top);
  path.moveTo(cx - w * 0.35f, top + w * 0.3f);
  path.lineTo(cx + w * 0.35f, top + w * 0.3f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw Bluetooth icon.
void DrawBluetoothIcon(gfx::Canvas* canvas,
                       const gfx::Rect& bounds,
                       SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int h = std::min(bounds.width(), bounds.height()) - 4;

  SkPath path;
  path.moveTo(cx, cy - h / 2);
  path.lineTo(cx + h * 0.3f, cy - h * 0.35f);
  path.lineTo(cx - h * 0.1f, cy);
  path.lineTo(cx + h * 0.3f, cy + h * 0.35f);
  path.lineTo(cx, cy + h / 2);
  path.moveTo(cx, cy - h / 2);
  path.lineTo(cx - h * 0.4f, cy - h * 0.1f);
  path.moveTo(cx, cy + h / 2);
  path.lineTo(cx - h * 0.4f, cy + h * 0.1f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw zoom/magnifier icon.
void DrawZoomIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.CenterPoint().x() - 2;
  int cy = bounds.CenterPoint().y() - 2;
  int r = std::min(bounds.width(), bounds.height()) / 3;

  SkPath path;
  path.addCircle(cx, cy, r);
  path.moveTo(cx + r * 0.7f, cy + r * 0.7f);
  path.lineTo(cx + r * 1.5f, cy + r * 1.5f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw more/3-dots icon.
void DrawMoreIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  float r = 1.5f;
  float spacing = 4;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  canvas->DrawCircle(gfx::Point(cx, cy - spacing), r, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), r, flags);
  canvas->DrawCircle(gfx::Point(cx, cy + spacing), r, flags);
}

// Draw back/chevron-left icon.
void DrawBackIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) / 2 - 1;

  SkPath path;
  path.moveTo(cx + w, cy - w * 0.8f);
  path.lineTo(cx - w * 0.3f, cy);
  path.lineTo(cx + w, cy + w * 0.8f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.8f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw search icon.
void DrawSearchIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.CenterPoint().x() - 2;
  int cy = bounds.CenterPoint().y() - 2;
  int r = std::min(bounds.width(), bounds.height()) / 3;

  SkPath path;
  path.addCircle(cx, cy, r);
  path.moveTo(cx + r * 0.7f, cy + r * 0.7f);
  path.lineTo(cx + r * 1.5f, cy + r * 1.5f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw file/document icon.
void DrawFileIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int left = bounds.x() + 3;
  int top = bounds.y() + 2;
  int w = bounds.width() - 6;
  int h = bounds.height() - 4;

  SkPath path;
  path.moveTo(left, top);
  path.lineTo(left + w * 0.7f, top);
  path.lineTo(left + w, top + h * 0.3f);
  path.lineTo(left + w, top + h);
  path.lineTo(left, top + h);
  path.close();
  path.moveTo(left + w * 0.7f, top);
  path.lineTo(left + w * 0.7f, top + h * 0.3f);
  path.lineTo(left + w, top + h * 0.3f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.2f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw serial/port icon.
void DrawSerialIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) - 4;

  SkPath path;
  path.addRect(cx - w / 2, cy - w * 0.25f, cx + w / 2, cy + w * 0.25f);
  for (int i = 0; i < 4; i++) {
    float x = cx - w * 0.3f + i * w * 0.2f;
    path.addCircle(x, cy - w * 0.08f, 1.5f);
    path.addCircle(x, cy + w * 0.08f, 1.5f);
  }

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.2f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw HID/device icon.
void DrawHidIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) - 4;

  SkPath path;
  path.addOval(SkRect::MakeLTRB(cx - w * 0.35f, cy - w * 0.45f,
                                 cx + w * 0.35f, cy + w * 0.45f));
  path.moveTo(cx, cy - w * 0.4f);
  path.lineTo(cx, cy - w * 0.15f);
  path.addCircle(cx, cy + w * 0.1f, w * 0.06f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw screen/monitor icon.
void DrawScreenIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int left = bounds.x() + 2;
  int top = bounds.y() + 2;
  int w = bounds.width() - 4;
  int h = bounds.height() - 4;

  SkPath path;
  path.addRect(left, top, left + w, top + h * 0.75f);
  path.moveTo(left + w * 0.35f, top + h * 0.75f);
  path.lineTo(left + w * 0.65f, top + h * 0.75f);
  path.moveTo(left + w * 0.5f, top + h * 0.75f);
  path.lineTo(left + w * 0.5f, top + h * 0.9f);
  path.lineTo(left + w * 0.3f, top + h * 0.9f);
  path.lineTo(left + w * 0.7f, top + h * 0.9f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.2f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw MIDI/music note icon.
void DrawMidiIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) - 4;

  SkPath path;
  path.moveTo(cx + w * 0.1f, cy + w * 0.3f);
  path.lineTo(cx + w * 0.1f, cy - w * 0.4f);
  path.lineTo(cx + w * 0.4f, cy - w * 0.5f);
  path.lineTo(cx + w * 0.4f, cy + w * 0.2f);
  path.addEllipse(SkRect::MakeLTRB(cx - w * 0.05f, cy + w * 0.2f,
                                    cx + w * 0.25f, cy + w * 0.45f));
  path.addEllipse(SkRect::MakeLTRB(cx + w * 0.25f, cy + w * 0.1f,
                                    cx + w * 0.55f, cy + w * 0.35f));

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.2f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw VR/headset icon.
void DrawVrIcon(gfx::Canvas* canvas,
                const gfx::Rect& bounds,
                SkColor color) {
  int left = bounds.x() + 1;
  int cy = bounds.CenterPoint().y();
  int w = bounds.width() - 2;
  int h = std::min(bounds.height(), w * 0.5f) - 2;

  SkPath path;
  path.addRoundRect(
      SkRect::MakeLTRB(left, cy - h / 2, left + w, cy + h / 2),
      4, 4);
  path.addOval(SkRect::MakeLTRB(left + w * 0.2f, cy - h * 0.35f,
                                 left + w * 0.45f, cy + h * 0.35f));
  path.addOval(SkRect::MakeLTRB(left + w * 0.55f, cy - h * 0.35f,
                                 left + w * 0.8f, cy + h * 0.35f));

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.2f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw sensors/radar icon.
void DrawSensorsIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int r = std::min(bounds.width(), bounds.height()) / 2 - 2;

  SkPath path;
  path.addCircle(cx, cy, r);
  path.addCircle(cx, cy, r * 0.65f);
  path.addCircle(cx, cy, r * 0.3f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.0f);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Format storage size.
std::u16string FormatStorage(int64_t bytes) {
  if (bytes < 1024) {
    return base::UTF8ToUTF16(base::NumberToString(bytes) + " B");
  }
  if (bytes < 1024 * 1024) {
    return base::UTF8ToUTF16(
        base::NumberToString(bytes / 1024) + " KB");
  }
  if (bytes < 1024 * 1024 * 1024) {
    return base::UTF8ToUTF16(
        base::NumberToString(bytes / (1024 * 1024)) + " MB");
  }
  return base::UTF8ToUTF16(
      base::NumberToString(bytes / (1024 * 1024 * 1024)) + " GB");
}

// Count non-default allowed permissions.
size_t CountAllowedPermissions(const AstraSiteSettingsEntry& site) {
  size_t count = 0;
  for (const auto& p : site.permissions) {
    if (p.setting == AstraContentSetting::kAllow) count++;
  }
  return count;
}

}  // namespace

// ===========================================================================
// AstraSiteRowView
// ===========================================================================

AstraSiteRowView::AstraSiteRowView(const AstraSiteSettingsEntry& site)
    : site_id_(site.id),
      display_name_(site.display_name),
      origin_(base::UTF8ToUTF16(site.origin)) {
  BuildUI();
  Update(site);
}

AstraSiteRowView::~AstraSiteRowView() = default;

void AstraSiteRowView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets::VH(8, 12),
      12));
  SetPreferredSize(gfx::Size(0, kSiteRowHeight));

  // Icon.
  icon_view_ = AddChildView(std::make_unique<views::ImageView>());
  icon_view_->SetPreferredSize(gfx::Size(kSiteRowIconSize, kSiteRowIconSize));
  icon_view_->SetImage(
      gfx::CreateVectorIcon(
          base::BindRepeating(&AstraSiteRowView::DrawSiteIcon,
                              base::Unretained(this)),
          gfx::Size(kSiteRowIconSize, kSiteRowIconSize)));

  // Text area (name + origin + permissions).
  auto* text_container = AddChildView(std::make_unique<views::View>());
  text_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 2));
  text_container->SetProperty(views::kFlexBehaviorKey,
                              views::FlexSpecification(
                                  views::LayoutFlexOrientation::kHorizontal,
                                  views::MinimumFlexSizeRule::kScaleToZero,
                                  views::MaximumFlexSizeRule::kUnbounded));

  name_label_ = text_container->AddChildView(
      std::make_unique<views::Label>());
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_PRIMARY));

  origin_label_ = text_container->AddChildView(
      std::make_unique<views::Label>());
  origin_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  origin_label_->SetAutoColorReadabilityEnabled(false);
  origin_label_->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));
  origin_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  permissions_label_ = text_container->AddChildView(
      std::make_unique<views::Label>());
  permissions_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  permissions_label_->SetAutoColorReadabilityEnabled(false);
  permissions_label_->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));
  permissions_label_->SetElideBehavior(gfx::ELIDE_END);

  // Right side: storage + more button.
  auto* right_container = AddChildView(std::make_unique<views::View>());
  right_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 8));
  right_container->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  storage_label_ = right_container->AddChildView(
      std::make_unique<views::Label>());
  storage_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  storage_label_->SetAutoColorReadabilityEnabled(false);
  storage_label_->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));

  more_button_ = right_container->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating([]() {})));
  more_button_->SetPreferredSize(gfx::Size(kIconSize, kIconSize));
  more_button_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(
          base::BindRepeating(&DrawMoreIcon),
          gfx::Size(kIconSize, kIconSize)));
}

void AstraSiteRowView::Update(const AstraSiteSettingsEntry& site) {
  site_id_ = site.id;
  display_name_ = site.display_name;
  origin_ = base::UTF8ToUTF16(site.origin);

  name_label_->SetText(site.display_name);
  origin_label_->SetText(base::UTF8ToUTF16(site.origin));
  storage_label_->SetText(FormatStorage(site.storage_bytes));

  size_t allowed = CountAllowedPermissions(site);
  std::u16string perm_text;
  if (allowed > 0) {
    perm_text = base::UTF8ToUTF16(
        base::NumberToString(allowed) + " permission" +
        (allowed > 1 ? "s" : "") + " allowed");
  } else {
    perm_text = u"No special permissions";
  }
  if (site.cookies_count > 0) {
    perm_text += u"  \u2022  " + base::UTF8ToUTF16(
        base::NumberToString(site.cookies_count) + " cookies");
  }
  permissions_label_->SetText(perm_text);
}

void AstraSiteRowView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  SkColor fg = cp->GetColor(ui::kColorLabelForeground);
  SkColor secondary = cp->GetColor(ui::kColorLabelForegroundSecondary);

  name_label_->SetEnabledColor(fg);
  origin_label_->SetEnabledColor(secondary);
  permissions_label_->SetEnabledColor(secondary);
  storage_label_->SetEnabledColor(secondary);
}

void AstraSiteRowView::DrawSiteIcon(gfx::Canvas* canvas,
                                     const gfx::Rect& bounds) {
  SkColor color = GetColorProvider()
      ? GetColorProvider()->GetColor(ui::kColorIcon) : SK_ColorGRAY;
  DrawGlobeIcon(canvas, bounds, color);
}

void AstraSiteRowView::DrawFaviconFallback(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds) {
  DrawSiteIcon(canvas, bounds);
}

// ===========================================================================
// AstraSiteSettingsCategoryView
// ===========================================================================

AstraSiteSettingsCategoryView::AstraSiteSettingsCategoryView(
    AstraSiteSettingsCategory category,
    const std::u16string& name,
    bool is_active)
    : category_(category), name_(name), is_active_(is_active) {
  BuildUI();
}

AstraSiteSettingsCategoryView::~AstraSiteSettingsCategoryView() = default;

void AstraSiteSettingsCategoryView::SetActive(bool active) {
  if (is_active_ == active) return;
  is_active_ = active;
  SchedulePaint();
}

void AstraSiteSettingsCategoryView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, 12), 10));
  SetPreferredSize(gfx::Size(0, kSidebarItemHeight));
  SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kCenter);

  icon_view_ = AddChildView(std::make_unique<views::ImageView>());
  icon_view_->SetPreferredSize(gfx::Size(16, 16));
  icon_view_->SetImage(
      gfx::CreateVectorIcon(
          base::BindRepeating(
              &AstraSiteSettingsCategoryView::DrawIcon,
              base::Unretained(this)),
          gfx::Size(16, 16)));

  label_ = AddChildView(std::make_unique<views::Label>(name_));
  label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label_->SetAutoColorReadabilityEnabled(false);
  label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraSiteSettingsCategoryView::DrawIcon(gfx::Canvas* canvas,
                                             const gfx::Rect& bounds) {
  SkColor color = GetColorProvider()
      ? GetColorProvider()->GetColor(
          is_active_ ? ui::kColorIcon : ui::kColorIconSecondary)
      : SK_ColorGRAY;
  AstraSiteSettingsPageView::DrawCategoryIcon(canvas, bounds, category_,
                                              color);
}

void AstraSiteSettingsCategoryView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  SkColor fg = is_active_
      ? cp->GetColor(ui::kColorButtonForeground)
      : cp->GetColor(ui::kColorLabelForeground);
  SkColor bg = is_active_
      ? cp->GetColor(ui::kColorButtonBackgroundProminent)
      : SK_ColorTRANSPARENT;

  SetBackground(views::CreateRoundedRectBackground(bg, 6));
  label_->SetEnabledColor(fg);
  SchedulePaint();
}

// ===========================================================================
// Static: DrawCategoryIcon
// ===========================================================================

void AstraSiteSettingsPageView::DrawCategoryIcon(
    gfx::Canvas* canvas,
    const gfx::Rect& bounds,
    AstraSiteSettingsCategory category,
    SkColor color) {
  switch (category) {
    case AstraSiteSettingsCategory::kAll:
    case AstraSiteSettingsCategory::kAllPermissions:
    case AstraSiteSettingsCategory::kAdditionalPermissions:
      DrawGlobeIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kRecent:
      DrawClockIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kCookies:
      DrawCookieIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kLocation:
      DrawLocationIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kCamera:
      DrawCameraIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kMicrophone:
      DrawMicIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kNotifications:
      DrawBellIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kJavaScript:
      DrawJsIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kImages:
      DrawImageIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kPopups:
      DrawPopupIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kSound:
      DrawSoundIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kAds:
      DrawAdIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kBackgroundSync:
      DrawSyncIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kAutomaticDownloads:
      DrawDownloadIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kMidi:
      DrawMidiIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kSerialPorts:
      DrawSerialIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kUsbDevices:
      DrawUsbIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kBluetoothDevices:
      DrawBluetoothIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kHidDevices:
      DrawHidIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kZoomLevels:
      DrawZoomIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kPdfDocuments:
      DrawFileIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kProtectedContent:
      DrawShieldIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kInsecureContent:
      DrawScreenIcon(canvas, bounds, color);
      break;
    case AstraSiteSettingsCategory::kNfcDevices:
    case AstraSiteSettingsCategory::kUnsandboxedPlugins:
    case AstraSiteSettingsCategory::kHandlers:
    case AstraSiteSettingsCategory::kPermissions:
    default:
      DrawMoreIcon(canvas, bounds, color);
      break;
  }
}

// ===========================================================================
// AstraSiteSettingsPageView
// ===========================================================================

AstraSiteSettingsPageView::AstraSiteSettingsPageView(
    AstraSiteSettingsModel* model)
    : model_(model) {
  BuildUI();
  if (model_) {
    scoped_observation_.Observe(model_);
  }
  RefreshFromModel();
}

AstraSiteSettingsPageView::~AstraSiteSettingsPageView() = default;

void AstraSiteSettingsPageView::SetModel(AstraSiteSettingsModel* model) {
  if (model_ == model) return;
  scoped_observation_.Reset();
  model_ = model;
  if (model_) {
    scoped_observation_.Observe(model_);
  }
  RefreshFromModel();
}

void AstraSiteSettingsPageView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Header bar.
  BuildHeader();

  // Main content area: sidebar + content.
  auto* main = AddChildView(std::make_unique<views::View>());
  main->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal));
  main->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded,
          /*adjust_height_for_width=*/false));

  // Sidebar (inside main).
  sidebar_view_ = main->AddChildView(std::make_unique<views::View>());
  sidebar_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(8, 8), 2));
  sidebar_view_->SetPreferredSize(gfx::Size(kSidebarWidth, 0));
  sidebar_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded,
          /*adjust_height_for_width=*/false));
  sidebar_ = sidebar_view_;

  // Content area (inside main).
  content_view_ = main->AddChildView(std::make_unique<views::View>());
  content_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  content_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kBoth,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Content header with title + sort + filter.
  content_header_ = content_view_->AddChildView(
      std::make_unique<views::View>());
  content_header_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, kContentPadding), 12));
  content_header_->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  header_label_ = content_header_->AddChildView(
      std::make_unique<views::Label>(u"Sites"));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetFontList(
      views::style::GetFont(views::style::CONTEXT_DIALOG_TITLE,
                            views::style::STYLE_PRIMARY));
  header_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Sort combobox.
  auto sort_model = std::make_unique<ui::SimpleComboboxModel>(
      std::vector<ui::SimpleComboboxModel::Item>{
          ui::SimpleComboboxModel::Item(u"Sort: Most visited"),
          ui::SimpleComboboxModel::Item(u"Sort: Name"),
          ui::SimpleComboboxModel::Item(u"Sort: Storage"),
          ui::SimpleComboboxModel::Item(u"Sort: Last visited"),
          ui::SimpleComboboxModel::Item(u"Sort: Permissions"),
      });
  sort_combobox_ = content_header_->AddChildView(
      std::make_unique<views::Combobox>(std::move(sort_model)));
  sort_combobox_->SetCallback(base::BindRepeating(
      &AstraSiteSettingsPageView::OnSortChanged,
      base::Unretained(this)));

  // Filter combobox.
  auto filter_model = std::make_unique<ui::SimpleComboboxModel>(
      std::vector<ui::SimpleComboboxModel::Item>{
          ui::SimpleComboboxModel::Item(u"All sites"),
          ui::SimpleComboboxModel::Item(u"Allowed"),
          ui::SimpleComboboxModel::Item(u"Blocked"),
          ui::SimpleComboboxModel::Item(u"With data"),
      });
  filter_combobox_ = content_header_->AddChildView(
      std::make_unique<views::Combobox>(std::move(filter_model)));
  filter_combobox_->SetCallback(base::BindRepeating(
      &AstraSiteSettingsPageView::OnFilterChanged,
      base::Unretained(this)));

  // Scroll view for site list.
  scroll_view_ = content_view_->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kBoth,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
  scroll_view_->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);

  site_list_view_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  site_list_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  site_list_ = site_list_view_;

  // Empty state.
  empty_view_ = content_view_->AddChildView(
      std::make_unique<views::View>());
  empty_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(40, 20), 12));
  empty_view_->SetVisible(false);

  auto* empty_icon = empty_view_->AddChildView(
      std::make_unique<views::ImageView>());
  empty_icon->SetPreferredSize(gfx::Size(48, 48));
  empty_icon->SetImage(
      gfx::CreateVectorIcon(
          base::BindRepeating(&DrawGlobeIcon),
          gfx::Size(48, 48)));

  auto* empty_label = empty_view_->AddChildView(
      std::make_unique<views::Label>(u"No sites found"));
  empty_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  empty_label->SetAutoColorReadabilityEnabled(false);
  empty_label->SetFontList(
      views::style::GetFont(views::style::CONTEXT_DIALOG_TITLE,
                            views::style::STYLE_PRIMARY));

  auto* empty_sub = empty_view_->AddChildView(
      std::make_unique<views::Label>(
          u"Sites you visit will appear here"));
  empty_sub->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  empty_sub->SetAutoColorReadabilityEnabled(false);
}

void AstraSiteSettingsPageView::BuildHeader() {
  auto* header = AddChildView(std::make_unique<views::View>());
  header->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, kContentPadding), 12));
  header->SetPreferredSize(gfx::Size(0, kHeaderHeight));
  header->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  header->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SkColorSetA(SK_ColorBLACK, 0x10)));

  back_button_ = header->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraSiteSettingsPageView::OnBackClicked,
                              base::Unretained(this))));
  back_button_->SetPreferredSize(gfx::Size(kIconSize, kIconSize));
  back_button_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(base::BindRepeating(&DrawBackIcon),
                            gfx::Size(kIconSize, kIconSize)));
  back_button_->SetTooltipText(u"Back");

  title_label_ = header->AddChildView(
      std::make_unique<views::Label>(u"Site settings"));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(views::style::CONTEXT_DIALOG_TITLE,
                            views::style::STYLE_PRIMARY));
  title_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Search field.
  search_field_ = header->AddChildView(
      std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"Search sites");
  search_field_->SetPreferredSize(gfx::Size(280, 32));
  search_field_->set_controller(
      base::BindRepeating(&AstraSiteSettingsPageView::OnSearchChanged,
                          base::Unretained(this)));

  more_button_ = header->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating([]() {})));
  more_button_->SetPreferredSize(gfx::Size(kIconSize, kIconSize));
  more_button_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(base::BindRepeating(&DrawMoreIcon),
                            gfx::Size(kIconSize, kIconSize)));
}

void AstraSiteSettingsPageView::RefreshFromModel() {
  if (!model_) return;

  UpdateSidebar();
  UpdateHeader();
  UpdateSiteList();
}

void AstraSiteSettingsPageView::UpdateSidebar() {
  if (!sidebar_view_) return;

  sidebar_view_->RemoveAllChildViews();

  // Build sidebar categories.
  auto categories = AstraSiteSettingsModel::GetCategories();
  for (const auto& [cat, name] : categories) {
    bool is_active = model_ && model_->GetCategory() == cat;
    auto* item = sidebar_view_->AddChildView(
        std::make_unique<AstraSiteSettingsCategoryView>(cat, name, is_active));
    // Set click handler.
    item->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  }
}

void AstraSiteSettingsPageView::UpdateHeader() {
  if (!model_ || !header_label_) return;

  size_t count = model_->GetFilteredSites().size();
  std::u16string text = base::UTF8ToUTF16(
      base::NumberToString(count) + " site" + (count != 1 ? "s" : ""));
  header_label_->SetText(text);
}

void AstraSiteSettingsPageView::UpdateSiteList() {
  if (!site_list_view_ || !model_) return;

  site_list_view_->RemoveAllChildViews();

  auto sites = model_->GetFilteredSites();

  if (sites.empty()) {
    site_list_view_->SetVisible(false);
    empty_view_->SetVisible(true);
    return;
  }

  site_list_view_->SetVisible(true);
  empty_view_->SetVisible(false);

  for (const auto& site : sites) {
    site_list_view_->AddChildView(
        std::make_unique<AstraSiteRowView>(site));
  }

  InvalidateLayout();
}

// -- Observer callbacks -----------------------------------------------------

void AstraSiteSettingsPageView::OnSitesChanged(
    AstraSiteSettingsModel* model) {
  UpdateHeader();
  UpdateSiteList();
}

void AstraSiteSettingsPageView::OnCategoryChanged(
    AstraSiteSettingsModel* model,
    AstraSiteSettingsCategory category) {
  UpdateSidebar();
  UpdateHeader();
  UpdateSiteList();
}

void AstraSiteSettingsPageView::OnSearchQueryChanged(
    AstraSiteSettingsModel* model,
    const std::string& query) {
  UpdateHeader();
  UpdateSiteList();
}

void AstraSiteSettingsPageView::OnSitePermissionChanged(
    AstraSiteSettingsModel* model,
    const std::string& site_id,
    AstraSitePermissionType type,
    AstraContentSetting setting) {
  UpdateSiteList();
}

void AstraSiteSettingsPageView::OnSiteSettingsModelShutdown(
    AstraSiteSettingsModel* model) {
  model_ = nullptr;
  scoped_observation_.Reset();
}

void AstraSiteSettingsPageView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  SkColor fg = cp->GetColor(ui::kColorLabelForeground);
  if (title_label_) title_label_->SetEnabledColor(fg);
  if (header_label_) header_label_->SetEnabledColor(fg);

  // Update sidebar background with separator.
  if (sidebar_view_) {
    sidebar_view_->SetBackground(views::CreateSolidBackground(
        cp->GetColor(ui::kColorDialogBackground)));
    sidebar_view_->SetBorder(views::CreateSolidSidedBorder(
        0, 0, 0, 1,
        SkColorSetA(SK_ColorBLACK, 0x10)));
  }
}

// -- Event handlers ---------------------------------------------------------

void AstraSiteSettingsPageView::OnSearchChanged() {
  if (!model_) return;
  model_->SetSearchQuery(base::UTF16ToUTF8(search_field_->GetText()));
}

void AstraSiteSettingsPageView::OnSortChanged() {
  if (!model_ || !sort_combobox_) return;
  int idx = sort_combobox_->GetSelectedIndex().value_or(0);
  AstraSiteSettingsSort sort = AstraSiteSettingsSort::kMostVisited;
  switch (idx) {
    case 0: sort = AstraSiteSettingsSort::kMostVisited; break;
    case 1: sort = AstraSiteSettingsSort::kName; break;
    case 2: sort = AstraSiteSettingsSort::kStorage; break;
    case 3: sort = AstraSiteSettingsSort::kLastVisited; break;
    case 4: sort = AstraSiteSettingsSort::kPermissionCount; break;
  }
  model_->SetSort(sort);
}

void AstraSiteSettingsPageView::OnFilterChanged() {
  if (!model_ || !filter_combobox_) return;
  int idx = filter_combobox_->GetSelectedIndex().value_or(0);
  AstraSiteSettingsFilter filter = AstraSiteSettingsFilter::kAll;
  switch (idx) {
    case 0: filter = AstraSiteSettingsFilter::kAll; break;
    case 1: filter = AstraSiteSettingsFilter::kAllowed; break;
    case 2: filter = AstraSiteSettingsFilter::kBlocked; break;
    case 3: filter = AstraSiteSettingsFilter::kWithData; break;
  }
  model_->SetFilter(filter);
}

void AstraSiteSettingsPageView::OnCategoryClicked(
    AstraSiteSettingsCategory category) {
  if (model_) {
    model_->SetCategory(category);
  }
}

void AstraSiteSettingsPageView::OnBackClicked() {
  // TODO(astra): Navigate back to settings main page.
  //   Chromium owner: chrome/browser/ui/webui/settings/settings_ui.h
}

void AstraSiteSettingsPageView::OnResetPermissionsClicked() {
  // TODO(astra): Reset permissions for selected site.
}

void AstraSiteSettingsPageView::OnClearDataClicked() {
  // TODO(astra): Clear site data via BrowsingDataRemover.
  //   Chromium owner: chrome/browser/browsing_data/browsing_data_remover.h
}

void AstraSiteSettingsPageView::BuildSidebar() {}
void AstraSiteSettingsPageView::BuildContent() {}
void AstraSiteSettingsPageView::BuildSiteList() {}
void AstraSiteSettingsPageView::BuildEmptyState() {}

}  // namespace astra
