#include "astra/ui/views/sidebar/astra_sidebar_bookmarks_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_bookmark_item_view.h"
#include "base/check.h"
#include "base/i18n/number_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "content/public/browser/web_contents.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Section title.
const char16_t kBookmarksTitle[] = u"Bookmarks";
const char16_t kEmptyBookmarksText[] = u"No bookmarks";
const char16_t kSearchPlaceholder[] = u"Search bookmarks...";

// Astra color IDs.
constexpr ui::ColorId kBookmarksHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kBookmarksBackgroundColorId =
    kColorAstraSidebarBackground;

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraSidebarBookmarksView::AstraSidebarBookmarksView(Browser* browser)
    : AstraSidebarSectionView(kBookmarksTitle,
                              AstraSidebarSectionType::kBookmarks),
      browser_(browser) {
  // Get profile from the browser.
  if (browser_) {
    profile_ = browser_->profile();
  }

  BuildBookmarksLayout();

  // Configure base section appearance.
  SetShowChevron(true);
  SetShowItemCount(true);
  SetShowAddButton(true);
  SetShowMoreButton(true);
  SetShowSearch(true);
  SetEmptyStateText(kEmptyBookmarksText);

  // Start observing the bookmark model.
  StartObservingModel();
}

AstraSidebarBookmarksView::~AstraSidebarBookmarksView() {
  // ScopedObservation automatically cleans up on destruction.
}

// =========================================================================
// Layout
// =========================================================================

void AstraSidebarBookmarksView::BuildBookmarksLayout() {
  // The base class builds header, content, and footer.
  // We just configure the content area for bookmark tree display.

  // Set search placeholder text.
  if (GetHeaderView() && GetSearchQuery().empty()) {
    // search_field_ is in the base class; we could access it via
    // a getter, but for now the base class default placeholder is fine.
    // TODO(astra): Expose search_field_ or add SetSearchPlaceholder.
  }

  // Initial build if model is available.
  if (bookmark_model_ && bookmark_model_->loaded()) {
    RebuildFromModel();
  }
}

// =========================================================================
// Model observation
// =========================================================================

void AstraSidebarBookmarksView::StartObservingModel() {
  if (!profile_) {
    return;
  }

  // TODO(astra): Use BookmarkModelFactory::GetForBrowserContext(profile_).
  // In a full Chromium build, this returns the profile's BookmarkModel.
  bookmark_model_ = BookmarkModelFactory::GetForBrowserContext(profile_);

  if (!bookmark_model_) {
    return;
  }

  bookmark_model_observation_.Reset();
  bookmark_model_observation_.Observe(bookmark_model_);

  if (bookmark_model_->loaded()) {
    RebuildFromModel();
  }
}

void AstraSidebarBookmarksView::StopObservingModel() {
  bookmark_model_observation_.Reset();
  bookmark_model_ = nullptr;
}

bookmarks::BookmarkModel* AstraSidebarBookmarksView::GetBookmarkModel() {
  return bookmark_model_;
}

// =========================================================================
// Bookmark data projection
// =========================================================================

void AstraSidebarBookmarksView::SetBookmarks(
    const std::vector<AstraBookmarkItemInfo>& bookmarks) {
  bookmarks_ = bookmarks;

  // Clear existing items and rebuild.
  RemoveAllItems();

  for (const auto& info : bookmarks) {
    // TODO(astra): Create actual AstraBookmarkItemView from info struct.
    // For now, we just update the count. The real implementation would
    // create item views based on the info.
    //
    // Since we're extending the section view with generic item management,
    // we need to create actual views. For the projection pattern, the
    // bookmarks are stored as data and rendered as views.
  }

  SetItemCount(static_cast<int>(bookmarks_.size()));
  SetEmpty(bookmarks_.empty());
  InvalidateLayout();
}

int AstraSidebarBookmarksView::GetBookmarkCount() const {
  return static_cast<int>(bookmarks_.size());
}

AstraBookmarkItemInfo AstraSidebarBookmarksView::GetBookmarkAt(
    int index) const {
  if (index < 0 || index >= static_cast<int>(bookmarks_.size())) {
    return AstraBookmarkItemInfo();
  }
  return bookmarks_[index];
}

void AstraSidebarBookmarksView::AddBookmark(
    const AstraBookmarkItemInfo& bookmark) {
  bookmarks_.push_back(bookmark);
  SetItemCount(static_cast<int>(bookmarks_.size()));
  SetEmpty(false);
  // TODO(astra): Create and add the actual item view.
  ApplySortOrder();
}

void AstraSidebarBookmarksView::RemoveBookmark(int index) {
  if (index < 0 || index >= static_cast<int>(bookmarks_.size())) {
    return;
  }
  bookmarks_.erase(bookmarks_.begin() + index);
  SetItemCount(static_cast<int>(bookmarks_.size()));
  SetEmpty(bookmarks_.empty());
  // TODO(astra): Remove the corresponding item view.

  // Adjust selection if needed.
  if (selected_index_ >= static_cast<int>(bookmarks_.size())) {
    selected_index_ = static_cast<int>(bookmarks_.size()) - 1;
  }
}

void AstraSidebarBookmarksView::UpdateBookmark(
    int index,
    const AstraBookmarkItemInfo& bookmark) {
  if (index < 0 || index >= static_cast<int>(bookmarks_.size())) {
    return;
  }
  bookmarks_[index] = bookmark;
  // TODO(astra): Update the corresponding item view.
}

// =========================================================================
// Selection
// =========================================================================

void AstraSidebarBookmarksView::SetSelectedBookmark(int index) {
  selected_index_ = index;
  // TODO(astra): Update visual selection state of item views.
}

void AstraSidebarBookmarksView::ClearSelection() {
  selected_index_ = -1;
  // TODO(astra): Clear visual selection on all items.
}

// =========================================================================
// Folders first
// =========================================================================

void AstraSidebarBookmarksView::SetShowFoldersFirst(bool show_first) {
  if (show_folders_first_ == show_first) {
    return;
  }
  show_folders_first_ = show_first;
  ApplySortOrder();
}

// =========================================================================
// Folder navigation
// =========================================================================

void AstraSidebarBookmarksView::SetCurrentFolder(
    const std::string& folder_id) {
  if (current_folder_id_ == folder_id) {
    return;
  }
  current_folder_id_ = folder_id;
  // TODO(astra): Rebuild the view to show only this folder's children.
}

std::vector<std::string> AstraSidebarBookmarksView::GetFolderPath() const {
  return folder_path_;
}

void AstraSidebarBookmarksView::NavigateToFolder(const std::string& folder_id) {
  if (folder_id.empty() || folder_id == current_folder_id_) {
    return;
  }
  folder_path_.push_back(current_folder_id_);
  SetCurrentFolder(folder_id);
}

void AstraSidebarBookmarksView::NavigateUp() {
  if (folder_path_.empty()) {
    return;
  }
  std::string parent_id = folder_path_.back();
  folder_path_.pop_back();
  SetCurrentFolder(parent_id);
}

bool AstraSidebarBookmarksView::CanNavigateUp() const {
  return !folder_path_.empty();
}

// =========================================================================
// Folder tree display
// =========================================================================

void AstraSidebarBookmarksView::SetShowFolderTree(bool show) {
  if (show_folder_tree_ == show) {
    return;
  }
  show_folder_tree_ = show;
  if (bookmark_model_ && bookmark_model_->loaded()) {
    RebuildFromModel();
  }
}

// =========================================================================
// Folder operations (presentation-only)
// =========================================================================

void AstraSidebarBookmarksView::NewFolder(const std::u16string& name) {
  if (delegate_) {
    delegate_->OnNewFolderRequested();
  }
  // TODO(astra): If no delegate, create directly via BookmarkModel.
  // Chromium owner: BookmarkModel::AddFolder
}

void AstraSidebarBookmarksView::DeleteFolder(const std::string& folder_id) {
  if (delegate_) {
    // Delegate handles deletion; view updates via observer.
  }
  // TODO(astra): Call BookmarkModel::Remove if no delegate.
}

void AstraSidebarBookmarksView::RenameFolder(const std::string& folder_id,
                                              const std::u16string& new_name) {
  if (delegate_) {
    // Delegate handles rename; view updates via observer.
  }
  // TODO(astra): Call BookmarkModel::SetTitle if no delegate.
}

// =========================================================================
// Sorting and filtering
// =========================================================================

void AstraSidebarBookmarksView::SortBookmarks(AstraSidebarSortOrder order) {
  SetSortOrder(order);
  ApplySortOrder();
}

void AstraSidebarBookmarksView::FilterBookmarks(AstraSidebarFilter filter) {
  SetFilter(filter);
  ApplyFilter();
}

void AstraSidebarBookmarksView::SearchBookmarks(const std::u16string& query) {
  SetSearchQuery(query);
  // Filtering is handled by OnSearchQueryChanged.
}

int AstraSidebarBookmarksView::GetVisibleBookmarkCount() const {
  // In a real implementation, this would account for filters.
  // For now, return the total count.
  // TODO(astra): Account for active filter when counting visible items.
  return static_cast<int>(bookmarks_.size());
}

void AstraSidebarBookmarksView::ApplySortOrder() {
  if (bookmarks_.empty()) {
    return;
  }

  std::sort(bookmarks_.begin(), bookmarks_.end(),
            [this](const AstraBookmarkItemInfo& a,
                   const AstraBookmarkItemInfo& b) {
              return CompareBookmarks(a, b, GetSortOrder(),
                                      show_folders_first_);
            });

  // TODO(astra): Reorder item views to match sorted data.
}

void AstraSidebarBookmarksView::ApplyFilter() {
  // TODO(astra): Show/hide item views based on current filter.
  // For now, just update the item count to reflect visible items.
  // SetItemCount(GetVisibleBookmarkCount());
}

// static
bool AstraSidebarBookmarksView::CompareBookmarks(
    const AstraBookmarkItemInfo& a,
    const AstraBookmarkItemInfo& b,
    AstraSidebarSortOrder order,
    bool folders_first) {
  // Folders first sorting.
  if (folders_first) {
    if (a.is_folder != b.is_folder) {
      return a.is_folder;  // Folders come before URLs.
    }
  }

  switch (order) {
    case AstraSidebarSortOrder::kAlphabetical:
      return a.title < b.title;

    case AstraSidebarSortOrder::kDateAdded:
      return a.date_added > b.date_added;  // Newest first.

    case AstraSidebarSortOrder::kDateModified:
      return a.date_modified > b.date_modified;  // Newest first.

    case AstraSidebarSortOrder::kMostVisited:
      // Bookmarks don't have visit count directly; fall back to title.
      // TODO(astra): Use visit count from HistoryService when available.
      return a.title < b.title;

    case AstraSidebarSortOrder::kManual:
      // Manual order — preserve original order (stable sort).
      return false;
  }
  return false;
}

// =========================================================================
// Bookmark bar filter
// =========================================================================

void AstraSidebarBookmarksView::SetShowOnlyBookmarksBar(bool show) {
  if (show_only_bookmarks_bar_ == show) {
    return;
  }
  show_only_bookmarks_bar_ = show;
  if (bookmark_model_ && bookmark_model_->loaded()) {
    RebuildFromModel();
  }
}

// =========================================================================
// Rebuild from model
// =========================================================================

void AstraSidebarBookmarksView::RebuildFromModel() {
  if (!bookmark_model_ || !bookmark_model_->loaded()) {
    return;
  }

  // Clear existing items via base class.
  RemoveAllItems();
  node_to_item_.clear();

  if (show_folder_tree_) {
    // Tree mode: show hierarchical structure.
    const bookmarks::BookmarkNode* bookmark_bar =
        bookmark_model_->bookmark_bar_node();
    if (bookmark_bar) {
      AddChildrenOf(bookmark_bar, /*depth=*/0);
    }

    // Add "Other Bookmarks" as a top-level folder if it has children.
    const bookmarks::BookmarkNode* other_node =
        bookmark_model_->other_node();
    if (other_node && other_node->children().size() > 0 &&
        !show_only_bookmarks_bar_) {
      auto other_item = CreateItemView(other_node, /*depth=*/0);
      other_item->SetExpanded(IsFolderExpanded(other_node));
      AstraBookmarkItemView* item_ptr = other_item.get();
      items_container()->AddChildView(std::move(other_item));
      node_to_item_[other_node] = item_ptr;

      if (IsFolderExpanded(other_node)) {
        AddChildrenOf(other_node, /*depth=*/1);
      }
    }

    // Add "Mobile Bookmarks" as a top-level folder if it has children.
    const bookmarks::BookmarkNode* mobile_node =
        bookmark_model_->mobile_node();
    if (mobile_node && mobile_node->children().size() > 0 &&
        !show_only_bookmarks_bar_) {
      auto mobile_item = CreateItemView(mobile_node, /*depth=*/0);
      mobile_item->SetExpanded(IsFolderExpanded(mobile_node));
      AstraBookmarkItemView* item_ptr = mobile_item.get();
      items_container()->AddChildView(std::move(mobile_item));
      node_to_item_[mobile_node] = item_ptr;

      if (IsFolderExpanded(mobile_node)) {
        AddChildrenOf(mobile_node, /*depth=*/1);
      }
    }
  } else {
    // Flat mode: show only current folder's children.
    // TODO(astra): Implement flat/folder navigation mode.
  }

  // Update item count in header.
  SetItemCount(GetItemViewCount());
  SetEmpty(GetItemViewCount() == 0);

  InvalidateLayout();
}

// =========================================================================
// Tree building helpers
// =========================================================================

void AstraSidebarBookmarksView::AddChildrenOf(
    const bookmarks::BookmarkNode* parent,
    int depth) {
  DCHECK(parent);
  if (!items_container()) {
    return;
  }

  for (const auto& child : parent->children()) {
    auto item = CreateItemView(child.get(), depth);

    if (child->is_folder()) {
      item->SetExpanded(IsFolderExpanded(child.get()));
    }

    AstraBookmarkItemView* item_ptr = item.get();
    items_container()->AddChildView(std::move(item));
    node_to_item_[child.get()] = item_ptr;

    if (child->is_folder() && IsFolderExpanded(child.get())) {
      AddChildrenOf(child.get(), depth + 1);
    }
  }
}

std::unique_ptr<AstraBookmarkItemView>
AstraSidebarBookmarksView::CreateItemView(
    const bookmarks::BookmarkNode* node,
    int depth) {
  DCHECK(node);

  AstraBookmarkItemView::Type type = AstraBookmarkItemView::Type::kUrl;
  if (node->is_folder()) {
    type = AstraBookmarkItemView::Type::kFolder;
  }

  auto item = std::make_unique<AstraBookmarkItemView>(node, type, depth);

  item->set_click_callback(base::BindRepeating(
      &AstraSidebarBookmarksView::OnBookmarkItemClicked,
      base::Unretained(this)));

  item->set_expand_callback(base::BindRepeating(
      &AstraSidebarBookmarksView::OnFolderToggled,
      base::Unretained(this)));

  return item;
}

// =========================================================================
// Click handlers
// =========================================================================

void AstraSidebarBookmarksView::OnBookmarkItemClicked(
    const bookmarks::BookmarkNode* node,
    bool open_in_new_tab) {
  if (!node) {
    return;
  }

  if (node->is_folder()) {
    OnFolderToggled(node);
    if (delegate_) {
      delegate_->OnFolderOpened(
          base::NumberToString(node->id()));
    }
    return;
  }

  // URL bookmark — open it.
  if (delegate_) {
    std::string bookmark_id = base::NumberToString(node->id());
    if (open_in_new_tab) {
      delegate_->OnBookmarkMiddleClicked(bookmark_id);
    } else {
      delegate_->OnBookmarkClicked(bookmark_id);
    }
  }

  OpenBookmark(node, open_in_new_tab);
}

void AstraSidebarBookmarksView::OnFolderToggled(
    const bookmarks::BookmarkNode* folder_node) {
  if (!folder_node || !folder_node->is_folder()) {
    return;
  }

  bool is_expanded = IsFolderExpanded(folder_node);
  SetFolderExpanded(folder_node, !is_expanded);

  auto it = node_to_item_.find(folder_node);
  if (it != node_to_item_.end()) {
    it->second->SetExpanded(!is_expanded);
  }

  // TODO(astra): Incrementally add/remove child views instead of full rebuild.
  RebuildFromModel();
}

void AstraSidebarBookmarksView::OpenBookmark(
    const bookmarks::BookmarkNode* node,
    bool open_in_new_tab) {
  if (!node || node->is_folder() || !browser_) {
    return;
  }

  const GURL& url = node->url();
  if (!url.is_valid()) {
    return;
  }

  if (open_in_new_tab) {
    // TODO(astra): Use chrome::NavigateParams for proper URL opening.
    // Chromium owner: chrome/browser/ui/browser_navigator.h
    if (browser_->tab_strip_model()) {
      // Placeholder for actual navigation.
    }
  } else {
    if (browser_->tab_strip_model()) {
      content::WebContents* active_contents =
          browser_->tab_strip_model()->GetActiveWebContents();
      if (active_contents) {
        // TODO(astra): Use proper navigation with transition type.
      }
    }
  }
}

// =========================================================================
// Folder expansion state
// =========================================================================

bool AstraSidebarBookmarksView::IsFolderExpanded(
    const bookmarks::BookmarkNode* folder_node) const {
  if (!folder_node) {
    return false;
  }
  return collapsed_folders_.find(folder_node->id()) ==
         collapsed_folders_.end();
}

void AstraSidebarBookmarksView::SetFolderExpanded(
    const bookmarks::BookmarkNode* folder_node,
    bool expanded) {
  if (!folder_node) {
    return;
  }

  int64_t id = folder_node->id();
  if (expanded) {
    collapsed_folders_.erase(id);
  } else {
    collapsed_folders_.insert(id);
  }

  // TODO(astra): Persist expanded state to PrefService.
  // Chromium owner: PrefService (components/prefs/pref_service.h)
}

// =========================================================================
// Rebuild bookmark info from views
// =========================================================================

void AstraSidebarBookmarksView::RebuildBookmarkInfoFromViews() {
  // TODO(astra): Populate bookmarks_ vector from item views.
  // This is needed when the model changes via observer.
}

// =========================================================================
// AstraSidebarSectionView overrides
// =========================================================================

void AstraSidebarBookmarksView::OnAddButtonClicked() {
  if (delegate_) {
    delegate_->OnAddBookmarkRequested();
  }
}

void AstraSidebarBookmarksView::OnSearchQueryChanged(
    const std::u16string& query) {
  // Filter bookmark items by search query.
  // TODO(astra): Implement actual search filtering of item views.
  AstraSidebarSectionView::OnSearchQueryChanged(query);
}

// =========================================================================
// bookmarks::BookmarkModelObserver
// =========================================================================

void AstraSidebarBookmarksView::BookmarkModelLoaded(
    bookmarks::BookmarkModel* /*model*/,
    bool /*ids_reassigned*/) {
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeAdded(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* /*parent*/,
    size_t /*index*/) {
  // TODO(astra): Insert the new node incrementally.
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeRemoved(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* /*parent*/,
    size_t /*index*/,
    const bookmarks::BookmarkNode* /*node*/,
    const std::set<GURL>& /*removed_urls*/) {
  // TODO(astra): Remove the node incrementally.
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeChanged(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* node) {
  auto it = node_to_item_.find(node);
  if (it != node_to_item_.end()) {
    it->second->SetTitle(base::UTF8ToUTF16(node->GetTitle()));
    return;
  }
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeMoved(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* /*old_parent*/,
    size_t /*old_index*/,
    const bookmarks::BookmarkNode* /*new_parent*/,
    size_t /*new_index*/) {
  // TODO(astra): Move the item in the tree incrementally.
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeChildrenReordered(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* /*node*/) {
  // TODO(astra): Reorder child items incrementally.
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeFaviconChanged(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* /*node*/) {
  // TODO(astra): Update just that item's favicon.
}

void AstraSidebarBookmarksView::BookmarkAllUserNodesRemoved(
    bookmarks::BookmarkModel* /*model*/,
    const std::set<GURL>& /*removed_urls*/) {
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkModelBeingDeleted(
    bookmarks::BookmarkModel* /*model*/) {
  StopObservingModel();
  RemoveAllItems();
  node_to_item_.clear();
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarBookmarksView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return AstraSidebarSectionView::CalculatePreferredSize(available_size);
}

void AstraSidebarBookmarksView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  AstraSidebarSectionView::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kTree;
  node_data->SetName("Bookmarks");
}

void AstraSidebarBookmarksView::OnThemeChanged() {
  AstraSidebarSectionView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Additional bookmark-specific theming can go here.
}

}  // namespace astra
