// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_features/astra_tab_features_view.h"

#include <memory>
#include <string>

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
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 320;
constexpr int kSectionPadding = 16;
constexpr int kSectionSpacing = 12;
constexpr int kNoteFieldHeight = 60;

// Format relative time (e.g. "3 hours ago", "2 days ago").
std::u16string FormatRelativeTime(base::Time time) {
  if (time.is_null()) return u"—";

  base::TimeDelta delta = base::Time::Now() - time;
  int days = delta.InDays();

  if (days == 0) {
    int hours = delta.InHours();
    if (hours == 0) {
      int minutes = delta.InMinutes();
      if (minutes < 1) return u"just now";
      return base::UTF8ToUTF16(
          std::to_string(minutes) + " min ago");
    }
    return base::UTF8ToUTF16(
        std::to_string(hours) + " hour" + (hours != 1 ? "s" : "") + " ago");
  }
  if (days == 1) return u"yesterday";
  if (days < 30) {
    return base::UTF8ToUTF16(
        std::to_string(days) + " days ago");
  }
  int months = days / 30;
  return base::UTF8ToUTF16(
      std::to_string(months) + " month" + (months != 1 ? "s" : "") + " ago");
}

}  // namespace

// ===========================================================================
// AstraTabFeaturesView
// ===========================================================================

AstraTabFeaturesView::AstraTabFeaturesView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                         views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabFeaturesView::~AstraTabFeaturesView() = default;

void AstraTabFeaturesView::SetTabTitle(const std::u16string& title) {
  tab_title_ = title;
  if (title_label_) {
    title_label_->SetText(title);
  }
}

void AstraTabFeaturesView::SetTabUrl(const std::string& url) {
  tab_url_ = url;
  if (url_label_) {
    url_label_->SetText(base::UTF8ToUTF16(url));
  }
}

void AstraTabFeaturesView::SetWorkspaceName(const std::u16string& name) {
  workspace_name_ = name;
  if (workspace_button_) {
    workspace_button_->SetText(name);
  }
}

void AstraTabFeaturesView::SetWorkspaceId(const std::string& id) {
  workspace_id_ = id;
}

void AstraTabFeaturesView::SetIsFavorite(bool is_favorite) {
  is_favorite_ = is_favorite;
  UpdateFavoriteButton();
}

void AstraTabFeaturesView::SetInReadingList(bool in_reading_list) {
  in_reading_list_ = in_reading_list;
  UpdateReadingListButton();
}

void AstraTabFeaturesView::SetNote(const std::u16string& note) {
  if (note_field_) {
    note_field_->SetText(note);
  }
}

std::u16string AstraTabFeaturesView::GetNote() const {
  return note_field_ ? note_field_->GetText() : std::u16string();
}

void AstraTabFeaturesView::SetTags(const std::vector<std::string>& tags) {
  tags_ = tags;
  // TODO(astra): Refresh tags container.
}

void AstraTabFeaturesView::SetLastVisited(base::Time time) {
  last_visited_ = time;
  if (last_visited_label_) {
    last_visited_label_->SetText(
        u"Last visited: " + FormatRelativeTime(time));
  }
}

void AstraTabFeaturesView::SetAddedTime(base::Time time) {
  added_time_ = time;
  if (added_time_label_) {
    added_time_label_->SetText(
        u"Added: " + FormatRelativeTime(time));
  }
}

void AstraTabFeaturesView::SetIsPinned(bool is_pinned) {
  is_pinned_ = is_pinned;
}

// -- Callbacks -------------------------------------------------------------

void AstraTabFeaturesView::SetWorkspaceChangedCallback(
    WorkspaceChangedCallback callback) {
  workspace_changed_callback_ = std::move(callback);
}

void AstraTabFeaturesView::SetFavoriteToggledCallback(
    FavoriteToggledCallback callback) {
  favorite_toggled_callback_ = std::move(callback);
}

void AstraTabFeaturesView::SetReadingListToggledCallback(
    ReadingListToggledCallback callback) {
  reading_list_toggled_callback_ = std::move(callback);
}

void AstraTabFeaturesView::SetNoteUpdatedCallback(
    NoteUpdatedCallback callback) {
  note_updated_callback_ = std::move(callback);
}

// -- UI building -----------------------------------------------------------

void AstraTabFeaturesView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildTabInfoHeader();
  BuildWorkspaceSection();
  BuildFavoriteSection();
  BuildReadingListSection();
  BuildNoteSection();
  BuildTagsSection();
  BuildMetadataSection();
}

void AstraTabFeaturesView::BuildTabInfoHeader() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 4));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  title_label_ = section->AddChildView(
      std::make_unique<views::Label>(tab_title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_DIALOG_TITLE,
          views::style::STYLE_PRIMARY));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  url_label_ = section->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(tab_url_)));
  url_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  url_label_->SetAutoColorReadabilityEnabled(false);
  url_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  url_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
}

void AstraTabFeaturesView::BuildWorkspaceSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* label = section->AddChildView(
      std::make_unique<views::Label>(u"📌 Workspace"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  workspace_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabFeaturesView::OnWorkspaceClicked,
              base::Unretained(this)),
          workspace_name_.empty() ? u"(none)" : workspace_name_));
}

void AstraTabFeaturesView::BuildFavoriteSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* label = section->AddChildView(
      std::make_unique<views::Label>(u"⭐ Favorite"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  label->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  favorite_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabFeaturesView::OnFavoriteToggled,
              base::Unretained(this)),
          u"Add"));
  UpdateFavoriteButton();
}

void AstraTabFeaturesView::BuildReadingListSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* label = section->AddChildView(
      std::make_unique<views::Label>(u"📚 Reading List"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  label->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  reading_list_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabFeaturesView::OnReadingListToggled,
              base::Unretained(this)),
          u"Add"));
  UpdateReadingListButton();
}

void AstraTabFeaturesView::BuildNoteSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* label = section->AddChildView(
      std::make_unique<views::Label>(u"📝 Note"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  note_field_ = section->AddChildView(
      std::make_unique<views::Textfield>());
  note_field_->SetPlaceholderText(u"Add a note about this tab..."));
  note_field_->SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kNoteFieldHeight));
  note_field_->SetBorder(views::CreateRoundedRectBorder(
      1, 4, SK_ColorGRAY));
  note_field_->set_controller(
      base::BindRepeating(
          &AstraTabFeaturesView::OnNoteChanged,
          base::Unretained(this)));
}

void AstraTabFeaturesView::BuildTagsSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* label = section->AddChildView(
      std::make_unique<views::Label>(u"🏷️  Tags"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  tags_container_ = section->AddChildView(
      std::make_unique<views::View>());
  tags_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetDefault(views::kFlexBehaviorKey,
                  views::FlexSpecification(
                      views::LayoutFlexOrientation::kHorizontal,
                      views::MinimumFlexSizeRule::kPreferred,
                      views::MaximumFlexSizeRule::kPreferred))
      .SetWrapBehavior(views::FlexLayout::WrapBehavior::kWrap));

  // Add tag button.
  auto* add_tag_button = tags_container_->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabFeaturesView::OnAddTagClicked,
              base::Unretained(this)),
          u"+ add"));
}

void AstraTabFeaturesView::BuildMetadataSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 6));

  last_visited_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Last visited: —"));
  last_visited_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  last_visited_label_->SetAutoColorReadabilityEnabled(false);
  last_visited_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  added_time_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Added: —"));
  added_time_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  added_time_label_->SetAutoColorReadabilityEnabled(false);
  added_time_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

// -- Update helpers --------------------------------------------------------

void AstraTabFeaturesView::UpdateFavoriteButton() {
  if (!favorite_button_) return;
  favorite_button_->SetText(is_favorite_ ? u"Remove" : u"Add");
  if (is_favorite_) {
    favorite_button_->SetProminent(true);
  } else {
    favorite_button_->SetProminent(false);
  }
}

void AstraTabFeaturesView::UpdateReadingListButton() {
  if (!reading_list_button_) return;
  reading_list_button_->SetText(
      in_reading_list_ ? u"Remove" : u"Add");
  if (in_reading_list_) {
    reading_list_button_->SetProminent(true);
  } else {
    reading_list_button_->SetProminent(false);
  }
}

// -- Event handlers --------------------------------------------------------

void AstraTabFeaturesView::OnFavoriteToggled() {
  is_favorite_ = !is_favorite_;
  UpdateFavoriteButton();
  if (favorite_toggled_callback_) {
    favorite_toggled_callback_.Run(is_favorite_);
  }
}

void AstraTabFeaturesView::OnReadingListToggled() {
  in_reading_list_ = !in_reading_list_;
  UpdateReadingListButton();
  if (reading_list_toggled_callback_) {
    reading_list_toggled_callback_.Run(in_reading_list_);
  }
}

void AstraTabFeaturesView::OnWorkspaceClicked() {
  // TODO(astra): Show workspace picker.
}

void AstraTabFeaturesView::OnNoteChanged() {
  if (note_updated_callback_) {
    note_updated_callback_.Run(note_field_->GetText());
  }
}

void AstraTabFeaturesView::OnAddTagClicked() {
  // TODO(astra): Show tag input dialog.
}

// -- views overrides -------------------------------------------------------

std::u16string AstraTabFeaturesView::GetWindowTitle() const {
  return u"Tab Features";
}

void AstraTabFeaturesView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
}

}  // namespace astra
