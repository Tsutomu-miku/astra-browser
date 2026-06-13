#ifndef ASTRA_UI_VIEWS_TAB_SEARCH_ASTRA_TAB_SEARCH_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_SEARCH_ASTRA_TAB_SEARCH_ITEM_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "skia/include/core/SkColor.h"
#include "ui/gfx/range/range.h"
#include "ui/views/view.h"

#include "astra/ui/views/tab_search/astra_tab_search_model.h"

namespace views {
class ImageButton;
class Label;
class StyledLabel;
}  // namespace views

namespace astra {

// =========================================================================
// Group header view — section header for search result groups
// =========================================================================
//
// Displays a group label (e.g. "Open Tabs", "Bookmarks") and a count badge.
// Used as a section divider in the tab search results list.
//
// Layout:
//   +--------------------------------------------------------------+
//   |  [Icon?]  Group Label                          (count)       |
//   +--------------------------------------------------------------+
//
// Accessibility:
//   - Role: kGroup
//   - Name: group label
//   - Description: count of items in the group
// =========================================================================

class AstraTabSearchGroupHeaderView : public views::View {
 public:
  AstraTabSearchGroupHeaderView(const std::u16string& title, size_t count);
  ~AstraTabSearchGroupHeaderView() override;

  AstraTabSearchGroupHeaderView(const AstraTabSearchGroupHeaderView&) = delete;
  AstraTabSearchGroupHeaderView& operator=(
      const AstraTabSearchGroupHeaderView&) = delete;

  // Update the title text.
  void SetTitle(const std::u16string& title);
  const std::u16string& title() const { return title_; }

  // Update the item count.
  void SetCount(size_t count);
  size_t count() const { return count_; }

  // Set the group type (used for accessibility).
  void SetGroupType(AstraTabSearchResultType type) { group_type_ = type; }
  AstraTabSearchResultType group_type() const { return group_type_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
  // Build the layout.
  void BuildLayout();

  // Update colors from color provider.
  void UpdateColors();

  std::u16string title_;
  size_t count_ = 0;
  AstraTabSearchResultType group_type_ =
      AstraTabSearchResultType::kOpenTab;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
};

// =========================================================================
// A single tab search result item view.
// =========================================================================
//
// Layout:
//   +-------------------------------------------------------------------+
//   |[C] [Favicon]  Tab Title (primary, with match highlights)  [Ws]  |
//   |    [Group?]  URL / Hostname (secondary)           [Audio][Close]|
//   +-------------------------------------------------------------------+
//
//   [C] = group color strip (left edge, if in group)
//   [Ws] = workspace name pill (right side)
//   [Audio] = audio indicator (speaker icon, if audible)
//   [Close] = close button (right, visible on hover/selection)
//
// Visual elements:
//   - Favicon (left side, 16x16).
//   - Tab title (primary label, bold, with search match highlighting).
//   - Tab hostname / URL (secondary label, below the title).
//   - Workspace name (right side, badge-style).
//   - Audio indicator (speaker / muted icon, if tab is audible).
//   - Close button (right side, visible on hover or selection).
//   - Group color strip (left edge, colored line for tab groups).
//   - Pinned indicator (small pin icon if tab is pinned).
//   - Keyboard shortcut hint (right side, e.g. "Ctrl+1").
//
// This is a pure presentation view — it does not own tab state.
// Tab data is projected from the AstraTabSearchModel.
//
// Accessibility:
//   - Role: kListItem
//   - Name: tab title
//   - Description: tab URL / domain + workspace + group
//   - States: kSelectable, kSelected (when selected)
//
// Chromium subsystems reused:
//   - TabStripModel / WebContents own actual tab data.
//   - views::StyledLabel for match-highlighted text.
//   - views::ImageButton for action buttons.
//
// TODO(astra): Show real favicons using Chrome's favicon service.
//   Chromium component: chrome/browser/favicon/favicon_service.h
//   Currently we use a colored placeholder square.
//
// TODO(astra): Use StyledLabel for match highlighting instead of plain Label.
//   Chromium component: views::StyledLabel (ui/views/controls/styled_label.h)
// =========================================================================

class AstraTabSearchItemView : public views::View {
 public:
  // Which result group this item belongs to.
  // Kept for backward compatibility with existing bubble code.
  enum class Group {
    kOpenTabs,        // Currently open tabs.
    kRecentlyClosed,  // Recently closed tabs.
    kBookmarks,       // Bookmark entries.
  };

  // Legacy data struct — kept for backward compatibility.
  // Prefer using AstraTabSearchItem from the model.
  struct TabInfo {
    std::u16string title;
    std::u16string host;
    int tab_index = -1;
    Group group = Group::kOpenTabs;
    std::string identifier;
  };

  explicit AstraTabSearchItemView(const AstraTabSearchItem& tab);
  explicit AstraTabSearchItemView(const TabInfo& tab_info,
                                  int display_index);
  ~AstraTabSearchItemView() override;

  AstraTabSearchItemView(const AstraTabSearchItemView&) = delete;
  AstraTabSearchItemView& operator=(const AstraTabSearchItemView&) = delete;

  // -- Tab data ------------------------------------------------------------

  // Set the full tab data for this item.  Updates all visual elements.
  void SetTab(const AstraTabSearchItem& tab);

  // Get the current tab data.
  const AstraTabSearchItem& GetTab() const { return tab_data_; }

  // -- Selection / highlighting -------------------------------------------

  // Set the item as selected (highlighted background).
  void SetSelected(bool selected);
  bool IsSelected() const { return selected_; }

  // Set the item as highlighted (e.g. hover state).
  void SetHighlighted(bool highlighted);
  bool IsHighlighted() const { return highlighted_; }

  // -- Match highlighting --------------------------------------------------

  // Set the character ranges in the title that match the search query.
  // These ranges will be visually highlighted (bold / different color).
  void SetTitleMatchRanges(const std::vector<gfx::Range>& ranges);

  // Set the character ranges in the URL/hostname that match the search query.
  void SetUrlMatchRanges(const std::vector<gfx::Range>& ranges);

  // Set all matches at once from a match vector.
  void SetMatches(const std::vector<AstraTabSearchMatch>& matches);

  // -- Visibility toggles --------------------------------------------------

  // Show or hide the favicon.
  void ShowFavicon(bool show);

  // Show or hide the workspace name.
  void ShowWorkspace(bool show);

  // Show or hide the audio indicator.
  void ShowAudioIndicator(bool show);

  // Show or hide the close button.
  void ShowCloseButton(bool show);

  // Show or hide the shortcut hint.
  void ShowShortcutHint(bool show);

  // Show or hide the site information (secondary text).
  void ShowSiteInfo(bool show);

  // -- Group color ---------------------------------------------------------

  // Set the group indicator color (left edge strip).
  // Pass SK_ColorTRANSPARENT to hide.
  void SetGroupColor(SkColor color);

  // -- Accessors -----------------------------------------------------------

  int tab_index() const { return tab_data_.tab_index; }
  int tab_id() const { return tab_data_.tab_id; }
  int item_id() const { return tab_data_.item_id; }
  const std::u16string& title() const { return tab_data_.title; }
  const std::u16string& hostname() const { return tab_data_.hostname; }
  const std::string& workspace_id() const { return tab_data_.workspace_id; }
  bool is_pinned() const { return tab_data_.is_pinned; }
  bool is_audible() const { return tab_data_.is_audible; }
  bool is_muted() const { return tab_data_.is_muted; }
  AstraTabSearchResultType result_type() const { return tab_data_.result_type; }

  // Legacy accessors (kept for backward compatibility).
  const std::u16string& host() const { return tab_data_.hostname; }
  Group group() const { return legacy_group_; }
  const std::string& identifier() const { return legacy_identifier_; }

  // -- Display index (for shortcut hints) ----------------------------------

  void SetDisplayIndex(int index);
  int display_index() const { return display_index_; }

  // -- Callbacks -----------------------------------------------------------

  // Callback invoked when the item is clicked or activated via keyboard.
  using ActivatedCallback = base::RepeatingClosure;
  void SetActivatedCallback(ActivatedCallback callback) {
    activated_callback_ = std::move(callback);
  }

  // Callback invoked when the close button is clicked.
  using CloseCallback = base::RepeatingClosure;
  void SetCloseCallback(CloseCallback callback) {
    close_callback_ = std::move(callback);
  }

  // Callback invoked when the middle mouse button is pressed (close tab).
  using MiddleClickCallback = base::RepeatingClosure;
  void SetMiddleClickCallback(MiddleClickCallback callback) {
    middle_click_callback_ = std::move(callback);
  }

  // -- views::View ---------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnGestureEvent(ui::GestureEvent* event) override;

 private:
  // Build the child views and layout.
  void BuildLayout();

  // Update all text labels from tab_data_.
  void UpdateTextFromTab();

  // Update the background color based on selection and hover state.
  void UpdateBackground();

  // Update close button visibility based on hover/selection state.
  void UpdateCloseButtonVisibility();

  // Update shortcut hint visibility and text based on display index.
  void UpdateShortcutHint();

  // Update all text colors from the color provider.
  void UpdateTextColors();

  // Update the favicon appearance.
  void UpdateFaviconAppearance();

  // Update the group color strip appearance.
  void UpdateGroupColorStrip();

  // Update the audio indicator icon based on audible/muted state.
  void UpdateAudioIndicator();

  // Update the workspace label visibility and text.
  void UpdateWorkspaceLabel();

  // Update the pinned indicator visibility.
  void UpdatePinnedIndicator();

  // Update the site information label (secondary row).
  void UpdateSiteInfo();

  // Get a human-readable result type string for accessibility.
  std::u16string GetResultTypeLabel() const;

  // The tab data this item displays.
  AstraTabSearchItem tab_data_;

  // Legacy fields (for backward compatibility).
  Group legacy_group_ = Group::kOpenTabs;
  std::string legacy_identifier_;

  // 1-based display index for shortcut hint.  0 means no shortcut hint.
  int display_index_ = 0;

  // State flags.
  bool selected_ = false;
  bool highlighted_ = false;
  bool focused_ = false;

  // Visibility flags.
  bool show_favicon_ = true;
  bool show_workspace_ = true;
  bool show_audio_indicator_ = true;
  bool show_close_button_ = false;  // Controlled by hover/selection.
  bool show_shortcut_hint_ = true;
  bool show_site_info_ = true;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> group_color_strip_ = nullptr;
  raw_ptr<views::View> favicon_placeholder_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> host_label_ = nullptr;
  raw_ptr<views::Label> workspace_label_ = nullptr;
  raw_ptr<views::Label> shortcut_label_ = nullptr;
  raw_ptr<views::View> audio_indicator_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
  raw_ptr<views::View> pinned_indicator_ = nullptr;

  // Match highlight ranges.
  std::vector<gfx::Range> title_match_ranges_;
  std::vector<gfx::Range> url_match_ranges_;

  // Callbacks.
  ActivatedCallback activated_callback_;
  CloseCallback close_callback_;
  MiddleClickCallback middle_click_callback_;

  base::WeakPtrFactory<AstraTabSearchItemView> weak_ptr_factory_{this};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_SEARCH_ASTRA_TAB_SEARCH_ITEM_VIEW_H_
