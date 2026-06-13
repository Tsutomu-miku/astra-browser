// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_HISTORY_PAGE_ASTRA_HISTORY_PAGE_VIEW_H_
#define ASTRA_UI_VIEWS_HISTORY_PAGE_ASTRA_HISTORY_PAGE_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

#include "astra/ui/views/history_page/astra_history_page_model.h"

namespace views {
class BoxLayout;
class FlexLayout;
class ImageButton;
class ImageView;
class Label;
class Textfield;
class ScrollView;
}  // namespace views

namespace astra {

class AstraHistoryEntryRow;
class AstraHistoryDaySection;

// Delegate for history page view interactions.
class AstraHistoryPageDelegate {
 public:
  virtual ~AstraHistoryPageDelegate() = default;

  // Called when a history entry is clicked (open URL).
  virtual void OnHistoryEntryClicked(const std::string& id) = 0;

  // Called when the bookmark star for an entry is toggled.
  virtual void OnBookmarkToggled(const std::string& id, bool bookmarked) = 0;

  // Called when "Clear browsing data" is requested.
  virtual void OnClearBrowsingData() = 0;

  // Called when a search query is typed.
  virtual void OnSearchQueryChanged(const std::u16string& query) = 0;

  // Called when the time filter is changed.
  virtual void OnFilterChanged(AstraHistoryFilter filter) = 0;

  // Called when a category filter is selected.
  virtual void OnCategoryFilterChanged(const std::string& category) = 0;

  // Called when a history entry is requested to be removed.
  virtual void OnRemoveEntry(const std::string& id) = 0;
};

// The full-page history view.
//
// Displays browsing history grouped by day, with search, time filter,
// and category filter controls in the header.  Each entry row shows a
// favicon, title, URL, time, bookmark button, and more-actions button.
//
// This is a Views-based alternative to Chromium's history WebUI page.
//
// Chromium owner: HistoryUI / BrowsingDataHandler
//   (chrome/browser/ui/webui/history/history_ui.cc)
//
// TODO(astra): Integrate with Chromium's HistoryService via a
// KeyedService.  Patch point:
// chrome/browser/ui/webui/history/history_ui.cc — replace WebUI with
// this Views page, or embed it in a constrained WebUI container.
class AstraHistoryPageView : public views::View,
                             public AstraHistoryPageObserver,
                             public views::TextfieldController {
 public:
  METADATA_HEADER(AstraHistoryPageView);

  AstraHistoryPageView();
  explicit AstraHistoryPageView(AstraHistoryPageModel* model);
  AstraHistoryPageView(const AstraHistoryPageView&) = delete;
  AstraHistoryPageView& operator=(const AstraHistoryPageView&) = delete;
  ~AstraHistoryPageView() override;

  // -- Model binding --------------------------------------------------------

  void SetModel(AstraHistoryPageModel* model);
  AstraHistoryPageModel* model() const { return model_; }

  // -- Delegate -------------------------------------------------------------

  void SetDelegate(AstraHistoryPageDelegate* delegate) { delegate_ = delegate; }
  AstraHistoryPageDelegate* delegate() const { return delegate_; }

  // -- Header controls ------------------------------------------------------

  views::Textfield* search_field() { return search_field_; }
  views::ImageButton* clear_data_button() { return clear_data_button_; }

  // -- Body access ----------------------------------------------------------

  views::ScrollView* scroll_view() { return scroll_view_; }
  views::View* content_view() { return content_view_; }

  // -- Refresh --------------------------------------------------------------

  // Rebuild the entire history list from the model.
  void RefreshFromModel();

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;

  // -- AstraHistoryPageObserver: -------------------------------------------

  void OnHistoryChanged(AstraHistoryPageModel* model) override;
  void OnFilterChanged(AstraHistoryPageModel* model,
                       AstraHistoryFilter filter) override;
  void OnSearchChanged(AstraHistoryPageModel* model,
                       const std::u16string& query) override;
  void OnHistoryEntryRemoved(AstraHistoryPageModel* model,
                             const std::string& id) override;
  void OnHistoryPageModelShutdown(AstraHistoryPageModel* model) override;

  // -- views::TextfieldController: -----------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

 private:
  // Build the header section (search, filters, clear button).
  void BuildHeader();

  // Build the body section (scrollable list of days).
  void BuildBody();

  // Build category filter chips.
  void BuildCategoryChips();

  // Update the filter dropdown label.
  void UpdateFilterLabel();

  // Update category chips from the model.
  void RefreshCategoryChips();

  // Rebuild the day sections from the model's filtered data.
  void RebuildDaySections();

  // Show or hide the empty state based on whether there are results.
  void UpdateEmptyState();

  // -- Event handlers -------------------------------------------------------

  void OnClearDataClicked();
  void OnFilterButtonClicked();
  void OnCategoryChipClicked(const std::string& category);

  // -- Custom icon painting -------------------------------------------------

  static void DrawSearchIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawFilterIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawBookmarkStar(gfx::Canvas* canvas,
                               const gfx::Rect& bounds,
                               SkColor color,
                               bool filled);
  static void DrawMoreIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color);
  static void DrawTrashIcon(gfx::Canvas* canvas,
                            const gfx::Rect& bounds,
                            SkColor color);

  raw_ptr<AstraHistoryPageModel> model_ = nullptr;
  raw_ptr<AstraHistoryPageDelegate> delegate_ = nullptr;

  base::ScopedObservation<AstraHistoryPageModel, AstraHistoryPageObserver>
      model_observation_{this};

  // Header views.
  raw_ptr<views::View> header_container_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ImageView> search_icon_ = nullptr;
  raw_ptr<views::LabelButton> filter_button_ = nullptr;
  raw_ptr<views::View> category_chips_container_ = nullptr;
  raw_ptr<views::LabelButton> clear_data_button_ = nullptr;

  // Body views.
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> content_view_ = nullptr;
  raw_ptr<views::View> empty_state_view_ = nullptr;

  // Day sections (owned by the view hierarchy via content_view_).
  std::vector<raw_ptr<AstraHistoryDaySection>> day_sections_;

  // Selected category chip (for visual state).
  std::string selected_category_;

  // Constants.
  static constexpr int kHeaderHeight = 120;
  static constexpr int kSearchFieldHeight = 40;
  static constexpr int kSidePadding = 24;
  static constexpr int kHeaderTitleSize = 24;
  static constexpr int kRowHeight = 56;
  static constexpr int kDaySectionSpacing = 24;
};

// A single day section in the history list.
class AstraHistoryDaySection : public views::View {
 public:
  METADATA_HEADER(AstraHistoryDaySection);

  explicit AstraHistoryDaySection(const AstraHistoryDay& day_data);
  AstraHistoryDaySection(const AstraHistoryDaySection&) = delete;
  AstraHistoryDaySection& operator=(const AstraHistoryDaySection&) = delete;
  ~AstraHistoryDaySection() override;

  // Get the number of entry rows in this section.
  size_t GetEntryCount() const { return entry_rows_.size(); }

  // Get a specific entry row by index.
  AstraHistoryEntryRow* GetEntryRow(size_t index) const;

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;

 private:
  void Build();

  raw_ptr<views::Label> date_label_ = nullptr;
  raw_ptr<views::Label> visit_count_label_ = nullptr;
  raw_ptr<views::View> entries_container_ = nullptr;

  std::vector<raw_ptr<AstraHistoryEntryRow>> entry_rows_;

  AstraHistoryDay day_data_;

  static constexpr int kDateLabelHeight = 32;
  static constexpr int kEntrySpacing = 4;
};

// A single history entry row.
class AstraHistoryEntryRow : public views::View {
 public:
  METADATA_HEADER(AstraHistoryEntryRow);

  explicit AstraHistoryEntryRow(const AstraHistoryEntry& entry);
  AstraHistoryEntryRow(const AstraHistoryEntryRow&) = delete;
  AstraHistoryEntryRow& operator=(const AstraHistoryEntryRow&) = delete;
  ~AstraHistoryEntryRow() override;

  // Accessors.
  const std::string& entry_id() const { return entry_.id; }
  const AstraHistoryEntry& entry() const { return entry_; }

  views::ImageButton* bookmark_button() { return bookmark_button_; }
  views::ImageButton* more_button() { return more_button_; }

  // Update the entry data and refresh display.
  void SetEntry(const AstraHistoryEntry& entry);

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Format the visit time as a short time string (e.g. "2:30 PM").
  std::u16string FormatVisitTime() const;

  // Draw the favicon placeholder (a colored circle with the first letter).
  void DrawFaviconPlaceholder(gfx::Canvas* canvas);

  AstraHistoryEntry entry_;

  raw_ptr<views::ImageView> favicon_view_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> url_label_ = nullptr;
  raw_ptr<views::Label> time_label_ = nullptr;
  raw_ptr<views::ImageButton> bookmark_button_ = nullptr;
  raw_ptr<views::ImageButton> more_button_ = nullptr;

  bool hovered_ = false;

  static constexpr int kFaviconSize = 20;
  static constexpr int kFaviconSpacing = 16;
  static constexpr int kTitleFontSize = 14;
  static constexpr int kUrlFontSize = 13;
  static constexpr int kButtonSize = 28;
  static constexpr int kButtonSpacing = 4;
  static constexpr int kRowPadding = 12;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_HISTORY_PAGE_ASTRA_HISTORY_PAGE_VIEW_H_
