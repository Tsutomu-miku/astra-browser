// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/info_bar/astra_info_bar_view.h"

#include <algorithm>
#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

// ===========================================================================
// AstraInfoBarView
// ===========================================================================

AstraInfoBarView::AstraInfoBarView(AstraInfoBarType type) : type_(type) {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Icon view.
  auto icon_view = std::make_unique<views::ImageView>();
  icon_view->SetImageSize(gfx::Size(kIconSize, kIconSize));
  icon_view->SetVisible(icon_visible_);
  icon_view_ = AddChildView(std::move(icon_view));

  // Message label.
  auto message_label = std::make_unique<views::Label>());
  message_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  message_label->SetAutoColorId(ui::kColorLabelForeground);
  message_label->SetElideBehavior(gfx::ELIDE_TAIL);
  message_label->SetMultiLine(false);
  message_label_ = AddChildView(std::move(message_label));

  // Link view.
  auto link_view = std::make_unique<views::Link>(u"Learn more");
  link_view->SetCallback(base::BindRepeating(
      &AstraInfoBarView::OnLinkClicked, base::Unretained(this)));
  link_view->SetVisible(link_visible_);
  link_view_ = AddChildView(std::move(link_view));

  // Actions container.
  auto actions_container = std::make_unique<views::View>();
  actions_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kActionSpacing));
  actions_container_ = AddChildView(std::move(actions_container));

  // Close button.
  auto close_button = std::make_unique<views::ImageButton>();
  close_button->SetTooltipText(u"Dismiss");
  close_button->SetAccessibleName(u"Dismiss info bar");
  close_button->SetCallback(base::BindRepeating(
      &AstraInfoBarView::OnCloseClicked, base::Unretained(this)));
  close_button->SetVisible(close_visible_);
  close_button_ = AddChildView(std::move(close_button));

  UpdateBarAppearance();
}

AstraInfoBarView::~AstraInfoBarView() = default;

void AstraInfoBarView::SetType(AstraInfoBarType type) {
  if (type_ == type) {
    return;
  }
  type_ = type;
  UpdateBarAppearance();
  SchedulePaint();
}

void AstraInfoBarView::SetIcon(const gfx::ImageSkia& icon) {
  icon_view_->SetImage(ui::ImageModel::FromImageSkia(icon));
}

void AstraInfoBarView::SetIconVisible(bool visible) {
  if (icon_visible_ == visible) {
    return;
  }
  icon_visible_ = visible;
  icon_view_->SetVisible(visible);
  InvalidateLayout();
}

bool AstraInfoBarView::IsIconVisible() const {
  return icon_visible_;
}

void AstraInfoBarView::SetMessage(const std::u16string& message) {
  message_ = message;
  message_label_->SetText(message);
  InvalidateLayout();
}

const std::u16string& AstraInfoBarView::GetMessage() const {
  return message_;
}

void AstraInfoBarView::SetLinkText(const std::u16string& link_text) {
  link_text_ = link_text;
  link_view_->SetText(link_text);
}

void AstraInfoBarView::SetLinkVisible(bool visible) {
  if (link_visible_ == visible) {
    return;
  }
  link_visible_ = visible;
  link_view_->SetVisible(visible);
  InvalidateLayout();
}

bool AstraInfoBarView::IsLinkVisible() const {
  return link_visible_;
}

void AstraInfoBarView::SetActions(
    const std::vector<AstraInfoBarAction>& actions) {
  ClearActions();
  for (const auto& action : actions) {
    AddAction(action);
  }
}

void AstraInfoBarView::AddAction(const AstraInfoBarAction& action) {
  if (action.is_link) {
    // Link-style action.
    auto link = std::make_unique<views::Link>(action.label);
    link->SetCallback(base::BindRepeating(
        &AstraInfoBarView::OnActionClicked, base::Unretained(this),
        action.action_id));
    actions_container_->AddChildView(std::move(link));
  } else {
    auto button = std::make_unique<views::LabelButton>(
        base::BindRepeating(&AstraInfoBarView::OnActionClicked,
                            base::Unretained(this), action.action_id),
        action.label);
    if (action.is_primary) {
      button->SetStyle(ui::ButtonStyle::kProminent);
    } else {
      button->SetStyle(ui::ButtonStyle::kTonal);
    }
    button->SetMinSize(gfx::Size(0, 32));
    auto* raw_button = actions_container_->AddChildView(std::move(button));
    action_buttons_.push_back(raw_button);
  }
  InvalidateLayout();
}

void AstraInfoBarView::ClearActions() {
  action_buttons_.clear();
  actions_container_->RemoveAllChildViews();
  InvalidateLayout();
}

size_t AstraInfoBarView::GetActionCount() const {
  return actions_container_->children().size();
}

void AstraInfoBarView::SetCloseButtonVisible(bool visible) {
  if (close_visible_ == visible) {
    return;
  }
  close_visible_ = visible;
  close_button_->SetVisible(visible);
  InvalidateLayout();
}

bool AstraInfoBarView::IsCloseButtonVisible() const {
  return close_visible_;
}

void AstraInfoBarView::SetExpanded(bool expanded) {
  if (expanded_ == expanded) {
    return;
  }
  expanded_ = expanded;
  SetVisible(expanded);
  // TODO(astra): Add slide animation for expand/collapse.
}

gfx::Size AstraInfoBarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  if (!expanded_) {
    return gfx::Size(0, 0);
  }
  int height = kBarHeight;
  return gfx::Size size;
  size.set_height(height);
  return size;
}

void AstraInfoBarView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int x = bounds.x() + kPaddingHorizontal;
  int y = bounds.y() + kPaddingVertical;
  int available_width = bounds.width() - kPaddingHorizontal * 2;
  int content_height = bounds.height() - kPaddingVertical * 2;

  // Accent bar on the left (drawn in OnPaint, not a child view).

  int current_x = x;

  // Icon.
  if (icon_visible_ && icon_view_->GetVisible()) {
    icon_view_->SetBounds(current_x,
                        y + (content_height - kIconSize) / 2,
                        kIconSize, kIconSize);
    current_x += kIconSize + kIconSpacing;
  }

  // Close button on the right.
  int close_x = bounds.right() - kPaddingHorizontal - kCloseButtonSize;
  int close_y = y + (content_height - kCloseButtonSize) / 2;
  if (close_visible_ && close_button_->GetVisible()) {
    close_button_->SetBounds(close_x, close_y, kCloseButtonSize,
                           kCloseButtonSize);
    close_x -= kActionSpacing;
  }

  // Actions on the right (before close button).
  int actions_width = actions_container_->GetPreferredSize().width();
  int actions_x = close_x - actions_width;
  int actions_y = y + (content_height - 32) / 2;  // Assuming 32px button height
  if (GetActionCount() > 0) {
    actions_container_->SetBounds(actions_x, actions_y, actions_width, 32);
    actions_x -= kActionSpacing;
  }

  // Message + link on the left (between icon and actions).
  int text_x = current_x;
  int text_width = actions_x - text_x - kActionSpacing;

  // Message label.
  int message_y = y + (content_height - 16) / 2;  // 16px text height
  message_label_->SetBounds(text_x, message_y, text_width, 16);

  // Link view (after message, same line for now).
  // TODO(astra): Handle multi-line info bars with link on next line.
  if (link_visible_ && link_view_->GetVisible()) {
    int link_width = link_view_->GetPreferredSize().width();
    link_view_->SetBounds(text_x + message_label_->width() + kActionSpacing,
                         message_y, link_width, 16);
  }
}

void AstraInfoBarView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateBarAppearance();
}

void AstraInfoBarView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  // Draw accent bar on the left side.
  SkColor accent_color = GetAccentColor();
  cc::PaintFlags flags;
  flags.setColor(accent_color);
  flags.setAntiAlias(false);
  canvas->DrawRect(gfx::Rect(bounds.x(), bounds.y(), kAccentBarWidth,
                             bounds.height()), flags);

  // Draw background.
  flags.setColor(GetBackgroundColor());
  canvas->DrawRect(gfx::Rect(bounds.x() + kAccentBarWidth, bounds.y(),
                             bounds.width() - kAccentBarWidth,
                             bounds.height()), flags);

  // Draw bottom border.
  flags.setColor(SkColorSetA(SK_ColorBLACK, 0x0D));
  canvas->DrawRect(gfx::Rect(bounds.x(), bounds.bottom() - 1,
                             bounds.width(), 1), flags);
}

void AstraInfoBarView::OnActionClicked(int action_id) {
  if (delegate_) {
    delegate_->OnInfoBarAction(action_id);
  }
}

void AstraInfoBarView::OnCloseClicked() {
  if (delegate_) {
    delegate_->OnInfoBarDismissed();
  }
  SetExpanded(false);
}

void AstraInfoBarView::OnLinkClicked() {
  if (delegate_) {
    delegate_->OnInfoBarLinkClicked();
  }
}

SkColor AstraInfoBarView::GetAccentColor() const {
  switch (type_) {
    case AstraInfoBarType::kInformation:
      return SkColorSetRGB(0x1A, 0x73, 0xE8);  // Blue
    case AstraInfoBarType::kWarning:
      return SkColorSetRGB(0xF9, 0xAB, 0x00);  // Yellow/Orange
    case AstraInfoBarType::kError:
      return SkColorSetRGB(0xEA, 0x43, 0x35);  // Red
    case AstraInfoBarType::kSuccess:
      return SkColorSetRGB(0x34, 0xA8, 0x53);  // Green
    case AstraInfoBarType::kPermission:
      return SkColorSetRGB(0x1A, 0x73, 0xE8);  // Blue
    case AstraInfoBarType::kExtension:
      return SkColorSetRGB(0xA1, 0x42, 0xF4);  // Purple
    case AstraInfoBarType::kPassword:
      return SkColorSetRGB(0x1A, 0x73, 0xE8);  // Blue
    case AstraInfoBarType::kAutofill:
      return SkColorSetRGB(0x1A, 0x73, 0xE8);  // Blue
  }
  return SkColorSetRGB(0x1A, 0x73, 0xE8);
}

SkColor AstraInfoBarView::GetBackgroundColor() const {
  switch (type_) {
    case AstraInfoBarType::kInformation:
      return SkColorSetRGB(0xE8, 0xF0, 0xFE);
    case AstraInfoBarType::kWarning:
      return SkColorSetRGB(0xFE, 0xF7, 0xE0);
    case AstraInfoBarType::kError:
      return SkColorSetRGB(0xFC, 0xE8, 0xE6);
    case AstraInfoBarType::kSuccess:
      return SkColorSetRGB(0xE6, 0xF4, 0xEA);
    case AstraInfoBarType::kPermission:
      return SkColorSetRGB(0xE8, 0xF0, 0xFE);
    case AstraInfoBarType::kExtension:
      return SkColorSetRGB(0xF3, 0xE8, 0xFD);
    case AstraInfoBarType::kPassword:
      return SkColorSetRGB(0xE8, 0xF0, 0xFE);
    case AstraInfoBarType::kAutofill:
      return SkColorSetRGB(0xE8, 0xF0, 0xFE);
  }
  return SK_ColorWHITE;
}

void AstraInfoBarView::UpdateBarAppearance() {
  // Update background and border colors.
  SchedulePaint();
}

void AstraInfoBarView::UpdateVisibility() {
  icon_view_->SetVisible(icon_visible_);
  link_view_->SetVisible(link_visible_);
  close_button_->SetVisible(close_visible_);
  InvalidateLayout();
}

// ===========================================================================
// AstraInfoBarContainerView
// ===========================================================================

AstraInfoBarContainerView::AstraInfoBarContainerView() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), kBarSpacing,
      views::BoxLayout::MainAxisAlignment::kStart,
      views::BoxLayout::CrossAxisAlignment::kStretch));
}

AstraInfoBarContainerView::~AstraInfoBarContainerView() = default;

AstraInfoBarView* AstraInfoBarContainerView::AddInfoBar(
    AstraInfoBarType type,
    const std::u16string& message,
    const std::vector<AstraInfoBarAction>& actions) {
  auto info_bar = std::make_unique<AstraInfoBarView>(type);
  info_bar->SetMessage(message);
  if (!actions.empty()) {
    info_bar->SetActions(actions);
  }
  AstraInfoBarView* raw_bar = info_bar.get();
  AddInfoBar(std::move(info_bar));
  return raw_bar;
}

void AstraInfoBarContainerView::AddInfoBar(
    std::unique_ptr<AstraInfoBarView> info_bar) {
  AstraInfoBarView* raw_bar = info_bar.get();
  // Add at the top (index 0) so new bars appear above older ones.
  AddChildViewAt(std::move(info_bar), 0);
  info_bars_.insert(info_bars_.begin(), raw_bar);
  InvalidateLayout();
}

void AstraInfoBarContainerView::RemoveInfoBar(AstraInfoBarView* info_bar) {
  auto it = std::find(info_bars_.begin(), info_bars_.end(), info_bar);
  if (it != info_bars_.end()) {
    info_bars_.erase(it);
  }
  RemoveChildViewT(info_bar);
  InvalidateLayout();
}

void AstraInfoBarContainerView::RemoveAllInfoBars() {
  info_bars_.clear();
  RemoveAllChildViews();
  InvalidateLayout();
}

size_t AstraInfoBarContainerView::GetInfoBarCount() const {
  return info_bars_.size();
}

AstraInfoBarView* AstraInfoBarContainerView::GetInfoBarAt(size_t index) const {
  if (index >= info_bars_.size()) {
    return nullptr;
  }
  return info_bars_[index];
}

gfx::Size AstraInfoBarContainerView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraInfoBarContainerView::Layout() {
  views::View::Layout();
}

}  // namespace astra
