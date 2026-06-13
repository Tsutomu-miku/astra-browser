// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_suggestions/astra_tab_suggestions_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkCanvas.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPaint.h"
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
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 360;
constexpr int kItemHeight = 96;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 8;
constexpr int kMaxVisibleItems = 4;
constexpr int kRelevanceBarHeight = 4;

}  // namespace

// ===========================================================================
// AstraTabSuggestionItemView
// ===========================================================================

AstraTabSuggestionItemView::AstraTabSuggestionItemView(
    const SuggestionInfo& info,
    OpenCallback open_callback,
    DismissCallback dismiss_callback)
    : suggestion_id_(info.suggestion_id),
      title_(info.title),
      url_(info.url),
      domain_(info.domain),
      reason_(info.reason),
      type_(info.type),
      relevance_score_(info.relevance_score),
      last_visited_(info.last_visited),
      visit_count_(info.visit_count),
      is_openable_(info.is_openable),
      open_callback_(std::move(open_callback)),
      dismiss_callback_(std::move(dismiss_callback)) {
  BuildLayout();
}

AstraTabSuggestionItemView::~AstraTabSuggestionItemView() = default;

void AstraTabSuggestionItemView::BuildLayout() {
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(10, 12),
      4));
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Title row: type icon + title.
  auto* title_row = AddChildView(std::make_unique<views::View>());
  title_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  title_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* icon = title_row->AddChildView(
      std::make_unique<views::Label>(TypeIcon(type_)));
  icon->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  icon->SetAutoColorReadabilityEnabled(false);
  icon->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  title_label_ = title_row->AddChildView(
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

  // Subtitle: domain + last visited.
  std::u16string subtitle = base::UTF8ToUTF16(domain_);
  if (!last_visited_.is_null()) {
    base::TimeDelta delta = base::Time::Now() - last_visited_;
    int minutes = delta.InMinutes();
    std::u16string time_str;
    if (minutes < 1) {
      time_str = u"just now";
    } else if (minutes < 60) {
      time_str = base::UTF8ToUTF16(
          std::to_string(minutes) + "m ago");
    } else if (delta.InHours() < 24) {
      time_str = base::UTF8ToUTF16(
          std::to_string(delta.InHours()) + "h ago");
    } else {
      time_str = base::UTF8ToUTF16(
          std::to_string(delta.InDays()) + "d ago");
    }
    subtitle += u" · " + time_str;
  }

  subtitle_label_ = AddChildView(
      std::make_unique<views::Label>(subtitle));
  subtitle_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  subtitle_label_->SetAutoColorReadabilityEnabled(false);
  subtitle_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  subtitle_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Reason.
  reason_label_ = AddChildView(
      std::make_unique<views::Label>(reason_));
  reason_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  reason_label_->SetAutoColorReadabilityEnabled(false);
  reason_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  reason_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Bottom row: relevance bar + buttons.
  auto* bottom_row = AddChildView(std::make_unique<views::View>());
  bottom_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  bottom_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Relevance bar (painted, takes flex space).
  auto* bar_spacer = bottom_row->AddChildView(
      std::make_unique<views::View>());
  bar_spacer->SetPreferredSize(gfx::Size(100, kRelevanceBarHeight));
  bar_spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  open_button_ = bottom_row->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraTabSuggestionItemView::OnOpenClicked,
              base::Unretained(this)),
          u"Open"));
  open_button_->SetEnabled(is_openable_);

  dismiss_button_ = bottom_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabSuggestionItemView::OnDismissClicked,
              base::Unretained(this)),
          u"×"));
}

void AstraTabSuggestionItemView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  PaintRelevanceBar(canvas);
}

void AstraTabSuggestionItemView::PaintRelevanceBar(
    gfx::Canvas* canvas) {
  // Paint the relevance bar in the bottom area.
  gfx::Rect bounds = GetLocalBounds();
  int bar_width = 100;
  int bar_x = 12;
  int bar_y = bounds.height() - 16;

  SkPaint bg_paint;
  bg_paint.setColor(SkColorSetA(SK_ColorGRAY, 0x30));
  bg_paint.setStyle(SkPaint::kFill_Style);
  bg_paint.setAntiAlias(true);

  SkRect bg_rect = SkRect::MakeXYWH(bar_x, bar_y, bar_width, kRelevanceBarHeight);
  canvas->drawRoundRect(bg_rect, 2, 2, bg_paint);

  int score_width = static_cast<int>(bar_width * relevance_score_ / 100.0);
  if (score_width < 2) score_width = 2;

  SkPaint fg_paint;
  const auto* cp = GetColorProvider();
  SkColor accent =
      cp ? cp->GetColor(ui::kColorButtonBackgroundProminent)
         : SkColorSetRGB(0x1A, 0x73, 0xE8);
  fg_paint.setColor(accent);
  fg_paint.setStyle(SkPaint::kFill_Style);
  fg_paint.setAntiAlias(true);

  SkRect fg_rect = SkRect::MakeXYWH(bar_x, bar_y, score_width, kRelevanceBarHeight);
  canvas->drawRoundRect(fg_rect, 2, 2, fg_paint);
}

void AstraTabSuggestionItemView::OnOpenClicked() {
  if (open_callback_) {
    open_callback_.Run(suggestion_id_);
  }
}

void AstraTabSuggestionItemView::OnDismissClicked() {
  if (dismiss_callback_) {
    dismiss_callback_.Run(suggestion_id_);
  }
}

std::u16string AstraTabSuggestionItemView::TypeIcon(SuggestionType type) {
  switch (type) {
    case SuggestionType::kContinue:
      return u"📌";
    case SuggestionType::kReopen:
      return u"🕐";
    case SuggestionType::kRelated:
      return u"🔗";
    case SuggestionType::kDaily:
      return u"📅";
    case SuggestionType::kMorningRoutine:
      return u"🌅";
    case SuggestionType::kEveningWindDown:
      return u"🌙";
  }
  return u"💡";
}

std::u16string AstraTabSuggestionItemView::TypeLabel(SuggestionType type) {
  switch (type) {
    case SuggestionType::kContinue:
      return u"Continue";
    case SuggestionType::kReopen:
      return u"Reopen";
    case SuggestionType::kRelated:
      return u"Related";
    case SuggestionType::kDaily:
      return u"Daily";
    case SuggestionType::kMorningRoutine:
      return u"Morning routine";
    case SuggestionType::kEveningWindDown:
      return u"Evening wind-down";
  }
  return u"Suggestion";
}

void AstraTabSuggestionItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  subtitle_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  reason_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));

  SchedulePaint();
}

// ===========================================================================
// AstraTabSuggestionsView
// ===========================================================================

AstraTabSuggestionsView::AstraTabSuggestionsView(
    views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabSuggestionsView::~AstraTabSuggestionsView() = default;

void AstraTabSuggestionsView::SetSuggestions(
    const std::vector<AstraTabSuggestionItemView::SuggestionInfo>& suggestions) {
  suggestions_ = suggestions;
  RefreshSuggestions();
}

void AstraTabSuggestionsView::SetOpenSuggestionCallback(
    OpenSuggestionCallback callback) {
  open_callback_ = std::move(callback);
}

void AstraTabSuggestionsView::SetDismissSuggestionCallback(
    DismissSuggestionCallback callback) {
  dismiss_callback_ = std::move(callback);
}

void AstraTabSuggestionsView::SetRefreshCallback(
    RefreshCallback callback) {
  refresh_callback_ = std::move(callback);
}

void AstraTabSuggestionsView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildHeaderSection();
  BuildSuggestionsList();
}

void AstraTabSuggestionsView::BuildHeaderSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  subtitle_label_ = section->AddChildView(
      std::make_unique<views::Label>(
          u"Based on your browsing habits"));
  subtitle_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  subtitle_label_->SetAutoColorReadabilityEnabled(false);
  subtitle_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  refresh_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabSuggestionsView::OnRefresh,
              base::Unretained(this)),
          u"🔄 Refresh suggestions"));
  refresh_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabSuggestionsView::BuildSuggestionsList() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kItemSpacing));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Recommended for you"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  auto* scroll_view = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view->SetClipHeight(kItemHeight * kMaxVisibleItems +
                              kItemSpacing * (kMaxVisibleItems - 1));
  scroll_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  suggestions_list_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  suggestions_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraTabSuggestionsView::RefreshSuggestions() {
  if (!suggestions_list_) return;

  suggestions_list_->RemoveAllChildViews();
  suggestion_views_.clear();

  // Sort by relevance score descending.
  auto sorted = suggestions_;
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
              return a.relevance_score > b.relevance_score;
            });

  for (const auto& suggestion : sorted) {
    auto* item = suggestions_list_->AddChildView(
        std::make_unique<AstraTabSuggestionItemView>(
            suggestion,
            base::BindRepeating(
                &AstraTabSuggestionsView::OnOpenSuggestion,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabSuggestionsView::OnDismissSuggestion,
                base::Unretained(this))));
    suggestion_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabSuggestionsView::OnOpenSuggestion(
    const std::string& id) {
  if (open_callback_) {
    open_callback_.Run(id);
  }
}

void AstraTabSuggestionsView::OnDismissSuggestion(
    const std::string& id) {
  if (dismiss_callback_) {
    dismiss_callback_.Run(id);
  }
}

void AstraTabSuggestionsView::OnRefresh() {
  if (refresh_callback_) {
    refresh_callback_.Run();
  }
}

std::u16string AstraTabSuggestionsView::GetWindowTitle() const {
  return u"Suggestions";
}

void AstraTabSuggestionsView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  if (subtitle_label_) {
    subtitle_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
}

}  // namespace astra
