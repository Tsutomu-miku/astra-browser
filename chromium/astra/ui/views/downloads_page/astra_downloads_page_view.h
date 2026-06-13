// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_DOWNLOADS_PAGE_ASTRA_DOWNLOADS_PAGE_VIEW_H_
#define ASTRA_UI_VIEWS_DOWNLOADS_PAGE_ASTRA_DOWNLOADS_PAGE_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"

#include "astra/ui/views/downloads_page/astra_downloads_page_model.h"

namespace views {
class BoxLayout;
class FlexLayout;
class ImageView;
class Label;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraDownloadsPageView — full downloads page view
// =========================================================================
//
// The main downloads page view, presenting a full-page list of all downloads
// with search, filtering by state and category, and per-download actions.
//
// Layout:
//   - Top toolbar: search field, state filter dropdown, category chips,
//     "Clear all" button
//   - Scrollable content area with downloads grouped by date
//     (Today, Yesterday, Last 7 days, Older)
//   - Each download item shows: icon, filename, URL/source, progress bar,
//     status text, action buttons, danger indicator
//   - Empty state view when no downloads match filters
//
// Chromium owner: Downloads page (chrome://downloads)
//   (chrome/browser/ui/webui/downloads/downloads_ui.h)
//
// TODO(astra): This is a Views-based alternative to the WebUI downloads page.
// Integrate with Chromium's download system via AstraDownloadsPageModel
// which projects DownloadManager state.
// Patch point: chrome/browser/ui/webui/downloads — replace or augment
// the WebUI page with a Views-based version.
class AstraDownloadsPageView : public views::View,
                               public views::TextfieldController,
                               public AstraDownloadsPageObserver {
 public:
  METADATA_HEADER(AstraDownloadsPageView);

  AstraDownloadsPageView();
  explicit AstraDownloadsPageView(AstraDownloadsPageModel* model);
  AstraDownloadsPageView(const AstraDownloadsPageView&) = delete;
  AstraDownloadsPageView& operator=(const AstraDownloadsPageView&) = delete;
  ~AstraDownloadsPageView() override;

  // -- Model management -----------------------------------------------------

  void SetModel(AstraDownloadsPageModel* model);
  AstraDownloadsPageModel* model() const { return model_; }

  // -- views::View ----------------------------------------------------------

  void OnThemeChanged() override;
  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  // -- TextfieldController --------------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

  // -- AstraDownloadsPageObserver -------------------------------------------

  void OnDownloadsChanged() override;
  void OnSearchChanged(const std::u16string& query) override;
  void OnFilterChanged() override;
  void OnDownloadsPageModelShutdown() override;

  // -- Accessors for testing ------------------------------------------------

  views::Textfield* search_field_for_test() { return search_field_; }
  views::View* category_chips_container_for_test() {
    return category_chips_container_;
  }
  views::ScrollView* scroll_view_for_test() { return scroll_view_; }
  views::View* content_container_for_test() { return content_container_; }
  views::View* empty_state_for_test() { return empty_state_; }
  views::View* toolbar_for_test() { return toolbar_; }
  views::View* filter_buttons_container_for_test() {
    return filter_buttons_container_;
  }
  views::LabelButton* clear_all_button_for_test() {
    return clear_all_button_;
  }

 private:
  // Date group identifiers.
  enum class DateGroup {
    kToday,
    kYesterday,
    kLast7Days,
    kOlder,
  };

  // Build the page UI.
  void Build();

  // Build the top toolbar.
  void BuildToolbar();

  // Build the scrollable content area.
  void BuildContentArea();

  // Rebuild the download list from the model.
  void RebuildDownloadList();

  // Rebuild category chips from model categories.
  void RebuildCategoryChips(const std::vector<std::string>& categories);

  // Create a date group section with the given title.
  views::View* CreateDateGroup(const std::u16string& title);

  // Create a single download item view.
  views::View* CreateDownloadItem(const std::string& id);

  // Update all download items from the model (without full rebuild).
  void UpdateAllDownloadItems();

  // Update a single download item by ID.
  void UpdateDownloadItem(const std::string& id);

  // Update empty state visibility.
  void UpdateEmptyState();

  // -- Action handlers ------------------------------------------------------

  void OnClearAllClicked();
  void OnFilterClicked(AstraDownloadsPageFilter filter);
  void OnCategoryChipClicked(const std::string& category);

  void OnPauseClicked(const std::string& id);
  void OnResumeClicked(const std::string& id);
  void OnCancelClicked(const std::string& id);
  void OnOpenClicked(const std::string& id);
  void OnShowInFolderClicked(const std::string& id);
  void OnMoreClicked(const std::string& id);

  // -- Custom icon drawing --------------------------------------------------

  // Helper to draw icons into a canvas.
  static void DrawDownloadIcon(gfx::Canvas* canvas,
                               const gfx::Rect& bounds,
                               SkColor color);
  static void DrawFolderIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawFileIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color);
  static void DrawPauseIcon(gfx::Canvas* canvas,
                            const gfx::Rect& bounds,
                            SkColor color);
  static void DrawResumeIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawCancelIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawDangerIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawSearchIcon(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color);
  static void DrawMoreIcon(gfx::Canvas* canvas,
                           const gfx::Rect& bounds,
                           SkColor color);

  // -- Helpers --------------------------------------------------------------

  // Get color for a category icon.
  SkColor GetCategoryColor(const std::string& category) const;

  // Format bytes as human-readable string.
  static std::u16 FormatBytes(int64_t bytes);

  // Format download state as text.
  static std::u16 FormatState(const AstraDownloadEntry& entry);

  // Determine which date group an entry belongs to.
  static DateGroup GetDateGroup(const base::Time& time);

  // Get label for a date group.
  static std::u16 GetDateGroupLabel(DateGroup group);

  // The model we observe and display.  Not owned.
  raw_ptr<AstraDownloadsPageModel> model_ = nullptr;

  // Observation tracker.
  base::ScopedObservation<AstraDownloadsPageModel,
                          AstraDownloadsPageObserver>
      model_observation_{this};

  // -- Child views ----------------------------------------------------------

  // Top toolbar.
  raw_ptr<views::View> toolbar_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::View> filter_buttons_container_ = nullptr;
  raw_ptr<views::View> category_chips_container_ = nullptr;
  raw_ptr<views::LabelButton> clear_all_button_ = nullptr;

  // Scrollable content.
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> content_container_ = nullptr;

  // Empty state.
  raw_ptr<views::View> empty_state_ = nullptr;

  // Currently active state filter button (for visual state tracking).
  raw_ptr<views::LabelButton> active_filter_button_ = nullptr;

  // Currently active category chip.
  std::string active_category_;

  // -- Constants ------------------------------------------------------------

  static constexpr int kToolbarHeight = 56;
  static constexpr int kToolbarSidePadding = 16;
  static constexpr int kSearchFieldWidth = 280;
  static constexpr int kSearchFieldHeight = 32;
  static constexpr int kFilterButtonHeight = 28;
  static constexpr int kChipHeight = 28;
  static constexpr int kChipHorizontalPadding = 12;
  static constexpr int kItemHeight = 72;
  static constexpr int kItemHorizontalPadding = 16;
  static constexpr int kItemVerticalPadding = 8;
  static constexpr int kIconSize = 40;
  static constexpr int kActionButtonSize = 28;
  static constexpr int kProgressBarHeight = 4;
  static constexpr int kDateGroupHeaderHeight = 36;
  static constexpr int kDateGroupSidePadding = 16;
  static constexpr int kEmptyStateIconSize = 64;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DOWNLOADS_PAGE_ASTRA_DOWNLOADS_PAGE_VIEW_H_
