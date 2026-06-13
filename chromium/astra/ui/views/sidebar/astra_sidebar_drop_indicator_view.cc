#include "astra/ui/views/sidebar/astra_sidebar_drop_indicator_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "base/check.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Astra color ID for the drag-and-drop indicator line/highlight.
// Uses the workspace accent color for a strong visual indicator.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kDropIndicatorColorId = kColorAstraWorkspaceAccent;

}  // namespace

AstraSidebarDropIndicatorView::AstraSidebarDropIndicatorView() {
  SetVisible(false);
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
}

AstraSidebarDropIndicatorView::~AstraSidebarDropIndicatorView() = default;

void AstraSidebarDropIndicatorView::SetMode(Mode mode) {
  if (mode_ == mode) {
    return;
  }
  mode_ = mode;
  SchedulePaint();
}

void AstraSidebarDropIndicatorView::ShowAtPosition(int y) {
  DCHECK(parent());

  // The indicator spans the full width of the parent.
  int width = parent()->width();
  int line_y = y - kLineThickness / 2;

  SetBounds(0, line_y, width, kLineThickness);
  SetVisible(true);
  SchedulePaint();
}

void AstraSidebarDropIndicatorView::ShowHighlight(const gfx::Rect& bounds) {
  SetMode(Mode::kHighlight);
  SetBounds(bounds);
  SetVisible(true);
  SchedulePaint();
}

void AstraSidebarDropIndicatorView::HideIndicator() {
  if (!GetVisible()) {
    return;
  }
  SetVisible(false);
}

void AstraSidebarDropIndicatorView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor color = color_provider->GetColor(kDropIndicatorColorId);

  switch (mode_) {
    case Mode::kInsertLine: {
      // Draw a solid horizontal line across the full width.
      gfx::Rect line_bounds = GetLocalBounds();
      cc::PaintFlags flags;
      flags.setColor(color);
      flags.setStyle(cc::PaintFlags::kFill_Style);
      flags.setAntiAlias(true);
      canvas->DrawRoundRect(line_bounds, kLineThickness / 2.0f, flags);
      break;
    }
    case Mode::kHighlight: {
      // Draw a rounded rect outline around the bounds.
      // TODO(astra): Adjust the highlight style to match Astra's visual
      // design language. For now, use a 2px stroke with the accent color.
      gfx::Rect bounds = GetLocalBounds();
      cc::PaintFlags flags;
      flags.setColor(color);
      flags.setStyle(cc::PaintFlags::kStroke_Style);
      flags.setStrokeWidth(2);
      flags.setAntiAlias(true);
      canvas->DrawRoundRect(bounds, kHighlightCornerRadius, flags);
      break;
    }
  }
}

}  // namespace astra
