// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/autofill_popup/astra_autofill_popup_view.h"

#include <algorithm>
#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/image_button.h"
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

// ===========================================================================
// AstraAutofillSuggestionView
// ===========================================================================

AstraAutofillSuggestionView::AstraAutofillSuggestionView(
    const AstraAutofillSuggestion& suggestion,
    int index)
    : suggestion_(suggestion), index_(index) {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  if (suggestion.is_separator) {
    // Separator row.
    SetPreferredSize(gfx::Size(0, 1));
    return;
  }

  // Icon.
  auto icon_view = std::make_unique<views::ImageView>();
  icon_view->SetImageSize(gfx::Size(kIconSize, kIconSize));
  icon_view_ = AddChildView(std::move(icon_view));

  // Main text.
  auto main_label = std::make_unique<views::Label>(suggestion.main_text);
  main_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  main_label->SetAutoColorId(ui::kColorLabelForeground);
  main_label->SetElideBehavior(gfx::ELIDE_TAIL);
  main_label_ = AddChildView(std::move(main_label));

  // Secondary text (optional).
  if (!suggestion.secondary_text.empty()) {
    auto secondary_label =
        std::make_unique<views::Label>(suggestion.secondary_text);
    secondary_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    secondary_label->SetAutoColorId(ui::kColorLabelForegroundSecondary);
    secondary_label->SetElideBehavior(gfx::ELIDE_TAIL);
    secondary_label_ = AddChildView(std::move(secondary_label));
  }

  // Delete button.
  if (suggestion.deletable) {
    auto delete_button = std::make_unique<views::ImageButton>();
    delete_button->SetTooltipText(u"Remove");
    delete_button->SetAccessibleName(u"Remove suggestion");
    delete_button_ = AddChildView(std::move(delete_button));
  }

  UpdateVisuals();
}

AstraAutofillSuggestionView::~AstraAutofillSuggestionView() = default;

void AstraAutofillSuggestionView::UpdateFromSuggestion(
    const AstraAutofillSuggestion& suggestion) {
  suggestion_ = suggestion;
  if (main_label_) {
    main_label_->SetText(suggestion.main_text);
  }
  if (secondary_label_ && !suggestion.secondary_text.empty()) {
    secondary_label_->SetText(suggestion.secondary_text);
  }
  UpdateVisuals();
}

void AstraAutofillSuggestionView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  UpdateVisuals();
}

gfx::Size AstraAutofillSuggestionView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  if (suggestion_.is_separator) {
    return gfx::Size(0, 1);
  }
  int height =
      suggestion_.secondary_text.empty() ? kRowHeight : kTallRowHeight;
  if (suggestion_.is_instruction) {
    height = kRowHeight;
  }
  return gfx::Size(0, height);
}

void AstraAutofillSuggestionView::Layout() {
  if (suggestion_.is_separator) {
    return;
  }

  gfx::Rect bounds = GetContentsBounds();
  int y = bounds.y();
  int height = bounds.height();

  // Icon on the left.
  int icon_x = kIconPadding;
  int icon_y = y + (height - kIconSize) / 2;
  if (icon_view_) {
    icon_view_->SetBounds(icon_x, icon_y, kIconSize, kIconSize);
  }

  // Delete button on the right.
  int delete_size = 20;
  int delete_x = bounds.right() - kRightPadding - delete_size;
  int delete_y = y + (height - delete_size) / 2;
  if (delete_button_ && delete_button_->GetVisible()) {
    delete_button_->SetBounds(delete_x, delete_y, delete_size, delete_size);
    delete_x -= kTextSpacing;
  } else {
    delete_x = bounds.right() - kRightPadding;
  }

  // Text area.
  int text_x = icon_x + kIconSize + kTextSpacing;
  int text_width = delete_x - text_x;

  if (secondary_label_ && secondary_label_->GetVisible()) {
    // Two-line layout.
    main_label_->SetBounds(text_x, y + 4, text_width, 18);
    secondary_label_->SetBounds(text_x, y + 24, text_width, 14);
  } else {
    // Single-line layout.
    main_label_->SetBounds(text_x, y + (height - 16) / 2, text_width, 16);
  }
}

void AstraAutofillSuggestionView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateVisuals();
}

void AstraAutofillSuggestionView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  if (suggestion_.is_separator) {
    cc::PaintFlags flags;
    flags.setColor(SkColorSetA(SK_ColorBLACK, 0x0D));
    canvas->DrawRect(gfx::Rect(0, 0, bounds.width(), 1), flags);
    return;
  }

  // Selected/hover background.
  if (selected_ || hovered_) {
    cc::PaintFlags flags;
    flags.setColor(selected_ ? SkColorSetA(SK_ColorBLACK, 0x14)
                              : SkColorSetA(SK_ColorBLACK, 0x0D));
    canvas->DrawRect(bounds, flags);
  }

  // Warning style.
  if (suggestion_.is_warning) {
    // TODO(astra): Add warning styling.
  }
}

void AstraAutofillSuggestionView::OnMouseEntered(
    const ui::MouseEvent& event) {
  hovered_ = true;
  SchedulePaint();
}

void AstraAutofillSuggestionView::OnMouseExited(
    const ui::MouseEvent& event) {
  hovered_ = false;
  SchedulePaint();
}

bool AstraAutofillSuggestionView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (event.IsLeftMouseButton() && !suggestion_.is_separator) {
    return true;
  }
  return views::View::OnMousePressed(event);
}

void AstraAutofillSuggestionView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (event.IsLeftMouseButton() && HitTestPoint(event.location()) &&
      !suggestion_.is_separator) {
    HandleClick();
    return;
  }
  views::View::OnMouseReleased(event);
}

void AstraAutofillSuggestionView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->SetName(suggestion_.main_text);
  if (!suggestion_.secondary_text.empty()) {
    node_data->SetDescription(suggestion_.secondary_text);
  }
  node_data->role = ax::mojom::Role::kListBoxOption;
}

void AstraAutofillSuggestionView::HandleClick() {
  // Parent handles click via index.
}

void AstraAutofillSuggestionView::UpdateVisuals() {
  // Update icon based on type (stub).
  // TODO(astra): Use proper vector icons from Chromium's icon set.

  if (main_label_) {
    main_label_->SetEnabled(!suggestion_.is_instruction);
  }

  SchedulePaint();
}

// ===========================================================================
// AstraAutofillPopupView
// ===========================================================================

AstraAutofillPopupView::AstraAutofillPopupView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                        views::BubbleBorder::TOP_LEFT) {
  set_close_on_deactivate(false);
  set_close_on_esc(false);
  set_margins(gfx::Insets());
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowCloseButton(false);
  set_adjust_if_offscreen(true);
  set_shadow(views::BubbleBorder::STANDARD_SHADOW);

  Build();
}

AstraAutofillPopupView::~AstraAutofillPopupView() = default;

void AstraAutofillPopupView::SetSuggestions(
    const std::vector<AstraAutofillSuggestion>& suggestions) {
  suggestions_ = suggestions;
  selected_index_ = suggestions_.empty() ? -1 : 0;
  RebuildSuggestions();
}

const std::vector<AstraAutofillSuggestion>&
AstraAutofillPopupView::GetSuggestions() const {
  return suggestions_;
}

size_t AstraAutofillPopupView::GetSuggestionCount() const {
  return suggestions_.size();
}

void AstraAutofillPopupView::SetSelectedIndex(int index) {
  if (index < 0 || index >= static_cast<int>(suggestion_views_.size())) {
    return;
  }
  if (selected_index_ == index) {
    return;
  }
  selected_index_ = index;
  UpdateSelectionHighlight();
}

void AstraAutofillPopupView::SelectNext() {
  if (suggestion_views_.empty()) {
    return;
  }
  int next = selected_index_ + 1;
  if (next >= static_cast<int>(suggestion_views_.size())) {
    next = 0;
  }
  SetSelectedIndex(next);
}

void AstraAutofillPopupView::SelectPrevious() {
  if (suggestion_views_.empty()) {
    return;
  }
  int prev = selected_index_ - 1;
  if (prev < 0) {
    prev = static_cast<int>(suggestion_views_.size()) - 1;
  }
  SetSelectedIndex(prev);
}

void AstraAutofillPopupView::ShowPopup() {
  views::Widget* widget = GetWidget();
  if (!widget) {
    return;
  }
  widget->Show();
}

void AstraAutofillPopupView::HidePopup() {
  views::Widget* widget = GetWidget();
  if (widget) {
    widget->Hide();
  }
}

bool AstraAutofillPopupView::IsPopupShowing() const {
  const views::Widget* widget = GetWidget();
  return widget && widget->IsVisible();
}

void AstraAutofillPopupView::SetFooterText(const std::u16string& text) {
  if (footer_label_) {
    footer_label_->SetText(text);
  }
}

void AstraAutofillPopupView::SetFooterVisible(bool visible) {
  if (footer_) {
    footer_->SetVisible(visible);
  }
}

bool AstraAutofillPopupView::HandleKeyEvent(const ui::KeyEvent& event) {
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
      AcceptSelectedSuggestion();
      return true;
    case ui::VKEY_ESCAPE:
      HidePopup();
      return true;
    case ui::VKEY_DELETE:
    case ui::VKEY_BACK:
      // Delete selected suggestion if deletable.
      if (selected_index_ >= 0 && delegate_ &&
          selected_index_ < static_cast<int>(suggestions_.size()) &&
          suggestions_[selected_index_].deletable) {
        delegate_->OnAutofillSuggestionDeleted(
            suggestions_[selected_index_].suggestion_id);
      }
      return false;  // Let the field handle the keystroke too.
    default:
      return false;
  }
}

std::u16string AstraAutofillPopupView::GetWindowTitle() const {
  return u"Autofill suggestions";
}

void AstraAutofillPopupView::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);
  if (delegate_) {
    delegate_->OnAutofillPopupClosed();
  }
}

gfx::Size AstraAutofillPopupView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int height = kFooterHeight;
  // Estimate based on suggestion count.
  for (const auto& s : suggestions_) {
    if (s.is_separator) {
      height += 1;
    } else if (s.secondary_text.empty()) {
      height += 40;
    } else {
      height += 52;
    }
  }
  height = std::min(height, kPopupMaxHeight);
  return gfx::Size(kPopupWidth, height);
}

void AstraAutofillPopupView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
}

void AstraAutofillPopupView::Build() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Scroll view for suggestions.
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->ClipHeightTo(0, kPopupMaxHeight - kFooterHeight);

  content_view_ = scroll_view->SetContents(std::make_unique<views::View>());
  content_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  scroll_view_ = AddChildView(std::move(scroll_view));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kUnbounded));

  // Footer.
  auto footer = std::make_unique<views::View>();
  footer->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(0, 12)));
  footer->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SkColorSetA(SK_ColorBLACK, 0x0D)));

  auto footer_label = std::make_unique<views::Label>(u"Manage...");
  footer_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  footer_label->SetAutoColorId(ui::kColorLinkForeground);
  footer_label_ = footer->AddChildView(std::move(footer_label));

  footer_ = AddChildView(std::move(footer));
  footer_->SetPreferredSize(gfx::Size(kPopupWidth, kFooterHeight));
}

void AstraAutofillPopupView::RebuildSuggestions() {
  if (!content_view_) {
    return;
  }

  suggestion_views_.clear();
  content_view_->RemoveAllChildViews();

  for (size_t i = 0; i < suggestions_.size(); i++) {
    auto view = std::make_unique<AstraAutofillSuggestionView>(
        suggestions_[i], static_cast<int>(i));
    AstraAutofillSuggestionView* raw_view = view.get();
    content_view_->AddChildView(std::move(view));
    suggestion_views_.push_back(raw_view);
  }

  UpdateSelectionHighlight();
  content_view_->InvalidateLayout();
  if (scroll_view_) {
    scroll_view_->Layout();
  }
}

void AstraAutofillPopupView::UpdateSelectionHighlight() {
  for (size_t i = 0; i < suggestion_views_.size(); i++) {
    suggestion_views_[i]->SetSelected(
        static_cast<int>(i) == selected_index_);
  }
}

void AstraAutofillPopupView::AcceptSelectedSuggestion() {
  if (selected_index_ < 0 || selected_index_ >= static_cast<int>(suggestions_.size())) {
    return;
  }
  if (delegate_) {
    delegate_->OnAutofillSuggestionAccepted(
        suggestions_[selected_index_].suggestion_id);
  }
}

void AstraAutofillPopupView::OnSuggestionClicked(int index) {
  SetSelectedIndex(index);
  AcceptSelectedSuggestion();
}

}  // namespace astra
