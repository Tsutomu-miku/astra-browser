// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_BOOKMARKS_MANAGER_ASTRA_BOOKMARKS_MANAGER_VIEW_H_
#define ASTRA_UI_VIEWS_BOOKMARKS_MANAGER_ASTRA_BOOKMARKS_MANAGER_VIEW_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"
#include "astra/ui/views/bookmarks_manager/astra_bookmarks_manager_model.h"

namespace views {
class BoxLayout;
class FlexLayout;
class ImageButton;
class Label;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

// Display mode for bookmark items.
enum class AstraBookmarksDisplayMode {
  kGrid,
  kList,
};

// A single bookmark card / list item view.
class AstraBookmarkItemView : public views::View {
 public:
  METADATA_HEADER(AstraBookmarkItemView);

  explicit AstraBookmarkItemView(const AstraBookmarkEntry& entry);
  ~AstraBookmarkItemView() override;

  AstraBookmarkItemView(const AstraBookmarkItemView&) = delete;
  AstraBookmarkItemView& operator=(const AstraBookmarkItemView&) = delete;

  // Update the bookmark data displayed.
  void Update(const AstraBookmarkEntry& entry);

  AstraBookmarkId bookmark_id() const { return entry_.id; }
  const std::u16string& title() const { return entry_.title; }
  const std::string& url() const { return entry_.url; }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;

  void SetDisplayMode(AstraBookmarksDisplayMode mode);

 private:
  void Build();
  void DrawFavicon(gfx::Canvas* canvas, const gfx::Rect& bounds);
  void DrawMoreIcon(gfx::Canvas* canvas, const gfx::Rect& bounds, SkColor color);

  AstraBookmarkEntry entry_;
  AstraBookmarksDisplayMode display_mode_ = AstraBookmarksDisplayMode::kGrid;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> url_label_ = nullptr;
  raw_ptr<views::ImageButton> more_button_ = nullptr;
};

// A folder tree item view for the left sidebar.
class AstraBookmarkFolderItemView : public views::View {
 public:
  METADATA_HEADER(AstraBookmarkFolderItemView);

  AstraBookmarkFolderItemView(const AstraBookmarkFolder& folder, int depth);
  ~AstraBookmarkFolderItemView() override;

  AstraBookmarkFolderItemView(const AstraBookmarkFolderItemView&) = delete;
  AstraBookmarkFolderItemView& operator=(const AstraBookmarkFolderItemView&) =
      delete;

  void Update(const AstraBookmarkFolder& folder);
  void SetExpanded(bool expanded);
  bool expanded() const { return expanded_; }

  AstraBookmarkId folder_id() const { return folder_id_; }
  int depth() const { return depth_; }

  // Callback when the expand/collapse arrow is clicked.
  using ExpandCallback = base::RepeatingCallback<void(AstraBookmarkId)>;
  void SetExpandCallback(ExpandCallback callback) {
    expand_callback_ = std::move(callback);
  }

  // Callback when the folder item is selected.
  using SelectCallback = base::RepeatingCallback<void(AstraBookmarkId)>;
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
  void OnExpandClicked();
  void DrawFolderIcon(gfx::Canvas* canvas, const gfx::Rect& bounds, SkColor color);
  void DrawChevron(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color,
                   bool expanded);

  AstraBookmarkId folder_id_ = 0;
  std::u16string title_;
  int total_bookmarks_ = 0;
  int depth_ = 0;
  bool expanded_ = true;
  bool selected_ = false;

  ExpandCallback expand_callback_;
  SelectCallback select_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::ImageButton> expand_button_ = nullptr;
};

// The full bookmarks manager page view.
//
// Layout:
//   +-------------------------------------------+
//   |  Search bar | Add bookmark | Sort | View  |  <- toolbar
//   +-----------------+-------------------------+
//   |                 |                         |
//   |  Folder tree    |   Bookmark grid / list  |  <- main content
//   |   (sidebar)     |                         |
//   |                 |                         |
//   +-----------------+-------------------------+
//   |  Status bar showing bookmark count         |  <- bottom bar
//   +-------------------------------------------+
//
// Chromium owner: Bookmarks UI (chrome/browser/ui/webui/bookmarks/)
//   This is a Views-based alternative to the WebUI bookmarks manager.
//
// TODO(astra): Wire up to Chromium's BookmarkModel.  Patch point:
// chrome/browser/ui/bookmarks/bookmark_tab_helper.cc or
// chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc.
class AstraBookmarksManagerView
    : public views::View,
      public AstraBookmarksManagerObserver,
      public views::TextfieldController {
 public:
  METADATA_HEADER(AstraBookmarksManagerView);

  AstraBookmarksManagerView();
  explicit AstraBookmarksManagerView(AstraBookmarksManagerModel* model);
  ~AstraBookmarksManagerView() override;

  AstraBookmarksManagerView(const AstraBookmarksManagerView&) = delete;
  AstraBookmarksManagerView& operator=(const AstraBookmarksManagerView&) =
      delete;

  // Set the model to observe.
  void SetModel(AstraBookmarksManagerModel* model);
  AstraBookmarksManagerModel* model() const { return model_; }

  // -- Display mode ---------------------------------------------------------

  void SetDisplayMode(AstraBookmarksDisplayMode mode);
  AstraBookmarksDisplayMode display_mode() const { return display_mode_; }

  // -- Folder selection -----------------------------------------------------

  void SetSelectedFolder(AstraBookmarkId folder_id);
  AstraBookmarkId selected_folder() const { return selected_folder_id_; }

  // -- AstraBookmarksManagerObserver: ---------------------------------------

  void OnBookmarksModelChanged() override;
  void OnBookmarkAdded(AstraBookmarkId id) override;
  void OnBookmarkRemoved(AstraBookmarkId id) override;
  void OnBookmarkChanged(AstraBookmarkId id) override;
  void OnFolderExpanded(AstraBookmarkId folder_id) override;
  void OnSearchChanged(const std::u16string& query) override;
  void OnBookmarksManagerModelShutdown() override;

  // -- views::View: --------------------------------------------------------

  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

  // -- TextfieldController: ------------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

  // Accessors for testing.
  views::Textfield* search_field_for_test() { return search_field_; }
  views::ImageButton* add_button_for_test() { return add_button_; }
  views::ImageButton* grid_view_button_for_test() { return grid_view_button_; }
  views::ImageButton* list_view_button_for_test() { return list_view_button_; }
  views::View* sidebar_for_test() { return sidebar_container_; }
  views::ScrollView* content_scroll_for_test() { return content_scroll_; }
  views::Label* status_label_for_test() { return status_label_; }
  size_t bookmark_item_count_for_test() const { return bookmark_items_.size(); }

 private:
  // Build the entire UI.
  void Build();

  // Build the top toolbar.
  void BuildToolbar();

  // Build the sidebar (folder tree).
  void BuildSidebar();

  // Build the main content area.
  void BuildContent();

  // Build the status bar.
  void BuildStatusBar();

  // Rebuild the folder tree from the model.
  void RebuildFolderTree();

  // Recursively add folder items to the sidebar.
  void AddFolderItemsRecursive(const AstraBookmarkFolder& folder,
                               int depth,
                               views::View* container);

  // Rebuild the bookmark grid/list from the model / current selection.
  void RebuildBookmarkContent();

  // Update the status bar text.
  void UpdateStatusBar();

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
  void OnAddBookmarkClicked();
  void OnGridViewClicked();
  void OnListViewClicked();
  void OnSortClicked();
  void OnFolderExpand(AstraBookmarkId folder_id);
  void OnFolderSelect(AstraBookmarkId folder_id);

  // Get the currently displayed folder (for folder-based view).
  const AstraBookmarkFolder* GetDisplayedFolder() const;

  // Get bookmarks to display (accounting for search and filters).
  std::vector<AstraBookmarkEntry> GetDisplayedBookmarks() const;

  // Model.
  raw_ptr<AstraBookmarksManagerModel> model_ = nullptr;
  base::ScopedObservation<AstraBookmarksManagerModel,
                          AstraBookmarksManagerObserver>
      model_observation_{this};

  // Display state.
  AstraBookmarksDisplayMode display_mode_ = AstraBookmarksDisplayMode::kGrid;
  AstraBookmarkId selected_folder_id_ = 0;  // 0 = showing bar + other + mobile

  // Child views.
  raw_ptr<views::View> toolbar_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ImageButton> add_button_ = nullptr;
  raw_ptr<views::ImageButton> sort_button_ = nullptr;
  raw_ptr<views::ImageButton> grid_view_button_ = nullptr;
  raw_ptr<views::ImageButton> list_view_button_ = nullptr;

  raw_ptr<views::View> sidebar_container_ = nullptr;
  raw_ptr<views::ScrollView> sidebar_scroll_ = nullptr;
  raw_ptr<views::View> folder_tree_ = nullptr;

  raw_ptr<views::ScrollView> content_scroll_ = nullptr;
  raw_ptr<views::View> content_container_ = nullptr;
  raw_ptr<views::View> empty_state_view_ = nullptr;

  raw_ptr<views::View> status_bar_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;

  // Owned bookmark item views (kept as raw_ptr list for easy access).
  std::vector<raw_ptr<AstraBookmarkItemView, VectorExperimental>>
      bookmark_items_;

  // Owned folder item views.
  std::vector<raw_ptr<AstraBookmarkFolderItemView, VectorExperimental>>
      folder_items_;

  // Layout constants.
  static constexpr int kToolbarHeight = 52;
  static constexpr int kSidebarWidth = 240;
  static constexpr int kStatusBarHeight = 28;
  static constexpr int kSidePadding = 16;
  static constexpr int kToolbarSpacing = 8;
  static constexpr int kBookmarkCardWidth = 200;
  static constexpr int kBookmarkCardHeight = 96;
  static constexpr int kBookmarkCardSpacing = 12;
  static constexpr int kSearchFieldWidth = 280;
  static constexpr int kButtonSize = 32;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_BOOKMARKS_MANAGER_ASTRA_BOOKMARKS_MANAGER_VIEW_H_
