// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/title_bar/astra_title_bar_view.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/base/ime/input_method.h"
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
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Draw a simple minimize icon (a horizontal line).
void DrawMinimizeIcon(gfx::Canvas* canvas, const gfx::Rect& bounds, SkColor color) {
  int line_width = bounds.width() * 0.6;
  int line_x = bounds.x() + (bounds.width() - line_width) / 2;
  int line_y = bounds.y() + bounds.height() / 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  canvas->DrawLine(gfx::Point(line_x, line_y),
                   gfx::Point(line_x + line_width, line_y), flags);
}

// Draw a maximize icon (a square outline).
void DrawMaximizeIcon(gfx::Canvas* canvas, const gfx::Rect& bounds, SkColor color) {
  int square_size = bounds.width() * 0.5;
  int square_x = bounds.x() + (bounds.width() - square_size) / 2;
  int square_y = bounds.y() + (bounds.height() - square_size) / 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);
  canvas->DrawRect(gfx::Rect(square_x, square_y, square_size, square_size), flags);
}

// Draw a restore icon (two overlapping squares).
void DrawRestoreIcon(gfx::Canvas* canvas, const gfx::Rect& bounds, SkColor color) {
  // Simplified restore icon: two overlapping rectangles.
  int w = bounds.width() * 0.45;
  int h = bounds.height() * 0.45;

  // Back square (top-left).
  int back_x = bounds.x() + (bounds.width() - w) / 2 - w * 0.2;
  int back_y = bounds.y() + (bounds.height() - h) / 2 - h * 0.2;

  // Front square (bottom-right).
  int front_x = bounds.x() + (bounds.width() - w) / 2 + w * 0.2;
  int front_y = bounds.y() + (bounds.height() - h) / 2 + h * 0.2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  // Draw back square (partially hidden).
  canvas->DrawRect(gfx::Rect(back_x, back_y, w, h), flags);

  // Draw front square (filled with background, then stroked).
  flags.setStyle(cc::PaintFlags::kFill_Style);
  // Use background color for fill — approximate with white/transparent.
  flags.setAlpha(0);  // Transparent fill
  canvas->DrawRect(gfx::Rect(front_x, front_y, w, h), flags);

  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAlpha(255);
  canvas->DrawRect(gfx::Rect(front_x, front_y, w, h), flags);
}

// Draw a close icon (an X).
void DrawCloseIcon(gfx::Canvas* canvas, const gfx::Rect& bounds, SkColor color) {
  int size = bounds.width() * 0.45;
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int half = size / 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);

  canvas->DrawLine(gfx::Point(cx - half, cy - half),
                   gfx::Point(cx + half, cy + half), flags);
  canvas->DrawLine(gfx::Point(cx + half, cy - half),
                   gfx::Point(cx - half, cy + half), flags);
}

}  // namespace

// ===========================================================================
// AstraTitleBarView
// ===========================================================================

AstraTitleBarView::AstraTitleBarView() {
  Build();
}

AstraTitleBarView::AstraTitleBarView(AstraTitleBarDelegate* delegate)
    : delegate_(delegate) {
  Build();
  if (delegate_) {
    SetTitle(delegate_->GetActiveTabTitle());
  }
}

AstraTitleBarView::~AstraTitleBarView() = default;

void AstraTitleBarView::SetTitle(const std::u16string& title) {
  if (title_ == title) {
    return;
  }
  title_ = title;
  if (title_label_) {
    title_label_->SetText(title_);
  }
}

const std::u16string& AstraTitleBarView::GetTitle() const {
  return title_;
}

void AstraTitleBarView::SetAppIcon(const gfx::ImageSkia& icon) {
  if (app_icon_) {
    app_icon_->SetImage(ui::ImageModel::FromImageSkia(icon));
  }
}

void AstraTitleBarView::SetAppIconVisible(bool visible) {
  if (app_icon_visible_ == visible) {
    return;
  }
  app_icon_visible_ = visible;
  if (app_icon_) {
    app_icon_->SetVisible(visible);
  }
  InvalidateLayout();
}

bool AstraTitleBarView::IsAppIconVisible() const {
  return app_icon_visible_;
}

void AstraTitleBarView::SetWorkspaceName(const std::u16string& name) {
  if (workspace_label_) {
    workspace_label_->SetText(name);
  }
}

void AstraTitleBarView::SetWorkspaceColor(SkColor color) {
  workspace_color_ = color;
  if (workspace_indicator_) {
    workspace_indicator_->SchedulePaint();
  }
}

void AstraTitleBarView::SetWorkspaceVisible(bool visible) {
  if (workspace_visible_ == visible) {
    return;
  }
  workspace_visible_ = visible;
  if (workspace_indicator_) {
    workspace_indicator_->SetVisible(visible);
  }
  if (workspace_label_) {
    workspace_label_->SetVisible(visible);
  }
  InvalidateLayout();
}

bool AstraTitleBarView::IsWorkspaceVisible() const {
  return workspace_visible_;
}

void AstraTitleBarView::SetWindowControlsVisible(bool visible) {
  if (window_controls_visible_ == visible) {
    return;
  }
  window_controls_visible_ = visible;
  if (minimize_button_) {
    minimize_button_->SetVisible(visible);
  }
  if (maximize_button_) {
    maximize_button_->SetVisible(visible);
  }
  if (close_button_) {
    close_button_->SetVisible(visible);
  }
  InvalidateLayout();
}

bool AstraTitleBarView::AreWindowControlsVisible() const {
  return window_controls_visible_;
}

void AstraTitleBarView::UpdateMaximizeButton(bool maximized) {
  // The button switches between maximize and restore icons.
  if (maximize_button_) {
    maximize_button_->SetTooltipText(maximized ? u"Restore" : u"Maximize");
    maximize_button_->SetAccessibleName(maximized ? u"Restore window"
                                               : u"Maximize window");
    maximize_button_->SchedulePaint();
  }
}

void AstraTitleBarView::SetTitleBarHeight(int height) {
  if (title_bar_height_ == height) {
    return;
  }
  title_bar_height_ = height;
  InvalidateLayout();
}

void AstraTitleBarView::SetBackgroundColor(SkColor color) {
  background_color_ = color;
  SchedulePaint();
}

void AstraTitleBarView::SetDarkMode(bool dark_mode) {
  if (dark_mode_ == dark_mode) {
    return;
  }
  dark_mode_ = dark_mode;
  UpdateControlIcons();
  SchedulePaint();
}

gfx::Size AstraTitleBarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(0, title_bar_height_);
}

void AstraTitleBarView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int y = bounds.y();
  int height = bounds.height();

  // Window controls on the right.
  if (window_controls_visible_ && close_button_ && maximize_button_ &&
      minimize_button_) {
    // Close button (rightmost).
    int close_x = bounds.right() - kControlButtonSize;
    close_button_->SetBounds(close_x, y, kControlButtonSize, height);

    // Maximize button.
    int max_x = close_x - kControlButtonSize - kControlButtonSpacing;
    maximize_button_->SetBounds(max_x, y, kControlButtonSize, height);

    // Minimize button.
    int min_x = max_x - kControlButtonSize - kControlButtonSpacing;
    minimize_button_->SetBounds(min_x, y, kControlButtonSize, height);
  }

  int left_edge = bounds.x();

  // App icon on the left.
  if (app_icon_visible_ && app_icon_) {
    app_icon_->SetBounds(left_edge + kAppIconPadding,
                        y + (height - kAppIconSize) / 2,
                        kAppIconSize, kAppIconSize);
    left_edge += kAppIconPadding + kAppIconSize + kAppIconPadding;
  }

  // Workspace indicator.
  if (workspace_visible_ && workspace_indicator_) {
    int dot_y = y + (height - kWorkspaceDotSize) / 2;
    workspace_indicator_->SetBounds(left_edge, dot_y,
                                  kWorkspaceDotSize, kWorkspaceDotSize);
    left_edge += kWorkspaceDotSize + kWorkspacePadding;

    if (workspace_label_) {
      int label_width = workspace_label_->GetPreferredSize().width();
      workspace_label_->SetBounds(left_edge, y, label_width, height);
      left_edge += label_width + kWorkspacePadding;
    }
  }

  // Title (between left items and right controls).
  int title_right = bounds.right() - kTitleRightPadding;
  if (window_controls_visible_) {
    title_right -= (kControlButtonSize * 3 + kControlButtonSpacing * 2);
  }

  int title_x = left_edge + kTitleLeftPadding;
  int title_width = title_right - title_x;
  if (title_width > 0 && title_label_) {
    title_label_->SetBounds(title_x, y, title_width, height);
  }
}

void AstraTitleBarView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateControlIcons();
}

void AstraTitleBarView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  // Draw background.
  cc::PaintFlags flags;
  flags.setColor(background_color_);
  canvas->DrawRect(bounds, flags);

  // Draw bottom border.
  flags.setColor(SkColorSetA(SK_ColorBLACK, 0x0D));
  canvas->DrawRect(gfx::Rect(bounds.x(), bounds.bottom() - 1,
                             bounds.width(), 1), flags);

  // Draw window control icons (custom drawing on top of buttons).
  // The buttons are ImageButtons with callback, but we draw the icons
  // manually via OnPaint on each button — simplified here via custom
  // button drawing.
  //
  // TODO(astra): Use proper vector icons from Chromium's icon set
  // instead of drawing them programmatically.
}

bool AstraTitleBarView::OnMousePressed(const ui::MouseEvent& event) {
  // Allow dragging the window by clicking on the title bar.
  // The actual window move is handled by the widget/frame.
  return true;
}

bool AstraTitleBarView::OnMouseDoubleClick(const ui::MouseEvent& event) {
  if (delegate_) {
    delegate_->OnTitleBarDoubleClicked();
  }
  return true;
}

void AstraTitleBarView::Build() {
  // App icon.
  auto app_icon = std::make_unique<views::ImageView>();
  app_icon->SetImageSize(gfx::Size(kAppIconSize, kAppIconSize));
  app_icon->SetVisible(app_icon_visible_);
  app_icon->SetInteractive(true);
  app_icon_ = AddChildView(std::move(app_icon));

  // Workspace indicator dot.
  class WorkspaceDot : public views::View {
   public:
    explicit WorkspaceDot(SkColor color) : color_(color) {}
    void SetColor(SkColor color) {
      color_ = color;
      SchedulePaint();
    }
    void OnPaint(gfx::Canvas* canvas) override {
      views::View::OnPaint(canvas);
      gfx::Rect bounds = GetContentsBounds();
      cc::PaintFlags flags;
      flags.setColor(color_);
      flags.setAntiAlias(true);
      int size = std::min(bounds.width(), bounds.height());
      gfx::Point center = bounds.CenterPoint();
      canvas->DrawCircle(center, size / 2.0f, flags);
    }
   private:
    SkColor color_;
  };

  auto workspace_dot = std::make_unique<WorkspaceDot>(workspace_color_);
  workspace_dot->SetVisible(workspace_visible_);
  workspace_dot->SetTooltipText(u"Current workspace");
  workspace_dot->SetAccessibleName(u"Current workspace");
  workspace_indicator_ = AddChildView(std::move(workspace_dot));

  // Workspace label.
  auto workspace_label = std::make_unique<views::Label>(u"Workspace");
  workspace_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  workspace_label->SetAutoColorId(ui::kColorLabelForeground);
  workspace_label->SetVisible(workspace_visible_);
  workspace_label->SetElideBehavior(gfx::ELIDE_TAIL);
  workspace_label_ = AddChildView(std::move(workspace_label));

  // Title label.
  auto title_label = std::make_unique<views::Label>(title_);
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label->SetAutoColorId(ui::kColorLabelForeground);
  title_label->SetElideBehavior(gfx::ELIDE_TAIL);
  title_label_ = AddChildView(std::move(title_label));

  // Window control buttons (right to left: close, maximize, minimize).
  auto close_button = std::make_unique<views::ImageButton>();
  close_button->SetTooltipText(u"Close");
  close_button->SetAccessibleName(u"Close window");
  close_button->SetCallback(base::BindRepeating(
      &AstraTitleBarView::OnCloseClicked, base::Unretained(this)));
  close_button->SetVisible(window_controls_visible_);
  close_button_ = AddChildView(std::move(close_button));

  auto maximize_button = std::make_unique<views::ImageButton>();
  maximize_button->SetTooltipText(u"Maximize");
  maximize_button->SetAccessibleName(u"Maximize window");
  maximize_button->SetCallback(base::BindRepeating(
      &AstraTitleBarView::OnMaximizeClicked, base::Unretained(this)));
  maximize_button->SetVisible(window_controls_visible_);
  maximize_button_ = AddChildView(std::move(maximize_button));

  auto minimize_button = std::make_unique<views::ImageButton>();
  minimize_button->SetTooltipText(u"Minimize");
  minimize_button->SetAccessibleName(u"Minimize window");
  minimize_button->SetCallback(base::BindRepeating(
      &AstraTitleBarView::OnMinimizeClicked, base::Unretained(this)));
  minimize_button->SetVisible(window_controls_visible_);
  minimize_button_ = AddChildView(std::move(minimize_button));
}

void AstraTitleBarView::OnMinimizeClicked() {
  if (delegate_) {
    delegate_->OnWindowControlClicked(AstraWindowControlType::kMinimize);
  }
}

void AstraTitleBarView::OnMaximizeClicked() {
  if (delegate_) {
    bool maximized = delegate_->IsWindowMaximized();
    if (maximized) {
      delegate_->OnWindowControlClicked(AstraWindowControlType::kRestore);
    } else {
      delegate_->OnWindowControlClicked(AstraWindowControlType::kMaximize);
    }
    UpdateMaximizeButton(!maximized);
  }
}

void AstraTitleBarView::OnCloseClicked() {
  if (delegate_) {
    delegate_->OnWindowControlClicked(AstraWindowControlType::kClose);
  }
}

void AstraTitleBarView::OnAppIconClicked() {
  if (delegate_) {
    delegate_->OnAppIconClicked();
  }
}

void AstraTitleBarView::OnWorkspaceClicked() {
  if (delegate_) {
    delegate_->OnWorkspaceClicked();
  }
}

void AstraTitleBarView::UpdateControlIcons() {
  SkColor icon_color = dark_mode_ ? SK_ColorWHITE : SK_ColorBLACK;

  // Update buttons to use custom icon colors.
  // In a real implementation, we'd use ImageModel with color IDs.
  if (minimize_button_) {
    minimize_button_->SchedulePaint();
  }
  if (maximize_button_) {
    maximize_button_->SchedulePaint();
  }
  if (close_button_) {
    close_button_->SchedulePaint();
  }
}

void AstraTitleBarView::UpdateTitleFromDelegate() {
  if (delegate_) {
    SetTitle(delegate_->GetActiveTabTitle());
  }
}

}  // namespace astra
