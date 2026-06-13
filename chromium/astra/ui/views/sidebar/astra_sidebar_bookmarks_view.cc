#include "astra/ui/views/sidebar/astra_sidebar_bookmarks_view.h"

#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_bookmark_item_view.h"
#include "base/check.h"
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
#include "ui/views/layout/box_layout.h"
#include "ui/views/controls/scroll_view.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kBookmarksSectionHeaderHeight = 28;
constexpr int kBookmarksSectionHorizontalPadding = 12;
constexpr int kBookmarksSectionVerticalPadding = 8;
constexpr int kBookmarksSectionHeaderFontSizeDelta = 1;
constexpr int kBookmarksCollapseButtonSize = 12;

// Section title.
const char16_t kBookmarksTitle[] = u"Bookmarks";

// Astra color IDs for the bookmarks panel.
// Uses the Astra sidebar color system from astra/ui/color/astra_color_ids.h.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kBookmarksHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kBookmarksBackgroundColorId =
    kColorAstraSidebarBackground;

}  // namespace

AstraSidebarBookmarksView::AstraSidebarBookmarksView(Browser* browser)
    : browser_(browser) {
  // Get profile from the browser.
  if (browser_) {
    profile_ = browser_->profile();
  }

  // TODO(astra): Get BookmarkModel from profile via BookmarkModelFactory.
  // Currently we look it up lazily in StartObservingModel().
  // Chromium owner: BookmarkModelFactory (chrome/browser/bookmarks/bookmark_model_factory.h)
  // Chromium patch point: none — use public factory API.

  BuildLayout();

  // Paint background.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  // Start observing the bookmark model if it's already loaded.
  // If not loaded yet, the BookmarkModelLoaded callback will trigger the
  // initial build.
  StartObservingModel();
}

AstraSidebarBookmarksView::~AstraSidebarBookmarksView() {
  // ScopedObservation automatically cleans up on destruction.
}

void AstraSidebarBookmarksView::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // --- Header row ---
  header_view_ = AddChildView(std::make_unique<views::View>());
  auto* header_layout = header_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kBookmarksSectionVerticalPadding,
                          kBookmarksSectionHorizontalPadding),
          0));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  header_view_->SetPreferredSize(
      gfx::Size(0, kBookmarksSectionHeaderHeight));

  // Collapse/expand chevron button.
  collapse_button_ =
      header_view_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraSidebarBookmarksView::ToggleSectionCollapsed,
                              base::Unretained(this))));
  collapse_button_->SetPreferredSize(
      gfx::Size(kBookmarksCollapseButtonSize, kBookmarksCollapseButtonSize));
  collapse_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  // TODO(astra): Set chevron vector icon.
  // Chromium owner: ui/views/vector_icons/chevron_*.h

  // Header label.
  header_label_ = header_view_->AddChildView(std::make_unique<views::Label>(kBookmarksTitle));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetFontList(
      header_label_->font_list().DeriveWithSizeDelta(
          kBookmarksSectionHeaderFontSizeDelta));
  header_layout->SetFlexForView(header_label_, 1);

  // Also make the entire header clickable to toggle collapse.
  // TODO(astra): The header should be clickable anywhere to toggle the
  // section. For now, only the button works. We could make the header a
  // LabelButton or install a mouse listener on the header view.

  // --- Scrollable tree area ---
  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetClipBounds(true);
  scroll_view_->SetBackgroundColor(SK_ColorTRANSPARENT);
  layout->SetFlexForView(scroll_view_, 1);

  tree_container_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  auto* tree_layout = tree_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, 4),
          2));
  tree_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Initial build if model is available.
  if (bookmark_model_ && bookmark_model_->loaded()) {
    RebuildFromModel();
  }
}

void AstraSidebarBookmarksView::StartObservingModel() {
  if (!profile_) {
    return;
  }

  // Get the BookmarkModel from the factory.
  // TODO(astra): This assumes BookmarkModelFactory is available and linked.
  // In a full Chromium build, this will return the profile's BookmarkModel.
  // For the overlay project, the factory may not be wired yet — handle gracefully.
  // Chromium owner: BookmarkModelFactory (chrome/browser/bookmarks/bookmark_model_factory.h)
  bookmark_model_ = BookmarkModelFactory::GetForBrowserContext(profile_);

  if (!bookmark_model_) {
    // BookmarkModel not available yet. It will be created asynchronously.
    // TODO(astra): Wait for BookmarkModel to be loaded. In production code,
    // we would use BookmarkModelFactory::GetForBrowserContext which triggers
    // creation, or observe profile initialization.
    // Chromium owner: ProfileKeyedServiceFactory (components/keyed_service/core/)
    return;
  }

  // Observe the model for changes.
  bookmark_model_observation_.Reset();
  bookmark_model_observation_.Observe(bookmark_model_);

  // If the model is already loaded, build the tree now.
  if (bookmark_model_->loaded()) {
    RebuildFromModel();
  }
  // If not loaded yet, BookmarkModelLoaded() will trigger the build.
}

void AstraSidebarBookmarksView::StopObservingModel() {
  bookmark_model_observation_.Reset();
  bookmark_model_ = nullptr;
}

void AstraSidebarBookmarksView::RebuildFromModel() {
  if (!bookmark_model_ || !bookmark_model_->loaded() || !tree_container_) {
    return;
  }

  // Clear existing items.
  tree_container_->RemoveAllChildViews();
  node_to_item_.clear();

  // Add the bookmark bar contents at top level.
  // In Chromium's BookmarkModel, the root has:
  //   - bookmark_bar_node(): the bookmarks bar
  //   - other_node(): "Other Bookmarks"
  //   - mobile_node(): Mobile bookmarks
  //
  // We show the bookmark bar's children at the top level, plus the
  // "Other Bookmarks" and "Mobile Bookmarks" as top-level folders.
  //
  // TODO(astra): Consider whether to show only the bookmark bar, or also
  // include Other Bookmarks and Mobile Bookmarks as top-level folders.
  // Chrome's bookmarks side panel shows all three.
  // Chromium owner: BookmarkModel::bookmark_bar_node(), other_node(), mobile_node()

  const bookmarks::BookmarkNode* bookmark_bar =
      bookmark_model_->bookmark_bar_node();
  if (bookmark_bar) {
    AddChildrenOf(bookmark_bar, /*depth=*/0);
  }

  // Add "Other Bookmarks" as a top-level folder if it has children.
  const bookmarks::BookmarkNode* other_node =
      bookmark_model_->other_node();
  if (other_node && other_node->children().size() > 0) {
    auto other_item = CreateItemView(other_node, /*depth=*/0);
    other_item->SetExpanded(IsFolderExpanded(other_node));
    AstraBookmarkItemView* item_ptr = other_item.get();
    tree_container_->AddChildView(std::move(other_item));
    node_to_item_[other_node] = item_ptr;

    // Add children if expanded.
    if (IsFolderExpanded(other_node)) {
      AddChildrenOf(other_node, /*depth=*/1);
    }
  }

  // Add "Mobile Bookmarks" as a top-level folder if it has children.
  const bookmarks::BookmarkNode* mobile_node =
      bookmark_model_->mobile_node();
  if (mobile_node && mobile_node->children().size() > 0) {
    auto mobile_item = CreateItemView(mobile_node, /*depth=*/0);
    mobile_item->SetExpanded(IsFolderExpanded(mobile_node));
    AstraBookmarkItemView* item_ptr = mobile_item.get();
    tree_container_->AddChildView(std::move(mobile_item));
    node_to_item_[mobile_node] = item_ptr;

    // Add children if expanded.
    if (IsFolderExpanded(mobile_node)) {
      AddChildrenOf(mobile_node, /*depth=*/1);
    }
  }

  InvalidateLayout();
}

void AstraSidebarBookmarksView::AddChildrenOf(
    const bookmarks::BookmarkNode* parent,
    int depth) {
  DCHECK(parent);
  if (!tree_container_) {
    return;
  }

  for (const auto& child : parent->children()) {
    auto item = CreateItemView(child.get(), depth);

    if (child->is_folder()) {
      item->SetExpanded(IsFolderExpanded(child.get()));
    }

    AstraBookmarkItemView* item_ptr = item.get();
    tree_container_->AddChildView(std::move(item));
    node_to_item_[child.get()] = item_ptr;

    // Recursively add children for folders that are expanded.
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

  // Click callback.
  item->set_click_callback(base::BindRepeating(
      &AstraSidebarBookmarksView::OnBookmarkClicked,
      base::Unretained(this)));

  // Expand/collapse callback (only used for folders).
  item->set_expand_callback(base::BindRepeating(
      &AstraSidebarBookmarksView::OnFolderToggled,
      base::Unretained(this)));

  return item;
}

void AstraSidebarBookmarksView::OnBookmarkClicked(
    const bookmarks::BookmarkNode* node,
    bool open_in_new_tab) {
  if (!node) {
    return;
  }

  if (node->is_folder()) {
    // Clicking a folder title toggles expansion (same as clicking the arrow).
    OnFolderToggled(node);
    return;
  }

  // URL bookmark — open it.
  OpenBookmark(node, open_in_new_tab);
}

void AstraSidebarBookmarksView::OnFolderToggled(
    const bookmarks::BookmarkNode* folder_node) {
  if (!folder_node || !folder_node->is_folder()) {
    return;
  }

  bool is_expanded = IsFolderExpanded(folder_node);
  SetFolderExpanded(folder_node, !is_expanded);

  // Update the item view's expand state.
  auto it = node_to_item_.find(folder_node);
  if (it != node_to_item_.end()) {
    it->second->SetExpanded(!is_expanded);
  }

  // TODO(astra): Instead of full rebuild, add/remove child views
  // incrementally when a folder is toggled. Full rebuild is simple but
  // causes flicker and loses scroll position.
  // For now, rebuild the entire tree to show/hide children.
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
    // Open in a new tab.
    // TODO(astra): Use chrome::NavigateParams or Browser::OpenURL for
    // proper URL opening with transition type, disposition, etc.
    // Chromium owner: Browser::OpenURL (chrome/browser/ui/browser.h)
    //   or chrome/browser/ui/browser_navigator.h
    //
    // For now, use AddTab with the URL.
    // TODO(astra): Use the proper Chromium navigation API.
    // The following is a conceptual implementation — in a real build we
    // would use browser_->OpenURL or Navigate() with the right params.
    if (browser_->tab_strip_model()) {
      // TODO(astra): Actually navigate to the URL. The exact API depends
      // on the Chromium version and build configuration.
      // Chromium owner: chrome/browser/ui/browser_navigator.h
    }
  } else {
    // Open in the active tab.
    // TODO(astra): Use Chromium's navigation system to open the bookmark
    // URL in the current tab. Use Navigate() or browser_->OpenURL() with
    // WindowOpenDisposition::CURRENT_TAB.
    // Chromium owner: chrome/browser/ui/browser_navigator.h
    if (browser_->tab_strip_model()) {
      content::WebContents* active_contents =
          browser_->tab_strip_model()->GetActiveWebContents();
      if (active_contents) {
        // TODO(astra): Use proper navigation with transition type
        // (ui::PAGE_TRANSITION_AUTO_BOOKMARK).
        // active_contents->GetController().LoadURL(...) or use OpenURL.
        // Chromium owner: content::NavigationController or chrome::Navigate.
      }
    }
  }
}

bool AstraSidebarBookmarksView::IsFolderExpanded(
    const bookmarks::BookmarkNode* folder_node) const {
  if (!folder_node) {
    return false;
  }
  // Folders are expanded by default; only collapsed ones are in the set.
  return collapsed_folders_.find(folder_node->id()) == collapsed_folders_.end();
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
  // Patch point: none — use public PrefService API.
}

void AstraSidebarBookmarksView::SetSectionCollapsed(bool collapsed) {
  if (is_collapsed_ == collapsed) {
    return;
  }
  is_collapsed_ = collapsed;

  // Show/hide the scroll view (tree content).
  if (scroll_view_) {
    scroll_view_->SetVisible(!is_collapsed_);
  }

  // TODO(astra): Update chevron direction (point right when collapsed,
  // down when expanded).
  InvalidateLayout();
}

void AstraSidebarBookmarksView::ToggleSectionCollapsed() {
  SetSectionCollapsed(!is_collapsed_);
}

// =========================================================================
// bookmarks::BookmarkModelObserver
// =========================================================================
//
// Primary reactive update path for the bookmark tree. All bookmark state
// changes originate from Chromium's BookmarkModel and flow through these
// observer methods. The sidebar bookmark view is a pure projection — it
// never mutates BookmarkModel directly (all mutations go through the
// model's own API when the user interacts with context menus etc.).
//
// TODO(astra): Implement incremental updates in each method instead of
// calling RebuildFromModel() (full rebuild). Full rebuilds are correct but
// inefficient for bookmark-heavy profiles.
//
// Performance rationale: With many bookmarks (1000+), full rebuilds cause
// noticeable jank on every change. Incremental updates are O(1) for single
// node changes and O(k) where k is the number of children affected.
//
// Chromium owner: BookmarkModel (components/bookmarks/browser/bookmark_model.h)
// Chromium observer: BookmarkModelObserver
//   (components/bookmarks/browser/bookmark_model_observer.h)

void AstraSidebarBookmarksView::BookmarkModelLoaded(
    bookmarks::BookmarkModel* /*model*/,
    bool /*ids_reassigned*/) {
  // The bookmark model has finished loading. Build the tree now.
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeAdded(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* /*parent*/,
    size_t /*index*/) {
  // TODO(astra): Insert the new node into the tree at the correct position
  // instead of rebuilding everything.
  // Chromium owner: BookmarkModel::AddURL / AddFolder
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeRemoved(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* /*parent*/,
    size_t /*index*/,
    const bookmarks::BookmarkNode* /*node*/,
    const std::set<GURL>& /*removed_urls*/) {
  // TODO(astra): Remove the node and all its descendants from the tree
  // instead of rebuilding everything.
  // Chromium owner: BookmarkModel::Remove
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeChanged(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* node) {
  // A node's title or URL changed. Update just that item's display.
  auto it = node_to_item_.find(node);
  if (it != node_to_item_.end()) {
    it->second->SetTitle(base::UTF8ToUTF16(node->GetTitle()));
    return;
  }
  // Fallback: full rebuild if we couldn't find the item.
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeMoved(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* /*old_parent*/,
    size_t /*old_index*/,
    const bookmarks::BookmarkNode* /*new_parent*/,
    size_t /*new_index*/) {
  // TODO(astra): Move the item in the tree without rebuilding everything.
  // Need to handle moves within the same parent (reorder) and moves between
  // parents (potentially changing depth).
  // Chromium owner: BookmarkModel::Move
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeChildrenReordered(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* /*node*/) {
  // TODO(astra): Reorder child items of the given parent folder without
  // rebuilding the entire tree.
  // Chromium owner: BookmarkModel::ReorderChildren
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkNodeFaviconChanged(
    bookmarks::BookmarkModel* /*model*/,
    const bookmarks::BookmarkNode* node) {
  // A bookmark's favicon changed.
  // TODO(astra): Update just that item's icon once favicon loading is
  // implemented. Currently we don't show real favicons.
  // Chromium owner: FaviconService (components/favicon/core/favicon_service.h)
  // For now, nothing to do — we'll handle this once icons are wired in.
}

void AstraSidebarBookmarksView::BookmarkAllUserNodesRemoved(
    bookmarks::BookmarkModel* /*model*/,
    const std::set<GURL>& /*removed_urls*/) {
  // All user bookmarks were removed (e.g., on profile reset).
  RebuildFromModel();
}

void AstraSidebarBookmarksView::BookmarkModelBeingDeleted(
    bookmarks::BookmarkModel* /*model*/) {
  // The model is being destroyed — stop observing and clear the tree.
  StopObservingModel();
  if (tree_container_) {
    tree_container_->RemoveAllChildViews();
  }
  node_to_item_.clear();
}

// =========================================================================
// views::View
// =========================================================================

gfx::Size AstraSidebarBookmarksView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarBookmarksView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kTree;
  node_data->SetName("Bookmarks");
}

void AstraSidebarBookmarksView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (header_label_) {
    header_label_->SetEnabledColor(
        color_provider->GetColor(kBookmarksHeaderTextColorId));
  }

  if (layer()) {
    layer()->SetColor(color_provider->GetColor(kBookmarksBackgroundColorId));
  }
}

}  // namespace astra
