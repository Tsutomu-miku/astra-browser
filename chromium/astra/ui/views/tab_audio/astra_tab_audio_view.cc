// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_audio/astra_tab_audio_view.h"

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
constexpr int kItemHeight = 56;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 6;
constexpr int kMaxVisibleItems = 6;

}  // namespace

// ===========================================================================
// AstraAudioTabItemView
// ===========================================================================

AstraAudioTabItemView::AstraAudioTabItemView(
    const TabInfo& info,
    ToggleMuteCallback mute_callback,
    CloseTabCallback close_callback,
    JumpToTabCallback jump_callback)
    : tab_id_(info.tab_id),
      title_(info.title),
      domain_(info.domain),
      audio_state_(info.audio_state),
      is_active_(info.is_active),
      audio_started_(info.audio_started),
      is_media_(info.is_media),
      is_background_(info.is_background),
      mute_callback_(std::move(mute_callback)),
      close_callback_(std::move(close_callback)),
      jump_callback_(std::move(jump_callback)) {
  BuildLayout();
}

AstraAudioTabItemView::~AstraAudioTabItemView() = default;

void AstraAudioTabItemView::SetMuted(bool muted) {
  audio_state_ = muted ? AudioState::kMuted : AudioState::kPlaying;
  if (icon_label_) {
    icon_label_->SetText(AudioStateIcon(audio_state_));
  }
  if (mute_button_) {
    mute_button_->SetText(muted ? u"Unmute" : u"Mute");
  }
}

void AstraAudioTabItemView::BuildLayout() {
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(8, 12),
      10));
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Icon.
  icon_label_ = AddChildView(
      std::make_unique<views::Label>(AudioStateIcon(audio_state_)));
  icon_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label_->SetAutoColorReadabilityEnabled(false);
  icon_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  icon_label_->SetPreferredSize(gfx::Size(24, 20));

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

  title_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          is_active_ ? views::style::STYLE_PRIMARY
                     : views::style::STYLE_SECONDARY));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  domain_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(domain_)));
  domain_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  domain_label_->SetAutoColorReadabilityEnabled(false);
  domain_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  domain_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Action buttons column.
  auto* buttons_col = AddChildView(std::make_unique<views::View>());
  buttons_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 6));

  mute_button_ = buttons_col->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraAudioTabItemView::OnMuteToggled,
              base::Unretained(this)),
          audio_state_ == AudioState::kMuted ? u"Unmute" : u"Mute"));

  close_button_ = buttons_col->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraAudioTabItemView::OnCloseClicked,
              base::Unretained(this)),
          u"Close"));
}

void AstraAudioTabItemView::OnMuteToggled() {
  bool new_muted = audio_state_ != AudioState::kMuted;
  SetMuted(new_muted);
  if (mute_callback_) {
    mute_callback_.Run(tab_id_, new_muted);
  }
}

void AstraAudioTabItemView::OnCloseClicked() {
  if (close_callback_) {
    close_callback_.Run(tab_id_);
  }
}

void AstraAudioTabItemView::OnTabClicked() {
  if (jump_callback_) {
    jump_callback_.Run(tab_id_);
  }
}

std::u16string AstraAudioTabItemView::AudioStateIcon(AudioState state) {
  switch (state) {
    case AudioState::kPlaying:
      return u"🔊";
    case AudioState::kMuted:
      return u"🔇";
    case AudioState::kAudible:
      return u"🔉";
  }
  return u"🔊";
}

std::u16string AstraAudioTabItemView::FormatAudioDuration(
    base::TimeDelta delta) {
  int minutes = delta.InMinutes();
  if (minutes < 1) return u"< 1 min";
  if (minutes < 60) {
    return base::UTF8ToUTF16(std::to_string(minutes) + " min");
  }
  int hours = minutes / 60;
  int mins = minutes % 60;
  if (mins == 0) {
    return base::UTF8ToUTF16(std::to_string(hours) + "h");
  }
  return base::UTF8ToUTF16(
      std::to_string(hours) + "h " + std::to_string(mins) + "m");
}

void AstraAudioTabItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  title_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  domain_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraTabAudioView
// ===========================================================================

AstraTabAudioView::AstraTabAudioView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabAudioView::~AstraTabAudioView() = default;

void AstraTabAudioView::SetAudioTabs(
    const std::vector<AstraAudioTabItemView::TabInfo>& tabs) {
  audio_tabs_ = tabs;
  playing_count_ = 0;
  muted_count_ = 0;
  for (const auto& tab : audio_tabs_) {
    if (tab.audio_state == AstraAudioTabItemView::AudioState::kMuted) {
      muted_count_++;
    } else {
      playing_count_++;
    }
  }
  RefreshTabs();
  RefreshSummary();
}

void AstraTabAudioView::SetToggleMuteCallback(
    ToggleMuteCallback callback) {
  mute_callback_ = std::move(callback);
}

void AstraTabAudioView::SetCloseTabCallback(
    CloseTabCallback callback) {
  close_callback_ = std::move(callback);
}

void AstraTabAudioView::SetJumpToTabCallback(
    JumpToTabCallback callback) {
  jump_callback_ = std::move(callback);
}

void AstraTabAudioView::SetMuteAllCallback(
    MuteAllCallback callback) {
  mute_all_callback_ = std::move(callback);
}

void AstraTabAudioView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildSummarySection();
  BuildTabsList();
}

void AstraTabAudioView::BuildSummarySection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  summary_label_ = section->AddChildView(
      std::make_unique<views::Label>(
          u"No audio tabs"));
  summary_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  summary_label_->SetAutoColorReadabilityEnabled(false);
  summary_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Action buttons row.
  auto* buttons_row = section->AddChildView(
      std::make_unique<views::View>());
  buttons_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));

  mute_all_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraTabAudioView::OnMuteAll,
              base::Unretained(this)),
          u"🔇 Mute All"));
  mute_all_button_->SetEnabled(false);
  mute_all_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  unmute_all_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabAudioView::OnUnmuteAll,
              base::Unretained(this)),
          u"🔊 Unmute All"));
  unmute_all_button_->SetEnabled(false);
  unmute_all_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabAudioView::BuildTabsList() {
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

  auto* header_label = section->AddChildView(
      std::make_unique<views::Label>(u"Audio Tabs"));
  header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label->SetAutoColorReadabilityEnabled(false);
  header_label->SetFontList(
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

  tabs_list_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  tabs_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraTabAudioView::RefreshTabs() {
  if (!tabs_list_) return;

  tabs_list_->RemoveAllChildViews();
  tab_views_.clear();

  // Sort: active first, then by audio state (playing before muted),
  // then by title.
  auto sorted = audio_tabs_;
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
              if (a.is_active != b.is_active) return a.is_active > b.is_active;
              bool a_muted =
                  a.audio_state == AstraAudioTabItemView::AudioState::kMuted;
              bool b_muted =
                  b.audio_state == AstraAudioTabItemView::AudioState::kMuted;
              if (a_muted != b_muted) return a_muted < b_muted;
              return a.title < b.title;
            });

  for (const auto& tab : sorted) {
    auto* item = tabs_list_->AddChildView(
        std::make_unique<AstraAudioTabItemView>(
            tab,
            base::BindRepeating(
                &AstraTabAudioView::OnToggleMute,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabAudioView::OnCloseTab,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabAudioView::OnJumpToTab,
                base::Unretained(this))));
    tab_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabAudioView::RefreshSummary() {
  if (!summary_label_) return;

  int total = playing_count_ + muted_count_;
  if (total == 0) {
    summary_label_->SetText(u"No audio tabs");
  } else {
    summary_label_->SetText(
        base::UTF8ToUTF16(
            std::to_string(total) + " tabs with audio · " +
            std::to_string(playing_count_) + " playing · " +
            std::to_string(muted_count_) + " muted"));
  }

  if (mute_all_button_) {
    mute_all_button_->SetEnabled(playing_count_ > 0);
  }
  if (unmute_all_button_) {
    unmute_all_button_->SetEnabled(muted_count_ > 0);
  }
}

void AstraTabAudioView::OnToggleMute(
    const std::string& tab_id, bool muted) {
  if (mute_callback_) {
    mute_callback_.Run(tab_id, muted);
  }
}

void AstraTabAudioView::OnCloseTab(const std::string& tab_id) {
  if (close_callback_) {
    close_callback_.Run(tab_id);
  }
}

void AstraTabAudioView::OnJumpToTab(const std::string& tab_id) {
  if (jump_callback_) {
    jump_callback_.Run(tab_id);
  }
}

void AstraTabAudioView::OnMuteAll() {
  if (mute_all_callback_) {
    mute_all_callback_.Run(true);
  }
}

void AstraTabAudioView::OnUnmuteAll() {
  if (mute_all_callback_) {
    mute_all_callback_.Run(false);
  }
}

std::u16string AstraTabAudioView::GetWindowTitle() const {
  return u"Audio Tabs";
}

void AstraTabAudioView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  if (summary_label_) {
    summary_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
}

}  // namespace astra
