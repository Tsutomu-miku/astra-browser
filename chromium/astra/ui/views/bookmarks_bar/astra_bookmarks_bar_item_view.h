#ifndef ASTRA_UI_VIEWS_BOOKMARKS_BAR_ASTRA_BOOKMARKS_BAR_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_BOOKMARKS_BAR_ASTRA_BOOKMARKS_BAR_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/image/image.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace views {
class ImageView;
class Label;
class MenuRunner;
}  // namespace views

namespace astra {

// =========================================================================
// AstraBookmarksBarItemDelegate — delegate for bookmark item actions
// =========================================================================
//
// Delegate interface for user actions on a bookmarks bar item.
// Implemented by the parent bookmarks bar view.
class AstraBookmarksBarItemDelegate {
 public:
  virtual ~AstraBookmarksBarItemDelegate() = default;

  // Called when a bookmark item is clicked (primary action).
  virtual void OnBookmarkClicked(int64_t bookmark_id,
                                 bool is_middle_click,
                                 bool is_shift_click) = 0;

  // Called when a folder item is clicked (shows folder menu).
  virtual void OnFolderClicked(int64_t folder_id,
                                views::View* anchor_view) = 0;

  // Called when a bookmark item is right-clicked (context menu).
  virtual void OnBookmarkRightClicked(int64_t bookmark_id,
                                       const gfx::Point& point) = 0;

  // Called when the user starts dragging a bookmark item.
  virtual void OnBookmarkDragStarted(int64_t bookmark_id,
                                      const ui::MouseEvent& event) = 0;
};

// =========================================================================
// AstraBookmarksBarItemView — single bookmark item on the bookmarks bar
// =========================================================================
//
// A single bookmark or folder item on the bookmarks bar.
// Shows:
//   - Favicon (for URL bookmarks) or folder icon (for folders)
//   - Title text
//
// Behavior:
//   - Left click: open bookmark (or show folder menu)
//   - Middle click: open in new tab
//   - Right click: show context menu
//   - Drag: reorder bookmark
//   - Hover: highlight + tooltip
//
// This is a pure presentation view — it does not own bookmark state.
// Data is set by the parent bar view via setter methods.
//
// Chromium pattern: BookmarksBarView / BookmarkBarView
//   (chrome/browser/ui/views/bookmarks/bookmark_bar_view.h)
//
// TODO(astra): Use real favicons from Chromium's FaviconService.
//   Chromium owner: FaviconService (chrome/browser/favicon/favicon_service.h)
// =========================================================================

class AstraBookmarksBarItemView : public views::LabelButton {
 public:
  AstraBookmarksBarItemView(int64_t bookmark_id,
                             const std::u16string& title,
                             bool is_folder);
  ~AstraBookmarksBarItemView() override;

  AstraBookmarksBarItemView(const AstraBookmarksBarItemView&) = delete;
  AstraBookmarksBarItemView& operator=(const AstraBookmarksBarItemView&) =
      delete;

  // -- Data setters -------------------------------------------------------

  void SetTitle(const std::u16string& title);
  const std::u16string& GetTitle() const { return title_; }

  void SetURL(const GURL& url);
  const GURL& GetURL() const { return url_; }

  void SetIsFolder(bool is_folder);
  bool IsFolder() const { return is_folder_; }

  void SetFavicon(const gfx::Image& favicon);

  void SetBookmarkId(int64_t id) { bookmark_id_ = id; }
  int64_t GetBookmarkId() const { return bookmark_id_; }

  // -- Display options ----------------------------------------------------

  void SetShowIcon(bool show);
  bool GetShowIcon() const { return show_icon_; }

  void SetShowText(bool show);
  bool GetShowText() const { return show_text_; }

  void SetMaxWidth(int max_width);
  int GetMaxWidth() const { return max_width_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraBookmarksBarItemDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- views::LabelButton -------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;

 private:
  // Update the icon view based on current state.
  void UpdateIcon();

  // Update the text label based on current state.
  void UpdateText();

  // Update colors from the color provider.
  void UpdateColors();

  // Button click handler.
  void OnButtonPressed();

  // The unique bookmark identifier.
  int64_t bookmark_id_ = -1;

  // Bookmark data.
  std::u16string title_;
  GURL url_;
  bool is_folder_ = false;
  gfx::Image favicon_;

  // Display options.
  bool show_icon_ = true;
  bool show_text_ = true;
  int max_width_ = 150;

  // Hover state.
  bool is_hovered_ = false;

  // Delegate for action callbacks.  Not owned.
  raw_ptr<AstraBookmarksBarItemDelegate> delegate_ = nullptr;

  // Child views.
  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;

  // Drag state.
  bool is_dragging_ = false;
  gfx::Point drag_start_point_;

  // -- Constants ---------------------------------------------------------

  static constexpr int kIconSize = 16;
  static constexpr int kIconTextSpacing = 6;
  static constexpr int kHorizontalPadding = 8;
  static constexpr int kVerticalPadding = 4;
  static constexpr int kMinWidth = 28;
  static constexpr int kDefaultHeight = 28;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_BOOKMARKS_BAR_ASTRA_BOOKMARKS_BAR_ITEM_VIEW_H_
