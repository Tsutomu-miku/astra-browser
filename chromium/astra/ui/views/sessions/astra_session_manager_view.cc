// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/sessions/astra_session_manager_view.h"

#include <algorithm>
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
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 360;
constexpr int kItemHeight = 88;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 8;
constexpr int kMaxVisibleItems = 4;

// Format relative time.
std::u16string FormatRelativeTime(base::Time time) {
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
        std::to_string(hours) + "h ago");
  }
  if (days == 1) return u"yesterday";
  if (days < 7) {
    return base::UTF8ToUTF16(
        std::to_string(days) + " days ago");
  }
  int weeks = days / 7;
  if (weeks == 1) return u"1 week ago";
  return base::UTF8ToUTF16(
      std::to_string(weeks) + " weeks ago");
}

}  // namespace

// ===========================================================================
// AstraSessionSnapshotItemView
// ===========================================================================

AstraSessionSnapshotItemView::AstraSessionSnapshotItemView(
    const SnapshotInfo& info,
    RestoreCallback restore_callback,
    DeleteCallback delete_callback,
    RenameCallback rename_callback)
    : session_id_(info.session_id),
      name_(info.name),
      description_(info.description),
      tab_count_(info.tab_count),
      window_count_(info.window_count),
      created_at_(info.created_at),
      workspace_id_(info.workspace_id),
      restore_callback_(std::move(restore_callback)),
      delete_callback_(std::move(delete_callback)),
      rename_callback_(std::move(rename_callback)) {
  BuildLayout();
}

AstraSessionSnapshotItemView::~AstraSessionSnapshotItemView() = default;

void AstraSessionSnapshotItemView::BuildLayout() {
  SetPreferredSize(gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(10, 12),
      6));
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Top row: name + restore button.
  auto* top_row = AddChildView(std::make_unique<views::View>());
  top_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  top_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Icon + name.
  auto* name_col = top_row->AddChildView(std::make_unique<views::View>());
  name_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  name_col->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  name_col->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  auto* icon = name_col->AddChildView(
      std::make_unique<views::Label>(u"📸"));
  icon->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  icon->SetAutoColorReadabilityEnabled(false);
  icon->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  name_label_ = name_col->AddChildView(
      std::make_unique<views::Label>(name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  name_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Restore button.
  restore_button_ = top_row->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraSessionSnapshotItemView::OnRestoreClicked,
              base::Unretained(this)),
          u"Restore"));

  // Detail line.
  detail_label_ = AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          std::to_string(tab_count_) + " tabs" +
          (window_count_ > 1 ?
              " · " + std::to_string(window_count_) + " windows" : "") +
          " · " + base::UTF16ToUTF8(FormatRelativeTime(created_at_)))));
  detail_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_label_->SetAutoColorReadabilityEnabled(false);
  detail_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Action buttons row.
  auto* actions_row = AddChildView(std::make_unique<views::View>());
  actions_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 6));

  rename_button_ = actions_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSessionSnapshotItemView::OnRenameClicked,
              base::Unretained(this)),
          u"Rename"));

  delete_button_ = actions_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSessionSnapshotItemView::OnDeleteClicked,
              base::Unretained(this)),
          u"Delete"));
}

void AstraSessionSnapshotItemView::SetName(const std::u16string& name) {
  name_ = name;
  if (name_label_) {
    name_label_->SetText(name);
  }
}

void AstraSessionSnapshotItemView::OnRestoreClicked() {
  if (restore_callback_) {
    restore_callback_.Run(session_id_);
  }
}

void AstraSessionSnapshotItemView::OnDeleteClicked() {
  if (delete_callback_) {
    delete_callback_.Run(session_id_);
  }
}

void AstraSessionSnapshotItemView::OnRenameClicked() {
  if (rename_callback_) {
    rename_callback_.Run(session_id_, name_);
  }
}

void AstraSessionSnapshotItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  detail_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraSessionManagerView
// ===========================================================================

AstraSessionManagerView::AstraSessionManagerView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraSessionManagerView::~AstraSessionManagerView() = default;

void AstraSessionManagerView::SetSnapshots(
    const std::vector<AstraSessionSnapshotItemView::SnapshotInfo>& snapshots) {
  snapshots_ = snapshots;
  RefreshSnapshots();
}

void AstraSessionManagerView::SetSaveSessionCallback(
    SaveSessionCallback callback) {
  save_callback_ = std::move(callback);
}

void AstraSessionManagerView::SetRestoreSessionCallback(
    RestoreSessionCallback callback) {
  restore_callback_ = std::move(callback);
}

void AstraSessionManagerView::SetDeleteSessionCallback(
    DeleteSessionCallback callback) {
  delete_callback_ = std::move(callback);
}

void AstraSessionManagerView::SetRenameSessionCallback(
    RenameSessionCallback callback) {
  rename_callback_ = std::move(callback);
}

void AstraSessionManagerView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildSaveButton();
  BuildSnapshotsList();
}

void AstraSessionManagerView::BuildSaveButton() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  save_button_ = section->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraSessionManagerView::OnSaveClicked,
              base::Unretained(this)),
          u"📸 Save current session"));
  save_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraSessionManagerView::BuildSnapshotsList() {
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

  count_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Saved Sessions (0)"));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  scroll_view_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(kItemHeight * kMaxVisibleItems +
                              kItemSpacing * (kMaxVisibleItems - 1));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  snapshots_list_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  snapshots_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraSessionManagerView::RefreshSnapshots() {
  if (!snapshots_list_) return;

  snapshots_list_->RemoveAllChildViews();
  snapshot_views_.clear();

  if (count_label_) {
    count_label_->SetText(base::UTF8ToUTF16(
        "Saved Sessions (" +
        std::to_string(snapshots_.size()) + ")"));
  }

  // Sort by date descending (newest first).
  auto sorted = snapshots_;
  std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) {
                return a.created_at > b.created_at;
              });

  for (const auto& snapshot : sorted) {
    auto* item = snapshots_list_->AddChildView(
        std::make_unique<AstraSessionSnapshotItemView>(
            snapshot,
            base::BindRepeating(
                &AstraSessionManagerView::OnRestoreSession,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraSessionManagerView::OnDeleteSession,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraSessionManagerView::OnRenameSession,
                base::Unretained(this))));
    snapshot_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraSessionManagerView::OnSaveClicked() {
  if (save_callback_) {
    save_callback_.Run();
  }
}

void AstraSessionManagerView::OnRestoreSession(
    const std::string& session_id) {
  if (restore_callback_) {
    restore_callback_.Run(session_id);
  }
}

void AstraSessionManagerView::OnDeleteSession(
    const std::string& session_id) {
  if (delete_callback_) {
    delete_callback_.Run(session_id);
  }
}

void AstraSessionManagerView::OnRenameSession(
    const std::string& session_id,
    const std::u16string& new_name) {
  if (rename_callback_) {
    rename_callback_.Run(session_id, new_name);
  }
}

std::u16string AstraSessionManagerView::GetWindowTitle() const {
  return u"Session Manager";
}

void AstraSessionManagerView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
}

}  // namespace astra
