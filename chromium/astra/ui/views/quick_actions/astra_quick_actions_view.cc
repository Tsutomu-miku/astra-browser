// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/quick_actions/astra_quick_actions_view.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 360;
constexpr int kSectionPadding = 16;
constexpr int kActionTileSize = 72;
constexpr int kActionIconSize = 28;
constexpr int kActionSpacing = 8;
constexpr int kTileCornerRadius = 8;

// Draw a quick action icon on a canvas.
void DrawActionIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    AstraQuickActionItemView::ActionIcon icon,
                    SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int s = kActionIconSize / 2;

  switch (icon) {
    case AstraQuickActionItemView::ActionIcon::kNewTab: {
      // Plus sign inside a rounded rect.
      SkPath path;
      path.addRoundRect(SkRect::MakeLTRB(
          cx - s, cy - s, cx + s, cy + s), 4, 4);
      canvas->DrawPath(path, flags);
      // Plus.
      canvas->DrawLine(gfx::Point(cx, cy - s + 6),
                       gfx::Point(cx, cy + s - 6), flags);
      canvas->DrawLine(gfx::Point(cx - s + 6, cy),
                       gfx::Point(cx + s - 6, cy), flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kCloseTab: {
      // X mark.
      canvas->DrawLine(gfx::Point(cx - s + 4, cy - s + 4),
                       gfx::Point(cx + s - 4, cy + s - 4), flags);
      canvas->DrawLine(gfx::Point(cx + s - 4, cy - s + 4),
                       gfx::Point(cx - s + 4, cy + s - 4), flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kPinTab: {
      // Pin / pushpin.
      SkPath path;
      path.moveTo(cx, cy - s);
      path.lineTo(cx + s - 2, cy - s / 2);
      path.lineTo(cx, cy + s - 2);
      path.lineTo(cx - s + 2, cy - s / 2);
      path.close();
      canvas->DrawPath(path, flags);
      // Head circle.
      flags.setStyle(cc::PaintFlags::kFill_Style);
      canvas->DrawCircle(gfx::Point(cx, cy - s + 4), 4, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kMuteTab: {
      // Speaker with slash.
      SkPath path;
      path.moveTo(cx - s + 2, cy - 4);
      path.lineTo(cx - s / 2, cy - 4);
      path.lineTo(cx + s / 2 - 2, cy - s + 4);
      path.lineTo(cx + s / 2 - 2, cy + s - 4);
      path.lineTo(cx - s / 2, cy + 4);
      path.lineTo(cx - s + 2, cy + 4);
      path.close();
      canvas->DrawPath(path, flags);
      // Slash.
      canvas->DrawLine(gfx::Point(cx + s - 6, cy - s + 6),
                       gfx::Point(cx - s / 2 + 4, cy + s - 4), flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kDuplicateTab: {
      // Two overlapping documents.
      SkPath path1;
      path1.addRoundRect(SkRect::MakeLTRB(
          cx - s, cy - s + 4, cx + 2, cy + 4), 2, 2);
      canvas->DrawPath(path1, flags);
      SkPath path2;
      path2.addRoundRect(SkRect::MakeLTRB(
          cx - 2, cy - s + 8, cx + s - 2, cy + s - 4), 2, 2);
      canvas->DrawPath(path2, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kSleepTab: {
      // Zzz / moon.
      SkPath path;
      path.moveTo(cx + s - 4, cy - s + 4);
      path.quadTo(cx + s - 4, cy - 4, cx + 4, cy + s - 4);
      path.quadTo(cx - s / 2, cy + s / 2, cx - s + 4, cy - 2);
      path.quadTo(cx - s / 2, cy - s / 2, cx + s - 4, cy - s + 4);
      path.close();
      canvas->DrawPath(path, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kBack: {
      // Left arrow.
      SkPath path;
      path.moveTo(cx + s - 4, cy);
      path.lineTo(cx - s + 4, cy);
      path.lineTo(cx - 2, cy - s + 4);
      path.moveTo(cx - s + 4, cy);
      path.lineTo(cx - 2, cy + s - 4);
      canvas->DrawPath(path, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kForward: {
      // Right arrow.
      SkPath path;
      path.moveTo(cx - s + 4, cy);
      path.lineTo(cx + s - 4, cy);
      path.lineTo(cx + 2, cy - s + 4);
      path.moveTo(cx + s - 4, cy);
      path.lineTo(cx + 2, cy + s - 4);
      canvas->DrawPath(path, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kReload: {
      // Circular arrow.
      SkPath path;
      SkRect oval = SkRect::MakeLTRB(
          cx - s + 4, cy - s + 4, cx + s - 4, cy + s - 4);
      path.addArc(oval, 45, 270);
      canvas->DrawPath(path, flags);
      // Arrowhead.
      flags.setStyle(cc::PaintFlags::kFill_Style);
      SkPath arrow;
      arrow.moveTo(cx + s - 6, cy - 4);
      arrow.lineTo(cx + s - 2, cy - 10);
      arrow.lineTo(cx + s - 2, cy + 2);
      arrow.close();
      canvas->DrawPath(arrow, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kBookmark: {
      // Bookmark ribbon.
      SkPath path;
      path.moveTo(cx - s + 4, cy - s + 2);
      path.lineTo(cx + s - 4, cy - s + 2);
      path.lineTo(cx + s - 4, cy + s - 2);
      path.lineTo(cx, cy + s / 2 - 2);
      path.lineTo(cx - s + 4, cy + s - 2);
      path.close();
      canvas->DrawPath(path, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kFocusMode: {
      // Target / crosshair.
      canvas->DrawCircle(gfx::Point(cx, cy), s - 4, flags);
      canvas->DrawLine(gfx::Point(cx - s + 4, cy),
                       gfx::Point(cx - 4, cy), flags);
      canvas->DrawLine(gfx::Point(cx + 4, cy),
                       gfx::Point(cx + s - 4, cy), flags);
      canvas->DrawLine(gfx::Point(cx, cy - s + 4),
                       gfx::Point(cx, cy - 4), flags);
      canvas->DrawLine(gfx::Point(cx, cy + 4),
                       gfx::Point(cx, cy + s - 4), flags);
      // Center dot.
      flags.setStyle(cc::PaintFlags::kFill_Style);
      canvas->DrawCircle(gfx::Point(cx, cy), 2, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kWorkspace: {
      // Folder with multiple tabs.
      SkPath path;
      path.moveTo(cx - s + 2, cy - s + 4);
      path.lineTo(cx - s / 2 - 2, cy - s + 4);
      path.lineTo(cx - s / 2 + 2, cy - s + 8);
      path.lineTo(cx + s - 2, cy - s + 8);
      path.lineTo(cx + s - 2, cy + s - 4);
      path.lineTo(cx - s + 2, cy + s - 4);
      path.close();
      canvas->DrawPath(path, flags);
      // Tab lines.
      canvas->DrawLine(gfx::Point(cx - s + 6, cy - 2),
                       gfx::Point(cx + s - 6, cy - 2), flags);
      canvas->DrawLine(gfx::Point(cx - s + 6, cy + 4),
                       gfx::Point(cx + s / 2, cy + 4), flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kSidebar: {
      // Left sidebar panel.
      SkPath outer;
      outer.addRoundRect(SkRect::MakeLTRB(
          cx - s + 2, cy - s + 2, cx + s - 2, cy + s - 2), 3, 3);
      canvas->DrawPath(outer, flags);
      // Sidebar divider.
      canvas->DrawLine(gfx::Point(cx - s / 2, cy - s + 4),
                       gfx::Point(cx - s / 2, cy + s - 4), flags);
      // Sidebar items (dots).
      flags.setStyle(cc::PaintFlags::kFill_Style);
      canvas->DrawCircle(gfx::Point(cx - s + 8, cy - s / 2), 2, flags);
      canvas->DrawCircle(gfx::Point(cx - s + 8, cy), 2, flags);
      canvas->DrawCircle(gfx::Point(cx - s + 8, cy + s / 2), 2, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kSplitView: {
      // Split screen (two panes).
      SkPath left;
      left.addRoundRect(SkRect::MakeLTRB(
          cx - s + 2, cy - s + 2, cx - 1, cy + s - 2), 2, 2);
      canvas->DrawPath(left, flags);
      SkPath right;
      right.addRoundRect(SkRect::MakeLTRB(
          cx + 1, cy - s + 2, cx + s - 2, cy + s - 2), 2, 2);
      canvas->DrawPath(right, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kReadingList: {
      // Open book.
      SkPath path;
      path.moveTo(cx, cy - s + 2);
      path.lineTo(cx - s + 2, cy - s + 6);
      path.lineTo(cx - s + 2, cy + s - 2);
      path.lineTo(cx, cy + s - 6);
      path.lineTo(cx + s - 2, cy + s - 2);
      path.lineTo(cx + s - 2, cy - s + 6);
      path.close();
      canvas->DrawPath(path, flags);
      // Pages.
      canvas->DrawLine(gfx::Point(cx, cy - s + 4),
                       gfx::Point(cx, cy + s - 6), flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kFind: {
      // Magnifying glass.
      SkPath rim;
      rim.addCircle(cx - s / 3, cy - s / 3, s - 6);
      canvas->DrawPath(rim, flags);
      // Handle.
      SkPath handle;
      handle.moveTo(cx + s / 3 - 4, cy + s / 3 - 4);
      handle.lineTo(cx + s - 4, cy + s - 4);
      canvas->DrawPath(handle, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kPrint: {
      // Printer.
      SkPath path;
      path.addRect(SkRect::MakeLTRB(
          cx - s + 4, cy - s + 2, cx + s - 4, cy - s / 2));
      path.addRect(SkRect::MakeLTRB(
          cx - s + 2, cy - s / 2, cx + s - 2, cy + s / 3));
      path.addRect(SkRect::MakeLTRB(
          cx - s + 6, cy + s / 3, cx + s - 6, cy + s - 2));
      canvas->DrawPath(path, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kZoomIn: {
      // Magnifying glass with plus.
      SkPath rim;
      rim.addCircle(cx - s / 3, cy - s / 3, s - 6);
      canvas->DrawPath(rim, flags);
      SkPath handle;
      handle.moveTo(cx + s / 3 - 4, cy + s / 3 - 4);
      handle.lineTo(cx + s - 4, cy + s - 4);
      canvas->DrawPath(handle, flags);
      // Plus.
      canvas->DrawLine(gfx::Point(cx - s / 3, cy - s / 3 - 4),
                       gfx::Point(cx - s / 3, cy - s / 3 + 4), flags);
      canvas->DrawLine(gfx::Point(cx - s / 3 - 4, cy - s / 3),
                       gfx::Point(cx - s / 3 + 4, cy - s / 3), flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kZoomOut: {
      // Magnifying glass with minus.
      SkPath rim;
      rim.addCircle(cx - s / 3, cy - s / 3, s - 6);
      canvas->DrawPath(rim, flags);
      SkPath handle;
      handle.moveTo(cx + s / 3 - 4, cy + s / 3 - 4);
      handle.lineTo(cx + s - 4, cy + s - 4);
      canvas->DrawPath(handle, flags);
      // Minus.
      canvas->DrawLine(gfx::Point(cx - s / 3 - 4, cy - s / 3),
                       gfx::Point(cx - s / 3 + 4, cy - s / 3), flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kFullscreen: {
      // Four corners expanding.
      SkPath path;
      // Top-left.
      path.moveTo(cx - s + 2, cy - s + 8);
      path.lineTo(cx - s + 2, cy - s + 2);
      path.lineTo(cx - s + 8, cy - s + 2);
      // Top-right.
      path.moveTo(cx + s - 8, cy - s + 2);
      path.lineTo(cx + s - 2, cy - s + 2);
      path.lineTo(cx + s - 2, cy - s + 8);
      // Bottom-right.
      path.moveTo(cx + s - 2, cy + s - 8);
      path.lineTo(cx + s - 2, cy + s - 2);
      path.lineTo(cx + s - 8, cy + s - 2);
      // Bottom-left.
      path.moveTo(cx - s + 8, cy + s - 2);
      path.lineTo(cx - s + 2, cy + s - 2);
      path.lineTo(cx - s + 2, cy + s - 8);
      canvas->DrawPath(path, flags);
      break;
    }
    case AstraQuickActionItemView::ActionIcon::kDevTools: {
      // Wrench / tools.
      SkPath path;
      path.addCircle(cx, cy, s - 4);
      // Wrench shape.
      path.moveTo(cx - 4, cy - s + 6);
      path.lineTo(cx - 4, cy - 4);
      path.lineTo(cx - s + 6, cy + s - 6);
      path.lineTo(cx + 4, cy + 4);
      path.lineTo(cx + 4, cy - s + 6);
      path.close();
      canvas->DrawPath(path, flags);
      break;
    }
  }
}

}  // namespace

// ===========================================================================
// AstraQuickActionItemView
// ===========================================================================

AstraQuickActionItemView::AstraQuickActionItemView(
    const ActionInfo& info,
    ActionCallback callback)
    : action_id_(info.action_id),
      label_(info.label),
      icon_(info.icon),
      callback_(std::move(callback)) {
  BuildLayout();
}

AstraQuickActionItemView::~AstraQuickActionItemView() = default;

void AstraQuickActionItemView::BuildLayout() {
  SetPreferredSize(gfx::Size(kActionTileSize, kActionTileSize + 8));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(6, 4),
      4));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetBorder(views::CreateRoundedRectBorder(
      1, kTileCornerRadius, SK_ColorGRAY));

  icon_view_ = AddChildView(std::make_unique<views::View>());
  icon_view_->SetPreferredSize(
      gfx::Size(kActionTileSize - 16, kActionIconSize + 8));

  label_view_ = AddChildView(
      std::make_unique<views::Label>(label_));
  label_view_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  label_view_->SetAutoColorReadabilityEnabled(false);
  label_view_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  label_view_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Paint callback for the icon.
  icon_view_->SetPaintCallback(
      base::BindRepeating(
          [](AstraQuickActionItemView* view, gfx::Canvas* canvas) {
            gfx::Rect bounds = view->icon_view_->GetContentsBounds();
            SkColor color = SK_ColorGRAY;
            if (view->GetColorProvider()) {
              color = view->GetColorProvider()->GetColor(
                  ui::kColorIcon);
            }
            if (view->is_active_) {
              color = view->GetColorProvider()
                          ? view->GetColorProvider()->GetColor(
                                ui::kColorIconDefault)
                          : SK_ColorBLUE;
            }
            DrawActionIcon(canvas, bounds, view->icon_, color);
          },
          base::Unretained(this)));
}

void AstraQuickActionItemView::SetActive(bool active) {
  is_active_ = active;
  SchedulePaint();
}

void AstraQuickActionItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  label_view_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  if (is_active_) {
    SetBackground(views::CreateRoundedRectBackground(
        cp->GetColor(ui::kColorButtonBackgroundProminent),
        kTileCornerRadius));
  } else {
    SetBackground(views::CreateRoundedRectBackground(
        cp->GetColor(ui::kColorDialogBackground),
        kTileCornerRadius));
  }
  icon_view_->SchedulePaint();
}

bool AstraQuickActionItemView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    is_pressed_ = true;
    SchedulePaint();
    return true;
  }
  return false;
}

void AstraQuickActionItemView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (is_pressed_ && event.IsOnlyLeftMouseButton()) {
    is_pressed_ = false;
    SchedulePaint();
    if (GetContentsBounds().Contains(event.location())) {
      if (callback_) {
        callback_.Run();
      }
    }
  }
}

void AstraQuickActionItemView::OnMouseEntered(
    const ui::MouseEvent& event) {
  is_hovered_ = true;
  SchedulePaint();
}

void AstraQuickActionItemView::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  is_pressed_ = false;
  SchedulePaint();
}

// ===========================================================================
// AstraQuickActionsView
// ===========================================================================

AstraQuickActionsView::AstraQuickActionsView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  // Define sections.
  sections_ = {
      {u"Tabs",
       {
           {"new-tab", u"New tab", ActionIcon::kNewTab},
           {"close-tab", u"Close", ActionIcon::kCloseTab},
           {"pin-tab", u"Pin", ActionIcon::kPinTab},
           {"mute-tab", u"Mute", ActionIcon::kMuteTab},
           {"duplicate-tab", u"Duplicate", ActionIcon::kDuplicateTab},
           {"sleep-tab", u"Sleep", ActionIcon::kSleepTab},
       }},
      {u"Astra",
       {
           {"focus-mode", u"Focus", ActionIcon::kFocusMode},
           {"workspace", u"Workspace", ActionIcon::kWorkspace},
           {"sidebar", u"Sidebar", ActionIcon::kSidebar},
           {"split-view", u"Split View", ActionIcon::kSplitView},
           {"reading-list", u"Reading", ActionIcon::kReadingList},
       }},
      {u"Tools",
       {
           {"find", u"Find", ActionIcon::kFind},
           {"print", u"Print", ActionIcon::kPrint},
           {"zoom-in", u"Zoom in", ActionIcon::kZoomIn},
           {"zoom-out", u"Zoom out", ActionIcon::kZoomOut},
           {"fullscreen", u"Fullscreen", ActionIcon::kFullscreen},
           {"devtools", u"DevTools", ActionIcon::kDevTools},
       }},
  };

  BuildUI();
}

AstraQuickActionsView::~AstraQuickActionsView() = default;

void AstraQuickActionsView::SetActionTriggeredCallback(
    ActionTriggeredCallback callback) {
  action_callback_ = std::move(callback);
}

void AstraQuickActionsView::SetActionActive(
    const std::string& action_id, bool active) {
  for (auto* item : action_items_) {
    if (item->action_id() == action_id) {
      item->SetActive(active);
      break;
    }
  }
}

void AstraQuickActionsView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  for (const auto& section : sections_) {
    BuildSection(section);
  }
}

void AstraQuickActionsView::BuildSection(const ActionSection& section) {
  auto* section_view = AddChildView(std::make_unique<views::View>());
  section_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section_view->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* title = section_view->AddChildView(
      std::make_unique<views::Label>(section.title));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetAutoColorReadabilityEnabled(false);
  title->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  // Grid of actions using FlexLayout with wrapping.
  auto* grid = section_view->AddChildView(std::make_unique<views::View>());
  grid->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetDefault(views::kFlexBehaviorKey,
                  views::FlexSpecification(
                      views::LayoutFlexOrientation::kHorizontal,
                      views::MinimumFlexSizeRule::kPreferred,
                      views::MaximumFlexSizeRule::kPreferred))
      .SetWrapBehavior(views::FlexLayout::WrapBehavior::kWrap)
      .SetColumnGap(kActionSpacing)
      .SetRowGap(kActionSpacing);

  for (const auto& action : section.actions) {
    auto* item = grid->AddChildView(
        std::make_unique<AstraQuickActionItemView>(
            action,
            base::BindRepeating(
                &AstraQuickActionsView::OnActionTriggered,
                base::Unretained(this),
                action.action_id)));
    action_items_.push_back(item);
  }
}

void AstraQuickActionsView::OnActionTriggered(
    const std::string& action_id) {
  if (action_callback_) {
    action_callback_.Run(action_id);
  }
}

std::u16string AstraQuickActionsView::GetWindowTitle() const {
  return u"Quick Actions";
}

void AstraQuickActionsView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
}

}  // namespace astra
