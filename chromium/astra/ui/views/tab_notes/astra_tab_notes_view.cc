// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_notes/astra_tab_notes_view.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
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
constexpr int kItemHeight = 64;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 4;
constexpr int kMaxVisibleItems = 6;
constexpr int kPreviewMaxLength = 60;

}  // namespace

// ===========================================================================
// AstraTabNoteItemView
// ===========================================================================

AstraTabNoteItemView::AstraTabNoteItemView(
    const NoteInfo& info,
    SelectCallback select_callback,
    DeleteCallback delete_callback)
    : note_id_(info.note_id),
      tab_id_(info.tab_id),
      page_title_(info.page_title),
      note_content_(info.note_content),
      page_url_(info.page_url),
      last_updated_(info.last_updated),
      has_note_(info.has_note),
      select_callback_(std::move(select_callback)),
      delete_callback_(std::move(delete_callback)) {
  BuildLayout();
}

AstraTabNoteItemView::~AstraTabNoteItemView() = default;

void AstraTabNoteItemView::SetContent(const std::u16string& content) {
  note_content_ = content;
  has_note_ = !content.empty();
  if (preview_label_) {
    preview_label_->SetText(note_content_);
  }
}

void AstraTabNoteItemView::BuildLayout() {
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(8, 12),
      4));
  SetBorder(views::CreateRoundedRectBorder(
      1, 8, SkColorSetA(SK_ColorGRAY, 0x40)));

  // Title row.
  title_label_ = AddChildView(
      std::make_unique<views::Label>(page_title_));
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

  // Preview + time row.
  auto* bottom_row = AddChildView(std::make_unique<views::View>());
  bottom_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  bottom_row->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Preview.
  std::u16string preview = note_content_;
  if (preview.size() > kPreviewMaxLength) {
    preview = preview.substr(0, kPreviewMaxLength) + u"...";
  }
  preview_label_ = bottom_row->AddChildView(
      std::make_unique<views::Label>(
          preview.empty() ? u"No note" : preview));
  preview_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  preview_label_->SetAutoColorReadabilityEnabled(false);
  preview_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  preview_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  preview_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Time.
  time_label_ = bottom_row->AddChildView(
      std::make_unique<views::Label>(FormatTime()));
  time_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  time_label_->SetAutoColorReadabilityEnabled(false);
  time_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

std::u16string AstraTabNoteItemView::FormatTime() {
  if (last_updated_.is_null()) {
    return u"";
  }

  base::TimeDelta delta = base::Time::Now() - last_updated_;

  if (delta < base::Minutes(1)) {
    return u"Just now";
  }
  if (delta < base::Hours(1)) {
    int mins = static_cast<int>(delta.InMinutes());
    return base::UTF8ToUTF16(std::to_string(mins) + "m ago");
  }
  if (delta < base::Days(1)) {
    int hours = static_cast<int>(delta.InHours());
    return base::UTF8ToUTF16(std::to_string(hours) + "h ago");
  }
  if (delta < base::Days(7)) {
    int days = static_cast<int>(delta.InDays());
    return base::UTF8ToUTF16(std::to_string(days) + "d ago");
  }

  return base::TimeFormatShortDate(last_updated_);
}

void AstraTabNoteItemView::OnSelect() {
  if (select_callback_) {
    select_callback_.Run(note_id_);
  }
}

void AstraTabNoteItemView::OnDelete() {
  if (delete_callback_) {
    delete_callback_.Run(note_id_);
  }
}

void AstraTabNoteItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  preview_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  time_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
}

// ===========================================================================
// AstraTabNotesView
// ===========================================================================

AstraTabNotesView::AstraTabNotesView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabNotesView::~AstraTabNotesView() = default;

void AstraTabNotesView::SetNotes(
    const std::vector<AstraTabNoteItemView::NoteInfo>& notes) {
  notes_ = notes;
  RefreshNotes();
}

void AstraTabNotesView::SetNoteSelectedCallback(
    NoteSelectedCallback callback) {
  select_callback_ = std::move(callback);
}

void AstraTabNotesView::SetNoteDeletedCallback(
    NoteDeletedCallback callback) {
  delete_callback_ = std::move(callback);
}

void AstraTabNotesView::SetAddNoteCallback(AddNoteCallback callback) {
  add_note_callback_ = std::move(callback);
}

void AstraTabNotesView::SetSearchCallback(SearchCallback callback) {
  search_callback_ = std::move(callback);
}

void AstraTabNotesView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildSearchBar();
  BuildNotesList();
  BuildFooter();
}

void AstraTabNotesView::BuildSearchBar() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  search_field_ = section->AddChildView(
      std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"🔍 Search notes...");
  search_field_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabNotesView::BuildNotesList() {
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

  notes_list_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  notes_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraTabNotesView::BuildFooter() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SkColorSetA(SK_ColorGRAY, 0x30)));

  add_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabNotesView::OnAddNote,
              base::Unretained(this)),
          u"+ Add note to current tab"));
}

void AstraTabNotesView::RefreshNotes() {
  if (!notes_list_) return;

  notes_list_->RemoveAllChildViews();
  note_views_.clear();

  for (const auto& note : notes_) {
    auto* item = notes_list_->AddChildView(
        std::make_unique<AstraTabNoteItemView>(
            note,
            base::BindRepeating(
                [](NoteSelectedCallback cb, const std::string& id) {
                  if (cb) cb.Run(id);
                },
                select_callback_),
            base::BindRepeating(
                [](NoteDeletedCallback cb, const std::string& id) {
                  if (cb) cb.Run(id);
                },
                delete_callback_)));
    note_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabNotesView::OnSearchTextChanged() {
  if (search_callback_ && search_field_) {
    search_callback_.Run(search_field_->GetText());
  }
}

void AstraTabNotesView::OnAddNote() {
  if (add_note_callback_) {
    add_note_callback_.Run();
  }
}

std::u16string AstraTabNotesView::GetWindowTitle() const {
  return u"Tab Notes";
}

void AstraTabNotesView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  // Sub-views handle their own theme changes.
}

}  // namespace astra
