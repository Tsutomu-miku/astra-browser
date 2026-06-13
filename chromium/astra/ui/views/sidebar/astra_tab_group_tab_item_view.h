#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_TAB_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_TAB_ITEM_VIEW_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
}  // namespace views

namespace astra {

// Data structure describing a tab within a tab group for sidebar presentation.
//
// This is a projection struct — the truth source is Chromium's WebContents
// and TabStripModel. The sidebar projects tab data into these structs for
// display within tab groups.
//
// Chromium owner: WebContents (content/public/browser/web_contents.h)
// Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
struct AstraTabGroupTabInfo {
  // Stable identifier for the tab. Maps to content::WebContents or a tab ID.
  std::string tab_id;

  // Tab title (from WebContents::GetTitle()).
  std::u16string title;

  // Tab URL.
  GURL url;

  // Whether this is the active tab in the tab strip.
  bool is_active = false;

  // Whether this tab is pinned.
  bool is_pinned = false;

  // Whether audio is playing in this tab.
  bool is_audible = false;

  // Whether the tab is muted.
  bool is_muted = false;

  // Whether the tab is currently loading.
  bool is_loading = false;

  // Favicon image.
  gfx::ImageSkia favicon;

  // Whether a favicon is available (vs. showing a placeholder).
  bool has_favicon = false;

  // Index of this tab within its parent group (0-based).
  int index_in_group = 0;

  // ID of the parent group.
  std::string group_id;

  // Last time the tab was accessed.
  base::Time last_accessed;
};

// Callback type for tab activation.
// |tab_id| is the string identifier of the tab.
using TabActivatedCallback =
    base::RepeatingCallback<void(const std::string& tab_id)>;

// Callback type for tab close.
using TabClosedCallback =
    base::RepeatingCallback<void(const std::string& tab_id)>;

// Callback type for tab middle-click.
using TabMiddleClickedCallback =
    base::RepeatingCallback<void(const std::string& tab_id)>;

// A tab item displayed within a tab group in the sidebar.
//
// Shows:
//   - Indented to show nested hierarchy under the group header
//   - Favicon (or placeholder)
//   - Tab title
//   - Audio/muted indicator
//   - Close button (appears on hover)
//
// This is a pure presentation view. Tab data comes from Chromium's
// TabStripModel and WebContents. The view never stores tab state.
//
// Chromium owner: Tab (chrome/browser/ui/tabs/tab.h)
//   TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
class AstraTabGroupTabItemView : public views::LabelButton {
 public:
  // Callback invoked when the close button is clicked.
  using CloseCallback = base::RepeatingClosure;

  AstraTabGroupTabItemView(const std::u16string& title,
                           int tab_index,
                           PressedCallback activate_callback,
                           CloseCallback close_callback);
  AstraTabGroupTabItemView(const AstraTabGroupTabItemView&) = delete;
  AstraTabGroupTabItemView& operator=(const AstraTabGroupTabItemView&) = delete;
  ~AstraTabGroupTabItemView() override;

  // -- Tab info ------------------------------------------------------------

  // Set all tab info at once. Convenience for bulk updates.
  void SetTabInfo(const AstraTabGroupTabInfo& info);

  // Get the tab ID.
  const std::string& GetTabId() const { return tab_id_; }

  // -- Title ---------------------------------------------------------------

  // Update the displayed tab title.
  void SetTitle(const std::u16string& title);
  std::u16string GetTitle() const;

  // -- URL -----------------------------------------------------------------

  // Set the tab URL (used for tooltip and accessibility).
  void SetUrl(const GURL& url);
  const GURL& GetUrl() const { return url_; }

  // -- Favicon -------------------------------------------------------------

  // Set the favicon image.
  void SetFavicon(const gfx::ImageSkia& favicon);

  // Set whether a favicon is available (show placeholder if not).
  void SetHasFavicon(bool has_favicon);

  // -- Active state --------------------------------------------------------

  // Set whether this tab is the active tab (highlighted).
  void SetActive(bool active);
  bool IsActive() const { return is_active_; }

  // -- Close button --------------------------------------------------------

  // Set whether the close button is currently visible.
  void SetCloseButtonVisible(bool visible);
  bool IsCloseButtonVisible() const;

  // Set whether the close button is shown at all (e.g. hide for pinned tabs).
  void SetShowCloseButton(bool show);
  bool GetShowCloseButton() const { return show_close_button_; }

  // -- Favicon visibility --------------------------------------------------

  // Set whether the favicon is shown.
  void SetShowFavicon(bool show);
  bool GetShowFavicon() const { return show_favicon_; }

  // -- Drag state ----------------------------------------------------------

  // Set whether this tab item is currently being dragged.
  void SetIsDragging(bool dragging);
  bool IsDragging() const { return is_dragging_; }

  // Set whether a drag operation is hovering over this item.
  void SetDragHovered(bool hovered);
  bool IsDragHovered() const { return is_drag_hovered_; }

  // -- Index in group ------------------------------------------------------

  // Set the index of this tab within its parent group.
  void SetIndexInGroup(int index);
  int GetIndexInGroup() const { return index_in_group_; }

  // -- Group ID ------------------------------------------------------------

  // Set the ID of the parent group.
  void SetGroupId(const std::string& group_id);
  const std::string& GetGroupId() const { return group_id_; }

  // -- Pinned state --------------------------------------------------------

  // Set whether this tab is pinned.
  void SetPinned(bool pinned);
  bool IsPinned() const { return is_pinned_; }

  // -- Audio state ---------------------------------------------------------

  // Set whether audio is playing in this tab.
  void SetIsAudible(bool audible);
  bool IsAudible() const { return is_audible_; }

  // Set whether this tab is muted.
  void SetIsMuted(bool muted);
  bool IsMuted() const { return is_muted_; }

  // -- Loading state -------------------------------------------------------

  // Set whether this tab is currently loading.
  void SetIsLoading(bool loading);
  bool IsLoading() const { return is_loading_; }

  // -- Tab index (legacy, for TabStripModel integration) -------------------

  // Get the TabStripModel index of the tab this item represents.
  int tab_index() const { return tab_index_legacy_; }
  void set_tab_index(int index) { tab_index_legacy_ = index; }

  // -- Callbacks -----------------------------------------------------------

  void set_tab_activated_callback(TabActivatedCallback callback) {
    tab_activated_callback_ = std::move(callback);
  }
  void set_tab_closed_callback(TabClosedCallback callback) {
    tab_closed_callback_ = std::move(callback);
  }
  void set_tab_middle_clicked_callback(TabMiddleClickedCallback callback) {
    tab_middle_clicked_callback_ = std::move(callback);
  }

  // -- views::View ---------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Update close button visibility based on hover state and settings.
  void UpdateCloseButtonVisibility();

  // Update audio indicator visibility and state.
  void UpdateAudioIndicator();

  // Update favicon visibility.
  void UpdateFaviconVisibility();

  // Handle middle-click.
  void HandleMiddleClick();

  // Stable identifier for the tab.
  std::string tab_id_;

  // URL of the tab.
  GURL url_;

  // Legacy: TabStripModel index. Kept for backward compatibility.
  // TODO(astra): Remove legacy tab_index_ and use tab_id_ exclusively.
  int tab_index_legacy_ = -1;

  // Index within the parent group.
  int index_in_group_ = 0;

  // Parent group ID.
  std::string group_id_;

  // State flags.
  bool is_active_ = false;
  bool is_pinned_ = false;
  bool is_audible_ = false;
  bool is_muted_ = false;
  bool is_loading_ = false;
  bool is_dragging_ = false;
  bool is_drag_hovered_ = false;

  // Display preferences.
  bool show_favicon_ = true;
  bool show_close_button_ = true;

  // Whether a favicon is available.
  bool has_favicon_ = false;

  // Favicon image.
  gfx::ImageSkia favicon_;

  // Callbacks for user actions.
  CloseCallback close_callback_;
  TabActivatedCallback tab_activated_callback_;
  TabClosedCallback tab_closed_callback_;
  TabMiddleClickedCallback tab_middle_clicked_callback_;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::ImageView> favicon_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
  raw_ptr<views::ImageButton> audio_button_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_TAB_GROUP_TAB_ITEM_VIEW_H_
