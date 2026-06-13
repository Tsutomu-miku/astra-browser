#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DROP_INDICATOR_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DROP_INDICATOR_VIEW_H_

#include "ui/views/view.h"

namespace astra {

// A visual drop indicator shown during sidebar drag-and-drop operations.
//
// Renders as a thin horizontal line that appears between items to indicate
// where a dragged item will be inserted. Can also render as a highlight
// around a container (e.g., a folder) to indicate dropping into it.
//
// This is a pure presentation view — it has no knowledge of drag data or
// drop logic. It is positioned and shown/hidden by the drop target view.
//
// TODO(astra): Add a "drop into container" highlight mode in addition to
// the insertion line. This would be used when hovering over a folder to
// indicate the tab will be moved into that folder, rather than inserted
// above/below it.
// TODO(astra): Use Chromium's color system (ui::ColorId + ColorProvider)
// for the indicator color instead of a hardcoded value.
// Chromium color subsystem: ui/color/color_id.h, ui/color/color_provider.h
class AstraSidebarDropIndicatorView : public views::View {
 public:
  // Mode of the drop indicator.
  enum class Mode {
    kInsertLine,   // Thin horizontal line between items (reorder / insert)
    kHighlight,    // Highlight around a container (drop into folder/workspace)
  };

  AstraSidebarDropIndicatorView();
  AstraSidebarDropIndicatorView(const AstraSidebarDropIndicatorView&) = delete;
  AstraSidebarDropIndicatorView& operator=(const AstraSidebarDropIndicatorView&) = delete;
  ~AstraSidebarDropIndicatorView() override;

  // Set the indicator mode (insertion line or highlight).
  void SetMode(Mode mode);

  // Show the indicator at the given vertical position (in parent coords),
  // spanning the full width of the parent.
  void ShowAtPosition(int y);

  // Show the indicator as a highlight around the given bounds (in parent coords).
  void ShowHighlight(const gfx::Rect& bounds);

  // Hide the indicator.
  void HideIndicator();

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  Mode mode_ = Mode::kInsertLine;

  // Thickness of the insertion line in DIPs.
  static constexpr int kLineThickness = 2;

  // Corner radius for the highlight mode.
  static constexpr int kHighlightCornerRadius = 6;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_DROP_INDICATOR_VIEW_H_
