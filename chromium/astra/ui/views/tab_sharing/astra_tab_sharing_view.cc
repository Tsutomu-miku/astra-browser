// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_sharing/astra_tab_sharing_view.h"

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

constexpr int kBubbleWidth = 340;
constexpr int kMethodItemHeight = 48;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 4;

}  // namespace

// ===========================================================================
// AstraShareMethodItemView
// ===========================================================================

AstraShareMethodItemView::AstraShareMethodItemView(
    const MethodInfo& info,
    SelectCallback select_callback)
    : method_id_(info.method_id),
      name_(info.name),
      description_(info.description),
      type_(info.type),
      select_callback_(std::move(select_callback)) {
  BuildLayout();
}

AstraShareMethodItemView::~AstraShareMethodItemView() = default;

void AstraShareMethodItemView::BuildLayout() {
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kMethodItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, 12),
      12));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Icon.
  icon_label_ = AddChildView(
      std::make_unique<views::Label>(MethodIcon(type_)));
  icon_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label_->SetAutoColorReadabilityEnabled(false);
  icon_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  icon_label_->SetPreferredSize(gfx::Size(28, 24));

  // Text column.
  auto* text_col = AddChildView(std::make_unique<views::View>());
  text_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(), 2));
  text_col->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  name_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  desc_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(description_));
  desc_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  desc_label_->SetAutoColorReadabilityEnabled(false);
  desc_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  desc_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
}

void AstraShareMethodItemView::OnClicked() {
  if (select_callback_) {
    select_callback_.Run(method_id_);
  }
}

std::u16string AstraShareMethodItemView::MethodIcon(ShareMethod type) {
  switch (type) {
    case ShareMethod::kEmail:
      return u"📧";
    case ShareMethod::kSms:
      return u"💬";
    case ShareMethod::kCopyLink:
      return u"🔗";
    case ShareMethod::kTwitter:
      return u"🐦";
    case ShareMethod::kFacebook:
      return u"📘";
    case ShareMethod::kLinkedIn:
      return u"💼";
    case ShareMethod::kWhatsApp:
      return u"💬";
    case ShareMethod::kTelegram:
      return u"✈️";
    case ShareMethod::kNotes:
      return u"📝";
    case ShareMethod::kWorkspace:
      return u"📁";
  }
  return u"🔗";
}

void AstraShareMethodItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  desc_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
}

// ===========================================================================
// AstraTabSharingView
// ===========================================================================

AstraTabSharingView::AstraTabSharingView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabSharingView::~AstraTabSharingView() = default;

void AstraTabSharingView::SetTabInfo(const TabInfo& info) {
  tab_info_ = info;
  RefreshTabInfo();
}

void AstraTabSharingView::SetShareMethods(
    const std::vector<AstraShareMethodItemView::MethodInfo>& methods) {
  share_methods_ = methods;
  RefreshShareMethods();
}

void AstraTabSharingView::SetShareMethodCallback(
    ShareMethodCallback callback) {
  share_method_callback_ = std::move(callback);
}

void AstraTabSharingView::SetCopyLinkCallback(
    CopyLinkCallback callback) {
  copy_link_callback_ = std::move(callback);
}

void AstraTabSharingView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildTabInfoSection();
  BuildCopyLinkSection();
  BuildShareMethodsSection();
}

void AstraTabSharingView::BuildTabInfoSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 4));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  title_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"No tab selected"));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  url_label_ = section->AddChildView(
      std::make_unique<views::Label>(u""));
  url_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  url_label_->SetAutoColorReadabilityEnabled(false);
  url_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  url_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
}

void AstraTabSharingView::BuildCopyLinkSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  copy_button_ = section->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraTabSharingView::OnCopyLink,
              base::Unretained(this)),
          u"🔗 Copy Link"));
  copy_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabSharingView::BuildShareMethodsSection() {
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
      std::make_unique<views::Label>(u"Share via"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  share_methods_list_ = section->AddChildView(
      std::make_unique<views::View>());
  share_methods_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
  share_methods_list_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabSharingView::RefreshTabInfo() {
  if (!title_label_ || !url_label_) return;

  if (tab_info_.title.empty() && tab_info_.url.empty()) {
    title_label_->SetText(u"No tab selected");
    url_label_->SetText(u"");
    if (copy_button_) copy_button_->SetEnabled(false);
    return;
  }

  title_label_->SetText(tab_info_.title);
  url_label_->SetText(base::UTF8ToUTF16(tab_info_.domain));
  if (copy_button_) copy_button_->SetEnabled(true);
}

void AstraTabSharingView::RefreshShareMethods() {
  if (!share_methods_list_) return;

  share_methods_list_->RemoveAllChildViews();
  method_views_.clear();

  for (const auto& method : share_methods_) {
    auto* item = share_methods_list_->AddChildView(
        std::make_unique<AstraShareMethodItemView>(
            method,
            base::BindRepeating(
                &AstraTabSharingView::OnShareMethodSelected,
                base::Unretained(this))));
    method_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabSharingView::OnCopyLink() {
  if (copy_link_callback_) {
    copy_link_callback_.Run();
  }
}

void AstraTabSharingView::OnShareMethodSelected(
    const std::string& method_id) {
  if (share_method_callback_) {
    share_method_callback_.Run(method_id);
  }
}

std::u16string AstraTabSharingView::GetWindowTitle() const {
  return u"Share";
}

void AstraTabSharingView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  if (title_label_) {
    title_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForeground));
  }
  if (url_label_) {
    url_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
}

}  // namespace astra
