// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/vertical_tabs/astra_vertical_tab_strip_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/ui/views/vertical_tabs/astra_vertical_tab_view.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
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
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kDefaultWidth = 240;
constexpr int kCollapsedWidth = 48;
constexpr int kSectionHeaderHeight = 28;
constexpr int kHeaderHeight = 44;
constexpr int kFooterHeight = 40;
constexpr int kIconSize = 16;
constexpr int kPinnedTabSize = 32;
constexpr int kPinnedSectionPadding = 8;

// Draw new tab (+) icon.
void DrawNewTabIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int size = std::min(bounds.width(), bounds.height()) / 2 - 1;

  SkPath path;
  path.moveTo(cx, cy - size);
  path.lineTo(cx, cy + size);
  path.moveTo(cx - size, cy);
  path.lineTo(cx + size, cy);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw collapse/chevron icon.
void DrawCollapseIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) / 3;

  SkPath path;
  path.moveTo(cx + w, cy - w);
  path.lineTo(cx - w * 0.3f, cy);
  path.lineTo(cx + w, cy + w);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw search icon.
void DrawSearchIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.CenterPoint().x() - 1;
  int cy = bounds.CenterPoint().y() - 1;
  int r = std::min(bounds.width(), bounds.height()) / 3;

  SkPath path;
  path.addCircle(cx, cy, r);
  path.moveTo(cx + r * 0.7f, cy + r * 0.7f);
  path.lineTo(cx + r * 1.4f, cy + r * 1.4f);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.3f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

// Draw menu/hamburger icon.
void DrawMenuIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.CenterPoint().x();
  int cy = bounds.CenterPoint().y();
  int w = std::min(bounds.width(), bounds.height()) / 2;

  SkPath path;
  path.moveTo(cx - w, cy - 3);
  path.lineTo(cx + w, cy - 3);
  path.moveTo(cx - w, cy);
  path.lineTo(cx + w, cy);
  path.moveTo(cx - w, cy + 3);
  path.lineTo(cx + w, cy + 3);

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.3f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

}  // namespace

// ===========================================================================
// AstraVerticalTabStripView
// ===========================================================================

AstraVerticalTabStripView::AstraVerticalTabStripView() {
  BuildUI();
}

AstraVerticalTabStripView::~AstraVerticalTabStripView() = default;

void AstraVerticalTabStripView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  SetPreferredSize(gfx::Size(kDefaultWidth, 0));

  // Header.
  BuildHeader();

  // Pinned tabs section.
  BuildPinnedSection();

  // Regular tabs section (scrollable).
  BuildTabList();

  // Footer.
  BuildFooter();
}

void AstraVerticalTabStripView::BuildHeader() {
  header_ = AddChildView(std::make_unique<views::View>());
  header_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(6, 8), 6));
  header_->SetPreferredSize(gfx::Size(0, kHeaderHeight));
  header_->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  header_->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SkColorSetA(SK_ColorBLACK, 0x10)));

  // Search field.
  search_field_ = header_->AddChildView(
      std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"Search tabs");
  search_field_->SetPreferredSize(gfx::Size(0, 30));
  search_field_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
  search_field_->set_controller(
      base::BindRepeating(&AstraVerticalTabStripView::OnSearchChanged,
                          base::Unretained(this)));

  // Collapse button.
  collapse_button_ = header_->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraVerticalTabStripView::OnCollapseClicked,
                              base::Unretained(this))));
  collapse_button_->SetPreferredSize(gfx::Size(kIconSize, kIconSize));
  collapse_button_->SetTooltipText(u"Collapse tab strip");
}

void AstraVerticalTabStripView::BuildPinnedSection() {
  pinned_section_ = AddChildView(std::make_unique<views::View>());
  pinned_section_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(0, 0), 4));

  // Section label.
  pinned_label_ = pinned_section_->AddChildView(
      std::make_unique<views::Label>(u"Pinned"));
  pinned_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  pinned_label_->SetAutoColorReadabilityEnabled(false);
  pinned_label_->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));
  pinned_label_->SetBorder(gfx::Insets::VH(4, kPinnedSectionPadding));

  // Pinned tabs container (grid-like).
  pinned_container_ = pinned_section_->AddChildView(
      std::make_unique<views::View>());
  pinned_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, kPinnedSectionPadding), 4));
  pinned_container_->SetPreferredSize(gfx::Size(0, kPinnedTabSize + 8));
}

void AstraVerticalTabStripView::BuildTabList() {
  tabs_section_ = AddChildView(std::make_unique<views::View>());
  tabs_section_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(8, 0), 0));
  tabs_section_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Section label.
  tabs_label_ = tabs_section_->AddChildView(
      std::make_unique<views::Label>(u"Tabs"));
  tabs_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tabs_label_->SetAutoColorReadabilityEnabled(false);
  tabs_label_->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));
  tabs_label_->SetBorder(gfx::Insets::VH(0, 12, 4, 12));

  // Scroll view for tab list.
  scroll_view_ = tabs_section_->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kBoth,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
  scroll_view_->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);

  tab_list_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  tab_list_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(0, 8), 2));
  tab_list_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraVerticalTabStripView::BuildFooter() {
  footer_ = AddChildView(std::make_unique<views::View>());
  footer_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, 8), 8));
  footer_->SetPreferredSize(gfx::Size(0, kFooterHeight));
  footer_->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  footer_->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SkColorSetA(SK_ColorBLACK, 0x10)));

  // New tab button.
  new_tab_button_ = footer_->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraVerticalTabStripView::OnNewTabClicked,
                              base::Unretained(this))));
  new_tab_button_->SetPreferredSize(gfx::Size(20, 20));
  new_tab_button_->SetTooltipText(u"New tab");

  // "New tab" label.
  auto* new_tab_label = footer_->AddChildView(
      std::make_unique<views::Label>(u"New tab"));
  new_tab_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  new_tab_label->SetAutoColorReadabilityEnabled(false);
  new_tab_label->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Menu button.
  menu_button_ = footer_->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating([]() {})));
  menu_button_->SetPreferredSize(gfx::Size(kIconSize, kIconSize));
  menu_button_->SetTooltipText(u"Tab actions");
}

// -- Tab management -------------------------------------------------------

void AstraVerticalTabStripView::AddTab(const AstraVerticalTabData& tab_data) {
  tabs_.push_back(tab_data);
  RebuildTabViews();
}

void AstraVerticalTabStripView::RemoveTab(const std::string& tab_id) {
  auto it = std::find_if(tabs_.begin(), tabs_.end(),
                         [&tab_id](const AstraVerticalTabData& t) {
                           return t.id == tab_id;
                         });
  if (it == tabs_.end()) return;

  bool was_pinned = it->is_pinned;
  tabs_.erase(it);

  if (was_pinned) {
    RebuildPinnedViews();
  } else {
    RebuildTabViews();
  }

  NotifyTabClosed(tab_id);
}

void AstraVerticalTabStripView::ActivateTab(const std::string& tab_id) {
  bool found = false;
  for (auto& tab : tabs_) {
    if (tab.id == tab_id) {
      tab.is_active = true;
      found = true;
    } else {
      tab.is_active = false;
    }
  }
  if (!found) return;

  // Update all tab views.
  RebuildTabViews();
  NotifyTabActivated(tab_id);
}

void AstraVerticalTabStripView::UpdateTab(const AstraVerticalTabData& tab_data) {
  auto* tab = FindTab(tab_data.id);
  if (!tab) return;

  bool was_pinned = tab->is_pinned;
  *tab = tab_data;

  if (was_pinned != tab_data.is_pinned) {
    RebuildPinnedViews();
    RebuildTabViews();
  } else {
    RebuildTabViews();
  }
}

size_t AstraVerticalTabStripView::GetPinnedTabCount() const {
  size_t count = 0;
  for (const auto& tab : tabs_) {
    if (tab.is_pinned) count++;
  }
  return count;
}

std::string AstraVerticalTabStripView::GetActiveTabId() const {
  for (const auto& tab : tabs_) {
    if (tab.is_active) return tab.id;
  }
  return std::string();
}

// -- Strip state ----------------------------------------------------------

void AstraVerticalTabStripView::SetCollapsed(bool collapsed) {
  if (is_collapsed_ == collapsed) return;
  is_collapsed_ = collapsed;

  // Toggle visibility of text elements.
  search_field_->SetVisible(!collapsed);
  pinned_label_->SetVisible(!collapsed);
  tabs_label_->SetVisible(!collapsed);

  if (footer_) {
    // In collapsed mode, show only icons in footer.
    for (auto* child : footer_->children()) {
      if (auto* label = dynamic_cast<views::Label*>(child)) {
        label->SetVisible(!collapsed);
      }
    }
  }

  SetPreferredSize(
      gfx::Size(collapsed ? kCollapsedWidth : strip_width_, 0));

  NotifyTabStripCollapsed(collapsed);
  InvalidateLayout();
}

void AstraVerticalTabStripView::ToggleCollapsed() {
  SetCollapsed(!is_collapsed_);
}

void AstraVerticalTabStripView::SetStripWidth(int width) {
  strip_width_ = width;
  if (!is_collapsed_) {
    SetPreferredSize(gfx::Size(width, 0));
    InvalidateLayout();
  }
}

int AstraVerticalTabStripView::GetStripWidth() const {
  return is_collapsed_ ? kCollapsedWidth : strip_width_;
}

// -- Populate -------------------------------------------------------------

void AstraVerticalTabStripView::PopulateSampleTabs() {
  tabs_.clear();

  // Pinned tabs.
  const char* pinned_titles[] = {"Gmail", "Calendar", "Drive", "Slack"};
  for (int i = 0; i < 4; i++) {
    AstraVerticalTabData tab;
    tab.id = "tab_pinned_" + base::NumberToString(i);
    tab.title = base::UTF8ToUTF16(pinned_titles[i]);
    tab.url = u"https://" + base::UTF8ToUTF16(
        std::string(pinned_titles[i]) + ".google.com");
    tab.is_pinned = true;
    tab.is_active = (i == 0);
    tab.index = i;
    tabs_.push_back(tab);
  }

  // Regular tabs.
  struct TabDef {
    const char* title;
    const char* url;
    bool audible;
    bool loading;
  };
  TabDef regular_tabs[] = {
      {"Astra Browser — Documentation", "https://astra.dev/docs", false, false},
      {"GitHub - astra-browser/astra", "https://github.com/astra/astra",
       false, false},
      {"Stack Overflow - Where Developers Learn", "https://stackoverflow.com",
       false, false},
      {"YouTube - Music and Videos", "https://www.youtube.com/watch?v=abc123",
       true, false},
      {"The New York Times - Breaking News", "https://www.nytimes.com",
       false, false},
      {"Gmail - Inbox", "https://mail.google.com", false, false},
      {"Google Calendar", "https://calendar.google.com", false, false},
      {"Notion - Workspace", "https://www.notion.so", false, false},
      {"Figma - Design System", "https://www.figma.com/file/design-system",
       false, false},
      {"MDN Web Docs - JavaScript",
       "https://developer.mozilla.org/en-US/docs/Web/JavaScript",
       false, true},
      {"Reddit - r/programming", "https://www.reddit.com/r/programming",
       false, false},
      {"Spotify Web Player", "https://open.spotify.com", true, false},
  };

  for (size_t i = 0; i < std::size(regular_tabs); i++) {
    AstraVerticalTabData tab;
    tab.id = "tab_regular_" + base::NumberToString(i);
    tab.title = base::UTF8ToUTF16(regular_tabs[i].title);
    tab.url = base::UTF8ToUTF16(regular_tabs[i].url);
    tab.is_pinned = false;
    tab.is_audible = regular_tabs[i].audible;
    tab.is_loading = regular_tabs[i].loading;
    tab.is_active = false;
    tab.index = static_cast<int>(i + 4);
    tabs_.push_back(tab);
  }

  RebuildPinnedViews();
  RebuildTabViews();
}

// -- Observer management ---------------------------------------------------

void AstraVerticalTabStripView::AddObserver(
    AstraVerticalTabStripObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraVerticalTabStripView::RemoveObserver(
    AstraVerticalTabStripObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- views::View -----------------------------------------------------------

void AstraVerticalTabStripView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  SkColor fg = cp->GetColor(ui::kColorLabelForeground);
  SkColor secondary = cp->GetColor(ui::kColorLabelForegroundSecondary);
  SkColor icon = cp->GetColor(ui::kColorIcon);
  SkColor bg = cp->GetColor(ui::kColorToolbarBackground);

  // Background.
  SetBackground(views::CreateSolidBackground(bg));

  // Labels.
  pinned_label_->SetEnabledColor(secondary);
  tabs_label_->SetEnabledColor(secondary);

  // Buttons.
  new_tab_button_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(base::BindRepeating(&DrawNewTabIcon),
                            gfx::Size(20, 20), icon));

  collapse_button_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(base::BindRepeating(&DrawCollapseIcon),
                            gfx::Size(kIconSize, kIconSize), icon));

  menu_button_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(base::BindRepeating(&DrawMenuIcon),
                            gfx::Size(kIconSize, kIconSize), icon));

  // Update footer label color.
  for (auto* child : footer_->children()) {
    if (auto* label = dynamic_cast<views::Label*>(child)) {
      label->SetEnabledColor(fg);
    }
  }
}

gfx::Size AstraVerticalTabStripView::CalculatePreferredSize() const {
  int width = is_collapsed_ ? kCollapsedWidth : strip_width_;
  return gfx::Size(width, 0);
}

// -- Private helpers -------------------------------------------------------

void AstraVerticalTabStripView::RebuildTabViews() {
  if (!tab_list_) return;
  tab_list_->RemoveAllChildViews();

  for (const auto& tab : tabs_) {
    if (tab.is_pinned) continue;

    auto* tab_view = tab_list_->AddChildView(
        std::make_unique<AstraVerticalTabView>(
            tab.id, tab.title, gfx::ImageSkia(),
            tab.is_active, tab.is_pinned, tab.is_audible,
            nullptr));
    tab_view->SetLoading(tab.is_loading);
    tab_view->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(
            views::LayoutFlexOrientation::kHorizontal,
            views::MinimumFlexSizeRule::kScaleToMinimum,
            views::MaximumFlexSizeRule::kUnbounded));
  }

  InvalidateLayout();
}

void AstraVerticalTabStripView::RebuildPinnedViews() {
  if (!pinned_container_) return;
  pinned_container_->RemoveAllChildViews();

  for (const auto& tab : tabs_) {
    if (!tab.is_pinned) continue;

    auto* tab_view = pinned_container_->AddChildView(
        std::make_unique<AstraVerticalTabView>(
            tab.id, tab.title, gfx::ImageSkia(),
            tab.is_active, tab.is_pinned, tab.is_audible,
            nullptr));
    tab_view->SetPreferredSize(
        gfx::Size(kPinnedTabSize, kPinnedTabSize));
  }

  // Show/hide pinned section based on whether there are pinned tabs.
  bool has_pinned = GetPinnedTabCount() > 0;
  pinned_section_->SetVisible(has_pinned);

  InvalidateLayout();
}

AstraVerticalTabData* AstraVerticalTabStripView::FindTab(
    const std::string& tab_id) {
  for (auto& tab : tabs_) {
    if (tab.id == tab_id) return &tab;
  }
  return nullptr;
}

// -- Event handlers --------------------------------------------------------

void AstraVerticalTabStripView::OnTabClicked(const std::string& tab_id) {
  ActivateTab(tab_id);
}

void AstraVerticalTabStripView::OnTabClosed(const std::string& tab_id) {
  RemoveTab(tab_id);
}

void AstraVerticalTabStripView::OnNewTabClicked() {
  NotifyNewTabRequested();
}

void AstraVerticalTabStripView::OnCollapseClicked() {
  ToggleCollapsed();
}

void AstraVerticalTabStripView::OnSearchChanged() {
  // TODO(astra): Filter tabs by search query.
  //   Would filter tabs_ list and rebuild views.
}

// -- Notify helpers --------------------------------------------------------

void AstraVerticalTabStripView::NotifyTabActivated(
    const std::string& tab_id) {
  for (auto& observer : observers_) {
    observer.OnTabActivated(tab_id);
  }
}

void AstraVerticalTabStripView::NotifyTabClosed(
    const std::string& tab_id) {
  for (auto& observer : observers_) {
    observer.OnTabClosed(tab_id);
  }
}

void AstraVerticalTabStripView::NotifyNewTabRequested() {
  for (auto& observer : observers_) {
    observer.OnNewTabRequested();
  }
}

void AstraVerticalTabStripView::NotifyTabStripCollapsed(bool collapsed) {
  for (auto& observer : observers_) {
    observer.OnTabStripCollapsed(collapsed);
  }
}

// -- Static icon drawing ---------------------------------------------------

void AstraVerticalTabStripView::DrawNewTabIcon(gfx::Canvas* canvas,
                                               const gfx::Rect& bounds,
                                               SkColor color) {
  ::astra::DrawNewTabIcon(canvas, bounds, color);
}

void AstraVerticalTabStripView::DrawCollapseIcon(gfx::Canvas* canvas,
                                                  const gfx::Rect& bounds,
                                                  SkColor color) {
  ::astra::DrawCollapseIcon(canvas, bounds, color);
}

void AstraVerticalTabStripView::DrawSearchIcon(gfx::Canvas* canvas,
                                                const gfx::Rect& bounds,
                                                SkColor color) {
  ::astra::DrawSearchIcon(canvas, bounds, color);
}

void AstraVerticalTabStripView::DrawMenuIcon(gfx::Canvas* canvas,
                                              const gfx::Rect& bounds,
                                              SkColor color) {
  ::astra::DrawMenuIcon(canvas, bounds, color);
}

}  // namespace astra
