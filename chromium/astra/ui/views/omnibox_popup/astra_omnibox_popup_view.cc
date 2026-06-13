// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/omnibox_popup/astra_omnibox_popup_view.h"

#include <algorithm>
#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Get the default icon name for a match type.
const char* GetDefaultIconName(AstraOmniboxMatchType type) {
  switch (type) {
    case AstraOmniboxMatchType::kSearchWhatYouTyped:
    case AstraOmniboxMatchType::kSearchSuggestion:
    case AstraOmniboxMatchType::kSearchHistory:
      return "search";
    case AstraOmniboxMatchType::kUrlHistory:
      return "history";
    case AstraOmniboxMatchType::kUrlBookmark:
      return "bookmark";
    case AstraOmniboxMatchType::kUrlOpenTab:
      return "tab";
    case AstraOmniboxMatchType::kUrlNavsuggest:
      return "globe";
    case AstraOmniboxMatchType::kClipboard:
      return "clipboard";
    case AstraOmniboxMatchType::kDocument:
      return "document";
    case AstraOmniboxMatchType::kAnswer:
      return "answer";
    case AstraOmniboxMatchType::kOmniboxAction:
      return "action";
    case AstraOmniboxMatchType::kExtensionCommand:
      return "extension";
  }
  return "search";
}

// Get answer badge text for an answer type.
std::u16string GetAnswerBadge(AstraOmniboxAnswerType type) {
  switch (type) {
    case AstraOmniboxAnswerType::kCalculator:
      return u"Calculator";
    case AstraOmniboxAnswerType::kWeather:
      return u"Weather";
    case AstraOmniboxAnswerType::kDictionary:
      return u"Dictionary";
    case AstraOmniboxAnswerType::kStock:
      return u"Stock";
    case AstraOmniboxAnswerType::kTranslation:
      return u"Translation";
    case AstraOmniboxAnswerType::kSunriseSunset:
      return u"Sunrise/Sunset";
    case AstraOmniboxAnswerType::kFlightStatus:
      return u"Flight";
    case AstraOmniboxAnswerType::kSports:
      return u"Sports";
    case AstraOmniboxAnswerType::kTimeZone:
      return u"Time";
    case AstraOmniboxAnswerType::kUnitConversion:
      return u"Conversion";
    default:
      return u"";
  }
}

}  // namespace

// ===========================================================================
// AstraOmniboxPopupMatchView
// ===========================================================================

AstraOmniboxPopupMatchView::AstraOmniboxPopupMatchView(
    const AstraOmniboxMatch& match,
    size_t index)
    : match_(match), index_(index) {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Icon view.
  auto icon_view = std::make_unique<views::ImageView>();
  icon_view->SetImageSize(gfx::Size(kIconSize, kIconSize));
  icon_view_ = AddChildView(std::move(icon_view));

  // Contents label (the main text).
  auto contents_label = std::make_unique<views::Label>(match.contents);
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  contents_label->SetAutoColorId(ui::kColorLabelForeground);
  contents_label->SetElideBehavior(gfx::ELIDE_TAIL);
  contents_label_ = AddChildView(std::move(contents_label));

  // Description label (secondary text).
  auto description_label = std::make_unique<views::Label>(match.description);
  description_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  description_label->SetAutoColorId(ui::kColorLabelForegroundSecondary);
  description_label->SetElideBehavior(gfx::ELIDE_TAIL);
  description_label_ = AddChildView(std::move(description_label));

  // Bookmark star icon (right side).
  if (match.is_bookmarked) {
    auto bookmark_icon = std::make_unique<views::ImageView>();
    bookmark_icon->SetImageSize(gfx::Size(16, 16));
    bookmark_icon->SetTooltipText(u"Bookmarked");
    bookmark_icon_ = AddChildView(std::move(bookmark_icon));
  }

  // Answer container (for answer-type matches).
  if (match.type == AstraOmniboxMatchType::kAnswer) {
    auto answer_container = std::make_unique<views::View>();
    answer_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal));
    answer_text_label_ = answer_container->AddChildView(
        std::make_unique<views::Label>(match.answer_text));
    answer_text_label_->SetAutoColorId(ui::kColorLabelForeground);
    answer_container_ = AddChildView(std::move(answer_container));
  }

  UpdateVisuals();
}

AstraOmniboxPopupMatchView::~AstraOmniboxPopupMatchView() = default;

void AstraOmniboxPopupMatchView::UpdateFromMatch(
    const AstraOmniboxMatch& match) {
  match_ = match;
  contents_label_->SetText(match.contents);
  description_label_->SetText(match.description);
  UpdateVisuals();
}

void AstraOmniboxPopupMatchView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  UpdateVisuals();
}

gfx::Size AstraOmniboxPopupMatchView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int height = kRowHeight;
  if (match_.type == AstraOmniboxMatchType::kAnswer) {
    height = kAnswerHeight;
  }
  return gfx::Size(0, height);
}

void AstraOmniboxPopupMatchView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int y = bounds.y();
  int height = bounds.height();

  // Icon on the left.
  int icon_x = kIconLeftPadding;
  int icon_y = y + (height - kIconSize) / 2;
  icon_view_->SetBounds(icon_x, icon_y, kIconSize, kIconSize);

  // Bookmark icon on the right.
  if (bookmark_icon_ && bookmark_icon_->GetVisible()) {
    int bm_size = 16;
    int bm_x = bounds.right() - kTextRightPadding - bm_size;
    int bm_y = y + (height - bm_size) / 2;
    bookmark_icon_->SetBounds(bm_x, bm_y, bm_size, bm_size);
  }

  // Text area in the middle.
  int text_x = icon_x + kIconSize + kIconTextSpacing;
  int text_width = bounds.right() - text_x - kTextRightPadding;

  // Contents (top line).
  contents_label_->SetBounds(text_x, y + 6, text_width, 18);

  // Description (bottom line).
  description_label_->SetBounds(text_x, y + 24, text_width, 14);

  // Answer container (if answer type).
  if (answer_container_ && answer_container_->GetVisible()) {
    answer_container_->SetBounds(text_x, y + 6, text_width, height - 12);
  }
}

void AstraOmniboxPopupMatchView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateVisuals();
}

void AstraOmniboxPopupMatchView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  // Selected background.
  if (selected_ || hovered_) {
    cc::PaintFlags flags;
    flags.setColor(selected_ ? SkColorSetA(SK_ColorBLACK, 0x14)
                              : SkColorSetA(SK_ColorBLACK, 0x0D));
    flags.setAntiAlias(false);
    canvas->DrawRect(bounds, flags);
  }

  // Left accent bar for selected state.
  if (selected_) {
    cc::PaintFlags flags;
    flags.setColor(SkColorSetRGB(0x1A, 0x73, 0xE8));
    flags.setAntiAlias(false);
    canvas->DrawRect(gfx::Rect(bounds.x(), bounds.y(), 3, bounds.height()),
                     flags);
  }
}

void AstraOmniboxPopupMatchView::OnMouseEntered(
    const ui::MouseEvent& event) {
  hovered_ = true;
  SchedulePaint();
}

void AstraOmniboxPopupMatchView::OnMouseExited(
    const ui::MouseEvent& event) {
  hovered_ = false;
  SchedulePaint();
}

bool AstraOmniboxPopupMatchView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (event.IsLeftMouseButton()) {
    return true;
  }
  return views::View::OnMousePressed(event);
}

void AstraOmniboxPopupMatchView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (event.IsLeftMouseButton() && HitTestPoint(event.location())) {
    HandlePrimaryClick();
    return;
  }
  views::View::OnMouseReleased(event);
}

void AstraOmniboxPopupMatchView::OnGestureEvent(
    ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP) {
    HandlePrimaryClick();
    event->SetHandled();
    return;
  }
  views::View::OnGestureEvent(event);
}

void AstraOmniboxPopupMatchView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->SetName(match_.contents);
  node_data->SetDescription(match_.description);
  node_data->role = ax::mojom::Role::kListBoxOption;
}

void AstraOmniboxPopupMatchView::HandlePrimaryClick() {
  // Parent handles the click via index.
  // We'll bubble up through the view hierarchy with a callback.
  // For now, the parent will detect clicks via its own event handling.
}

void AstraOmniboxPopupMatchView::UpdateVisuals() {
  // Update icon (use default based on type).
  // TODO(astra): Use real vector icons from Chromium's icon set.

  // Update labels.
  contents_label_->SetEnabled(true);
  description_label_->SetEnabled(true);

  SchedulePaint();
}

// ===========================================================================
// AstraOmniboxPopupSectionHeader
// ===========================================================================

AstraOmniboxPopupSectionHeader::AstraOmniboxPopupSectionHeader(
    const std::u16string& title) {
  auto title_label = std::make_unique<views::Label>(title);
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label->SetAutoColorId(ui::kColorLabelForegroundSecondary);
  title_label->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
      views::style::STYLE_SECONDARY));
  title_label_ = AddChildView(std::move(title_label));
}

AstraOmniboxPopupSectionHeader::~AstraOmniboxPopupSectionHeader() = default;

void AstraOmniboxPopupSectionHeader::SetTitle(
    const std::u16string& title) {
  title_label_->SetText(title);
}

gfx::Size AstraOmniboxPopupSectionHeader::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(0, kHeaderHeight);
}

void AstraOmniboxPopupSectionHeader::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  title_label_->SetBounds(kHeaderLeftPadding, 0,
                          bounds.width() - kHeaderLeftPadding,
                          kHeaderHeight);
}

void AstraOmniboxPopupSectionHeader::OnThemeChanged() {
  views::View::OnThemeChanged();
}

// ===========================================================================
// AstraOmniboxPopupView
// ===========================================================================

AstraOmniboxPopupView::AstraOmniboxPopupView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                        views::BubbleBorder::TOP_LEFT) {
  set_close_on_deactivate(false);
  set_close_on_esc(false);
  set_margins(gfx::Insets());
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowCloseButton(false);
  set_adjust_if_offscreen(false);
  set_shadow(views::BubbleBorder::STANDARD_SHADOW);

  Build();
}

AstraOmniboxPopupView::~AstraOmniboxPopupView() = default;

void AstraOmniboxPopupView::SetModel(AstraOmniboxPopupModel* model) {
  if (model_ == model) {
    return;
  }

  model_observation_.Reset();
  model_ = model;

  if (model_) {
    model_observation_.Observe(model_);
    RebuildMatches();
  } else {
    match_views_.clear();
    section_headers_.clear();
    if (content_view_) {
      content_view_->RemoveAllChildViews();
    }
  }
}

void AstraOmniboxPopupView::ShowPopup() {
  if (model_) {
    model_->Show();
  }

  // Create and show the widget.
  views::Widget* widget = GetWidget();
  if (!widget) {
    // If not already created, create the widget.
    // In Chromium, this would be done by the anchor.
    return;
  }
  widget->Show();
}

void AstraOmniboxPopupView::HidePopup() {
  if (model_) {
    model_->Hide();
  }
  views::Widget* widget = GetWidget();
  if (widget) {
    widget->Hide();
  }
}

bool AstraOmniboxPopupView::IsPopupShowing() const {
  const views::Widget* widget = GetWidget();
  return widget && widget->IsVisible();
}

void AstraOmniboxPopupView::SelectNext() {
  if (model_) {
    model_->SelectNext();
  }
}

void AstraOmniboxPopupView::SelectPrevious() {
  if (model_) {
    model_->SelectPrevious();
  }
}

void AstraOmniboxPopupView::AcceptSelectedMatch() {
  if (!model_) {
    return;
  }
  const AstraOmniboxMatch* match = model_->GetSelectedMatch();
  if (match && delegate_) {
    delegate_->OnOmniboxMatchSelected(*match);
  }
}

bool AstraOmniboxPopupView::HandleKeyEvent(const ui::KeyEvent& event) {
  if (event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }

  switch (event.key_code()) {
    case ui::VKEY_DOWN:
      SelectNext();
      return true;
    case ui::VKEY_UP:
      SelectPrevious();
      return true;
    case ui::VKEY_RETURN:
      AcceptSelectedMatch();
      return true;
    case ui::VKEY_TAB:
      if (delegate_) {
        delegate_->OnOmniboxTabPressed();
      }
      return true;
    case ui::VKEY_ESCAPE:
      HidePopup();
      return true;
    default:
      return false;
  }
}

AstraOmniboxPopupMatchView* AstraOmniboxPopupView::GetMatchViewAt(
    size_t index) {
  if (index >= match_views_.size()) {
    return nullptr;
  }
  return match_views_[index];
}

std::u16string AstraOmniboxPopupView::GetWindowTitle() const {
  return u"Omnibox suggestions";
}

void AstraOmniboxPopupView::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);
  if (delegate_) {
    delegate_->OnOmniboxPopupClosed();
  }
}

gfx::Size AstraOmniboxPopupView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(kPopupWidth, kPopupMinHeight);
}

void AstraOmniboxPopupView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
}

void AstraOmniboxPopupView::OnSuggestionsChanged(
    AstraOmniboxPopupModel* model) {
  DCHECK_EQ(model_, model);
  RebuildMatches();
}

void AstraOmniboxPopupView::OnSelectedMatchChanged(
    AstraOmniboxPopupModel* model) {
  DCHECK_EQ(model_, model);
  UpdateSelectionHighlight();
  EnsureSelectedVisible();
}

void AstraOmniboxPopupView::OnPopupVisibilityChanged(
    AstraOmniboxPopupModel* model,
    bool visible) {
  DCHECK_EQ(model_, model);
  if (visible) {
    ShowPopup();
  } else {
    HidePopup();
  }
}

void AstraOmniboxPopupView::OnOmniboxPopupModelShutdown(
    AstraOmniboxPopupModel* model) {
  DCHECK_EQ(model_, model);
  model_observation_.Reset();
  model_ = nullptr;
}

void AstraOmniboxPopupView::Build() {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->ClipHeightTo(0, kPopupMaxHeight);
  scroll_view->SetBackgroundColor(SK_ColorWHITE);

  content_view_ = scroll_view->SetContents(std::make_unique<views::View>());
  content_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  scroll_view_ = AddChildView(std::move(scroll_view));
}

void AstraOmniboxPopupView::RebuildMatches() {
  if (!content_view_ || !model_) {
    return;
  }

  match_views_.clear();
  section_headers_.clear();
  content_view_->RemoveAllChildViews();

  const auto& matches = model_->GetMatches();
  if (matches.empty()) {
    // Empty state.
    // TODO(astra): Show empty state or hide popup.
    content_view_->InvalidateLayout();
    return;
  }

  // Build grouped matches.
  auto grouped = model_->GetGroupedMatches();

  size_t global_index = 0;
  for (const auto& group : grouped) {
    // Section header.
    auto header = std::make_unique<AstraOmniboxPopupSectionHeader>(
        group.first);
    section_headers_.push_back(header.get());
    content_view_->AddChildView(std::move(header));

    // Match views for this group.
    for (const auto& match : group.second) {
      auto match_view =
          std::make_unique<AstraOmniboxPopupMatchView>(match, global_index);
      AstraOmniboxPopupMatchView* raw_view = match_view.get();
      content_view_->AddChildView(std::move(match_view));
      match_views_.push_back(raw_view);
      global_index++;
    }
  }

  UpdateSelectionHighlight();
  content_view_->InvalidateLayout();
  if (scroll_view_) {
    scroll_view_->Layout();
  }
}

void AstraOmniboxPopupView::UpdateMatches() {
  if (!model_) {
    return;
  }
  const auto& matches = model_->GetMatches();
  for (size_t i = 0; i < match_views_.size() && i < matches.size(); i++) {
    match_views_[i]->UpdateFromMatch(matches[i]);
  }
}

void AstraOmniboxPopupView::UpdateSelectionHighlight() {
  if (!model_) {
    return;
  }
  size_t selected_index = model_->GetSelectedIndex();
  for (size_t i = 0; i < match_views_.size(); i++) {
    match_views_[i]->SetSelected(i == selected_index);
  }
}

void AstraOmniboxPopupView::EnsureSelectedVisible() {
  if (!model_ || !scroll_view_ || match_views_.empty()) {
    return;
  }
  size_t selected_index = model_->GetSelectedIndex();
  if (selected_index >= match_views_.size()) {
    return;
  }

  // Scroll the selected item into view.
  views::View* selected_view = match_views_[selected_index];
  if (selected_view) {
    scroll_view_->ScrollContentsToShow(
        gfx::Point(0, selected_view->y()),
        selected_view->size());
  }
}

void AstraOmniboxPopupView::OnMatchClicked(size_t index) {
  if (!model_) {
    return;
  }
  model_->SetSelectedIndex(index);
  AcceptSelectedMatch();
}

}  // namespace astra
