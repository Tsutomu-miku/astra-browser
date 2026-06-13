// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_overview/astra_tab_overview_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkCanvas.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPaint.h"
#include "skia/core/SkPath.h"
#include "skia/core/SkRRect.h"
#include "skia/core/SkRect.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 380;
constexpr int kSectionPadding = 12;
constexpr int kTileWidth = 104;
constexpr int kTileHeight = 100;
constexpr int kTileSpacing = 8;
constexpr int kFaviconSize = 28;
constexpr int kMaxVisibleRows = 5;

}  // namespace

// ===========================================================================
// AstraTabOverviewTileView
// ===========================================================================

AstraTabOverviewTileView::AstraTabOverviewTileView(
    const TabInfo& info,
    ClickCallback click_callback,
    CloseCallback close_callback)
    : tab_id_(info.tab_id),
      title_(info.title),
      domain_(info.domain),
      favicon_color_(info.favicon_color),
      is_active_(info.is_active),
      is_pinned_(info.is_pinned),
      is_audible_(info.is_audible),
      tab_index_(info.tab_index),
      click_callback_(std::move(click_callback)),
      close_callback_(std::move(close_callback)) {
  BuildLayout();
}

AstraTabOverviewTileView::~AstraTabOverviewTileView() = default;

void AstraTabOverviewTileView::BuildLayout() {
  SetPreferredSize(gfx::Size(kTileWidth, kTileHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(8, 8),
      4));
  SetBorder(views::CreateRoundedRectBorder(
      is_active_ ? 2 : 1, 8,
      is_active_ ? SkColorSetRGB(0x1A, 0x73, 0xE8) : SK_ColorGRAY));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Favicon area placeholder.
  favicon_area_ = AddChildView(std::make_unique<views::View>());
  favicon_area_->SetPreferredSize(
      gfx::Size(kTileWidth - 16, kFaviconSize + 4));

  // Title.
  title_label_ = AddChildView(
      std::make_unique<views::Label>(title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Domain.
  domain_label_ = AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(domain_)));
  domain_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  domain_label_->SetAutoColorReadabilityEnabled(false);
  domain_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  domain_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  domain_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabOverviewTileView::OnClicked() {
  if (click_callback_) {
    click_callback_.Run(tab_id_);
  }
}

void AstraTabOverviewTileView::OnCloseClicked() {
  if (close_callback_) {
    close_callback_.Run(tab_id_);
  }
}

void AstraTabOverviewTileView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  PaintFavicon(canvas);
}

void AstraTabOverviewTileView::PaintFavicon(gfx::Canvas* canvas) {
  gfx::Rect bounds = favicon_area_->bounds();
  int x = bounds.x() + (bounds.width() - kFaviconSize) / 2;
  int y = bounds.y() + (bounds.height() - kFaviconSize) / 2;

  SkPaint paint;
  paint.setColor(favicon_color_);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setAntiAlias(true);

  SkRect rect = SkRect::MakeXYWH(x, y, kFaviconSize, kFaviconSize);
  canvas->drawRoundRect(rect, 6, 6, paint);

  // Draw a simple "globe" line icon in white.
  SkPaint icon_paint;
  icon_paint.setColor(SK_ColorWHITE);
  icon_paint.setStyle(SkPaint::kStroke_Style);
  icon_paint.setStrokeWidth(1.5);
  icon_paint.setAntiAlias(true);

  // Globe: circle + horizontal + vertical lines.
  int cx = x + kFaviconSize / 2;
  int cy = y + kFaviconSize / 2;
  int r = kFaviconSize / 2 - 4;

  canvas->drawCircle(cx, cy, r, icon_paint);
  canvas->drawLine(x + 3, cy, x + kFaviconSize - 3, cy, icon_paint);
  canvas->drawLine(cx, y + 3, cx, y + kFaviconSize - 3, icon_paint);

  // Audio indicator.
  if (is_audible_) {
    SkPaint audio_paint;
    audio_paint.setColor(SkColorSetRGB(0x34, 0xA8, 0x53));
    audio_paint.setStyle(SkPaint::kFill_Style);
    audio_paint.setAntiAlias(true);

    int audio_size = 10;
    int ax = bounds.right() - audio_size - 2;
    int ay = bounds.y() + 2;
    SkRect audio_rect = SkRect::MakeXYWH(ax, ay, audio_size, audio_size);
    canvas->drawRoundRect(audio_rect, audio_size / 2, audio_size / 2,
                          audio_paint);
  }

  // Pinned indicator.
  if (is_pinned_) {
    SkPaint pin_paint;
    pin_paint.setColor(SkColorSetRGB(0xEA, 0x43, 0x35));
    pin_paint.setStyle(SkPaint::kFill_Style);
    pin_paint.setAntiAlias(true);

    int pin_size = 10;
    int px = bounds.x() + 2;
    int py = bounds.y() + 2;
    SkRect pin_rect = SkRect::MakeXYWH(px, py, pin_size, pin_size);
    canvas->drawRoundRect(pin_rect, pin_size / 2, pin_size / 2, pin_paint);
  }
}

void AstraTabOverviewTileView::PaintCloseButton(gfx::Canvas* canvas) {
  // Close button painted in top-right corner on hover.
  // For simplicity, we don't draw it statically.
}

void AstraTabOverviewTileView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  domain_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 8));

  SchedulePaint();
}

// ===========================================================================
// AstraTabOverviewView
// ===========================================================================

AstraTabOverviewView::AstraTabOverviewView(
    views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabOverviewView::~AstraTabOverviewView() = default;

void AstraTabOverviewView::SetTabs(
    const std::vector<AstraTabOverviewTileView::TabInfo>& tabs) {
  tabs_ = tabs;
  RefreshTabs();
  RefreshSummary();
}

void AstraTabOverviewView::SetGroupMode(GroupMode mode) {
  group_mode_ = mode;
  RefreshTabs();
  RefreshSummary();
}

void AstraTabOverviewView::SetTabClickCallback(
    TabClickCallback callback) {
  click_callback_ = std::move(callback);
}

void AstraTabOverviewView::SetTabCloseCallback(
    TabCloseCallback callback) {
  close_callback_ = std::move(callback);
}

void AstraTabOverviewView::SetSearchCallback(
    SearchCallback callback) {
  search_callback_ = std::move(callback);
}

void AstraTabOverviewView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildSearchBar();
  BuildSummaryRow();
  BuildTabsGrid();
}

void AstraTabOverviewView::BuildSearchBar() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  search_field_ = section->AddChildView(
      std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"🔍 Search tabs...");
  search_field_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabOverviewView::BuildSummaryRow() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(8, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  summary_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"0 tabs"));
  summary_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  summary_label_->SetAutoColorReadabilityEnabled(false);
  summary_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  summary_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabOverviewView::BuildTabsGrid() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kTileSpacing));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  scroll_view_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(
      kTileHeight * kMaxVisibleRows + kTileSpacing * (kMaxVisibleRows - 1));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  tabs_grid_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  tabs_grid_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
}

void AstraTabOverviewView::RefreshTabs() {
  if (!tabs_grid_) return;

  tabs_grid_->RemoveAllChildViews();
  tile_views_.clear();

  // Sort: active first, then by tab index.
  auto sorted = tabs_;
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
              if (a.is_active != b.is_active) return a.is_active > b.is_active;
              return a.tab_index < b.tab_index;
            });

  for (const auto& tab : sorted) {
    auto* tile = tabs_grid_->AddChildView(
        std::make_unique<AstraTabOverviewTileView>(
            tab,
            base::BindRepeating(
                &AstraTabOverviewView::OnTabClicked,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabOverviewView::OnTabClosed,
                base::Unretained(this))));
    tile_views_.push_back(tile);
  }

  InvalidateLayout();
}

void AstraTabOverviewView::RefreshSummary() {
  if (!summary_label_) return;

  summary_label_->SetText(
      base::UTF8ToUTF16(
          std::to_string(tabs_.size()) + " tabs" +
          (window_count_ > 1
               ? " · " + std::to_string(window_count_) + " windows"
               : "") +
          " · " + base::UTF16ToUTF8(GroupModeLabel(group_mode_))));
}

void AstraTabOverviewView::OnTabClicked(const std::string& tab_id) {
  if (click_callback_) {
    click_callback_.Run(tab_id);
  }
}

void AstraTabOverviewView::OnTabClosed(const std::string& tab_id) {
  if (close_callback_) {
    close_callback_.Run(tab_id);
  }
}

void AstraTabOverviewView::OnSearchTextChanged() {
  if (search_callback_ && search_field_) {
    search_callback_.Run(search_field_->GetText());
  }
}

std::u16string AstraTabOverviewView::GroupModeLabel(GroupMode mode) {
  switch (mode) {
    case GroupMode::kNone:
      return u"flat";
    case GroupMode::kByDomain:
      return u"by domain";
    case GroupMode::kByWindow:
      return u"by window";
    case GroupMode::kByRecent:
      return u"by recent";
  }
  return u"flat";
}

std::u16string AstraTabOverviewView::GetWindowTitle() const {
  return u"Tab Overview";
}

void AstraTabOverviewView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  if (summary_label_) {
    summary_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
}

}  // namespace astra
