// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/workspace/astra_workspace_hibernation_view.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

#include "astra/browser/astra_workspace_service.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 360;
constexpr int kItemHeight = 72;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 8;
constexpr int kAccentDotSize = 10;
constexpr int kMaxVisibleItems = 6;

// Parse a hex color string like "#RRGGBB" to SkColor.
SkColor ParseHexColor(const std::string& hex) {
  if (hex.empty() || hex[0] != '#') return SK_ColorGRAY;
  std::string clean = hex.substr(1);
  if (clean.size() != 6) return SK_ColorGRAY;
  unsigned int val = 0;
  if (sscanf(clean.c_str(), "%x", &val) != 1) return SK_ColorGRAY;
  return SkColorSetRGB(SkColorGetR(val), SkColorGetG(val), SkColorGetB(val));
}

// Format relative time string (e.g. "3 days ago", "1 week ago").
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
  if (weeks < 4) {
    return base::UTF8ToUTF16(
        std::to_string(weeks) + " weeks ago");
  }
  int months = days / 30;
  if (months == 0) return u"1 month ago";
  return base::UTF8ToUTF16(
      std::to_string(months) + " months ago");
}

// Format memory size (e.g. "256 MB", "1.2 GB").
std::u16string FormatMemoryBytes(int64_t bytes) {
  if (bytes < 1024) return u"0 KB";
  if (bytes < 1024 * 1024) {
    return base::UTF8ToUTF16(
        std::to_string(bytes / 1024) + " KB";
  }
  if (bytes < 1024LL * 1024 * 1024) {
    int mb = bytes / (1024 * 1024);
    return base::UTF8ToUTF16(std::to_string(mb) + " MB");
  }
  int gb = bytes / (1024 * 1024 * 1024);
  return base::UTF8ToUTF16(std::to_string(gb) + " GB");
}

// Estimate memory saved based on tab count.
int64_t EstimateMemorySaved(int tab_count, int window_count) {
  // Rough estimate: ~20 MB per tab average.
  return static_cast<int64_t>(tab_count) * 20LL * 1024LL * 1024LL;
}

}  // namespace

// ===========================================================================
// AstraHibernatedWorkspaceItemView
// ===========================================================================

AstraHibernatedWorkspaceItemView::AstraHibernatedWorkspaceItemView(
    const AstraWorkspace& workspace,
    RestoreCallback restore_callback,
    DeleteCallback delete_callback)
    : workspace_id_(workspace.id),
      workspace_name_(workspace.name),
      accent_color_(workspace.color),
      tab_count_(workspace.tab_count),
      window_count_(workspace.window_count),
      last_used_time_(workspace.last_used_time),
      memory_saved_bytes_(EstimateMemorySaved(workspace.tab_count,
                       workspace.window_count)),
      restore_callback_(std::move(restore_callback)),
      delete_callback_(std::move(delete_callback)) {
  BuildLayout();
}

AstraHibernatedWorkspaceItemView::~AstraHibernatedWorkspaceItemView() = default;

void AstraHibernatedWorkspaceItemView::BuildLayout() {
  SetPreferredSize(gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(12, 14),
      12));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Accent color dot.
  accent_dot_ = AddChildView(std::make_unique<views::View>());
  accent_dot_->SetPreferredSize(
      gfx::Size(kAccentDotSize, kAccentDotSize));
  accent_dot_->SetBackground(
      views::CreateRoundedRectBackground(
          ParseHexColor(accent_color_), kAccentDotSize / 2));

  // Text column.
  auto* text_col = AddChildView(std::make_unique<views::View>());
  text_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(), 3));
  text_col->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  name_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(workspace_name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  detail_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          std::to_string(tab_count_) + " tabs · " +
          base::UTF16ToUTF8(FormatRelativeTime(last_used_time_)))));
  detail_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_label_->SetAutoColorReadabilityEnabled(false);
  detail_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  memory_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(u"💾 " +
          FormatMemoryBytes(memory_saved_bytes_) + u" saved"));
  memory_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  memory_label_->SetAutoColorReadabilityEnabled(false);
  memory_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Restore button.
  restore_button_ = AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraHibernatedWorkspaceItemView::OnRestoreClicked,
              base::Unretained(this)),
          u"Restore"));
}

void AstraHibernatedWorkspaceItemView::OnRestoreClicked() {
  if (restore_callback_) {
    restore_callback_.Run(workspace_id_);
  }
}

void AstraHibernatedWorkspaceItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  detail_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  memory_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraWorkspaceHibernationView
// ===========================================================================

AstraWorkspaceHibernationView::AstraWorkspaceHibernationView(
    views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraWorkspaceHibernationView::~AstraWorkspaceHibernationView() = default;

void AstraWorkspaceHibernationView::SetHibernatedWorkspaces(
    const std::vector<AstraWorkspace>& workspaces) {
  hibernated_workspaces_ = workspaces;
  RefreshWorkspaceList();
}

void AstraWorkspaceHibernationView::SetAutoHibernateEnabled(bool enabled) {
  auto_hibernate_enabled_ = enabled;
}

void AstraWorkspaceHibernationView::SetAutoHibernateHours(int hours) {
  auto_hibernate_hours_ = hours;
}

void AstraWorkspaceHibernationView::SetRestoreCallback(
    RestoreCallback callback) {
  restore_callback_ = std::move(callback);
}

void AstraWorkspaceHibernationView::SetDeleteCallback(
    DeleteCallback callback) {
  delete_callback_ = std::move(callback);
}

void AstraWorkspaceHibernationView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildInfoSection();
  BuildWorkspaceList();
  BuildSettingsSection();
}

void AstraWorkspaceHibernationView::BuildInfoSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 6));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  info_label_ = section->AddChildView(
      std::make_unique<views::Label>(
          u"💤 Hibernation saves memory by unloading "
          u"inactive workspaces."));
  info_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  info_label_->SetAutoColorReadabilityEnabled(false);
  info_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  info_label_->SetMultiLine(true);
}

void AstraWorkspaceHibernationView::BuildWorkspaceList() {
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
      std::make_unique<views::Label>(u"Hibernated (0)"));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  scroll_view_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(kItemHeight * kMaxVisibleItems +
                                + kItemSpacing * (kMaxVisibleItems - 1));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  workspace_list_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  workspace_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraWorkspaceHibernationView::BuildSettingsSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));

  auto* label = section->AddChildView(
      std::make_unique<views::Label>(u"⚙ Auto-hibernate"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  auto* desc = section->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          "After " + std::to_string(auto_hibernate_hours_) +
          " hours of inactivity")));
  desc->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  desc->SetAutoColorReadabilityEnabled(false);
  desc->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraWorkspaceHibernationView::RefreshWorkspaceList() {
  if (!workspace_list_) return;

  workspace_list_->RemoveAllChildViews();
  workspace_items_.clear();

  if (count_label_) {
    count_label_->SetText(u"Hibernated (" +
        base::UTF8ToUTF16(
            std::to_string(hibernated_workspaces_.size())) +
        u")");
  }

  for (const auto& ws : hibernated_workspaces_) {
    auto* item = workspace_list_->AddChildView(
        std::make_unique<AstraHibernatedWorkspaceItemView>(
            ws,
            base::BindRepeating(
                &AstraWorkspaceHibernationView::OnRestoreWorkspace,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraWorkspaceHibernationView::OnDeleteWorkspace,
                base::Unretained(this))));
    workspace_items_.push_back(item);
  }

  InvalidateLayout();
}

void AstraWorkspaceHibernationView::OnRestoreWorkspace(
    const std::string& workspace_id) {
  if (restore_callback_) {
    restore_callback_.Run(workspace_id);
  }
}

void AstraWorkspaceHibernationView::OnDeleteWorkspace(
    const std::string& workspace_id) {
  if (delete_callback_) {
    delete_callback_.Run(workspace_id);
  }
}

void AstraWorkspaceHibernationView::OnAutoHibernateToggled() {
  // TODO(astra): Toggle auto-hibernate setting.
}

std::u16string AstraWorkspaceHibernationView::GetWindowTitle() const {
  return u"Hibernated Workspaces";
}

void AstraWorkspaceHibernationView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
}

}  // namespace astra
