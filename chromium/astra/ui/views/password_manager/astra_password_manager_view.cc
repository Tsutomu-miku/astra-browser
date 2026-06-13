// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/password_manager/astra_password_manager_view.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "skia/core/SkRect.h"
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
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

// Helper to get a deterministic color for a site (for favicon placeholders).
SkColor GetSiteColor(const std::string& site_name) {
  size_t hash = std::hash<std::string>{}(site_name);
  uint8_t r = static_cast<uint8_t>((hash & 0xFF) + 100);
  uint8_t g = static_cast<uint8_t>(((hash >> 8) & 0xFF) + 100);
  uint8_t b = static_cast<uint8_t>(((hash >> 16) & 0xFF) + 100);
  r = std::min(r, static_cast<uint8_t>(210));
  g = std::min(g, static_cast<uint8_t>(210));
  b = std::min(b, static_cast<uint8_t>(210));
  return SkColorSetRGB(r, g, b);
}

// Get first character of site name, uppercase.
char16_t GetSiteInitial(const std::u16string& site_name) {
  if (site_name.empty()) {
    return '?';
  }
  char16_t c = site_name[0];
  if (c >= u'a' && c <= u'z') {
    c = c - u'a' + u'A';
  }
  return c;
}

}  // namespace

// ===========================================================================
// AstraPasswordManagerView
// ===========================================================================

BEGIN_METADATA(AstraPasswordManagerView)
END_METADATA

AstraPasswordManagerView::AstraPasswordManagerView() {
  Build();
}

AstraPasswordManagerView::AstraPasswordManagerView(
    AstraPasswordManagerModel* model)
    : model_(model) {
  Build();
  if (model_) {
    model_observation_.Observe(model_);
    RefreshFromModel();
  }
}

AstraPasswordManagerView::~AstraPasswordManagerView() = default;

void AstraPasswordManagerView::SetModel(AstraPasswordManagerModel* model) {
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

void AstraPasswordManagerView::RefreshFromModel() {
  if (!model_) {
    RebuildPasswordList();
    RebuildSidebar();
    UpdateStatusBar();
    UpdateEmptyState();
    ClearDetailPanel();
    return;
  }

  RebuildSidebar();
  RebuildPasswordList();
  UpdateStatusBar();
  UpdateEmptyState();

  // If we have a selected password, update the detail panel.
  if (!selected_password_id_.empty()) {
    UpdateDetailPanel(selected_password_id_);
  } else {
    ClearDetailPanel();
  }
}

gfx::Size AstraPasswordManagerView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(1000, 600);
}

void AstraPasswordManagerView::Layout() {
  gfx::Rect bounds = GetContentsBounds();

  // Toolbar at top.
  if (toolbar_ && toolbar_->GetVisible()) {
    toolbar_->SetBounds(bounds.x(), bounds.y(), bounds.width(),
                        kToolbarHeight);
  }

  // Status bar at bottom.
  if (status_bar_ && status_bar_->GetVisible()) {
    status_bar_->SetBounds(bounds.x(), bounds.bottom() - kStatusBarHeight,
                           bounds.width(), kStatusBarHeight);
  }

  // Middle area: sidebar + content + detail panel.
  int middle_top = toolbar_ ? toolbar_->bounds().bottom() : bounds.y();
  int middle_bottom =
      status_bar_ ? status_bar_->bounds().y() : bounds.bottom();
  int middle_height = middle_bottom - middle_top;

  // Sidebar on the left.
  if (sidebar_scroll_ && sidebar_scroll_->GetVisible()) {
    sidebar_scroll_->SetBounds(bounds.x(), middle_top, kSidebarWidth,
                               middle_height);
  }

  // Detail panel on the right.
  int detail_width = 0;
  if (detail_panel_ && detail_panel_->GetVisible()) {
    detail_width = kDetailPanelWidth;
    detail_panel_->SetBounds(bounds.right() - detail_width, middle_top,
                             detail_width, middle_height);
  }

  // Content area in the middle.
  int content_x = sidebar_scroll_
                      ? sidebar_scroll_->bounds().right()
                      : bounds.x();
  int content_width =
      (detail_panel_ ? detail_panel_->bounds().x() : bounds.right()) -
      content_x;
  if (content_scroll_ && content_scroll_->GetVisible()) {
    content_scroll_->SetBounds(content_x, middle_top, content_width,
                               middle_height);
  }
}

void AstraPasswordManagerView::OnThemeChanged() {
  views::View::OnThemeChanged();
  // Colors will be updated when children repaint.
}

// -- AstraPasswordManagerObserver: ------------------------------------------

void AstraPasswordManagerView::OnPasswordsChanged() {
  RebuildPasswordList();
  RebuildSidebar();
  UpdateStatusBar();
  UpdateEmptyState();
}

void AstraPasswordManagerView::OnPasswordAdded(const std::string& id) {
  RebuildPasswordList();
  UpdateStatusBar();
  UpdateEmptyState();
}

void AstraPasswordManagerView::OnPasswordRemoved(const std::string& id) {
  if (selected_password_id_ == id) {
    selected_password_id_.clear();
    ClearDetailPanel();
  }
  RebuildPasswordList();
  UpdateStatusBar();
  UpdateEmptyState();
}

void AstraPasswordManagerView::OnPasswordUpdated(const std::string& id) {
  // Find the row and update it.
  for (auto* row : password_rows_) {
    if (row->entry_id() == id) {
      const auto* entry = model_ ? model_->GetPassword(id) : nullptr;
      if (entry) {
        row->SetEntry(*entry);
      }
      break;
    }
  }
  if (selected_password_id_ == id) {
    UpdateDetailPanel(id);
  }
  UpdateStatusBar();
}

void AstraPasswordManagerView::OnSearchChanged(const std::u16string& query) {
  // Update search field if it doesn't already have this text.
  if (search_field_ && search_field_->GetText() != query) {
    search_field_->SetText(query);
  }
  RebuildPasswordList();
  UpdateEmptyState();
}

void AstraPasswordManagerView::OnFilterChanged() {
  RebuildPasswordList();
  RebuildSidebar();
  UpdateEmptyState();
}

void AstraPasswordManagerView::OnPasswordManagerModelShutdown() {
  model_ = nullptr;
  model_observation_.Reset();
}

// -- TextfieldController: ---------------------------------------------------

void AstraPasswordManagerView::ContentsChanged(views::Textfield* sender,
                                               const std::u16string& new_contents) {
  if (sender == search_field_) {
    if (model_) {
      model_->SetSearchQuery(new_contents);
    }
    if (delegate_) {
      delegate_->OnSearchQueryChanged(new_contents);
    }
  }
}

// -- Build methods ----------------------------------------------------------

void AstraPasswordManagerView::Build() {
  BuildToolbar();
  BuildSidebar();
  BuildContent();
  BuildDetailPanel();
  BuildStatusBar();
}

void AstraPasswordManagerView::BuildToolbar() {
  toolbar_ = AddChildView(std::make_unique<views::View>());

  auto* layout = toolbar_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetInteriorMargin(gfx::Insets::VH(0, kSidePadding));
  layout->SetDefault(views::kMarginsKey,
                     gfx::Insets::VH(0, kToolbarSpacing));

  // Title / Key icon placeholder.
  auto title_label = std::make_unique<views::Label>(u"Passwords");
  title_label->SetFontList(title_label->font_list().Derive(4, gfx::Font::NORMAL,
                                                          gfx::Font::Weight::BOLD));
  title_label->SetProperty(views::kFlexBehaviorKey,
                           views::FlexSpecification::ForSizeInfo(
                               views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded));
  toolbar_->AddChildView(std::move(title_label));

  // Search field.
  auto search_field = std::make_unique<views::Textfield>();
  search_field->SetPlaceholderText(u"Search passwords...");
  search_field->SetBackgroundColor(SK_ColorTRANSPARENT);
  search_field->SetBorder(views::CreateRoundedRectBorder(
      1, 8, SkColorSetRGB(200, 200, 200)));
  search_field->set_controller(this);
  search_field->SetProperty(
      views::kPreferredSizeKey,
      gfx::Size(kSearchFieldWidth, 36));
  search_field_ = toolbar_->AddChildView(std::move(search_field));

  // Add button.
  auto add_button = std::make_unique<views::ImageButton>(base::BindRepeating(
      &AstraPasswordManagerView::OnAddButtonClicked,
      base::Unretained(this)));
  add_button->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  add_button->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  add_button->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  add_button_ = toolbar_->AddChildView(std::move(add_button));

  // Sort button.
  auto sort_button = std::make_unique<views::ImageButton>(base::BindRepeating(
      &AstraPasswordManagerView::OnSortButtonClicked,
      base::Unretained(this)));
  sort_button->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  sort_button_ = toolbar_->AddChildView(std::move(sort_button));

  // More button.
  auto more_button = std::make_unique<views::ImageButton>(base::BindRepeating(
      &AstraPasswordManagerView::OnMoreButtonClicked,
      base::Unretained(this)));
  more_button->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  more_button_ = toolbar_->AddChildView(std::move(more_button));
}

void AstraPasswordManagerView::BuildSidebar() {
  sidebar_scroll_ = AddChildView(std::make_unique<views::ScrollView>());
  sidebar_scroll_->SetBackgroundColor(SK_ColorTRANSPARENT);
  sidebar_scroll_->SetDrawOverflowIndicator(false);

  sidebar_content_ = sidebar_scroll_->SetContents(
      std::make_unique<views::View>());
  sidebar_content_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));

  // Filters section.
  filter_section_ = sidebar_content_->AddChildView(
      std::make_unique<views::View>());
  filter_section_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  filter_section_->SetBorder(
      views::CreateEmptyBorder(gfx::Insets::VH(8, 0)));

  // Categories section.
  auto category_label = std::make_unique<views::Label>(u"Categories");
  category_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  category_label->SetFontList(
      category_label->font_list().Derive(-1, gfx::Font::NORMAL,
                                         gfx::Font::Weight::BOLD));
  category_label->SetBorder(views::CreateEmptyBorder(8, 12, 4, 0));
  sidebar_content_->AddChildView(std::move(category_label));

  category_section_ = sidebar_content_->AddChildView(
      std::make_unique<views::View>());
  category_section_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
}

void AstraPasswordManagerView::BuildContent() {
  content_scroll_ = AddChildView(std::make_unique<views::ScrollView>());
  content_scroll_->SetBackgroundColor(SK_ColorTRANSPARENT);

  content_container_ = content_scroll_->SetContents(
      std::make_unique<views::View>());
  content_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(16, 16),
          kGroupSpacing));

  // Empty state (initially hidden).
  empty_state_view_ = content_container_->AddChildView(
      std::make_unique<views::View>());
  empty_state_view_->SetVisible(false);
  empty_state_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(),
          8,
          true));

  auto empty_label = std::make_unique<views::Label>(u"No passwords found");
  empty_label->SetFontList(empty_label->font_list().Derive(
      2, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
  empty_state_view_->AddChildView(std::move(empty_label));

  auto empty_hint = std::make_unique<views::Label>(
      u"Add your first password to get started.");
  empty_hint->SetEnabled(false);
  empty_state_view_->AddChildView(std::move(empty_hint));
}

void AstraPasswordManagerView::BuildDetailPanel() {
  detail_panel_ = AddChildView(std::make_unique<views::View>());
  detail_panel_->SetVisible(false);
  detail_panel_->SetBorder(views::CreateSolidSidedBorder(
      0, 1, 0, 0, SkColorSetRGB(220, 220, 220)));
  detail_panel_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(16, 16),
      12));

  // Detail panel title.
  auto detail_title = std::make_unique<views::Label>(u"Password details");
  detail_title->SetFontList(detail_title->font_list().Derive(
      2, gfx::Font::NORMAL, gfx::Font::Weight::BOLD));
  detail_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_panel_->AddChildView(std::move(detail_title));

  // Site name, username, password, etc. will be added dynamically.
}

void AstraPasswordManagerView::BuildStatusBar() {
  status_bar_ = AddChildView(std::make_unique<views::View>());
  status_bar_->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SkColorSetRGB(220, 220, 220)));

  auto* layout = status_bar_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetInteriorMargin(gfx::Insets::VH(0, kSidePadding));

  status_label_ = status_bar_->AddChildView(
      std::make_unique<views::Label>(u"0 passwords"));
  status_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeInfo(
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kUnbounded));

  issues_label_ = status_bar_->AddChildView(
      std::make_unique<views::Label>(u""));
  issues_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  issues_label_->SetEnabledColor(SkColorSetRGB(200, 80, 80));
}

// -- Rebuild methods --------------------------------------------------------

void AstraPasswordManagerView::RebuildSidebar() {
  // Clear existing filter items.
  filter_section_->RemoveAllChildViews();
  category_section_->RemoveAllChildViews();
  sidebar_items_.clear();

  if (!model_) {
    return;
  }

  // Build filter items.
  struct FilterInfo {
    AstraPasswordFilter filter;
    const char* label;
    int count;
  };

  std::vector<FilterInfo> filters;
  filters.push_back({AstraPasswordFilter::kAll, "All Passwords",
                     static_cast<int>(model_->GetCount())});

  // Count weak passwords.
  int weak_count = 0, leaked_count = 0, reused_count = 0, fav_count = 0;
  for (const auto& entry : model_->GetPasswords()) {
    if (entry.is_weak) weak_count++;
    if (entry.is_leaked) leaked_count++;
    if (entry.is_reused) reused_count++;
    if (entry.is_favorited) fav_count++;
  }

  filters.push_back({AstraPasswordFilter::kWeak, "Weak Passwords", weak_count});
  filters.push_back(
      {AstraPasswordFilter::kReused, "Reused Passwords", reused_count});
  filters.push_back(
      {AstraPasswordFilter::kLeaked, "Leaked Passwords", leaked_count});
  filters.push_back(
      {AstraPasswordFilter::kFavorites, "Favorites", fav_count});

  for (const auto& info : filters) {
    auto item = std::make_unique<AstraPasswordSidebarItem>(
        base::UTF8ToUTF16(info.label), info.count, true);
    item->SetSelected(selected_filter_ == info.filter &&
                      selected_category_.empty());
    item->SetClickCallback(base::BindRepeating(
        &AstraPasswordManagerView::OnSidebarFilterClicked,
        base::Unretained(this), info.filter));
    sidebar_items_.push_back(
        filter_section_->AddChildView(std::move(item)));
  }

  // Build category items.
  auto categories = model_->GetCategories();
  for (const auto& cat : categories) {
    int count = 0;
    for (const auto& entry : model_->GetPasswords()) {
      if (entry.category == cat) count++;
    }
    auto item = std::make_unique<AstraPasswordSidebarItem>(
        base::UTF8ToUTF16(cat), count, false);
    item->SetSelected(selected_category_ == cat);
    item->SetClickCallback(base::BindRepeating(
        &AstraPasswordManagerView::OnSidebarCategoryClicked,
        base::Unretained(this), cat));
    sidebar_items_.push_back(
        category_section_->AddChildView(std::move(item)));
  }
}

void AstraPasswordManagerView::RebuildPasswordList() {
  // Clear existing group sections and password rows.
  password_rows_.clear();

  // Remove all child views except the empty state.
  std::vector<views::View*> children = content_container_->children();
  for (auto* child : children) {
    if (child != empty_state_view_) {
      content_container_->RemoveChildViewT(child);
    }
  }

  if (!model_) {
    return;
  }

  const auto& groups = model_->GetGroupedPasswords();
  if (groups.empty()) {
    return;
  }

  for (const auto& group : groups) {
    auto section = std::make_unique<AstraPasswordGroupSection>(group);

    // Wire up callbacks for each row.
    for (size_t i = 0; i < section->GetEntryCount(); ++i) {
      auto* row = section->GetEntryRow(i);
      row->SetClickCallback(base::BindRepeating(
          &AstraPasswordManagerView::OnPasswordRowClicked,
          base::Unretained(this)));
      row->SetFavoriteCallback(base::BindRepeating(
          &AstraPasswordManagerView::OnPasswordRowFavoriteClicked,
          base::Unretained(this)));
      row->SetCopyCallback(base::BindRepeating(
          &AstraPasswordManagerView::OnPasswordRowCopyClicked,
          base::Unretained(this)));
      row->SetDeleteCallback(base::BindRepeating(
          &AstraPasswordManagerView::OnPasswordRowDeleteClicked,
          base::Unretained(this)));
      row->SetSelected(row->entry_id() == selected_password_id_);
      password_rows_.push_back(row);
    }

    content_container_->AddChildView(std::move(section));
  }
}

void AstraPasswordManagerView::UpdateStatusBar() {
  if (!model_) {
    status_label_->SetText(u"0 passwords");
    issues_label_->SetText(u"");
    return;
  }

  size_t count = model_->GetFilteredPasswords().size();
  std::u16string text = base::NumberToString16(count) +
                        (count == 1 ? u" password" : u" passwords");
  status_label_->SetText(text);

  size_t issues = model_->GetPasswordIssuesCount();
  if (issues > 0) {
    issues_label_->SetText(base::NumberToString16(issues) +
                           u" issue(s) found");
  } else {
    issues_label_->SetText(u"All passwords secure");
    issues_label_->SetEnabledColor(SkColorSetRGB(80, 160, 80));
  }
}

void AstraPasswordManagerView::UpdateEmptyState() {
  bool empty = !model_ || model_->GetFilteredPasswords().empty();
  empty_state_view_->SetVisible(empty);
  content_scroll_->SetVisible(!empty || empty_state_view_->GetVisible());
}

void AstraPasswordManagerView::UpdateDetailPanel(const std::string& id) {
  if (!model_ || !detail_panel_) {
    return;
  }

  const auto* entry = model_->GetPassword(id);
  if (!entry) {
    ClearDetailPanel();
    return;
  }

  detail_panel_->SetVisible(true);

  // Clear existing content (keep layout).
  detail_panel_->RemoveAllChildViews();
  detail_panel_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(16, 16),
      10));

  // Site name.
  auto site_label = std::make_unique<views::Label>(entry->site_name);
  site_label->SetFontList(site_label->font_list().Derive(
      3, gfx::Font::NORMAL, gfx::Font::Weight::BOLD));
  site_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_panel_->AddChildView(std::move(site_label));

  // Username field.
  auto username_title = std::make_unique<views::Label>(u"Username");
  username_title->SetFontList(
      username_title->font_list().Derive(-1, gfx::Font::NORMAL,
                                         gfx::Font::Weight::BOLD));
  username_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  username_title->SetEnabled(false);
  detail_panel_->AddChildView(std::move(username_title));

  auto username_value = std::make_unique<views::Label>(entry->username);
  username_value->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_panel_->AddChildView(std::move(username_value));

  // Password field.
  auto password_title = std::make_unique<views::Label>(u"Password");
  password_title->SetFontList(
      password_title->font_list().Derive(-1, gfx::Font::NORMAL,
                                         gfx::Font::Weight::BOLD));
  password_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  password_title->SetEnabled(false);
  detail_panel_->AddChildView(std::move(password_title));

  std::u16string hidden_pw(entry->password.size(), u'*');
  auto password_value = std::make_unique<views::Label>(hidden_pw);
  password_value->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_panel_->AddChildView(std::move(password_value));

  // URL.
  auto url_title = std::make_unique<views::Label>(u"Website");
  url_title->SetFontList(url_title->font_list().Derive(-1, gfx::Font::NORMAL,
                                                       gfx::Font::Weight::BOLD));
  url_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  url_title->SetEnabled(false);
  detail_panel_->AddChildView(std::move(url_title));

  auto url_value =
      std::make_unique<views::Label>(base::UTF8ToUTF16(entry->url));
  url_value->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  url_value->SetEnabledColor(SkColorSetRGB(66, 133, 244));
  detail_panel_->AddChildView(std::move(url_value));

  // Notes.
  if (!entry->notes.empty()) {
    auto notes_title = std::make_unique<views::Label>(u"Notes");
    notes_title->SetFontList(notes_title->font_list().Derive(
        -1, gfx::Font::NORMAL, gfx::Font::Weight::BOLD));
    notes_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    notes_title->SetEnabled(false);
    detail_panel_->AddChildView(std::move(notes_title));

    auto notes_value = std::make_unique<views::Label>(entry->notes);
    notes_value->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    notes_value->SetMultiLine(true);
    detail_panel_->AddChildView(std::move(notes_value));
  }

  // Category.
  auto cat_title = std::make_unique<views::Label>(u"Category");
  cat_title->SetFontList(cat_title->font_list().Derive(-1, gfx::Font::NORMAL,
                                                       gfx::Font::Weight::BOLD));
  cat_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  cat_title->SetEnabled(false);
  detail_panel_->AddChildView(std::move(cat_title));

  auto cat_value =
      std::make_unique<views::Label>(base::UTF8ToUTF16(entry->category));
  cat_value->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail_panel_->AddChildView(std::move(cat_value));

  // Spacer.
  auto spacer = std::make_unique<views::View>();
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeInfo(
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
  detail_panel_->AddChildView(std::move(spacer));

  // Buttons row.
  auto buttons_row = std::make_unique<views::View>();
  buttons_row->SetLayoutManager(std::make_unique<views::FlexLayout>(
      views::LayoutOrientation::kHorizontal,
      views::LayoutAlignment::kStretch,
      views::FlexDistribution::kSpaceEvenly));

  auto edit_btn = std::make_unique<views::LabelButton>(base::BindRepeating(
      &AstraPasswordManagerView::OnDetailEditClicked,
      base::Unretained(this)), u"Edit");
  edit_btn->SetStyle(ui::ButtonStyle::kTonal);
  buttons_row->AddChildView(std::move(edit_btn));

  auto delete_btn = std::make_unique<views::LabelButton>(base::BindRepeating(
      &AstraPasswordManagerView::OnDetailDeleteClicked,
      base::Unretained(this)), u"Delete");
  delete_btn->SetStyle(ui::ButtonStyle::kTonal);
  buttons_row->AddChildView(std::move(delete_btn));

  detail_panel_->AddChildView(std::move(buttons_row));

  Layout();
}

void AstraPasswordManagerView::ClearDetailPanel() {
  if (detail_panel_) {
    detail_panel_->SetVisible(false);
  }
  selected_password_id_.clear();
  Layout();
}

// -- Event handlers ---------------------------------------------------------

void AstraPasswordManagerView::OnAddButtonClicked() {
  if (delegate_) {
    delegate_->OnAddPassword();
  }
}

void AstraPasswordManagerView::OnSortButtonClicked() {
  // TODO(astra): Show sort menu.
}

void AstraPasswordManagerView::OnMoreButtonClicked() {
  // TODO(astra): Show more options menu (import, export, settings).
}

void AstraPasswordManagerView::OnSidebarFilterClicked(AstraPasswordFilter filter) {
  selected_filter_ = filter;
  selected_category_.clear();
  if (model_) {
    model_->SetFilter(filter);
    model_->SetCategoryFilter("");
  }
  if (delegate_) {
    delegate_->OnFilterChanged(filter);
  }
  RebuildSidebar();
}

void AstraPasswordManagerView::OnSidebarCategoryClicked(
    const std::string& category) {
  selected_category_ = category;
  selected_filter_ = AstraPasswordFilter::kAll;
  if (model_) {
    model_->SetCategoryFilter(category);
    model_->SetFilter(AstraPasswordFilter::kAll);
  }
  if (delegate_) {
    delegate_->OnCategoryFilterChanged(category);
  }
  RebuildSidebar();
}

void AstraPasswordManagerView::OnPasswordRowClicked(const std::string& id) {
  selected_password_id_ = id;

  // Update selected state on all rows.
  for (auto* row : password_rows_) {
    row->SetSelected(row->entry_id() == id);
  }

  UpdateDetailPanel(id);

  if (delegate_) {
    delegate_->OnPasswordEntryClicked(id);
  }
}

void AstraPasswordManagerView::OnPasswordRowFavoriteClicked(
    const std::string& id) {
  if (model_) {
    model_->ToggleFavorite(id);
  }
}

void AstraPasswordManagerView::OnPasswordRowCopyClicked(const std::string& id) {
  if (model_) {
    model_->CopyPassword(id);
  }
  if (delegate_) {
    delegate_->OnCopyPassword(id);
  }
}

void AstraPasswordManagerView::OnPasswordRowDeleteClicked(
    const std::string& id) {
  if (model_) {
    model_->RemovePassword(id);
  }
  if (delegate_) {
    delegate_->OnRemovePassword(id);
  }
}

void AstraPasswordManagerView::OnDetailEditClicked() {
  // TODO(astra): Show edit dialog.
}

void AstraPasswordManagerView::OnDetailDeleteClicked() {
  if (!selected_password_id_.empty()) {
    OnPasswordRowDeleteClicked(selected_password_id_);
  }
}

// -- Custom icon painting ---------------------------------------------------

void AstraPasswordManagerView::DrawKeyIcon(gfx::Canvas* canvas,
                                           const gfx::Rect& bounds,
                                           SkColor color) {
  // Simple key icon: circle + stem.
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int ring_r = std::min(bounds.width(), bounds.height()) / 4;

  // Key ring.
  canvas->DrawCircle(gfx::Point(cx - ring_r / 2, cy), ring_r, flags);

  // Key stem.
  SkPath path;
  path.moveTo(cx, cy);
  path.lineTo(cx + ring_r * 2, cy);
  path.lineTo(cx + ring_r * 2, cy + ring_r / 2);
  path.lineTo(cx + ring_r * 1.5, cy + ring_r / 2);
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraPasswordManagerView::DrawLockIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = std::min(bounds.width(), bounds.height()) / 2;
  int h = w * 0.8f;

  // Lock body (rectangle with rounded top).
  SkRect body = SkRect::MakeXYWH(cx - w / 2, cy - h / 3, w, h * 0.7f);
  canvas->DrawRoundRect(gfx::Rect(body.x(), body.y(), body.width(),
                                  body.height()),
                        3, flags);

  // Shackle (arc on top).
  SkPath shackle;
  shackle.moveTo(cx - w / 3, cy - h / 3);
  shackle.arcTo(SkRect::MakeXYWH(cx - w / 3, cy - h * 2 / 3, w * 2 / 3,
                                  h * 2 / 3),
                180, 180, false);
  shackle.lineTo(cx + w / 3, cy - h / 3);
  canvas->DrawPath(shackle, flags);
}

void AstraPasswordManagerView::DrawCopyIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);

  int x = bounds.x();
  int y = bounds.y();
  int w = bounds.width();
  int h = bounds.height();
  int margin = 4;

  // Back rectangle.
  canvas->DrawRect(
      gfx::Rect(x + margin, y + margin, w - margin - 3, h - margin - 3),
      flags);

  // Front rectangle (offset).
  canvas->DrawRect(
      gfx::Rect(x + margin + 3, y + margin + 3, w - margin - 3,
                h - margin - 3),
      flags);
}

void AstraPasswordManagerView::DrawEyeIcon(gfx::Canvas* canvas,
                                           const gfx::Rect& bounds,
                                           SkColor color,
                                           bool visible) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int eye_w = bounds.width() / 2;
  int eye_h = bounds.height() / 4;

  // Eye outline (ellipse).
  SkPath eye;
  eye.moveTo(cx - eye_w, cy);
  eye.arcTo(SkRect::MakeXYWH(cx - eye_w, cy - eye_h, eye_w, eye_h * 2),
            90, 180, false);
  eye.arcTo(SkRect::MakeXYWH(cx, cy - eye_h, eye_w, eye_h * 2),
            270, 180, false);
  eye.close();
  canvas->DrawPath(eye, flags);

  // Pupil.
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawCircle(gfx::Point(cx, cy), eye_h / 2, flags);

  if (!visible) {
    // Crossed-out line.
    flags.setStyle(cc::PaintFlags::kStroke_Style);
    flags.setStrokeWidth(1.5f);
    canvas->DrawLine(gfx::Point(bounds.x() + 2, bounds.bottom() - 2),
                     gfx::Point(bounds.right() - 2, bounds.y() + 2), flags);
  }
}

void AstraPasswordManagerView::DrawStarIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color,
                                            bool filled) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(filled ? cc::PaintFlags::kFill_Style
                        : cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int outer_r = std::min(bounds.width(), bounds.height()) / 2 - 1;
  int inner_r = outer_r * 0.4f;

  SkPath star;
  for (int i = 0; i < 10; ++i) {
    float angle = -M_PI / 2 + i * M_PI / 5;
    float r = (i % 2 == 0) ? outer_r : inner_r;
    float x = cx + r * cos(angle);
    float y = cy + r * sin(angle);
    if (i == 0) {
      star.moveTo(x, y);
    } else {
      star.lineTo(x, y);
    }
  }
  star.close();
  canvas->DrawPath(star, flags);
}

void AstraPasswordManagerView::DrawWarningIcon(gfx::Canvas* canvas,
                                               const gfx::Rect& bounds,
                                               SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = bounds.width() - 2;
  int h = bounds.height() - 2;

  // Triangle (warning sign).
  SkPath triangle;
  triangle.moveTo(cx, cy - h / 2);
  triangle.lineTo(cx + w / 2, cy + h / 2);
  triangle.lineTo(cx - w / 2, cy + h / 2);
  triangle.close();
  canvas->DrawPath(triangle, flags);

  // Exclamation mark.
  cc::PaintFlags text_flags;
  text_flags.setColor(SK_ColorWHITE);
  text_flags.setStyle(cc::PaintFlags::kFill_Style);
  text_flags.setAntiAlias(true);
  text_flags.setStrokeWidth(1.5f);

  canvas->DrawLine(gfx::Point(cx, cy - h / 4),
                   gfx::Point(cx, cy + h / 8), text_flags);
  canvas->DrawCircle(gfx::Point(cx, cy + h / 3), 1.5f, text_flags);
}

void AstraPasswordManagerView::DrawSearchIcon(gfx::Canvas* canvas,
                                              const gfx::Rect& bounds,
                                              SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  int cx = bounds.x() + bounds.width() * 0.4;
  int cy = bounds.y() + bounds.height() * 0.4;
  int r = std::min(bounds.width(), bounds.height()) / 4;

  // Magnifying glass circle.
  canvas->DrawCircle(gfx::Point(cx, cy), r, flags);

  // Handle.
  int handle_len = r * 0.8f;
  canvas->DrawLine(
      gfx::Point(cx + r * 0.7f, cy + r * 0.7f),
      gfx::Point(cx + r * 0.7f + handle_len * 0.7f,
                 cy + r * 0.7f + handle_len * 0.7f),
      flags);
}

void AstraPasswordManagerView::DrawAddIcon(gfx::Canvas* canvas,
                                           const gfx::Rect& bounds,
                                           SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) / 3;

  canvas->DrawLine(gfx::Point(cx - size, cy), gfx::Point(cx + size, cy),
                   flags);
  canvas->DrawLine(gfx::Point(cx, cy - size), gfx::Point(cx, cy + size),
                   flags);
}

void AstraPasswordManagerView::DrawMoreIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  int cy = bounds.y() + bounds.height() / 2;
  int dot_r = 2;
  int spacing = bounds.width() / 4;

  canvas->DrawCircle(
      gfx::Point(bounds.x() + spacing, cy), dot_r, flags);
  canvas->DrawCircle(
      gfx::Point(bounds.x() + bounds.width() / 2, cy), dot_r, flags);
  canvas->DrawCircle(
      gfx::Point(bounds.right() - spacing, cy), dot_r, flags);
}

void AstraPasswordManagerView::DrawDeleteIcon(gfx::Canvas* canvas,
                                              const gfx::Rect& bounds,
                                              SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);

  int x = bounds.x() + 3;
  int y = bounds.y() + 3;
  int w = bounds.width() - 6;
  int h = bounds.height() - 6;

  // Trash can body.
  canvas->DrawRect(gfx::Rect(x + 2, y + h / 3, w - 4, h * 2 / 3), flags);

  // Trash can lid.
  canvas->DrawLine(gfx::Point(x, y + h / 3),
                   gfx::Point(x + w, y + h / 3), flags);
  canvas->DrawLine(gfx::Point(x + w / 4, y),
                   gfx::Point(x + w * 3 / 4, y), flags);
  canvas->DrawLine(gfx::Point(x + w / 4, y),
                   gfx::Point(x + w / 4, y + h / 3), flags);
  canvas->DrawLine(gfx::Point(x + w * 3 / 4, y),
                   gfx::Point(x + w * 3 / 4, y + h / 3), flags);
}

void AstraPasswordManagerView::DrawEditIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);

  int x = bounds.x() + 4;
  int y = bounds.y() + 4;
  int w = bounds.width() - 8;
  int h = bounds.height() - 8;

  // Pencil shape.
  SkPath pencil;
  pencil.moveTo(x + w, y + h * 0.2f);
  pencil.lineTo(x + w * 0.8f, y);
  pencil.lineTo(x + w * 0.2f, y + h * 0.6f);
  pencil.lineTo(x, y + h);
  pencil.lineTo(x + w * 0.4f, y + h * 0.8f);
  pencil.close();
  canvas->DrawPath(pencil, flags);
}

void AstraPasswordManagerView::DrawSortIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = bounds.width() / 3;

  // Up arrow.
  canvas->DrawLine(gfx::Point(cx, cy - 6),
                   gfx::Point(cx - w / 2, cy - 1), flags);
  canvas->DrawLine(gfx::Point(cx, cy - 6),
                   gfx::Point(cx + w / 2, cy - 1), flags);

  // Down arrow.
  canvas->DrawLine(gfx::Point(cx, cy + 6),
                   gfx::Point(cx - w / 2, cy + 1), flags);
  canvas->DrawLine(gfx::Point(cx, cy + 6),
                   gfx::Point(cx + w / 2, cy + 1), flags);
}

// ===========================================================================
// AstraPasswordEntryRow
// ===========================================================================

BEGIN_METADATA(AstraPasswordEntryRow)
END_METADATA

AstraPasswordEntryRow::AstraPasswordEntryRow(const AstraPasswordEntry& entry)
    : entry_(entry) {
  // Create child views.
  site_name_label_ = AddChildView(std::make_unique<views::Label>(entry.site_name));
  site_name_label_->SetFontList(
      site_name_label_->font_list().Derive(1, gfx::Font::NORMAL,
                                           gfx::Font::Weight::MEDIUM));
  site_name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  username_label_ = AddChildView(std::make_unique<views::Label>(entry.username));
  username_label_->SetEnabled(false);
  username_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  favorite_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraPasswordEntryRow::OnFavoriteClicked,
                          base::Unretained(this))));
  favorite_button_->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));

  copy_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraPasswordEntryRow::OnCopyClicked,
                          base::Unretained(this))));
  copy_button_->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));

  more_button_ = AddChildView(std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraPasswordEntryRow::OnMoreClicked,
                          base::Unretained(this))));
  more_button_->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
}

AstraPasswordEntryRow::~AstraPasswordEntryRow() = default;

void AstraPasswordEntryRow::SetEntry(const AstraPasswordEntry& entry) {
  entry_ = entry;
  site_name_label_->SetText(entry.site_name);
  username_label_->SetText(entry.username);
  SchedulePaint();
}

void AstraPasswordEntryRow::SetSelected(bool selected) {
  selected_ = selected;
  SchedulePaint();
}

gfx::Size AstraPasswordEntryRow::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(available_size.width().value_or(400), kRowHeight);
}

void AstraPasswordEntryRow::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int x = bounds.x() + kRowPadding;
  int y = bounds.y();

  // Favicon area (drawn in OnPaint).
  int favicon_right = x + kFaviconSize + kFaviconSpacing;

  // Site name and username (vertically stacked).
  int text_x = favicon_right;
  int text_width = bounds.width() - kRowPadding * 2 - kFaviconSize -
                   kFaviconSpacing - kButtonSize * 3 - kButtonSpacing * 3;

  site_name_label_->SetBounds(text_x, y + 8, text_width, 20);
  username_label_->SetBounds(text_x, y + 30, text_width, 18);

  // Buttons on the right.
  int btn_y = y + (kRowHeight - kButtonSize) / 2;
  int btn_x = bounds.right() - kRowPadding - kButtonSize;

  more_button_->SetBounds(btn_x, btn_y, kButtonSize, kButtonSize);
  btn_x -= kButtonSize + kButtonSpacing;

  copy_button_->SetBounds(btn_x, btn_y, kButtonSize, kButtonSize);
  btn_x -= kButtonSize + kButtonSpacing;

  favorite_button_->SetBounds(btn_x, btn_y, kButtonSize, kButtonSize);
}

void AstraPasswordEntryRow::OnThemeChanged() {
  views::View::OnThemeChanged();
}

void AstraPasswordEntryRow::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  // Background (hover / selected).
  if (selected_) {
    cc::PaintFlags bg_flags;
    bg_flags.setColor(SkColorSetARGB(20, 66, 133, 244));
    bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawRect(bounds, bg_flags);
  } else if (hovered_) {
    cc::PaintFlags bg_flags;
    bg_flags.setColor(SkColorSetARGB(10, 0, 0, 0));
    bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawRect(bounds, bg_flags);
  }

  // Favicon placeholder.
  DrawFaviconPlaceholder(canvas);

  // Warning badges.
  DrawWarningBadges(canvas);

  // Favorite star icon.
  gfx::Rect fav_bounds = favorite_button_->bounds();
  SkColor star_color = entry_.is_favorited
                           ? SkColorSetRGB(255, 193, 7)
                           : SkColorSetRGB(180, 180, 180);
  AstraPasswordManagerView::DrawStarIcon(canvas, fav_bounds, star_color,
                                         entry_.is_favorited);

  // Copy icon.
  gfx::Rect copy_bounds = copy_button_->bounds();
  AstraPasswordManagerView::DrawCopyIcon(
      canvas, copy_bounds, SkColorSetRGB(120, 120, 120));

  // More icon.
  gfx::Rect more_bounds = more_button_->bounds();
  AstraPasswordManagerView::DrawMoreIcon(
      canvas, more_bounds, SkColorSetRGB(120, 120, 120));
}

bool AstraPasswordEntryRow::OnMousePressed(const ui::MouseEvent& event) {
  return true;
}

void AstraPasswordEntryRow::OnMouseReleased(const ui::MouseEvent& event) {
  if (click_callback_) {
    click_callback_.Run(entry_.id);
  }
}

void AstraPasswordEntryRow::OnMouseEntered(const ui::MouseEvent& event) {
  hovered_ = true;
  SchedulePaint();
}

void AstraPasswordEntryRow::OnMouseExited(const ui::MouseEvent& event) {
  hovered_ = false;
  SchedulePaint();
}

void AstraPasswordEntryRow::DrawFaviconPlaceholder(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetContentsBounds();
  int x = bounds.x() + kRowPadding;
  int y = bounds.y() + (kRowHeight - kFaviconSize) / 2;

  SkColor color = GetSiteColor(base::UTF16ToUTF8(entry_.site_name));

  // Circle background.
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  canvas->DrawCircle(gfx::Point(x + kFaviconSize / 2, y + kFaviconSize / 2),
                     kFaviconSize / 2, flags);

  // Initial letter.
  char16_t initial = GetSiteInitial(entry_.site_name);
  std::u16string text(1, initial);

  cc::PaintFlags text_flags;
  text_flags.setColor(SK_ColorWHITE);
  text_flags.setStyle(cc::PaintFlags::kFill_Style);
  text_flags.setAntiAlias(true);
  text_flags.setTextSize(kFaviconSize * 0.6f);
  text_flags.setTextAlign(cc::PaintFlags::kCenter_Align);

  canvas->DrawStringRect(text, gfx::FontList(), SK_ColorWHITE,
                         gfx::Rect(x, y, kFaviconSize, kFaviconSize),
                         gfx::ALIGN_CENTER, gfx::ALIGN_MIDDLE, 0);
}

void AstraPasswordEntryRow::DrawWarningBadges(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetContentsBounds();
  int text_x = bounds.x() + kRowPadding + kFaviconSize + kFaviconSpacing;

  int badge_x = text_x;
  int badge_y = bounds.y() + 32;

  if (entry_.is_weak) {
    gfx::Rect badge_rect(badge_x, badge_y, kBadgeSize, kBadgeSize);
    AstraPasswordManagerView::DrawWarningIcon(
        canvas, badge_rect, SkColorSetRGB(200, 80, 80));
    badge_x += kBadgeSize + kBadgeSpacing;
  }

  if (entry_.is_leaked) {
    gfx::Rect badge_rect(badge_x, badge_y, kBadgeSize, kBadgeSize);
    AstraPasswordManagerView::DrawWarningIcon(
        canvas, badge_rect, SkColorSetRGB(200, 80, 80));
    badge_x += kBadgeSize + kBadgeSpacing;
  }

  if (entry_.is_reused) {
    gfx::Rect badge_rect(badge_x, badge_y, kBadgeSize, kBadgeSize);
    AstraPasswordManagerView::DrawLockIcon(
        canvas, badge_rect, SkColorSetRGB(200, 140, 60));
  }
}

// ===========================================================================
// AstraPasswordGroupSection
// ===========================================================================

BEGIN_METADATA(AstraPasswordGroupSection)
END_METADATA

AstraPasswordGroupSection::AstraPasswordGroupSection(
    const AstraPasswordGroup& group_data)
    : group_data_(group_data) {
  Build();
}

AstraPasswordGroupSection::~AstraPasswordGroupSection() = default;

AstraPasswordEntryRow* AstraPasswordGroupSection::GetEntryRow(
    size_t index) const {
  if (index < entry_rows_.size()) {
    return entry_rows_[index];
  }
  return nullptr;
}

gfx::Size AstraPasswordGroupSection::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = available_size.width().value_or(400);
  int height = kLabelHeight +
               entry_rows_.size() * 64 +
               (entry_rows_.size() - 1) * kEntrySpacing;
  return gfx::Size(width, height);
}

void AstraPasswordGroupSection::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int y = bounds.y();

  if (group_label_) {
    group_label_->SetBounds(bounds.x(), y, bounds.width(), kLabelHeight);
    y += kLabelHeight;
  }

  if (entries_container_) {
    entries_container_->SetBounds(bounds.x(), y, bounds.width(),
                                  bounds.height() - kLabelHeight);

    int row_y = 0;
    for (size_t i = 0; i < entry_rows_.size(); ++i) {
      entry_rows_[i]->SetBounds(0, row_y, bounds.width(), 64);
      row_y += 64 + kEntrySpacing;
    }
  }
}

void AstraPasswordGroupSection::OnThemeChanged() {
  views::View::OnThemeChanged();
}

void AstraPasswordGroupSection::Build() {
  group_label_ = AddChildView(std::make_unique<views::Label>(group_data_.label));
  group_label_->SetFontList(
      group_label_->font_list().Derive(0, gfx::Font::NORMAL,
                                       gfx::Font::Weight::BOLD));
  group_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  group_label_->SetEnabledColor(SkColorSetRGB(120, 120, 120));

  entries_container_ = AddChildView(std::make_unique<views::View>());

  for (const auto& entry : group_data_.entries) {
    auto row = std::make_unique<AstraPasswordEntryRow>(entry);
    entry_rows_.push_back(
        entries_container_->AddChildView(std::move(row)));
  }
}

// ===========================================================================
// AstraPasswordSidebarItem
// ===========================================================================

BEGIN_METADATA(AstraPasswordSidebarItem)
END_METADATA

AstraPasswordSidebarItem::AstraPasswordSidebarItem(const std::u16string& label,
                                                   int count,
                                                   bool is_filter)
    : label_(label), count_(count), is_filter_(is_filter) {
  label_view_ = AddChildView(std::make_unique<views::Label>(label_));
  label_view_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  count_view_ = AddChildView(std::make_unique<views::Label>(
      base::NumberToString16(count_)));
  count_view_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  count_view_->SetEnabled(false);
}

AstraPasswordSidebarItem::~AstraPasswordSidebarItem() = default;

void AstraPasswordSidebarItem::SetSelected(bool selected) {
  selected_ = selected;
  SchedulePaint();
}

void AstraPasswordSidebarItem::SetCount(int count) {
  count_ = count;
  count_view_->SetText(base::NumberToString16(count_));
}

void AstraPasswordSidebarItem::SetLabel(const std::u16string& label) {
  label_ = label;
  label_view_->SetText(label_);
}

gfx::Size AstraPasswordSidebarItem::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(available_size.width().value_or(240), kItemHeight);
}

void AstraPasswordSidebarItem::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  label_view_->SetBounds(bounds.x() + kItemPadding,
                         bounds.y(),
                         bounds.width() - kItemPadding * 2 - 40,
                         kItemHeight);
  count_view_->SetBounds(bounds.right() - kItemPadding - 40,
                         bounds.y(),
                         40,
                         kItemHeight);
}

void AstraPasswordSidebarItem::OnThemeChanged() {
  views::View::OnThemeChanged();
}

void AstraPasswordSidebarItem::OnPaint(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetContentsBounds();

  if (selected_) {
    cc::PaintFlags flags;
    flags.setColor(SkColorSetARGB(25, 66, 133, 244));
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawRoundRect(bounds, 6, flags);
  } else if (hovered_) {
    cc::PaintFlags flags;
    flags.setColor(SkColorSetARGB(10, 0, 0, 0));
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawRoundRect(bounds, 6, flags);
  }

  views::View::OnPaint(canvas);
}

bool AstraPasswordSidebarItem::OnMousePressed(const ui::MouseEvent& event) {
  return true;
}

void AstraPasswordSidebarItem::OnMouseReleased(const ui::MouseEvent& event) {
  if (click_callback_) {
    click_callback_.Run();
  }
}

void AstraPasswordSidebarItem::OnMouseEntered(const ui::MouseEvent& event) {
  hovered_ = true;
  SchedulePaint();
}

void AstraPasswordSidebarItem::OnMouseExited(const ui::MouseEvent& event) {
  hovered_ = false;
  SchedulePaint();
}

}  // namespace astra
