// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/history_page/astra_history_page_view.h"

#include <memory>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/base/l10n/l10n_util.h"
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
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

// Helper to get a deterministic color for a host (for favicon placeholders).
SkColor GetHostColor(const std::string& host) {
  // Simple hash-based color generation.
  size_t hash = std::hash<std::string>{}(host);
  uint8_t r = static_cast<uint8_t>((hash & 0xFF) + 100);
  uint8_t g = static_cast<uint8_t>(((hash >> 8) & 0xFF) + 100);
  uint8_t b = static_cast<uint8_t>(((hash >> 16) & 0xFF) + 100);
  // Clamp to a reasonable brightness range.
  r = std::min(r, static_cast<uint8_t>(200));
  g = std::min(g, static_cast<uint8_t>(200));
  b = std::min(b, static_cast<uint8_t>(200));
  return SkColorSetRGB(r, g, b);
}

// Get first character of host name, uppercase.
char16_t GetHostInitial(const std::string& host) {
  if (host.empty()) {
    return '?';
  }
  char c = host[0];
  if (c >= 'a' && c <= 'z') {
    c = c - 'a' + 'A';
  }
  return static_cast<char16_t>(c);
}

}  // namespace

// ===========================================================================
// AstraHistoryPageView
// ===========================================================================

BEGIN_METADATA(AstraHistoryPageView)
END_METADATA

AstraHistoryPageView::AstraHistoryPageView() {
  BuildHeader();
  BuildBody();
}

AstraHistoryPageView::AstraHistoryPageView(AstraHistoryPageModel* model)
    : model_(model) {
  BuildHeader();
  BuildBody();
  if (model_) {
    model_observation_.Observe(model_);
    RefreshFromModel();
  }
}

AstraHistoryPageView::~AstraHistoryPageView() = default;

void AstraHistoryPageView::SetModel(AstraHistoryPageModel* model) {
  if (model_ == model) {
    return;
  }
  if (model_observation_.IsObserving()) {
    model_observation_.Reset();
  }
  model_ = model;
  if (model_) {
    model_observation_.Observe(model_);
  }
  RefreshFromModel();
}

void AstraHistoryPageView::RefreshFromModel() {
  if (!model_) {
    RebuildDaySections();
    RefreshCategoryChips();
    UpdateEmptyState();
    return;
  }

  UpdateFilterLabel();
  RefreshCategoryChips();
  RebuildDaySections();
  UpdateEmptyState();
}

gfx::Size AstraHistoryPageView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Default to a reasonable size, but will be managed by parent.
  return gfx::Size(800, 600);
}

void AstraHistoryPageView::Layout() {
  gfx::Rect bounds = GetContentsBounds();

  // Header at top.
  if (header_container_ && header_container_->GetVisible()) {
    int header_height = header_container_->GetHeightForWidth(bounds.width());
    header_container_->SetBounds(bounds.x(), bounds.y(),
                                 bounds.width(), header_height);
  }

  // Body fills the rest.
  int header_bottom = header_container_
                          ? header_container_->bounds().bottom()
                          : bounds.y();
  if (scroll_view_ && scroll_view_->GetVisible()) {
    scroll_view_->SetBounds(bounds.x(), header_bottom,
                            bounds.width(),
                            bounds.bottom() - header_bottom);
  }
}

void AstraHistoryPageView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraHistoryPageView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  // Background.
  cc::PaintFlags bg_flags;
  bg_flags.setColor(SK_ColorWHITE);
  bg_flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawRect(bounds, bg_flags);
}

// -- AstraHistoryPageObserver: ---------------------------------------------

void AstraHistoryPageView::OnHistoryChanged(AstraHistoryPageModel* model) {
  DCHECK_EQ(model, model_);
  RebuildDaySections();
  RefreshCategoryChips();
  UpdateEmptyState();
}

void AstraHistoryPageView::OnFilterChanged(AstraHistoryPageModel* model,
                                           AstraHistoryFilter filter) {
  DCHECK_EQ(model, model_);
  UpdateFilterLabel();
}

void AstraHistoryPageView::OnSearchChanged(AstraHistoryPageModel* model,
                                           const std::u16string& query) {
  DCHECK_EQ(model, model_);
  // Search field is already updated by user typing, no need to sync back.
}

void AstraHistoryPageView::OnHistoryEntryRemoved(AstraHistoryPageModel* model,
                                                 const std::string& id) {
  DCHECK_EQ(model, model_);
  // Full rebuild for simplicity.
  RebuildDaySections();
  UpdateEmptyState();
}

void AstraHistoryPageView::OnHistoryPageModelShutdown(
    AstraHistoryPageModel* model) {
  DCHECK_EQ(model, model_);
  model_observation_.Reset();
  model_ = nullptr;
}

// -- views::TextfieldController: ------------------------------------------

void AstraHistoryPageView::ContentsChanged(views::Textfield* sender,
                                           const std::u16string& new_contents) {
  if (model_) {
    model_->SetSearchQuery(new_contents);
  }
  if (delegate_) {
    delegate_->OnSearchQueryChanged(new_contents);
  }
}

// -- Private methods --------------------------------------------------------

void AstraHistoryPageView::BuildHeader() {
  auto header = std::make_unique<views::View>();
  header->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(16, kSidePadding)));
  header_container_ = AddChildView(std::move(header));

  auto* layout = header_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  layout->set_between_child_spacing(12);

  // Top row: title + clear data button.
  auto top_row = std::make_unique<views::View>();
  auto* top_layout = top_row->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  top_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  top_layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  top_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  auto title_label = std::make_unique<views::Label>(u"History");
  title_label->SetFontList(gfx::FontList().Derive(
      kHeaderTitleSize - gfx::FontList().GetDefaultFontSize(),
      gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD));
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  top_row->AddChildView(std::move(title_label));

  // Spacer to push button to the right.
  auto spacer = std::make_unique<views::View>();
  spacer->SetProperty(views::kFlexBehaviorKey,
                      views::FlexSpecification::ForSizeRule(
                          views::MinimumFlexSizeRule::kScaleToZero,
                          views::MaximumFlexSizeRule::kUnbounded));
  top_row->AddChildView(std::move(spacer));

  // Clear browsing data button.
  auto clear_button = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraHistoryPageView::OnClearDataClicked,
                          base::Unretained(this)),
      u"Clear browsing data");
  clear_button->SetTooltipText(u"Clear browsing data");
  clear_button->SetAccessibleName(u"Clear browsing data");
  clear_data_button_ = top_row->AddChildView(std::move(clear_button));

  header_container_->AddChildView(std::move(top_row));

  // Search row: search field + filter button.
  auto search_row = std::make_unique<views::View>();
  auto* search_layout = search_row->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  search_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  search_layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  search_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  // Search textfield with icon.
  auto search_field = std::make_unique<views::Textfield>();
  search_field->SetPlaceholderText(u"Search history");
  search_field->SetAccessibleName(u"Search history");
  search_field->SetController(this);
  search_field->SetBorder(views::CreateRoundedRectBorder(
      1, 20, SkColorSetA(SK_ColorBLACK, 0x1A)));
  search_field->SetBackgroundColor(SK_ColorWHITE);
  search_field->set_placeholder_font_list(gfx::FontList());
  search_field_ = search_row->AddChildView(std::move(search_field));

  search_field_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Filter button (time range dropdown placeholder).
  auto filter_btn = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraHistoryPageView::OnFilterButtonClicked,
                          base::Unretained(this)),
      u"All time");
  filter_btn->SetTooltipText(u"Filter by time");
  filter_btn->SetAccessibleName(u"Filter history by time");
  filter_button_ = search_row->AddChildView(std::move(filter_btn));

  header_container_->AddChildView(std::move(search_row));

  // Category chips row.
  auto chips_row = std::make_unique<views::View>();
  auto* chips_layout = chips_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  chips_layout->set_between_child_spacing(8);
  chips_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  category_chips_container_ = header_container_->AddChildView(std::move(chips_row));

  BuildCategoryChips();
}

void AstraHistoryPageView::BuildBody() {
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->SetBackgroundColor(SK_ColorWHITE);
  scroll_view->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);
  scroll_view_ = AddChildView(std::move(scroll_view));

  // Content view inside scroll.
  auto content = std::make_unique<views::View>();
  auto* content_layout = content->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  content_layout->set_between_child_spacing(kDaySectionSpacing);
  content_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  content->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(16, kSidePadding)));
  content_view_ = content.get();
  scroll_view_->SetContents(std::move(content));

  // Empty state (hidden initially).
  auto empty_state = std::make_unique<views::View>();
  empty_state->SetVisible(false);
  empty_state_view_ = content_view_->AddChildView(std::move(empty_state));
}

void AstraHistoryPageView::BuildCategoryChips() {
  if (!category_chips_container_) {
    return;
  }

  category_chips_container_->RemoveAllChildViews();

  // "All" chip (always present).
  auto all_chip = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraHistoryPageView::OnCategoryChipClicked,
                          base::Unretained(this), std::string()),
      u"All");
  all_chip->SetTooltipText(u"Show all categories");
  all_chip->SetAccessibleName(u"All categories filter");
  all_chip->SetStyle(views::Button::STYLE_BUTTON);
  category_chips_container_->AddChildView(std::move(all_chip));

  // Category chips will be populated from the model in RefreshCategoryChips.
}

void AstraHistoryPageView::RefreshCategoryChips() {
  if (!category_chips_container_ || !model_) {
    return;
  }

  // Remove all chips except the "All" chip (first child).
  while (category_chips_container_->children().size() > 1) {
    category_chips_container_->RemoveChildViewAt(
        category_chips_container_->children().size() - 1);
  }

  std::vector<std::string> categories = model_->GetCategories();
  for (const auto& category : categories) {
    auto chip = std::make_unique<views::LabelButton>(
        base::BindRepeating(&AstraHistoryPageView::OnCategoryChipClicked,
                            base::Unretained(this), category),
        base::UTF8ToUTF16(category));
    chip->SetTooltipText(base::UTF8ToUTF16("Filter by " + category));
    chip->SetAccessibleName(base::UTF8ToUTF16(category + " category filter"));
    chip->SetStyle(views::Button::STYLE_BUTTON);

    // Highlight selected category.
    if (category == model_->GetCategoryFilter()) {
      // TODO(astra): Use a proper "selected" chip style.
      chip->SetTextStyle(views::Button::STYLE_BUTTON);
    }

    category_chips_container_->AddChildView(std::move(chip));
  }
}

void AstraHistoryPageView::UpdateFilterLabel() {
  if (!filter_button_ || !model_) {
    return;
  }
  auto options = model_->GetFilterOptions();
  for (const auto& option : options) {
    if (option.first == model_->GetFilter()) {
      filter_button_->SetText(option.second);
      break;
    }
  }
}

void AstraHistoryPageView::RebuildDaySections() {
  if (!content_view_) {
    return;
  }

  day_sections_.clear();

  // Remove all day section children but keep empty state.
  // Easier: remove all children and re-add empty state.
  content_view_->RemoveAllChildViews();

  // Re-add empty state (hidden).
  auto empty_state = std::make_unique<views::View>();
  empty_state->SetVisible(false);
  empty_state_view_ = content_view_->AddChildView(std::move(empty_state));

  if (!model_) {
    return;
  }

  const auto& days = model_->GetDays();
  for (const auto& day : days) {
    auto day_section = std::make_unique<AstraHistoryDaySection>(day);
    day_sections_.push_back(
        content_view_->AddChildView(std::move(day_section)));
  }

  InvalidateLayout();
}

void AstraHistoryPageView::UpdateEmptyState() {
  if (!empty_state_view_ || !content_view_) {
    return;
  }

  bool has_results = model_ && !model_->GetDays().empty();
  empty_state_view_->SetVisible(!has_results);

  if (!has_results) {
    // Build empty state content.
    empty_state_view_->RemoveAllChildViews();

    auto* layout = empty_state_view_->SetLayoutManager(
        std::make_unique<views::BoxLayout>(
            views::BoxLayout::Orientation::kVertical));
    layout->set_main_axis_alignment(
        views::BoxLayout::MainAxisAlignment::kCenter);
    layout->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);
    layout->set_between_child_spacing(12);

    empty_state_view_->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(80, 24)));

    auto icon_label = std::make_unique<views::Label>(u"🕐");
    icon_label->SetFontList(gfx::FontList().Derive(
        32, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
    empty_state_view_->AddChildView(std::move(icon_label));

    std::u16string message;
    if (model_ && !model_->GetSearchQuery().empty()) {
      message = u"No results found for your search";
    } else if (model_ && model_->GetFilter() != AstraHistoryFilter::kAll) {
      message = u"No history for this time period";
    } else {
      message = u"No browsing history yet";
    }
    auto msg_label = std::make_unique<views::Label>(message);
    msg_label->SetFontList(gfx::FontList().Derive(
        2, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::MEDIUM));
    msg_label->SetAutoColorReadabilityEnabled(false);
    msg_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x8A));
    empty_state_view_->AddChildView(std::move(msg_label));
  }
}

// -- Event handlers ---------------------------------------------------------

void AstraHistoryPageView::OnClearDataClicked() {
  if (delegate_) {
    delegate_->OnClearBrowsingData();
  } else if (model_) {
    model_->ClearAllHistory();
  }
}

void AstraHistoryPageView::OnFilterButtonClicked() {
  // Cycle through filter options as a simple placeholder.
  // TODO(astra): Replace with a proper dropdown menu.
  if (!model_) {
    return;
  }
  auto options = model_->GetFilterOptions();
  auto current = model_->GetFilter();
  // Find current index and advance.
  size_t idx = 0;
  for (size_t i = 0; i < options.size(); ++i) {
    if (options[i].first == current) {
      idx = i;
      break;
    }
  }
  size_t next_idx = (idx + 1) % options.size();
  model_->SetFilter(options[next_idx].first);

  if (delegate_) {
    delegate_->OnFilterChanged(options[next_idx].first);
  }
}

void AstraHistoryPageView::OnCategoryChipClicked(const std::string& category) {
  if (!model_) {
    return;
  }

  // Toggle: clicking the same chip again clears the filter.
  std::string new_filter = (category == model_->GetCategoryFilter())
                               ? std::string()
                               : category;
  model_->SetCategoryFilter(new_filter);

  if (delegate_) {
    delegate_->OnCategoryFilterChanged(new_filter);
  }
}

// -- Custom icon painting ---------------------------------------------------

void AstraHistoryPageView::DrawSearchIcon(gfx::Canvas* canvas,
                                          const gfx::Rect& bounds,
                                          SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Magnifying glass circle.
  SkPath path;
  SkRect circle = SkRect::MakeXYWH(cx - size, cy - size, size * 2, size * 2);
  path.addArc(circle, -45, 360);
  canvas->DrawPath(path, flags);

  // Handle.
  canvas->DrawLine(
      gfx::Point(cx + size * 0.7f, cy + size * 0.7f),
      gfx::Point(cx + size * 1.3f, cy + size * 1.3f),
      flags);
}

void AstraHistoryPageView::DrawFilterIcon(gfx::Canvas* canvas,
                                          const gfx::Rect& bounds,
                                          SkColor color) {
  int left = bounds.x() + bounds.width() * 0.25f;
  int right = bounds.x() + bounds.width() * 0.75f;
  int top = bounds.y() + bounds.height() * 0.25f;
  int bottom = bounds.y() + bounds.height() * 0.75f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(left, top);
  path.lineTo(right, top);
  path.lineTo(bounds.CenterPoint().x(), bounds.CenterPoint().y());
  path.lineTo(bounds.CenterPoint().x(), bottom);
  path.moveTo(left, top);
  canvas->DrawPath(path, flags);
}

void AstraHistoryPageView::DrawBookmarkStar(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color,
                                            bool filled) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(filled ? cc::PaintFlags::kFill_Style
                        : cc::PaintFlags::kStroke_Style);

  // 5-point star.
  SkPath path;
  for (int i = 0; i < 5; ++i) {
    float outer_angle = -M_PI / 2 + i * 2 * M_PI / 5;
    float inner_angle = outer_angle + M_PI / 5;
    float outer_x = cx + size * cos(outer_angle);
    float outer_y = cy + size * sin(outer_angle);
    float inner_x = cx + size * 0.4f * cos(inner_angle);
    float inner_y = cy + size * 0.4f * sin(inner_angle);
    if (i == 0) {
      path.moveTo(outer_x, outer_y);
    } else {
      path.lineTo(outer_x, outer_y);
    }
    path.lineTo(inner_x, inner_y);
  }
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraHistoryPageView::DrawMoreIcon(gfx::Canvas* canvas,
                                        const gfx::Rect& bounds,
                                        SkColor color) {
  int cy = bounds.y() + bounds.height() / 2;
  int cx = bounds.x() + bounds.width() / 2;
  float dot_radius = 2.0f;
  float spacing = 5.0f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  // Three dots horizontally.
  canvas->DrawCircle(gfx::Point(cx - spacing, cy), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx + spacing, cy), dot_radius, flags);
}

void AstraHistoryPageView::DrawTrashIcon(gfx::Canvas* canvas,
                                         const gfx::Rect& bounds,
                                         SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Trash can body.
  SkPath path;
  path.moveTo(cx - size * 0.7f, cy - size * 0.3f);
  path.lineTo(cx - size * 0.7f, cy + size);
  path.lineTo(cx + size * 0.7f, cy + size);
  path.lineTo(cx + size * 0.7f, cy - size * 0.3f);

  // Lid.
  path.moveTo(cx - size, cy - size * 0.5f);
  path.lineTo(cx + size, cy - size * 0.5f);

  // Lid handle.
  path.moveTo(cx - size * 0.4f, cy - size * 0.8f);
  path.lineTo(cx + size * 0.4f, cy - size * 0.8f);
  path.lineTo(cx + size * 0.4f, cy - size * 0.5f);
  path.lineTo(cx - size * 0.4f, cy - size * 0.5f);
  path.lineTo(cx - size * 0.4f, cy - size * 0.8f);

  canvas->DrawPath(path, flags);
}

// ===========================================================================
// AstraHistoryDaySection
// ===========================================================================

BEGIN_METADATA(AstraHistoryDaySection)
END_METADATA

AstraHistoryDaySection::AstraHistoryDaySection(const AstraHistoryDay& day_data)
    : day_data_(day_data) {
  Build();
}

AstraHistoryDaySection::~AstraHistoryDaySection() = default;

AstraHistoryEntryRow* AstraHistoryDaySection::GetEntryRow(size_t index) const {
  if (index < entry_rows_.size()) {
    return entry_rows_[index];
  }
  return nullptr;
}

gfx::Size AstraHistoryDaySection::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = available_size.width().value_or(600);
  int height = kDateLabelHeight + kEntrySpacing;
  for (const auto& row : entry_rows_) {
    height += row->GetHeightForWidth(width) + kEntrySpacing;
  }
  return gfx::Size(width, height);
}

void AstraHistoryDaySection::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int y = bounds.y();

  // Date label.
  if (date_label_) {
    date_label_->SetBounds(bounds.x(), y, bounds.width(), kDateLabelHeight);
    y += kDateLabelHeight + kEntrySpacing;
  }

  // Entry rows.
  for (const auto& row : entry_rows_) {
    int row_height = row->GetHeightForWidth(bounds.width());
    row->SetBounds(bounds.x(), y, bounds.width(), row_height);
    y += row_height + kEntrySpacing;
  }
}

void AstraHistoryDaySection::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraHistoryDaySection::Build() {
  // Date label row: date + visit count.
  auto date_row = std::make_unique<views::View>();
  auto* date_layout = date_row->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  date_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  date_layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  date_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  auto date_label = std::make_unique<views::Label>(day_data_.date_label);
  date_label->SetFontList(gfx::FontList().Derive(
      2, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD));
  date_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  date_label->SetAutoColorReadabilityEnabled(false);
  date_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0xDE));
  date_label_ = date_row->AddChildView(std::move(date_label));

  // Spacer.
  auto spacer = std::make_unique<views::View>();
  spacer->SetProperty(views::kFlexBehaviorKey,
                      views::FlexSpecification::ForSizeRule(
                          views::MinimumFlexSizeRule::kScaleToZero,
                          views::MaximumFlexSizeRule::kUnbounded));
  date_row->AddChildView(std::move(spacer));

  // Visit count.
  std::u16string count_text = base::NumberToString16(day_data_.total_visits) +
                              u" visits";
  auto count_label = std::make_unique<views::Label>(count_text);
  count_label->SetFontList(gfx::FontList().Derive(
      -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
  count_label->SetAutoColorReadabilityEnabled(false);
  count_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x61));
  visit_count_label_ = date_row->AddChildView(std::move(count_label));

  AddChildView(std::move(date_row));

  // Entries container.
  auto entries_container = std::make_unique<views::View>();
  entries_container_ = AddChildView(std::move(entries_container));

  // Create entry rows.
  for (const auto& entry : day_data_.entries) {
    auto entry_row = std::make_unique<AstraHistoryEntryRow>(entry);
    entry_rows_.push_back(
        entries_container_->AddChildView(std::move(entry_row)));
  }
}

// ===========================================================================
// AstraHistoryEntryRow
// ===========================================================================

BEGIN_METADATA(AstraHistoryEntryRow)
END_METADATA

AstraHistoryEntryRow::AstraHistoryEntryRow(const AstraHistoryEntry& entry)
    : entry_(entry) {
  // Favicon view.
  auto favicon_view = std::make_unique<views::ImageView>();
  favicon_view->SetImageSize(gfx::Size(kFaviconSize, kFaviconSize));
  favicon_view->SetTooltipText(base::UTF8ToUTF16(entry.host));
  favicon_view_ = AddChildView(std::move(favicon_view));

  // Title label.
  auto title_label = std::make_unique<views::Label>(entry.title);
  title_label->SetFontList(gfx::FontList().Derive(
      0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label->SetAutoColorReadabilityEnabled(false);
  title_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0xDE));
  title_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label_ = AddChildView(std::move(title_label));

  // URL label.
  auto url_label = std::make_unique<views::Label>(base::UTF8ToUTF16(entry.url));
  url_label->SetFontList(gfx::FontList().Derive(
      -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
  url_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  url_label->SetAutoColorReadabilityEnabled(false);
  url_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x61));
  url_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  url_label_ = AddChildView(std::move(url_label));

  // Time label.
  auto time_label = std::make_unique<views::Label>(FormatVisitTime());
  time_label->SetFontList(gfx::FontList().Derive(
      -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
  time_label->SetAutoColorReadabilityEnabled(false);
  time_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x61));
  time_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  time_label_ = AddChildView(std::move(time_label));

  // Bookmark button.
  auto bookmark_btn = std::make_unique<views::ImageButton>();
  bookmark_btn->SetTooltipText(entry.is_bookmarked ? u"Remove bookmark"
                                                   : u"Add bookmark");
  bookmark_btn->SetAccessibleName(u"Toggle bookmark");
  bookmark_btn->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  bookmark_button_ = AddChildView(std::move(bookmark_btn));

  // More actions button.
  auto more_btn = std::make_unique<views::ImageButton>();
  more_btn->SetTooltipText(u"More actions");
  more_btn->SetAccessibleName(u"More actions for this history entry");
  more_btn->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  more_button_ = AddChildView(std::move(more_btn));
}

AstraHistoryEntryRow::~AstraHistoryEntryRow() = default;

void AstraHistoryEntryRow::SetEntry(const AstraHistoryEntry& entry) {
  entry_ = entry;
  if (title_label_) {
    title_label_->SetText(entry.title);
  }
  if (url_label_) {
    url_label_->SetText(base::UTF8ToUTF16(entry.url));
  }
  if (time_label_) {
    time_label_->SetText(FormatVisitTime());
  }
  if (bookmark_button_) {
    bookmark_button_->SetTooltipText(entry.is_bookmarked ? u"Remove bookmark"
                                                         : u"Add bookmark");
  }
  SchedulePaint();
}

gfx::Size AstraHistoryEntryRow::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(available_size.width().value_or(600), 56);
}

void AstraHistoryEntryRow::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int center_y = bounds.y() + bounds.height() / 2;

  // Favicon on left.
  int x = bounds.x() + kRowPadding;
  if (favicon_view_) {
    favicon_view_->SetBounds(x, center_y - kFaviconSize / 2,
                             kFaviconSize, kFaviconSize);
    x += kFaviconSize + kFaviconSpacing;
  }

  // Right side: buttons + time.
  int right_x = bounds.right() - kRowPadding;

  // More button.
  if (more_button_) {
    right_x -= kButtonSize;
    more_button_->SetBounds(right_x, center_y - kButtonSize / 2,
                            kButtonSize, kButtonSize);
    right_x -= kButtonSpacing;
  }

  // Bookmark button.
  if (bookmark_button_) {
    right_x -= kButtonSize;
    bookmark_button_->SetBounds(right_x, center_y - kButtonSize / 2,
                                kButtonSize, kButtonSize);
    right_x -= kButtonSpacing;
  }

  // Time label.
  if (time_label_) {
    int time_width = 60;
    right_x -= time_width;
    time_label_->SetBounds(right_x, center_y - 10, time_width, 20);
    right_x -= 12;
  }

  // Text area (title + url) fills the middle.
  int text_width = right_x - x;
  if (text_width > 0) {
    if (title_label_) {
      title_label_->SetBounds(x, center_y - 16, text_width, 18);
    }
    if (url_label_) {
      url_label_->SetBounds(x, center_y + 2, text_width, 16);
    }
  }
}

void AstraHistoryEntryRow::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraHistoryEntryRow::OnPaint(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetContentsBounds();

  // Hover background.
  if (hovered_) {
    cc::PaintFlags hover_flags;
    hover_flags.setColor(SkColorSetA(SK_ColorBLACK, 0x08));
    hover_flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawRoundRect(bounds, 8, hover_flags);
  }

  views::View::OnPaint(canvas);

  // Draw favicon placeholder.
  if (favicon_view_ && entry_.favicon.isNull()) {
    gfx::Rect favicon_bounds = favicon_view_->GetContentsBounds();
    DrawFaviconPlaceholder(canvas);
  }

  // Draw bookmark star on the button.
  if (bookmark_button_) {
    gfx::Rect btn_bounds = bookmark_button_->GetContentsBounds();
    SkColor star_color = entry_.is_bookmarked
                             ? SkColorSetRGB(0x1A, 0x73, 0xE8)
                             : SkColorSetA(SK_ColorBLACK, 0x61);
    AstraHistoryPageView::DrawBookmarkStar(
        canvas, btn_bounds, star_color, entry_.is_bookmarked);
  }

  // Draw more icon.
  if (more_button_) {
    gfx::Rect btn_bounds = more_button_->GetContentsBounds();
    AstraHistoryPageView::DrawMoreIcon(
        canvas, btn_bounds, SkColorSetA(SK_ColorBLACK, 0x61));
  }
}

bool AstraHistoryEntryRow::OnMousePressed(const ui::MouseEvent& event) {
  return true;  // Consume press to enable release.
}

void AstraHistoryEntryRow::OnMouseReleased(const ui::MouseEvent& event) {
  // TODO(astra): Check if click was on a button vs row body.
}

void AstraHistoryEntryRow::OnMouseEntered(const ui::MouseEvent& event) {
  hovered_ = true;
  SchedulePaint();
}

void AstraHistoryEntryRow::OnMouseExited(const ui::MouseEvent& event) {
  hovered_ = false;
  SchedulePaint();
}

void AstraHistoryEntryRow::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kListItem;
  node_data->SetName(entry_.title);
}

std::u16string AstraHistoryEntryRow::FormatVisitTime() const {
  // Format as short time, e.g. "2:30 PM".
  // TODO(astra): Use base::TimeFormatTimeOfDay or proper i18n formatting.
  base::Time::Exploded exploded;
  entry_.visit_time.LocalExplode(&exploded);

  int hour = exploded.hour;
  std::u16string period = u"AM";
  if (hour >= 12) {
    period = u"PM";
    if (hour > 12) {
      hour -= 12;
    }
  }
  if (hour == 0) {
    hour = 12;
  }

  std::u16string result = base::NumberToString16(hour) + u":" +
                          base::ASCIIToUTF16(base::StringPrintf(
                              "%02d", exploded.minute)) +
                          u" " + period;
  return result;
}

void AstraHistoryEntryRow::DrawFaviconPlaceholder(gfx::Canvas* canvas) {
  gfx::Rect bounds = favicon_view_->GetContentsBounds();
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int radius = std::min(bounds.width(), bounds.height()) / 2;

  SkColor bg_color = GetHostColor(entry_.host);

  // Background circle.
  cc::PaintFlags bg_flags;
  bg_flags.setColor(bg_color);
  bg_flags.setAntiAlias(true);
  bg_flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawCircle(gfx::Point(cx, cy), radius, bg_flags);

  // Letter.
  char16_t initial = GetHostInitial(entry_.host);
  std::u16string initial_str(1, initial);

  cc::PaintFlags text_flags;
  text_flags.setColor(SK_ColorWHITE);
  text_flags.setAntiAlias(true);
  text_flags.setTextSize(radius * 1.2f);
  text_flags.setTextAlign(cc::PaintFlags::kCenter);
  text_flags.setStyle(cc::PaintFlags::kFill_Style);

  canvas->DrawStringRect(initial_str, gfx::FontList(), SK_ColorWHITE,
                         gfx::Rect(cx - radius, cy - radius,
                                   radius * 2, radius * 2),
                         gfx::ALIGN_CENTER);
}

}  // namespace astra
