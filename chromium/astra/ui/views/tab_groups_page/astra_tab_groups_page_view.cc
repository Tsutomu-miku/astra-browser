// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_groups_page/astra_tab_groups_page_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/i18n/case_conversion.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/models/simple_combobox_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_view.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/layout/table_layout.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

// Draw a chevron/expand arrow icon.
void DrawChevronIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color,
                     bool expanded) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.3f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  if (expanded) {
    // Pointing up.
    path.moveTo(cx - s, cy + s * 0.3f);
    path.lineTo(cx, cy - s * 0.7f);
    path.lineTo(cx + s, cy + s * 0.3f);
  } else {
    // Pointing down.
    path.moveTo(cx - s, cy - s * 0.3f);
    path.lineTo(cx, cy + s * 0.7f);
    path.lineTo(cx + s, cy - s * 0.3f);
  }
  canvas->DrawPath(path, flags);
}

// Draw a pin icon.
void DrawPinIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color,
                 bool filled) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(
      filled ? cc::PaintFlags::kFill_Style : cc::PaintFlags::kStroke_Style);

  SkPath path;
  // Pushpin shape.
  path.moveTo(cx, cy - s);
  path.lineTo(cx + s * 0.6f, cy - s * 0.3f);
  path.lineTo(cx + s * 0.4f, cy + s * 0.5f);
  path.lineTo(cx, cy + s);
  path.lineTo(cx - s * 0.4f, cy + s * 0.5f);
  path.lineTo(cx - s * 0.6f, cy - s * 0.3f);
  path.close();
  canvas->DrawPath(path, flags);
}

// Draw more (three dots) icon.
void DrawMoreIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float dot_r = 1.5f;
  float spacing = 4;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  canvas->DrawCircle(gfx::Point(cx - spacing, cy), dot_r, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), dot_r, flags);
  canvas->DrawCircle(gfx::Point(cx + spacing, cy), dot_r, flags);
}

// Draw an add/plus icon.
void DrawAddIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.3f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(cx - s, cy);
  path.lineTo(cx + s, cy);
  path.moveTo(cx, cy - s);
  path.lineTo(cx, cy + s);
  canvas->DrawPath(path, flags);
}

// Draw a search icon.
void DrawSearchIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2 - 2;
  int cy = bounds.y() + bounds.height() / 2 - 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.3f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  canvas->DrawCircle(gfx::Point(cx, cy), s, flags);

  SkPath handle;
  handle.moveTo(cx + s * 0.7f, cy + s * 0.7f);
  handle.lineTo(cx + s * 1.4f, cy + s * 1.4f);
  canvas->DrawPath(handle, flags);
}

// Draw a tab icon (for tab items).
void DrawTabIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int x = bounds.x() + (bounds.width() - 14) / 2;
  int y = bounds.y() + (bounds.height() - 12) / 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Tab shape with rounded top corners.
  SkPath path;
  path.moveTo(x + 2, y + 12);
  path.lineTo(x, y + 12);
  path.lineTo(x, y + 2);
  path.arcTo(SkRect::MakeXYWH(x, y, 4, 4), 180, -90, false);
  path.lineTo(x + 10, y);
  path.arcTo(SkRect::MakeXYWH(x + 10, y, 4, 4), 90, -90, false);
  path.lineTo(x + 14, y + 12);
  canvas->DrawPath(path, flags);
}

// Draw close (X) icon.
void DrawCloseIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.3f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(cx - s, cy - s);
  path.lineTo(cx + s, cy + s);
  path.moveTo(cx + s, cy - s);
  path.lineTo(cx - s, cy + s);
  canvas->DrawPath(path, flags);
}

}  // namespace

// ===========================================================================
// AstraTabGroupCardView
// ===========================================================================

AstraTabGroupCardView::AstraTabGroupCardView(
    const std::string& group_id)
    : group_id_(group_id) {
  BuildLayout();
  SetFocusBehavior(FocusBehavior::ALWAYS);
}

AstraTabGroupCardView::~AstraTabGroupCardView() = default;

void AstraTabGroupCardView::UpdateFromModel(
    const AstraTabGroupsPageModel* model) {
  const auto* group = model->GetGroup(group_id_);
  if (!group) {
    return;
  }

  title_label_->SetText(group->title);
  tab_count_label_->SetText(base::NumberToString16(group->total_tabs) + u" tabs");
  pin_button_->SetToggled(group->is_pinned);

  // Color stripe.
  SkColor group_color = AstraTabGroupsPageModel::GetGroupColor(group->color);
  color_stripe_->SetBackground(views::CreateSolidBackground(group_color));

  // Collapsed state.
  bool expanded = group->state != AstraTabGroupState::kCollapsed;
  SetExpanded(expanded);

  // Rebuild tab items.
  tabs_container_->RemoveAllChildViews();
  tab_items_.clear();

  if (expanded) {
    for (const auto& tab : group->tabs) {
      auto* tab_item = tabs_container_->AddChildView(
          std::make_unique<views::View>());
      tab_item->SetLayoutManager(std::make_unique<views::FlexLayout>())
          ->SetOrientation(views::LayoutOrientation::kHorizontal)
          .SetCrossAxisAlignment(views::LayoutAlignment::kCenter)
          .SetInteriorMargin(gfx::Insets::VH(4, 8));
      tab_item->SetPreferredSize(gfx::Size(0, 28));

      // Favicon / tab icon.
      auto icon = std::make_unique<views::ImageView>();
      icon->SetImageSize(gfx::Size(16, 16));
      if (!tab.favicon.isNull()) {
        icon->SetImage(tab.favicon);
      }
      tab_item->AddChildView(std::move(icon));

      // Tab title.
      auto title = std::make_unique<views::Label>();
      title->SetText(tab.title);
      title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      title->SetElideBehavior(gfx::ELIDE_TAIL);
      title->SetFontList(views::style::GetFont(
          views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
      title->SetAutoColorReadabilityEnabled(false);
      tab_item->AddChildView(std::move(title));
      static_cast<views::FlexLayout*>(tab_item->GetLayoutManager())
          ->SetFlexForView(title, 1);

      // Close button.
      auto close_btn = std::make_unique<views::ImageButton>();
      close_btn->SetMinSize(gfx::Size(16, 16));
      close_btn->SetTooltipText(u"Close tab");
      tab_item->AddChildView(std::move(close_btn));

      // Hover effect.
      tab_item->SetBorder(views::CreateEmptyBorder(gfx::Insets(0)));

      tab_items_.push_back(tab_item);
    }
  }

  UpdateColors();
  SchedulePaint();
}

void AstraTabGroupCardView::SetExpanded(bool expanded) {
  if (expanded_ == expanded) {
    return;
  }
  expanded_ = expanded;
  if (tabs_container_) {
    tabs_container_->SetVisible(expanded_);
  }
  SchedulePaint();
}

void AstraTabGroupCardView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

void AstraTabGroupCardView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  cc::PaintFlags flags;
  flags.setColor(is_hovered_ ? SkColorSetA(SK_ColorBLACK, 0x08)
                             : SkColorSetA(SK_ColorBLACK, 0x04));
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  // Card background with rounded corners.
  SkPath path;
  float radius = 8.0f;
  path.addRoundRect(
      SkRect::MakeXYWH(bounds.x(), bounds.y(), bounds.width(), bounds.height()),
      radius, radius);
  canvas->DrawPath(path, flags);

  // Border.
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setColor(SkColorSetA(SK_ColorBLACK, 0x0D));
  flags.setStrokeWidth(1);
  canvas->DrawPath(path, flags);
}

bool AstraTabGroupCardView::OnMousePressed(const ui::MouseEvent& event) {
  RequestFocus();
  return true;
}

void AstraTabGroupCardView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_ = true;
  SchedulePaint();
}

void AstraTabGroupCardView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_ = false;
  SchedulePaint();
}

void AstraTabGroupCardView::BuildLayout() {
  RemoveAllChildViews();
  tab_items_.clear();
  header_ = nullptr;
  color_stripe_ = nullptr;
  title_label_ = nullptr;
  tab_count_label_ = nullptr;
  collapse_button_ = nullptr;
  pin_button_ = nullptr;
  more_button_ = nullptr;
  tabs_container_ = nullptr;

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Header row.
  auto header = std::make_unique<views::View>();
  auto* header_layout =
      header->SetLayoutManager(std::make_unique<views::FlexLayout>());
  header_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  header_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  header_layout->SetInteriorMargin(gfx::Insets::VH(8, 12));

  // Color stripe (left accent bar).
  auto color_stripe = std::make_unique<views::View>();
  color_stripe->SetPreferredSize(gfx::Size(4, 20));
  color_stripe->SetBackground(views::CreateSolidBackground(SK_ColorGRAY));
  color_stripe_ = header->AddChildView(std::move(color_stripe));
  color_stripe_->SetProperty(views::kMarginsKey, gfx::Insets(0, 0, 0, 10));

  // Title.
  auto title = std::make_unique<views::Label>();
  title->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_EMPHASIZED));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetAutoColorReadabilityEnabled(false);
  title_label_ = header->AddChildView(std::move(title));
  header_layout->SetFlexForView(title_label_, 1);
  title_label_->SetProperty(views::kMarginsKey, gfx::Insets(0, 0, 0, 8));

  // Tab count.
  auto tab_count = std::make_unique<views::Label>();
  tab_count->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  tab_count->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tab_count->SetAutoColorReadabilityEnabled(false);
  tab_count_label_ = header->AddChildView(std::move(tab_count));
  tab_count_label_->SetProperty(views::kMarginsKey,
                                gfx::Insets(0, 0, 0, 8));

  // Pin button.
  auto pin = std::make_unique<views::ImageButton>();
  pin->SetMinSize(gfx::Size(20, 20));
  pin->SetCallback(base::BindRepeating(
      &AstraTabGroupCardView::OnPinClicked, base::Unretained(this)));
  pin->SetTooltipText(u"Pin group");
  pin_button_ = header->AddChildView(std::move(pin));

  // More button.
  auto more = std::make_unique<views::ImageButton>();
  more->SetMinSize(gfx::Size(20, 20));
  more->SetCallback(base::BindRepeating(
      &AstraTabGroupCardView::OnMoreClicked, base::Unretained(this)));
  more->SetTooltipText(u"More options");
  more_button_ = header->AddChildView(std::move(more));
  more_button_->SetProperty(views::kMarginsKey, gfx::Insets(0, 8, 0, 0));

  // Collapse button.
  auto collapse = std::make_unique<views::ImageButton>();
  collapse->SetMinSize(gfx::Size(20, 20));
  collapse->SetCallback(base::BindRepeating(
      &AstraTabGroupCardView::OnCollapseClicked, base::Unretained(this)));
  collapse->SetTooltipText(u"Collapse group");
  collapse_button_ = header->AddChildView(std::move(collapse));
  collapse_button_->SetProperty(views::kMarginsKey,
                                gfx::Insets(0, 4, 0, 0));

  header_ = AddChildView(std::move(header));

  // Tabs container.
  auto tabs_container = std::make_unique<views::View>();
  tabs_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(4, 4), 2));
  tabs_container_ = AddChildView(std::move(tabs_container));
  tabs_container_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, 0, 8, 0)));

  // Set minimum size.
  SetMinSize(gfx::Size(kCardMinWidth, 120));

  UpdateColors();
}

void AstraTabGroupCardView::UpdateColors() {
  if (!GetWidget()) {
    return;
  }

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor text_color = color_provider->GetColor(ui::kColorLabelForeground);
  SkColor secondary_color =
      color_provider->GetColor(ui::kColorLabelForegroundSecondary);
  SkColor icon_color = color_provider->GetColor(ui::kColorIcon);

  if (title_label_) {
    title_label_->SetEnabledColor(text_color);
  }
  if (tab_count_label_) {
    tab_count_label_->SetEnabledColor(secondary_color);
  }

  SchedulePaint();
}

int AstraTabGroupCardView::GetTabItemCount() const {
  return static_cast<int>(tab_items_.size());
}

void AstraTabGroupCardView::OnCollapseClicked() {
  // TODO(astra): Notify model/delegate.
  SetExpanded(!expanded_);
}

void AstraTabGroupCardView::OnPinClicked() {
  // TODO(astra): Notify model/delegate.
}

void AstraTabGroupCardView::OnMoreClicked() {
  // TODO(astra): Show context menu.
}

void AstraTabGroupCardView::OnCardClicked() {
  // TODO(astra): Activate group.
}

// ===========================================================================
// AstraTabGroupsPageView
// ===========================================================================

AstraTabGroupsPageView::AstraTabGroupsPageView() {
  BuildUI();
}

AstraTabGroupsPageView::~AstraTabGroupsPageView() {
  if (scoped_observation_.IsObserving()) {
    scoped_observation_.RemoveObserver();
  }
}

void AstraTabGroupsPageView::SetModel(AstraTabGroupsPageModel* model) {
  if (model_ == model) {
    return;
  }

  if (scoped_observation_.IsObserving()) {
    scoped_observation_.RemoveObserver();
  }

  model_ = model;

  if (model_) {
    scoped_observation_.Observe(model_);
    RefreshFromModel();
  }
}

void AstraTabGroupsPageView::RefreshFromModel() {
  if (!model_) {
    return;
  }

  RefreshCategories();
  RefreshGroupCards();
  UpdateEmptyState();

  // Update stats label.
  if (stats_label_) {
    size_t groups = model_->GetGroupCount();
    size_t tabs = model_->GetTotalTabCount();
    stats_label_->SetText(
        base::NumberToString16(groups) + u" groups · " +
        base::NumberToString16(tabs) + u" tabs");
  }
}

void AstraTabGroupsPageView::OnThemeChanged() {
  views::View::OnThemeChanged();

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor bg_color = color_provider->GetColor(ui::kColorDialogBackground);
  SetBackground(views::CreateSolidBackground(bg_color));

  SchedulePaint();
}

void AstraTabGroupsPageView::Layout() {
  views::View::Layout();

  gfx::Rect bounds = GetContentsBounds();

  // Toolbar at top.
  if (toolbar_) {
    toolbar_->SetBounds(bounds.x(), bounds.y(), bounds.width(), kToolbarHeight);
  }

  int content_y = bounds.y() + kToolbarHeight;
  int content_height = bounds.height() - kToolbarHeight;

  // Sidebar on left.
  if (categories_sidebar_) {
    categories_sidebar_->SetBounds(bounds.x(), content_y,
                                   kSidebarWidth, content_height);
  }

  // Content area to the right of sidebar.
  int content_x = bounds.x() + kSidebarWidth;
  int content_width = bounds.width() - kSidebarWidth;

  if (content_scroll_view_) {
    content_scroll_view_->SetBounds(content_x, content_y,
                                    content_width, content_height);
  }
}

gfx::Size AstraTabGroupsPageView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(900, 600);
}

void AstraTabGroupsPageView::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  if (sender == search_field_ && model_) {
    model_->SetSearchQuery(new_contents);
  }
}

bool AstraTabGroupsPageView::HandleKeyEvent(
    views::Textfield* sender,
    const ui::KeyEvent& key_event) {
  if (key_event.key_code() == ui::VKEY_ESCAPE) {
    search_field_->SetText(std::u16string());
    return true;
  }
  return false;
}

void AstraTabGroupsPageView::OnTabGroupsChanged(
    AstraTabGroupsPageModel* model) {
  DCHECK_EQ(model, model_);
  RefreshGroupCards();
  RefreshCategories();
  UpdateEmptyState();
  RefreshFromModel();
}

void AstraTabGroupsPageView::OnTabGroupAdded(
    AstraTabGroupsPageModel* model,
    const std::string& group_id) {
  DCHECK_EQ(model, model_);
  RefreshGroupCards();
}

void AstraTabGroupsPageView::OnTabGroupRemoved(
    AstraTabGroupsPageModel* model,
    const std::string& group_id) {
  DCHECK_EQ(model, model_);
  RefreshGroupCards();
}

void AstraTabGroupsPageView::OnTabGroupUpdated(
    AstraTabGroupsPageModel* model,
    const std::string& group_id) {
  DCHECK_EQ(model, model_);
  // Find the card and update it.
  for (auto* card : group_cards_) {
    if (card->group_id() == group_id) {
      card->UpdateFromModel(model_);
      break;
    }
  }
}

void AstraTabGroupsPageView::OnFilterChanged(
    AstraTabGroupsPageModel* model) {
  DCHECK_EQ(model, model_);
  RefreshGroupCards();
  UpdateEmptyState();
}

void AstraTabGroupsPageView::OnSearchChanged(
    AstraTabGroupsPageModel* model,
    const std::u16string& query) {
  DCHECK_EQ(model, model_);
  if (search_field_->GetText() != query) {
    search_field_->SetText(query);
  }
  RefreshGroupCards();
  UpdateEmptyState();
}

void AstraTabGroupsPageView::OnTabGroupsPageModelShutdown(
    AstraTabGroupsPageModel* model) {
  DCHECK_EQ(model, model_);
  scoped_observation_.RemoveObserver();
  model_ = nullptr;
}

int AstraTabGroupsPageView::GetGroupCardCount() const {
  return static_cast<int>(group_cards_.size());
}

AstraTabGroupCardView* AstraTabGroupsPageView::GetGroupCardAt(
    int index) const {
  if (index < 0 || index >= static_cast<int>(group_cards_.size())) {
    return nullptr;
  }
  return group_cards_[index];
}

void AstraTabGroupsPageView::BuildUI() {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  BuildToolbar();
  BuildCategoriesSidebar();
  BuildContentArea();
}

void AstraTabGroupsPageView::BuildToolbar() {
  auto toolbar = std::make_unique<views::View>();
  toolbar->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SkColorSetA(SK_ColorBLACK, 0x0D)));

  auto* layout = toolbar->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetInteriorMargin(gfx::Insets::VH(0, 16));

  // Search field.
  auto search = std::make_unique<views::Textfield>();
  search->SetPlaceholderText(u"Search tab groups");
  search->SetAccessibleName(u"Search tab groups");
  search->set_controller(this);
  search->SetBorder(views::CreateRoundedRectBorder(
      1, 20, SkColorSetA(SK_ColorBLACK, 0x1A)));
  search->SetVerticalAlignment(views::Textfield::Alignment::ALIGN_CENTER);
  search_field_ = toolbar->AddChildView(std::move(search));
  search_field_->SetProperty(views::kMarginsKey, gfx::Insets::VH(0, 0, 0, 12));
  search_field_->SetPreferredSize(gfx::Size(240, 32));

  // Sort combobox.
  auto sort_combobox = std::make_unique<views::Combobox>(
      std::make_unique<ui::SimpleComboboxModel>(
          std::vector<ui::SimpleComboboxModel::Item>{
              ui::SimpleComboboxModel::Item(u"Sort: Recent"),
              ui::SimpleComboboxModel::Item(u"Sort: Name"),
              ui::SimpleComboboxModel::Item(u"Sort: Tab count"),
              ui::SimpleComboboxModel::Item(u"Sort: Created"),
              ui::SimpleComboboxModel::Item(u"Sort: Memory"),
              ui::SimpleComboboxModel::Item(u"Sort: Color"),
          }));
  sort_combobox->SetCallback(base::BindRepeating(
      &AstraTabGroupsPageView::OnSortChanged, base::Unretained(this)));
  sort_combobox_ = toolbar->AddChildView(std::move(sort_combobox));
  sort_combobox_->SetProperty(views::kMarginsKey,
                              gfx::Insets::VH(0, 0, 0, 8));

  // Filter combobox.
  auto filter_combobox = std::make_unique<views::Combobox>(
      std::make_unique<ui::SimpleComboboxModel>(
          std::vector<ui::SimpleComboboxModel::Item>{
              ui::SimpleComboboxModel::Item(u"All groups"),
              ui::SimpleComboboxModel::Item(u"Expanded only"),
              ui::SimpleComboboxModel::Item(u"Collapsed only"),
              ui::SimpleComboboxModel::Item(u"Frozen only"),
              ui::SimpleComboboxModel::Item(u"Pinned"),
              ui::SimpleComboboxModel::Item(u"With unread tabs"),
          }));
  filter_combobox->SetCallback(base::BindRepeating(
      &AstraTabGroupsPageView::OnFilterChanged, base::Unretained(this)));
  filter_combobox_ = toolbar->AddChildView(std::move(filter_combobox));

  // Spacer.
  auto spacer = std::make_unique<views::View>();
  toolbar->AddChildView(std::move(spacer));
  layout->SetFlexForView(spacer.get(), 1);

  // New group button.
  auto new_group_btn = views::MdTextButton::CreatePrimaryUiButton(
      base::BindRepeating(
          &AstraTabGroupsPageView::OnNewGroup, base::Unretained(this)),
      u"New group");
  new_group_button_ = toolbar->AddChildView(std::move(new_group_btn));

  toolbar_ = AddChildView(std::move(toolbar));
}

void AstraTabGroupsPageView::BuildCategoriesSidebar() {
  auto sidebar = std::make_unique<views::View>();
  sidebar->SetBackground(
      views::CreateSolidBackground(SkColorSetRGB(0xF8, 0xF9, 0xFA)));
  sidebar->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 0, 1, SkColorSetA(SK_ColorBLACK, 0x0D)));

  auto* layout = sidebar->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);

  // Header.
  auto header = std::make_unique<views::Label>();
  header->SetText(u"Categories");
  header->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_EMPHASIZED));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  categories_header_ = sidebar->AddChildView(std::move(header));
  categories_header_->SetProperty(views::kMarginsKey,
                                   gfx::Insets::VH(16, 16));

  // Categories container.
  auto container = std::make_unique<views::View>();
  container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  categories_container_ = sidebar->AddChildView(std::move(container));
  layout->SetFlexForView(categories_container_, 1);

  categories_sidebar_ = AddChildView(std::move(sidebar));
}

void AstraTabGroupsPageView::BuildContentArea() {
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->SetBackgroundColor(SK_ColorTRANSPARENT);
  scroll_view->SetDrawOverflowIndicator(false);

  auto content = std::make_unique<views::View>();
  content->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(16), kCardSpacing));

  groups_container_ = content.get();
  scroll_view->SetContents(std::move(content));

  content_scroll_view_ = AddChildView(std::move(scroll_view));

  BuildEmptyState();
}

void AstraTabGroupsPageView::BuildEmptyState() {
  auto empty = std::make_unique<views::View>();
  auto* layout = empty->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  auto title = std::make_unique<views::Label>();
  title->SetText(u"No tab groups");
  title->SetFontList(views::style::GetFont(
      views::style::CONTEXT_DIALOG_TITLE, views::style::STYLE_PRIMARY));
  title->SetAutoColorReadabilityEnabled(false);
  empty_state_title_ = empty->AddChildView(std::move(title));
  empty_state_title_->SetProperty(views::kMarginsKey,
                                  gfx::Insets::VH(0, 0, 8, 0));

  auto desc = std::make_unique<views::Label>();
  desc->SetText(u"Create your first tab group to organize your tabs");
  desc->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  desc->SetAutoColorReadabilityEnabled(false);
  empty_state_desc_ = empty->AddChildView(std::move(desc));

  empty_state_ = groups_container_->AddChildView(std::move(empty));
  empty_state_->SetVisible(false);
}

void AstraTabGroupsPageView::RefreshGroupCards() {
  if (!model_ || !groups_container_) {
    return;
  }

  // Clear old cards.
  group_cards_.clear();
  groups_container_->RemoveAllChildViews();
  empty_state_ = nullptr;

  BuildEmptyState();

  auto filtered = model_->GetFilteredGroups();

  if (filtered.empty()) {
    if (!model_->GetSearchQuery().empty()) {
      empty_state_title_->SetText(u"No groups match your search");
      empty_state_desc_->SetText(u"Try a different search term");
    } else if (!model_->GetCategoryFilter().empty()) {
      empty_state_title_->SetText(u"No groups in this category");
      empty_state_desc_->SetText(u"Try a different category");
    } else {
      empty_state_title_->SetText(u"No tab groups");
      empty_state_desc_->SetText(
          u"Create your first tab group to organize your tabs");
    }
    empty_state_->SetVisible(true);
    return;
  }

  empty_state_->SetVisible(false);

  for (const auto& group : filtered) {
    auto card = std::make_unique<AstraTabGroupCardView>(group.id);
    card->UpdateFromModel(model_);
    group_cards_.push_back(
        groups_container_->AddChildView(std::move(card)));
  }

  groups_container_->InvalidateLayout();
}

void AstraTabGroupsPageView::RefreshCategories() {
  if (!model_ || !categories_container_) {
    return;
  }

  categories_container_->RemoveAllChildViews();

  // "All groups" item.
  auto all_item = std::make_unique<views::Label>();
  all_item->SetText(u"All groups");
  all_item->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  all_item->SetAutoColorReadabilityEnabled(false);
  all_item->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY));
  all_item->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(8, 16)));
  auto* all_label =
      categories_container_->AddChildView(std::move(all_item));
  all_label->SetBackground(
      views::CreateSolidBackground(SkColorSetRGB(0xE8, 0xF0, 0xFE)));

  auto categories = model_->GetCategories();
  for (const auto& cat : categories) {
    auto item = std::make_unique<views::Label>();
    item->SetText(cat.name + u" (" +
                  base::NumberToString16(cat.count) + u")");
    item->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    item->SetAutoColorReadabilityEnabled(false);
    item->SetFontList(views::style::GetFont(
        views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
    item->SetBorder(
        views::CreateEmptyBorder(gfx::Insets::VH(8, 24)));
    categories_container_->AddChildView(std::move(item));
  }

  categories_container_->InvalidateLayout();
}

void AstraTabGroupsPageView::UpdateEmptyState() {
  if (!model_ || !groups_container_) {
    return;
  }
  auto filtered = model_->GetFilteredGroups();
  if (empty_state_) {
    empty_state_->SetVisible(filtered.empty());
  }
}

void AstraTabGroupsPageView::OnSortChanged() {
  if (!model_ || !sort_combobox_) {
    return;
  }
  int index = sort_combobox_->GetSelectedIndex().value_or(0);
  AstraTabGroupSortType sort_type = AstraTabGroupSortType::kLastAccessed;
  switch (index) {
    case 0: sort_type = AstraTabGroupSortType::kLastAccessed; break;
    case 1: sort_type = AstraTabGroupSortType::kName; break;
    case 2: sort_type = AstraTabGroupSortType::kTabCount; break;
    case 3: sort_type = AstraTabGroupSortType::kCreatedDate; break;
    case 4: sort_type = AstraTabGroupSortType::kMemoryUsage; break;
    case 5: sort_type = AstraTabGroupSortType::kColor; break;
  }
  model_->SetSortType(sort_type);
}

void AstraTabGroupsPageView::OnFilterChanged() {
  if (!model_ || !filter_combobox_) {
    return;
  }
  int index = filter_combobox_->GetSelectedIndex().value_or(0);
  AstraTabGroupFilter filter = AstraTabGroupFilter::kAll;
  switch (index) {
    case 0: filter = AstraTabGroupFilter::kAll; break;
    case 1: filter = AstraTabGroupFilter::kExpandedOnly; break;
    case 2: filter = AstraTabGroupFilter::kCollapsedOnly; break;
    case 3: filter = AstraTabGroupFilter::kFrozenOnly; break;
    case 4: filter = AstraTabGroupFilter::kPinned; break;
    case 5: filter = AstraTabGroupFilter::kUnreadTabs; break;
  }
  model_->SetFilter(filter);
}

void AstraTabGroupsPageView::OnCategorySelected(
    const std::string& category_id) {
  if (!model_) {
    return;
  }
  selected_category_ = category_id;
  model_->SetCategoryFilter(category_id);
}

void AstraTabGroupsPageView::OnNewGroup() {
  if (delegate_) {
    delegate_->OnCreateNewGroup();
  }
  if (model_) {
    model_->CreateGroup(u"New Group", AstraTabGroupColor::kBlue);
  }
}

void AstraTabGroupsPageView::DrawGroupIndicator(gfx::Canvas* canvas,
                                                 const gfx::Rect& bounds,
                                                 SkColor color) {
  // Drawn in each card's color stripe instead.
}

}  // namespace astra
