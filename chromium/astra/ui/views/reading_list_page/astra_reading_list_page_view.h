// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_READING_LIST_PAGE_ASTRA_READING_LIST_PAGE_VIEW_H_
#define ASTRA_UI_VIEWS_READING_LIST_PAGE_ASTRA_READING_LIST_PAGE_VIEW_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"
#include "astra/ui/views/reading_list_page/astra_reading_list_model.h"

namespace views {
class BoxLayout;
class FlexLayout;
class ImageButton;
class Label;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

// Display mode for reading list items.
enum class AstraReadingListDisplayMode {
  kList,
  kGrid,
};

// A single reading list item view (used in both list and grid mode).
class AstraReadingListItemView : public views::View {
 public:
  METADATA_HEADER(AstraReadingListItemView);

  explicit AstraReadingListItemView(const AstraReadingListEntry& entry);
  ~AstraReadingListItemView() override;

  AstraReadingListItemView(const AstraReadingListItemView&) = delete;
  AstraReadingListItemView& operator=(const AstraReadingListItemView&) = delete;

  // Update the entry data displayed.
  void Update(const AstraReadingListEntry& entry);

  const std::string& entry_id() const { return entry_.id; }
  const std::u16string& title() const { return entry_.title; }

  // Set the display mode (list or grid).
  void SetDisplayMode(AstraReadingListDisplayMode mode);
  AstraReadingListDisplayMode display_mode() const { return display_mode_; }

  // Set whether this item is selected.
  void SetSelected(bool selected);
  bool selected() const { return selected_; }

  // Callback when the item is clicked.
  using ClickCallback = base::RepeatingCallback<void(const std::string& id)>;
  void SetClickCallback(ClickCallback callback) {
    click_callback_ = std::move(callback);
  }

  // Callback when the favorite button is clicked.
  using FavoriteCallback =
      base::RepeatingCallback<void(const std::string& id)>;
  void SetFavoriteCallback(FavoriteCallback callback) {
    favorite_callback_ = std::move(callback);
  }

  // Callback when the more button is clicked.
  using MoreCallback = base::RepeatingCallback<void(const std::string& id)>;
  void SetMoreCallback(MoreCallback callback) {
    more_callback_ = std::move(callback);
  }

  // Layout constants (public for use by parent layout).
  static constexpr int kListItemHeight = 120;
  static constexpr int kGridItemWidth = 280;
  static constexpr int kGridItemHeight = 200;

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnThemeChanged() override;

 private:
  void Build();
  void DrawFavicon(gfx::Canvas* canvas, const gfx::Rect& bounds);
  void DrawStar(gfx::Canvas* canvas, const gfx::Rect& bounds, SkColor color);
  void DrawClock(gfx::Canvas* canvas, const gfx::Rect& bounds, SkColor color);
  void DrawMore(gfx::Canvas* canvas, const gfx::Rect& bounds, SkColor color);
  void DrawBookmarkIcon(gfx::Canvas* canvas,
                        const gfx::Rect& bounds,
                        SkColor color);

  AstraReadingListEntry entry_;
  AstraReadingListDisplayMode display_mode_ = AstraReadingListDisplayMode::kList;
  bool selected_ = false;

  ClickCallback click_callback_;
  FavoriteCallback favorite_callback_;
  MoreCallback more_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> site_label_ = nullptr;
  raw_ptr<views::Label> preview_label_ = nullptr;
  raw_ptr<views::Label> read_time_label_ = nullptr;
  raw_ptr<views::Label> date_label_ = nullptr;
  raw_ptr<views::ImageButton> favorite_button_ = nullptr;
  raw_ptr<views::ImageButton> more_button_ = nullptr;
};

// A sidebar filter item view (for filter options like All, Unread, Favorites).
class AstraReadingListFilterItemView : public views::View {
 public:
  METADATA_HEADER(AstraReadingListFilterItemView);

  AstraReadingListFilterItemView(const std::u16string& label,
                                 int count,
                                 bool is_selected = false);
  ~AstraReadingListFilterItemView() override;

  AstraReadingListFilterItemView(const AstraReadingListFilterItemView&) =
      delete;
  AstraReadingListFilterItemView& operator=(
      const AstraReadingListFilterItemView&) = delete;

  void SetSelected(bool selected);
  bool selected() const { return selected_; }

  void SetCount(int count);

  using SelectCallback = base::RepeatingClosure;
  void SetSelectCallback(SelectCallback callback) {
    select_callback_ = std::move(callback);
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnThemeChanged() override;

 private:
  void DrawBookmarkIcon(gfx::Canvas* canvas,
                        const gfx::Rect& bounds,
                        SkColor color);

  std::u16string label_;
  int count_ = 0;
  bool selected_ = false;

  SelectCallback select_callback_;

  raw_ptr<views::Label> label_view_ = nullptr;
  raw_ptr<views::Label> count_view_ = nullptr;

  static constexpr int kItemHeight = 36;
  static constexpr int kIconSize = 18;
  static constexpr int kIconPadding = 12;
};

// A folder sidebar item view.
class AstraReadingListFolderItemView : public views::View {
 public:
  METADATA_HEADER(AstraReadingListFolderItemView);

  AstraReadingListFolderItemView(const AstraReadingListFolder& folder,
                                 bool is_selected = false);
  ~AstraReadingListFolderItemView() override;

  AstraReadingListFolderItemView(const AstraReadingListFolderItemView&) =
      delete;
  AstraReadingListFolderItemView& operator=(
      const AstraReadingListFolderItemView&) = delete;

  void Update(const AstraReadingListFolder& folder);
  void SetSelected(bool selected);
  bool selected() const { return selected_; }

  const std::string& folder_id() const { return folder_id_; }

  using SelectCallback = base::RepeatingCallback<void(const std::string& id)>;
  void SetSelectCallback(SelectCallback callback) {
    select_callback_ = std::move(callback);
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnThemeChanged() override;

 private:
  void DrawFolderIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color);

  std::string folder_id_;
  std::u16string name_;
  int entry_count_ = 0;
  bool selected_ = false;

  SelectCallback select_callback_;

  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;

  static constexpr int kItemHeight = 32;
  static constexpr int kIconSize = 16;
  static constexpr int kIconPadding = 12;
};

// The detail panel view (right side, shows selected entry details).
class AstraReadingListDetailView : public views::View {
 public:
  METADATA_HEADER(AstraReadingListDetailView);

  AstraReadingListDetailView();
  ~AstraReadingListDetailView() override;

  AstraReadingListDetailView(const AstraReadingListDetailView&) = delete;
  AstraReadingListDetailView& operator=(const AstraReadingListDetailView&) =
      delete;

  void SetEntry(const AstraReadingListEntry* entry);
  void Clear();

  // Callbacks.
  using MarkReadCallback = base::RepeatingClosure;
  void SetMarkReadCallback(MarkReadCallback callback) {
    mark_read_callback_ = std::move(callback);
  }

  using RemoveCallback = base::RepeatingClosure;
  void SetRemoveCallback(RemoveCallback callback) {
    remove_callback_ = std::move(callback);
  }

  using ShareCallback = base::RepeatingClosure;
  void SetShareCallback(ShareCallback callback) {
    share_callback_ = std::move(callback);
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;

 private:
  void Build();
  void UpdateContent();
  void DrawBookmarkFilled(gfx::Canvas* canvas,
                          const gfx::Rect& bounds,
                          SkColor color);
  void DrawTrashIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color);
  void DrawShareIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color);
  void DrawClockIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color);

  const AstraReadingListEntry* entry_ = nullptr;

  MarkReadCallback mark_read_callback_;
  RemoveCallback remove_callback_;
  ShareCallback share_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> site_label_ = nullptr;
  raw_ptr<views::Label> preview_label_ = nullptr;
  raw_ptr<views::Label> read_time_label_ = nullptr;
  raw_ptr<views::Label> date_label_ = nullptr;
  raw_ptr<views::Label> category_label_ = nullptr;
  raw_ptr<views::Label> empty_label_ = nullptr;
  raw_ptr<views::View> content_container_ = nullptr;
  raw_ptr<views::ImageButton> mark_read_button_ = nullptr;
  raw_ptr<views::ImageButton> remove_button_ = nullptr;
  raw_ptr<views::ImageButton> share_button_ = nullptr;

  static constexpr int kPanelWidth = 320;
  static constexpr int kButtonSize = 32;
};

// Empty state view for the reading list.
class AstraReadingListEmptyView : public views::View {
 public:
  METADATA_HEADER(AstraReadingListEmptyView);

  AstraReadingListEmptyView();
  ~AstraReadingListEmptyView() override;

  AstraReadingListEmptyView(const AstraReadingListEmptyView&) = delete;
  AstraReadingListEmptyView& operator=(const AstraReadingListEmptyView&) =
      delete;

  void SetIsSearchEmpty(bool is_search_empty);

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;

 private:
  void DrawEmptyIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color);

  bool is_search_empty_ = false;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> subtitle_label_ = nullptr;
};

// The full reading list page view.
//
// Layout:
//   +----------------------------------------------------------------+
//   |  Search | Sort | View Toggle | Add to Reading List             |  <- toolbar
//   +-------------------+------------------------+-------------------+
//   |                   |                        |                   |
//   |  Sidebar          |   Reading list items   |   Detail panel    |
//   |  (filters +       |   (list or grid view)  |   (selected      |
//   |   folders)        |                        |    entry details) |
//   |                   |                        |                   |
//   +-------------------+------------------------+-------------------+
//
// Chromium owner: Reading List UI (chrome/browser/ui/webui/side_panel/reading_list/)
//   This is a Views-based alternative to the WebUI reading list.
//
// TODO(astra): Wire up to Chromium's ReadingListModel.  Patch point:
// chrome/browser/reading_list/reading_list_model_factory.cc or
// chrome/browser/ui/webui/side_panel/reading_list/reading_list_page_handler.cc.
class AstraReadingListPageView
    : public views::View,
      public AstraReadingListObserver,
      public views::TextfieldController {
 public:
  METADATA_HEADER(AstraReadingListPageView);

  AstraReadingListPageView();
  explicit AstraReadingListPageView(AstraReadingListModel* model);
  ~AstraReadingListPageView() override;

  AstraReadingListPageView(const AstraReadingListPageView&) = delete;
  AstraReadingListPageView& operator=(const AstraReadingListPageView&) = delete;

  // Set the model to observe.
  void SetModel(AstraReadingListModel* model);
  AstraReadingListModel* model() const { return model_; }

  // -- Display mode ---------------------------------------------------------

  void SetDisplayMode(AstraReadingListDisplayMode mode);
  AstraReadingListDisplayMode display_mode() const { return display_mode_; }

  // -- Selection ------------------------------------------------------------

  void SetSelectedEntry(const std::string& id);
  const std::string& selected_entry_id() const { return selected_entry_id_; }

  // -- AstraReadingListObserver: --------------------------------------------

  void OnReadingListChanged() override;
  void OnEntryAdded(const std::string& id) override;
  void OnEntryRemoved(const std::string& id) override;
  void OnEntryUpdated(const std::string& id) override;
  void OnFolderAdded(const std::string& id) override;
  void OnFolderRemoved(const std::string& id) override;
  void OnSearchChanged(const std::u16string& query) override;
  void OnFilterChanged() override;
  void OnReadingListModelShutdown() override;

  // -- views::View: ---------------------------------------------------------

  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

  // -- TextfieldController: -------------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

  // Accessors for testing.
  views::Textfield* search_field_for_test() { return search_field_; }
  views::ImageButton* add_button_for_test() { return add_button_; }
  views::ImageButton* sort_button_for_test() { return sort_button_; }
  views::ImageButton* list_view_button_for_test() { return list_view_button_; }
  views::ImageButton* grid_view_button_for_test() { return grid_view_button_; }
  views::View* sidebar_for_test() { return sidebar_container_; }
  views::ScrollView* content_scroll_for_test() { return content_scroll_; }
  views::View* detail_panel_for_test() { return detail_panel_; }
  size_t list_item_count_for_test() const { return list_items_.size(); }
  size_t folder_item_count_for_test() const { return folder_items_.size(); }
  size_t filter_item_count_for_test() const { return filter_items_.size(); }
  AstraReadingListEmptyView* empty_view_for_test() { return empty_view_; }

 private:
  // Build the entire UI.
  void Build();

  // Build the top toolbar.
  void BuildToolbar();

  // Build the sidebar (filters + folders).
  void BuildSidebar(views::View* parent);

  // Build the main content area.
  void BuildContent(views::View* parent);

  // Build the detail panel.
  void BuildDetailPanel(views::View* parent);

  // Rebuild the sidebar filter and folder lists from the model.
  void RebuildSidebar();

  // Rebuild the reading list items from the model.
  void RebuildListContent();

  // Update the detail panel for the current selection.
  void UpdateDetailPanel();

  // Draw custom icon helpers.
  void DrawSearchIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color);
  void DrawAddIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color);
  void DrawGridIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);
  void DrawListIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);
  void DrawSortIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);

  // Button handlers.
  void OnAddEntryClicked();
  void OnSortClicked();
  void OnListViewClicked();
  void OnGridViewClicked();
  void OnEntryClicked(const std::string& id);
  void OnEntryFavorite(const std::string& id);
  void OnEntryMore(const std::string& id);
  void OnFilterSelected(AstraReadingListFilter filter);
  void OnFolderSelected(const std::string& folder_id);
  void OnDetailMarkRead();
  void OnDetailRemove();
  void OnDetailShare();

  // Get the currently displayed entries (accounting for search and filters).
  std::vector<AstraReadingListEntry> GetDisplayedEntries() const;

  // Model.
  raw_ptr<AstraReadingListModel> model_ = nullptr;
  base::ScopedObservation<AstraReadingListModel, AstraReadingListObserver>
      model_observation_{this};

  // Display state.
  AstraReadingListDisplayMode display_mode_ =
      AstraReadingListDisplayMode::kList;
  std::string selected_entry_id_;
  AstraReadingListFilter current_sidebar_filter_ =
      AstraReadingListFilter::kAll;
  std::string selected_folder_id_;

  // Child views.
  raw_ptr<views::View> toolbar_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ImageButton> add_button_ = nullptr;
  raw_ptr<views::ImageButton> sort_button_ = nullptr;
  raw_ptr<views::ImageButton> list_view_button_ = nullptr;
  raw_ptr<views::ImageButton> grid_view_button_ = nullptr;

  raw_ptr<views::View> sidebar_container_ = nullptr;
  raw_ptr<views::ScrollView> sidebar_scroll_ = nullptr;
  raw_ptr<views::View> filter_list_ = nullptr;
  raw_ptr<views::View> folder_list_ = nullptr;
  raw_ptr<views::Label> folders_header_label_ = nullptr;

  raw_ptr<views::ScrollView> content_scroll_ = nullptr;
  raw_ptr<views::View> content_container_ = nullptr;
  raw_ptr<AstraReadingListEmptyView> empty_view_ = nullptr;

  raw_ptr<AstraReadingListDetailView> detail_panel_ = nullptr;

  // Owned item views.
  std::vector<raw_ptr<AstraReadingListItemView, VectorExperimental>> list_items_;
  std::vector<raw_ptr<AstraReadingListFilterItemView, VectorExperimental>>
      filter_items_;
  std::vector<raw_ptr<AstraReadingListFolderItemView, VectorExperimental>>
      folder_items_;

  // Layout constants.
  static constexpr int kToolbarHeight = 52;
  static constexpr int kSidebarWidth = 220;
  static constexpr int kDetailPanelWidth = 320;
  static constexpr int kContentPadding = 16;
  static constexpr int kToolbarSpacing = 8;
  static constexpr int kSearchFieldWidth = 280;
  static constexpr int kButtonSize = 32;
  static constexpr int kListItemSpacing = 8;
  static constexpr int kGridItemSpacing = 12;
  static constexpr int kSidebarSectionSpacing = 8;
  static constexpr int kSidebarSectionHeaderHeight = 32;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_READING_LIST_PAGE_ASTRA_READING_LIST_PAGE_VIEW_H_
