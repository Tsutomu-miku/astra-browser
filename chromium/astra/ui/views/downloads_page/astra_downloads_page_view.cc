// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/downloads_page/astra_downloads_page_view.h"

#include <algorithm>
#include <map>
#include <memory>
#include <utility>

#include "base/i18n/case_conversion.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
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
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/progress_bar.h"
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
#include "ui/views/widget/widget.h"

#include "astra/ui/views/downloads_page/astra_downloads_page_model.h"

namespace astra {

namespace {

// Custom view that draws an icon programmatically.
class AstraIconView : public views::View {
 public:
  using PaintCallback =
      base::RepeatingCallback<void(gfx::Canvas*, const gfx::Rect&, SkColor)>;

  AstraIconView(PaintCallback callback, const gfx::Size& size)
      : paint_callback_(std::move(callback)), size_(size) {
    SetPreferredSize(size);
  }

  void SetColor(SkColor color) {
    color_ = color;
    SchedulePaint();
  }

  void OnPaint(gfx::Canvas* canvas) override {
    views::View::OnPaint(canvas);
    if (paint_callback_)
      paint_callback_.Run(canvas, GetContentsBounds(), color_);
  }

 private:
  PaintCallback paint_callback_;
  gfx::Size size_;
  SkColor color_ = SK_ColorBLACK;
};

// A button that draws its icon programmatically.
class AstraIconButton : public views::ImageButton {
 public:
  using PaintCallback =
      base::RepeatingCallback<void(gfx::Canvas*, const gfx::Rect&, SkColor)>;

  AstraIconButton(PressedCallback callback,
                  PaintCallback paint_callback,
                  const gfx::Size& size)
      : views::ImageButton(std::move(callback)),
        paint_callback_(std::move(paint_callback)),
        icon_size_(size) {
    SetMinSize(size);
    SetImageHorizontalAlignment(ALIGN_CENTER);
    SetImageVerticalAlignment(ALIGN_MIDDLE);
  }

  void SetIconColor(SkColor color) {
    icon_color_ = color;
    SchedulePaint();
  }

  void PaintButtonContents(gfx::Canvas* canvas) override {
    views::ImageButton::PaintButtonContents(canvas);
    if (paint_callback_) {
      gfx::Rect bounds = GetContentsBounds();
      // Center the icon within the button.
      int x = bounds.x() + (bounds.width() - icon_size_.width()) / 2;
      int y = bounds.y() + (bounds.height() - icon_size_.height()) / 2;
      gfx::Rect icon_bounds(x, y, icon_size_.width(), icon_size_.height());
      SkColor color = GetEnabled() ? icon_color_
                                   : SkColorSetA(icon_color_, 0x4D);
      paint_callback_.Run(canvas, icon_bounds, color);
    }
  }

 private:
  PaintCallback paint_callback_;
  gfx::Size icon_size_;
  SkColor icon_color_ = SK_ColorBLACK;
};

// A category chip button.
class AstraCategoryChip : public views::LabelButton {
 public:
  AstraCategoryChip(PressedCallback callback, const std::u16string& text)
      : views::LabelButton(std::move(callback), text) {
    SetHorizontalAlignment(gfx::ALIGN_CENTER);
    SetBorder(views::CreateRoundedRectBorder(
        1, 14, SkColorSetA(SK_ColorBLACK, 0x1A)));
    SetEnabledTextColors(SK_ColorBLACK);
    size_t label_context = views::style::CONTEXT_BUTTON;
    SetTextSubpixelRenderingEnabled(false);
    SetMinSize(gfx::Size(0, 28));
  }

  void SetActive(bool active) {
    active_ = active;
    if (active) {
      SetBorder(views::CreateRoundedRectBorder(
          1, 14, SkColorSetRGB(0x1A, 0x73, 0xE8)));
      SetBackground(views::CreateSolidBackground(
          SkColorSetRGB(0xE8, 0xF0, 0xFE)));
      SetEnabledTextColors(SkColorSetRGB(0x1A, 0x73, 0xE8));
    } else {
      SetBorder(views::CreateRoundedRectBorder(
          1, 14, SkColorSetA(SK_ColorBLACK, 0x1A)));
      SetBackground(nullptr);
      SetEnabledTextColors(SK_ColorBLACK);
    }
    SchedulePaint();
  }

  bool active() const { return active_; }

 private:
  bool active_ = false;
};

}  // namespace

// =========================================================================
// AstraDownloadsPageView
// =========================================================================

BEGIN_METADATA(AstraDownloadsPageView)
END_METADATA

AstraDownloadsPageView::AstraDownloadsPageView() {
  Build();
}

AstraDownloadsPageView::AstraDownloadsPageView(AstraDownloadsPageModel* model)
    : model_(model) {
  Build();
  if (model_)
    model_observation_.Observe(model_);
}

AstraDownloadsPageView::~AstraDownloadsPageView() = default;

void AstraDownloadsPageView::SetModel(AstraDownloadsPageModel* model) {
  if (model_ == model)
    return;

  model_observation_.Reset();
  model_ = model;
  if (model_)
    model_observation_.Observe(model_);

  RebuildDownloadList();
  UpdateEmptyState();
}

// -- views::View ------------------------------------------------------------

void AstraDownloadsPageView::OnThemeChanged() {
  views::View::OnThemeChanged();
  // TODO(astra): Update all colors from the theme's ColorProvider.
  SchedulePaint();
}

void AstraDownloadsPageView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int y = bounds.y();
  int width = bounds.width();

  // Toolbar at top.
  if (toolbar_ && toolbar_->GetVisible()) {
    toolbar_->SetBounds(bounds.x(), y, width, kToolbarHeight);
    y += kToolbarHeight;
  }

  // Content area fills remaining space.
  int content_height = bounds.bottom() - y;
  if (scroll_view_) {
    scroll_view_->SetBounds(bounds.x(), y, width, content_height);
  }
}

gfx::Size AstraDownloadsPageView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(400, 600);
}

// -- TextfieldController ----------------------------------------------------

void AstraDownloadsPageView::ContentsChanged(views::Textfield* sender,
                                             const std::u16string& new_contents) {
  if (model_ && sender == search_field_) {
    model_->SetSearchQuery(new_contents);
  }
}

// -- AstraDownloadsPageObserver ---------------------------------------------

void AstraDownloadsPageView::OnDownloadsChanged() {
  RebuildDownloadList();
  UpdateEmptyState();
}

void AstraDownloadsPageView::OnSearchChanged(const std::u16string& query) {
  if (search_field_ && search_field_->GetText() != query) {
    search_field_->SetText(query);
  }
}

void AstraDownloadsPageView::OnFilterChanged() {
  // Update filter button states.
  if (model_ && filter_buttons_container_) {
    AstraDownloadsPageFilter current_filter = model_->GetFilter();
    for (views::View* child : filter_buttons_container_->children()) {
      auto* button = static_cast<views::LabelButton*>(child);
      // The button's tag stores the filter value.
      AstraDownloadsPageFilter button_filter =
          static_cast<AstraDownloadsPageFilter>(button->tag());
      bool is_active = (button_filter == current_filter);
      if (is_active) {
        button->SetBackground(views::CreateSolidBackground(
            SkColorSetRGB(0xE8, 0xF0, 0xFE)));
        button->SetEnabledTextColors(SkColorSetRGB(0x1A, 0x73, 0xE8));
      } else {
        button->SetBackground(nullptr);
        button->SetEnabledTextColors(SK_ColorBLACK);
      }
    }
  }

  // Update category chips.
  if (model_ && category_chips_container_) {
    const std::string& active_cat = model_->GetCategoryFilter();
    for (views::View* child : category_chips_container_->children()) {
      auto* chip = static_cast<AstraCategoryChip*>(child);
      std::string category = base::UTF16ToUTF8(chip->GetTooltipText());
      chip->SetActive(category == active_cat);
    }
  }
}

void AstraDownloadsPageView::OnDownloadsPageModelShutdown() {
  model_observation_.Reset();
  model_ = nullptr;
}

// -- Build ------------------------------------------------------------------

void AstraDownloadsPageView::Build() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  SetBackground(views::CreateSolidBackground(SK_ColorWHITE));

  BuildToolbar();
  BuildContentArea();
}

void AstraDownloadsPageView::BuildToolbar() {
  auto toolbar = std::make_unique<views::View>();
  toolbar->SetBackground(views::CreateSolidBackground(SK_ColorWHITE));
  toolbar->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SkColorSetA(SK_ColorBLACK, 0x0D)));

  auto* layout = toolbar->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(8, kToolbarSidePadding), 8));

  // Row 1: search field + filter buttons + clear all
  auto top_row = std::make_unique<views::View>();
  auto* top_layout =
      top_row->SetLayoutManager(std::make_unique<views::FlexLayout>());
  top_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  top_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  top_layout->SetDefault(views::kMarginsKey, gfx::Insets::TLBR(0, 0, 0, 8));

  // Search field.
  auto search_container = std::make_unique<views::View>();
  search_container->SetBorder(views::CreateRoundedRectBorder(
      1, kSearchFieldHeight / 2, SkColorSetA(SK_ColorBLACK, 0x1A)));
  auto* search_layout = search_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 12), 8));
  search_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  search_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto search_icon = std::make_unique<AstraIconView>(
      base::BindRepeating(&AstraDownloadsPageView::DrawSearchIcon),
      gfx::Size(16, 16));
  search_icon->SetColor(SkColorSetA(SK_ColorBLACK, 0x66));
  search_container->AddChildView(std::move(search_icon));

  auto search_field = std::make_unique<views::Textfield>();
  search_field->SetPlaceholderText(u"Search downloads");
  search_field->SetAccessibleName(u"Search downloads");
  search_field->set_controller(this);
  search_field->SetBorder(views::NullBorder());
  search_field->SetBackgroundColor(SK_ColorTRANSPARENT);
  search_field->SetPreferredSize(
      gfx::Size(kSearchFieldWidth - 40, kSearchFieldHeight - 8));
  search_field_ = search_field.get();
  search_container->AddChildView(std::move(search_field));

  search_container->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded));
  top_row->AddChildView(std::move(search_container));

  // Filter buttons container.
  auto filter_container = std::make_unique<views::View>();
  auto* filter_layout = filter_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 0));
  filter_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  filter_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto filter_options = std::vector<std::pair<AstraDownloadsPageFilter, std::u16string>>{
      {AstraDownloadsPageFilter::kAll, u"All"},
      {AstraDownloadsPageFilter::kInProgress, u"In progress"},
      {AstraDownloadsPageFilter::kCompleted, u"Completed"},
      {AstraDownloadsPageFilter::kCancelled, u"Cancelled"},
      {AstraDownloadsPageFilter::kInterrupted, u"Interrupted"},
  };

  for (size_t i = 0; i < filter_options.size(); ++i) {
    auto filter_btn = std::make_unique<views::LabelButton>(
        base::BindRepeating(&AstraDownloadsPageView::OnFilterClicked,
                            base::Unretained(this), filter_options[i].first),
        filter_options[i].second);
    filter_btn->SetMinSize(gfx::Size(0, kFilterButtonHeight));
    filter_btn->set_tag(static_cast<int>(filter_options[i].first));
    filter_btn->SetBorder(views::NullBorder());
    filter_btn->SetEnabledTextColors(SK_ColorBLACK);

    // Make first button active by default.
    if (i == 0) {
      filter_btn->SetBackground(views::CreateSolidBackground(
          SkColorSetRGB(0xE8, 0xF0, 0xFE)));
      filter_btn->SetEnabledTextColors(SkColorSetRGB(0x1A, 0x73, 0xE8));
      active_filter_button_ = filter_btn.get();
    }

    filter_container->AddChildView(std::move(filter_btn));
  }

  filter_buttons_container_ = filter_container.get();
  top_row->AddChildView(std::move(filter_container));

  // Spacer.
  auto spacer = std::make_unique<views::View>();
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded,
                               /*adjust_height_for_width=*/false,
                               views::FlexSpecification::Weight(1)));
  top_row->AddChildView(std::move(spacer));

  // Clear all button.
  auto clear_all = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraDownloadsPageView::OnClearAllClicked,
                          base::Unretained(this)),
      u"Clear all");
  clear_all->SetMinSize(gfx::Size(0, kFilterButtonHeight));
  clear_all_button_ = clear_all.get();
  top_row->AddChildView(std::move(clear_all));

  toolbar->AddChildView(std::move(top_row));

  // Row 2: category chips.
  auto chips_row = std::make_unique<views::View>();
  auto* chips_layout =
      chips_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 8));
  chips_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  chips_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  category_chips_container_ = chips_row.get();

  toolbar->AddChildView(std::move(chips_row));

  toolbar_ = toolbar.get();
  AddChildView(std::move(toolbar));
}

void AstraDownloadsPageView::BuildContentArea() {
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->SetBackgroundColor(SK_ColorWHITE);
  scroll_view->SetDrawOverflowIndicator(false);

  auto content = std::make_unique<views::View>();
  content->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  content_container_ = content.get();

  // Empty state (initially hidden).
  auto empty_state = std::make_unique<views::View>();
  empty_state->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 16));
  auto* empty_layout = static_cast<views::BoxLayout*>(
      empty_state->GetLayoutManager());
  empty_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  empty_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto empty_icon = std::make_unique<AstraIconView>(
      base::BindRepeating(&AstraDownloadsPageView::DrawDownloadIcon),
      gfx::Size(kEmptyStateIconSize, kEmptyStateIconSize));
  empty_icon->SetColor(SkColorSetA(SK_ColorBLACK, 0x33));
  empty_state->AddChildView(std::move(empty_icon));

  auto empty_title = std::make_unique<views::Label>(u"No downloads");
  empty_title->SetFontList(
      views::style::GetFont(views::style::CONTEXT_DIALOG_TITLE,
                            views::style::STYLE_PRIMARY));
  empty_title->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x80));
  empty_state->AddChildView(std::move(empty_title));

  auto empty_subtitle = std::make_unique<views::Label>(
      u"Downloads will appear here as they start.");
  empty_subtitle->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x50));
  empty_state->AddChildView(std::move(empty_subtitle));

  empty_state->SetVisible(false);
  empty_state_ = empty_state.get();
  content->AddChildView(std::move(empty_state));

  scroll_view->SetContents(std::move(content));
  scroll_view_ = scroll_view.get();
  AddChildView(std::move(scroll_view));

  // Rebuild category chips from model if available.
  if (model_) {
    auto categories = model_->GetCategories();
    RebuildCategoryChips(categories);
    RebuildDownloadList();
    UpdateEmptyState();
  }
}

void AstraDownloadsPageView::RebuildDownloadList() {
  if (!content_container_ || !model_)
    return;

  // Remove all existing date groups (keep empty state view).
  std::vector<views::View*> to_remove;
  for (views::View* child : content_container_->children()) {
    if (child != empty_state_)
      to_remove.push_back(child);
  }
  for (views::View* child : to_remove) {
    content_container_->RemoveChildViewT(child);
  }

  // Group downloads by date.
  const auto& downloads = model_->GetDownloads();
  std::map<DateGroup, std::vector<const AstraDownloadEntry*>> groups;

  for (const auto& entry : downloads) {
    DateGroup group = GetDateGroup(entry.start_time);
    groups[group].push_back(&entry);
  }

  // Create date groups in order: Today, Yesterday, Last 7 days, Older
  std::vector<DateGroup> group_order = {
      DateGroup::kToday,
      DateGroup::kYesterday,
      DateGroup::kLast7Days,
      DateGroup::kOlder,
  };

  for (DateGroup group : group_order) {
    auto it = groups.find(group);
    if (it == groups.end() || it->second.empty())
      continue;

    views::View* group_view = CreateDateGroup(GetDateGroupLabel(group));
    auto* group_layout = static_cast<views::BoxLayout*>(
        group_view->GetLayoutManager());

    for (const auto* entry : it->second) {
      auto item = CreateDownloadItem(entry->id);
      group_view->AddChildView(std::move(item));
    }

    content_container_->AddChildView(std::move(group_view));
  }

  // Update category chips (categories may have changed).
  auto categories = model_->GetCategories();
  RebuildCategoryChips(categories);
}

void AstraDownloadsPageView::RebuildCategoryChips(
    const std::vector<std::string>& categories) {
  if (!category_chips_container_)
    return;

  category_chips_container_->RemoveAllChildViews();

  // "All" chip (empty category = all).
  auto all_chip = std::make_unique<AstraCategoryChip>(
      base::BindRepeating(&AstraDownloadsPageView::OnCategoryChipClicked,
                          base::Unretained(this), std::string()),
      u"All");
  all_chip->SetTooltipText(u"Show all categories");
  all_chip->SetActive(model_ && model_->GetCategoryFilter().empty());
  category_chips_container_->AddChildView(std::move(all_chip));

  for (const auto& category : categories) {
    auto chip = std::make_unique<AstraCategoryChip>(
        base::BindRepeating(&AstraDownloadsPageView::OnCategoryChipClicked,
                            base::Unretained(this), category),
        base::UTF8ToUTF16(category));
    chip->SetTooltipText(base::UTF8ToUTF16(category));
    chip->SetActive(model_ && model_->GetCategoryFilter() == category);
    category_chips_container_->AddChildView(std::move(chip));
  }
}

views::View* AstraDownloadsPageView::CreateDateGroup(
    const std::u16string& title) {
  auto group = std::make_unique<views::View>();
  group->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Header.
  auto header = std::make_unique<views::View>();
  auto* header_layout =
      header->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kDateGroupSidePadding)));
  header_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  header->SetPreferredSize(gfx::Size(0, kDateGroupHeaderHeight));

  auto title_label = std::make_unique<views::Label>(title);
  title_label->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_PRIMARY));
  title_label->SetFontList(title_label->font_list().Derive(
      0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD));
  title_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x80));
  header->AddChildView(std::move(title_label));

  group->AddChildView(std::move(header));

  return group.release();
}

views::View* AstraDownloadsPageView::CreateDownloadItem(
    const std::string& id) {
  if (!model_)
    return new views::View();

  const AstraDownloadEntry* entry = model_->GetDownload(id);
  if (!entry)
    return new views::View();

  auto item = std::make_unique<views::View>();
  item->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kItemVerticalPadding, kItemHorizontalPadding)));
  item->SetPreferredSize(gfx::Size(0, kItemHeight));

  auto* layout =
      item->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(), 12));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // File icon.
  auto icon_view = std::make_unique<AstraIconView>(
      base::BindRepeating(&AstraDownloadsPageView::DrawFileIcon),
      gfx::Size(kIconSize, kIconSize));
  icon_view->SetColor(GetCategoryColor(entry->category));
  item->AddChildView(std::move(icon_view));

  // Text + progress section (flexible width).
  auto text_section = std::make_unique<views::View>();
  auto* text_layout = text_section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(), 2));
  text_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  text_section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded));

  // Filename.
  auto filename_label = std::make_unique<views::Label>(entry->file_name);
  filename_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  filename_label->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_PRIMARY));
  filename_label->SetFontList(filename_label->font_list().Derive(
      0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD));
  filename_label->SetEnabledColor(SK_ColorBLACK);
  filename_label->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded));
  text_section->AddChildView(std::move(filename_label));

  // URL / source.
  auto url_label = std::make_unique<views::Label>(base::UTF8ToUTF16(entry->url));
  url_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  url_label->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));
  url_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x66));
  url_label->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded));
  text_section->AddChildView(std::move(url_label));

  // Progress bar (for in-progress/paused).
  if (entry->state == AstraDownloadPageState::kInProgress ||
      entry->state == AstraDownloadPageState::kPaused) {
    auto progress_bar = std::make_unique<views::ProgressBar>();
    progress_bar->SetPreferredSize(gfx::Size(0, kProgressBarHeight));
    double progress = entry->total_bytes > 0
                          ? static_cast<double>(entry->received_bytes) /
                                entry->total_bytes
                          : 0.0;
    progress_bar->SetValue(progress);
    progress_bar->SetForegroundColor(SkColorSetRGB(0x1A, 0x73, 0xE8));
    progress_bar->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                                 views::MaximumFlexSizeRule::kUnbounded));
    text_section->AddChildView(std::move(progress_bar));
  }

  // Status text.
  auto status_label = std::make_unique<views::Label>(FormatState(*entry));
  status_label->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));
  status_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x66));
  text_section->AddChildView(std::move(status_label));

  // Danger warning.
  if (entry->danger_type != AstraDownloadDangerType::kSafe) {
    auto danger_label = std::make_unique<views::Label>(u"This file may be dangerous");
    danger_label->SetFontList(
        views::style::GetFont(views::style::CONTEXT_LABEL,
                              views::style::STYLE_SECONDARY));
    danger_label->SetEnabledColor(SkColorSetRGB(0xEA, 0x43, 0x35));
    text_section->AddChildView(std::move(danger_label));
  }

  item->AddChildView(std::move(text_section));

  // Action buttons.
  auto actions_container = std::make_unique<views::View>();
  auto* actions_layout = actions_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 4));
  actions_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kEnd);
  actions_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  gfx::Size icon_size(16, 16);
  gfx::Size button_size(kActionButtonSize, kActionButtonSize);

  // Pause/Resume button.
  if (entry->state == AstraDownloadPageState::kInProgress) {
    auto pause_btn = std::make_unique<AstraIconButton>(
        base::BindRepeating(&AstraDownloadsPageView::OnPauseClicked,
                            base::Unretained(this), id),
        base::BindRepeating(&AstraDownloadsPageView::DrawPauseIcon),
        icon_size);
    pause_btn->SetIconColor(SK_ColorBLACK);
    pause_btn->SetTooltipText(u"Pause");
    pause_btn->SetAccessibleName(u"Pause download");
    actions_container->AddChildView(std::move(pause_btn));
  } else if (entry->state == AstraDownloadPageState::kPaused) {
    auto resume_btn = std::make_unique<AstraIconButton>(
        base::BindRepeating(&AstraDownloadsPageView::OnResumeClicked,
                            base::Unretained(this), id),
        base::BindRepeating(&AstraDownloadsPageView::DrawResumeIcon),
        icon_size);
    resume_btn->SetIconColor(SK_ColorBLACK);
    resume_btn->SetTooltipText(u"Resume");
    resume_btn->SetAccessibleName(u"Resume download");
    actions_container->AddChildView(std::move(resume_btn));
  }

  // Cancel button (for active downloads).
  if (entry->state == AstraDownloadPageState::kInProgress ||
      entry->state == AstraDownloadPageState::kPaused) {
    auto cancel_btn = std::make_unique<AstraIconButton>(
        base::BindRepeating(&AstraDownloadsPageView::OnCancelClicked,
                            base::Unretained(this), id),
        base::BindRepeating(&AstraDownloadsPageView::DrawCancelIcon),
        icon_size);
    cancel_btn->SetIconColor(SK_ColorBLACK);
    cancel_btn->SetTooltipText(u"Cancel");
    cancel_btn->SetAccessibleName(u"Cancel download");
    actions_container->AddChildView(std::move(cancel_btn));
  }

  // Show in folder button.
  if (entry->state == AstraDownloadPageState::kComplete ||
      entry->state == AstraDownloadPageState::kCancelled ||
      entry->state == AstraDownloadPageState::kInterrupted) {
    auto folder_btn = std::make_unique<AstraIconButton>(
        base::BindRepeating(&AstraDownloadsPageView::OnShowInFolderClicked,
                            base::Unretained(this), id),
        base::BindRepeating(&AstraDownloadsPageView::DrawFolderIcon),
        icon_size);
    folder_btn->SetIconColor(SK_ColorBLACK);
    folder_btn->SetTooltipText(u"Show in folder");
    folder_btn->SetAccessibleName(u"Show download in folder");
    actions_container->AddChildView(std::move(folder_btn));
  }

  // Open button (for completed, openable downloads).
  if (entry->state == AstraDownloadPageState::kComplete && entry->is_openable) {
    auto open_btn = std::make_unique<views::LabelButton>(
        base::BindRepeating(&AstraDownloadsPageView::OnOpenClicked,
                            base::Unretained(this), id),
        u"Open");
    open_btn->SetMinSize(gfx::Size(0, kActionButtonSize));
    actions_container->AddChildView(std::move(open_btn));
  }

  // More button.
  auto more_btn = std::make_unique<AstraIconButton>(
      base::BindRepeating(&AstraDownloadsPageView::OnMoreClicked,
                          base::Unretained(this), id),
      base::BindRepeating(&AstraDownloadsPageView::DrawMoreIcon),
      icon_size);
  more_btn->SetIconColor(SK_ColorBLACK);
  more_btn->SetTooltipText(u"More");
  more_btn->SetAccessibleName(u"More options");
  actions_container->AddChildView(std::move(more_btn));

  item->AddChildView(std::move(actions_container));

  item->SetAccessibleName(entry->file_name);

  return item.release();
}

void AstraDownloadsPageView::UpdateEmptyState() {
  if (!empty_state_ || !model_)
    return;

  bool has_downloads = !model_->GetDownloads().empty();
  empty_state_->SetVisible(!has_downloads);
}

// -- Action handlers --------------------------------------------------------

void AstraDownloadsPageView::OnClearAllClicked() {
  if (model_)
    model_->ClearAllDownloads();
}

void AstraDownloadsPageView::OnFilterClicked(AstraDownloadsPageFilter filter) {
  if (model_)
    model_->SetFilter(filter);
}

void AstraDownloadsPageView::OnCategoryChipClicked(const std::string& category) {
  if (model_)
    model_->SetCategoryFilter(category);
}

void AstraDownloadsPageView::OnPauseClicked(const std::string& id) {
  if (model_)
    model_->PauseDownload(id);
}

void AstraDownloadsPageView::OnResumeClicked(const std::string& id) {
  if (model_)
    model_->ResumeDownload(id);
}

void AstraDownloadsPageView::OnCancelClicked(const std::string& id) {
  if (model_)
    model_->CancelDownload(id);
}

void AstraDownloadsPageView::OnOpenClicked(const std::string& id) {
  if (model_)
    model_->OpenDownload(id);
}

void AstraDownloadsPageView::OnShowInFolderClicked(const std::string& id) {
  if (model_)
    model_->ShowInFolder(id);
}

void AstraDownloadsPageView::OnMoreClicked(const std::string& id) {
  // TODO(astra): Show a context menu with more options (remove, retry, etc.)
}

// -- Custom icon drawing ----------------------------------------------------

void AstraDownloadsPageView::DrawDownloadIcon(gfx::Canvas* canvas,
                                              const gfx::Rect& bounds,
                                              SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 0.4;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Arrow pointing down.
  SkPath path;
  path.moveTo(cx, cy - size);
  path.lineTo(cx, cy + size * 0.3f);
  path.lineTo(cx - size * 0.6f, cy + size * 0.3f);
  path.moveTo(cx, cy + size * 0.3f);
  path.lineTo(cx + size * 0.6f, cy + size * 0.3f);
  canvas->DrawPath(path, flags);

  // Tray at bottom.
  canvas->DrawLine(gfx::Point(cx - size * 0.8f, cy + size * 0.8f),
                   gfx::Point(cx + size * 0.8f, cy + size * 0.8f), flags);
}

void AstraDownloadsPageView::DrawFolderIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  int x = bounds.x() + (bounds.width() - bounds.width() * 0.8f) / 2;
  int y = bounds.y() + (bounds.height() - bounds.height() * 0.7f) / 2;
  float w = bounds.width() * 0.8f;
  float h = bounds.height() * 0.7f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Folder shape.
  SkPath path;
  path.moveTo(x, y + h * 0.3f);
  path.lineTo(x, y + h);
  path.lineTo(x + w, y + h);
  path.lineTo(x + w, y + h * 0.3f);
  path.lineTo(x + w * 0.35f, y + h * 0.3f);
  path.lineTo(x + w * 0.25f, y);
  path.lineTo(x, y);
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraDownloadsPageView::DrawFileIcon(gfx::Canvas* canvas,
                                          const gfx::Rect& bounds,
                                          SkColor color) {
  int x = bounds.x() + (bounds.width() - bounds.width() * 0.65f) / 2;
  int y = bounds.y() + (bounds.height() - bounds.height() * 0.75f) / 2;
  float w = bounds.width() * 0.65f;
  float h = bounds.height() * 0.75f;
  float fold = w * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // File with folded corner.
  SkPath path;
  path.moveTo(x, y);
  path.lineTo(x + w - fold, y);
  path.lineTo(x + w, y + fold);
  path.lineTo(x + w, y + h);
  path.lineTo(x, y + h);
  path.close();
  canvas->DrawPath(path, flags);

  // Fold line.
  SkPath fold_path;
  fold_path.moveTo(x + w - fold, y);
  fold_path.lineTo(x + w - fold, y + fold);
  fold_path.lineTo(x + w, y + fold);
  canvas->DrawPath(fold_path, flags);

  // Document lines.
  canvas->DrawLine(gfx::Point(x + w * 0.2f, y + h * 0.55f),
                   gfx::Point(x + w * 0.8f, y + h * 0.55f), flags);
  canvas->DrawLine(gfx::Point(x + w * 0.2f, y + h * 0.7f),
                   gfx::Point(x + w * 0.8f, y + h * 0.7f), flags);
  canvas->DrawLine(gfx::Point(x + w * 0.2f, y + h * 0.85f),
                   gfx::Point(x + w * 0.55f, y + h * 0.85f), flags);
}

void AstraDownloadsPageView::DrawPauseIcon(gfx::Canvas* canvas,
                                           const gfx::Rect& bounds,
                                           SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float bar_width = std::min(bounds.width(), bounds.height()) * 0.25f;
  float bar_height = std::min(bounds.width(), bounds.height()) * 0.6f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(0);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  // Two vertical bars.
  canvas->DrawRect(
      gfx::Rect(cx - bar_width * 1.5f, cy - bar_height / 2, bar_width,
                bar_height),
      flags);
  canvas->DrawRect(
      gfx::Rect(cx + bar_width * 0.5f, cy - bar_height / 2, bar_width,
                bar_height),
      flags);
}

void AstraDownloadsPageView::DrawResumeIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(0);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  // Play triangle (pointing right).
  SkPath path;
  path.moveTo(cx - size * 0.5f, cy - size * 0.6f);
  path.lineTo(cx + size * 0.6f, cy);
  path.lineTo(cx - size * 0.5f, cy + size * 0.6f);
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraDownloadsPageView::DrawCancelIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // X mark.
  canvas->DrawLine(gfx::Point(cx - size, cy - size),
                   gfx::Point(cx + size, cy + size), flags);
  canvas->DrawLine(gfx::Point(cx + size, cy - size),
                   gfx::Point(cx - size, cy + size), flags);
}

void AstraDownloadsPageView::DrawDangerIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Warning triangle (exclamation inside).
  SkPath path;
  path.moveTo(cx, cy - size);
  path.lineTo(cx + size * 0.9f, cy + size * 0.6f);
  path.lineTo(cx - size * 0.9f, cy + size * 0.6f);
  path.close();
  canvas->DrawPath(path, flags);

  // Exclamation mark.
  canvas->DrawLine(gfx::Point(cx, cy - size * 0.2f),
                   gfx::Point(cx, cy + size * 0.3f), flags);
  // Dot at bottom.
  cc::PaintFlags dot_flags = flags;
  dot_flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawCircle(gfx::Point(cx, cy + size * 0.55f), 1.5f, dot_flags);
}

void AstraDownloadsPageView::DrawSearchIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  int cx = bounds.x() + bounds.width() * 0.4f;
  int cy = bounds.y() + bounds.height() * 0.4f;
  float radius = std::min(bounds.width(), bounds.height()) * 0.3f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Magnifying glass circle.
  canvas->DrawCircle(gfx::Point(cx, cy), radius, flags);

  // Handle.
  canvas->DrawLine(
      gfx::Point(cx + radius * 0.7f, cy + radius * 0.7f),
      gfx::Point(cx + radius * 1.4f, cy + radius * 1.4f), flags);
}

void AstraDownloadsPageView::DrawMoreIcon(gfx::Canvas* canvas,
                                          const gfx::Rect& bounds,
                                          SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float dot_radius = 1.8f;
  float spacing = 5;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  // Three dots vertically.
  canvas->DrawCircle(gfx::Point(cx, cy - spacing), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx, cy + spacing), dot_radius, flags);
}

// -- Helpers ----------------------------------------------------------------

SkColor AstraDownloadsPageView::GetCategoryColor(
    const std::string& category) const {
  if (category == "Images")
    return SkColorSetRGB(0x34, 0xA8, 0x53);
  if (category == "Videos")
    return SkColorSetRGB(0xEA, 0x43, 0x35);
  if (category == "Audio")
    return SkColorSetRGB(0xA1, 0x42, 0xF4);
  if (category == "Documents")
    return SkColorSetRGB(0x1A, 0x73, 0xE8);
  if (category == "Archives")
    return SkColorSetRGB(0xFB, 0xBC, 0x04);
  return SkColorSetRGB(0x5F, 0x63, 0x68);
}

std::u16string AstraDownloadsPageView::FormatBytes(int64_t bytes) {
  if (bytes < 0)
    return u"Unknown size";
  if (bytes < 1024)
    return base::NumberToString16(bytes) + u" B";
  if (bytes < 1024 * 1024)
    return base::UTF8ToUTF16(base::StringPrintf(
        "%.1f KB", static_cast<double>(bytes) / 1024));
  if (bytes < 1024 * 1024 * 1024)
    return base::UTF8ToUTF16(base::StringPrintf(
        "%.1f MB", static_cast<double>(bytes) / (1024 * 1024)));
  return base::UTF8ToUTF16(base::StringPrintf(
      "%.1f GB", static_cast<double>(bytes) / (1024 * 1024 * 1024)));
}

std::u16string AstraDownloadsPageView::FormatState(
    const AstraDownloadEntry& entry) {
  switch (entry.state) {
    case AstraDownloadPageState::kInProgress: {
      std::u16 received = FormatBytes(entry.received_bytes);
      std::u16 total = FormatBytes(entry.total_bytes);
      return received + u" of " + total;
    }
    case AstraDownloadPageState::kPaused:
      return u"Paused — " + FormatBytes(entry.received_bytes) + u" of " +
             FormatBytes(entry.total_bytes);
    case AstraDownloadPageState::kComplete:
      return FormatBytes(entry.total_bytes);
    case AstraDownloadPageState::kCancelled:
      return u"Cancelled";
    case AstraDownloadPageState::kInterrupted:
      return u"Interrupted — check connection";
  }
  return std::u16string();
}

AstraDownloadsPageView::DateGroup AstraDownloadsPageView::GetDateGroup(
    const base::Time& time) {
  base::Time now = base::Time::Now();

  // Normalize to start of day.
  base::Time start_of_today = now.LocalMidnight();
  base::Time start_of_day = time.LocalMidnight();

  if (start_of_day == start_of_today)
    return DateGroup::kToday;

  if (start_of_day == start_of_today - base::Days(1))
    return DateGroup::kYesterday;

  if (time > now - base::Days(7))
    return DateGroup::kLast7Days;

  return DateGroup::kOlder;
}

std::u16string AstraDownloadsPageView::GetDateGroupLabel(DateGroup group) {
  switch (group) {
    case DateGroup::kToday:
      return u"Today";
    case DateGroup::kYesterday:
      return u"Yesterday";
    case DateGroup::kLast7Days:
      return u"Last 7 days";
    case DateGroup::kOlder:
      return u"Older";
  }
  return std::u16string();
}

}  // namespace astra
