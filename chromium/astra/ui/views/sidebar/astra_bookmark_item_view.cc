#include "astra/ui/views/sidebar/astra_bookmark_item_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/utf_string_conversions.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kBookmarkItemHeight = 32;
constexpr int kBookmarkItemHorizontalPadding = 12;
constexpr int kBookmarkItemIndentPerLevel = 16;
constexpr int kBookmarkItemCornerRadius = 6;
constexpr int kBookmarkExpandArrowSize = 12;

// Astra color IDs for bookmark item styling.
constexpr ui::ColorId kBookmarkItemTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kBookmarkItemHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kBookmarkItemActiveBgColorId =
    kColorAstraSidebarItemSelectedBackground;

// Parse bookmark info types.
constexpr int kBookmarkBarHorizontalPadding = 8;

}  // namespace

AstraBookmarkItemView::AstraBookmarkItemView(
    const bookmarks::BookmarkNode* node,
    Type type,
    int depth)
    : node_(node), type_(type), depth_(depth) {
  // The base class BuildLayout is called in constructor.
  // We need to build our custom layout after the base.
  // Actually, BuildLayout is virtual and called from base constructor.
  // Since it's virtual, our override will be called... wait, no.
  // In C++, the base class constructor calls the base class version of
  // virtual functions, not the derived class version.
  // So we need to call our layout setup separately.

  // Initialize state from node.
  if (node_) {
    if (node_->is_url()) {
      url_ = node_->url();
    }
    SetTitle(base::UTF8ToUTF16(node_->GetTitle()));
  }

  // Apply folder-specific setup.
  if (type_ == Type::kFolder) {
    SetChevronVisible(true);
    SetChevronRotated(is_folder_expanded_);
  }

  // Apply depth indentation.
  ApplyDepthIndentation();

  // Update the icon.
  UpdateIcon();
}

AstraBookmarkItemView::~AstraBookmarkItemView() = default;

void AstraBookmarkItemView::BuildLayout() {
  // Call base class to build the common layout first.
  AstraSidebarItemView::BuildLayout();

  // For bookmark items, we move the chevron from trailing to leading
  // area (before the icon), since folder expand arrows are on the left.
  // We'll do this adjustment in the constructor after base BuildLayout.
}

// =========================================================================
// Bookmark info
// =========================================================================

void AstraBookmarkItemView::SetBookmarkInfo(const GURL& url,
                                            const std::u16string& title,
                                            bool is_folder) {
  url_ = url;
  type_ = is_folder ? Type::kFolder : Type::kUrl;
  SetTitle(title);

  // Update folder-specific state.
  SetChevronVisible(is_folder);
  if (is_folder) {
    SetChevronRotated(is_folder_expanded_);
  }

  UpdateIcon();
  SchedulePaint();
}

// =========================================================================
// Title override
// =========================================================================

void AstraBookmarkItemView::SetTitle(const std::u16string& title) {
  AstraSidebarItemView::SetTitle(title);
}

// =========================================================================
// Folder state
// =========================================================================

void AstraBookmarkItemView::SetFolderExpanded(bool expanded) {
  if (type_ != Type::kFolder) {
    return;
  }
  if (is_folder_expanded_ == expanded) {
    return;
  }
  is_folder_expanded_ = expanded;
  SetChevronRotated(expanded);
  UpdateExpandChevron();
}

void AstraBookmarkItemView::UpdateExpandChevron() {
  if (!chevron_view()) {
    return;
  }
  // TODO(astra): Use proper vector icons for expanded/collapsed state.
  //   Currently the chevron view is a placeholder.
  //   Chromium owner: ui/views/vector_icons/chevron_*.h
}

// =========================================================================
// Bookmark count
// =========================================================================

void AstraBookmarkItemView::SetBookmarkCount(int count) {
  if (bookmark_count_ == count) {
    return;
  }
  bookmark_count_ = count;

  if (show_bookmark_count_) {
    SetBadgeText(base::NumberToString16(count));
  }
}

void AstraBookmarkItemView::ShowBookmarkCount(bool show) {
  if (show_bookmark_count_ == show) {
    return;
  }
  show_bookmark_count_ = show;

  if (show) {
    SetBadgeText(base::NumberToString16(bookmark_count_));
  } else {
    ShowBadge(false);
  }
}

// =========================================================================
// Bookmark bar styling
// =========================================================================

void AstraBookmarkItemView::SetIsBookmarkBar(bool is_bar) {
  if (is_bookmark_bar_ == is_bar) {
    return;
  }
  is_bookmark_bar_ = is_bar;
  // TODO(astra): Apply special styling for the bookmark bar item.
  //   Could include bold text, different background, etc.
  OnThemeChanged();
}

// =========================================================================
// Active state override
// =========================================================================

void AstraBookmarkItemView::SetActive(bool active) {
  AstraSidebarItemView::SetActive(active);
}

// =========================================================================
// Icon
// =========================================================================

void AstraBookmarkItemView::UpdateIcon() {
  // TODO(astra): Load real icons:
  //   - Folders: use vector icon from ui/views/vector_icons/folder_*.h
  //   - URLs: use favicon via BookmarkModel::GetFavicon()
  //
  // For now, we just show a placeholder icon to reserve space.
  // Chromium owner: FaviconService (components/favicon/core/favicon_service.h)

  // Create a small colored square as a placeholder icon.
  gfx::ImageSkia placeholder_icon;
  // The actual icon will be provided by Chromium's favicon service.

  // For folder vs URL distinction, we rely on the chevron and text.
  // TODO(astra): Set proper icon once we have vector icons available.

  icon_view()->SetVisible(true);
}

// =========================================================================
// Depth indentation
// =========================================================================

void AstraBookmarkItemView::ApplyDepthIndentation() {
  // Apply additional left padding based on depth.
  // We adjust the border to add left indentation.
  int left_indent = depth_ * kBookmarkItemIndentPerLevel;
  // TODO(astra): Adjust layout padding for depth.
  //   For now, we use the base class layout and just note the depth.
}

// =========================================================================
// Hit testing for expand chevron
// =========================================================================

bool AstraBookmarkItemView::IsPointInExpandChevron(
    const gfx::Point& point) const {
  if (!chevron_view() || type_ != Type::kFolder) {
    return false;
  }
  // Check if the point is within the chevron bounds.
  gfx::Point chevron_point = point;
  ConvertPointToTarget(this, chevron_view(), &chevron_point);
  return chevron_view()->GetLocalBounds().Contains(chevron_point);
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraBookmarkItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = AstraSidebarItemView::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kBookmarkItemHeight));
  return size;
}

void AstraBookmarkItemView::OnThemeChanged() {
  AstraSidebarItemView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Bookmark bar item gets special styling (bold text).
  if (is_bookmark_bar_ && title_label()) {
    title_label()->SetFontList(
        title_label()->font_list().DeriveWithWeight(gfx::Font::Weight::BOLD));
  }
}

bool AstraBookmarkItemView::OnMousePressed(const ui::MouseEvent& event) {
  if (!IsEnabled()) {
    return false;
  }

  // Let base class handle general mouse behavior.
  return AstraSidebarItemView::OnMousePressed(event);
}

void AstraBookmarkItemView::OnMouseReleased(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton() ||
      (event.flags() & ui::EF_MIDDLE_MOUSE_BUTTON)) {
    // Check if the click is on the expand chevron (folders only).
    if (type_ == Type::kFolder &&
        IsPointInExpandChevron(event.location())) {
      if (delegate_) {
        delegate_->OnBookmarkFolderExpandedToggled(node_);
      }
      return;
    }
  }

  // Base class handles general click behavior.
  AstraSidebarItemView::OnMouseReleased(event);
}

void AstraBookmarkItemView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_internal_ = true;
  AstraSidebarItemView::OnMouseEntered(event);
}

void AstraBookmarkItemView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_internal_ = false;
  AstraSidebarItemView::OnMouseExited(event);
}

// =========================================================================
// Click handling
// =========================================================================

void AstraBookmarkItemView::OnItemClicked() {
  if (!delegate_ || !node_) {
    return;
  }

  // Determine if this should open in a new tab.
  // TODO(astra): Check for Ctrl key and middle-click properly.
  bool open_in_new_tab = false;

  delegate_->OnBookmarkItemClicked(node_, open_in_new_tab);
}

}  // namespace astra
