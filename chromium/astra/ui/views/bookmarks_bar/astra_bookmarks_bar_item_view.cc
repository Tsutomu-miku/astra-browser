#include "astra/ui/views/bookmarks_bar/astra_bookmarks_bar_item_view.h"

#include "base/i18n/number_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view_class_properties.h"

namespace astra {

AstraBookmarksBarItemView::AstraBookmarksBarItemView(
    int64_t bookmark_id,
    const std::u16string& title,
    bool is_folder)
    : views::LabelButton(
          base::BindRepeating(
              &AstraBookmarksBarItemView::OnButtonPressed,
              base::Unretained(this)),
          title),
      bookmark_id_(bookmark_id),
      title_(title),
      is_folder_(is_folder) {
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Set tooltip
  SetTooltipText(title);

  // Build the internal layout.
  // LabelButton provides the base button behavior; we customize
  // the layout by adding child views for icon + text.
  // In a real implementation, we'd use a custom layout.
}

AstraBookmarksBarItemView::~AstraBookmarksBarItemView() = default;

void AstraBookmarksBarItemView::SetTitle(const std::u16string& title) {
  title_ = title;
  SetText(title);
  SetTooltipText(title);
}

void AstraBookmarksBarItemView::SetURL(const GURL& url) {
  url_ = url;
  // Update tooltip to show URL.
  if (!url.is_empty()) {
    SetTooltipText(title_ + u"\n" + base::UTF8ToUTF16(url.spec()));
  }
}

void AstraBookmarksBarItemView::SetIsFolder(bool is_folder) {
  if (is_folder_ == is_folder) {
    return;
  }
  is_folder_ = is_folder;
  UpdateIcon();
  SchedulePaint();
}

void AstraBookmarksBarItemView::SetFavicon(const gfx::Image& favicon) {
  favicon_ = favicon;
  UpdateIcon();
}

void AstraBookmarksBarItemView::SetShowIcon(bool show) {
  if (show_icon_ == show) {
    return;
  }
  show_icon_ = show;
  UpdateIcon();
  Layout();
}

void AstraBookmarksBarItemView::SetShowText(bool show) {
  if (show_text_ == show) {
    return;
  }
  show_text_ = show;
  UpdateText();
  Layout();
}

void AstraBookmarksBarItemView::SetMaxWidth(int max_width) {
  if (max_width_ == max_width) {
    return;
  }
  max_width_ = max_width;
  UpdateText();
  Layout();
}

void AstraBookmarksBarItemView::UpdateIcon() {
  // In a real implementation, we'd update the icon_view_ with the
  // appropriate favicon or folder icon.
  // For now, this is a placeholder.
  SchedulePaint();
}

void AstraBookmarksBarItemView::UpdateText() {
  if (!show_text_) {
    SetText(u"");
    return;
  }

  // Apply max width constraint via elision.
  // In a real implementation, we'd measure the text and elide.
  SetText(title_);
}

void AstraBookmarksBarItemView::UpdateColors() {
  // Colors would be updated from the color provider in production.
  SchedulePaint();
}

void AstraBookmarksBarItemView::OnButtonPressed() {
  if (delegate_) {
    delegate_->OnBookmarkClicked(bookmark_id_, false, false);
  }
}

gfx::Size AstraBookmarksBarItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = kMinWidth + 2 * kHorizontalPadding;
  if (show_icon_) {
    width += kIconSize;
    if (show_text_) {
      width += kIconTextSpacing;
    }
  }
  if (show_text_ && !title_.empty()) {
    // Estimate text width.
    // In production, we'd measure with the actual font.
    int text_width = static_cast<int>(title_.size()) * 7;  // rough estimate
    if (max_width_ > 0) {
      text_width = std::min(text_width, max_width_);
    }
    width += text_width;
  }

  int height = kDefaultHeight;
  return gfx::Size(width, height);
}

void AstraBookmarksBarItemView::OnThemeChanged() {
  views::LabelButton::OnThemeChanged();
  UpdateColors();
}

bool AstraBookmarksBarItemView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (event.IsOnlyMiddleMouseButton() && delegate_) {
    delegate_->OnBookmarkClicked(bookmark_id_, true, false);
    return true;
  }
  if (event.IsOnlyRightMouseButton() && delegate_) {
    delegate_->OnBookmarkRightClicked(bookmark_id_,
                                       event.location());
    return true;
  }
  if (event.IsOnlyLeftMouseButton()) {
    drag_start_point_ = event.location();
    is_dragging_ = false;
  }
  return views::LabelButton::OnMousePressed(event);
}

bool AstraBookmarksBarItemView::OnMouseDragged(
    const ui::MouseEvent& event) {
  if (!is_dragging_ && (event.flags() & ui::EF_LEFT_MOUSE_BUTTON)) {
    gfx::Vector2d delta = event.location() - drag_start_point_;
    // Start drag after a threshold.
    if (abs(delta.x()) > 5 || abs(delta.y()) > 5) {
      is_dragging_ = true;
      if (delegate_) {
        delegate_->OnBookmarkDragStarted(bookmark_id_, event);
      }
    }
  }
  return views::LabelButton::OnMouseDragged(event);
}

void AstraBookmarksBarItemView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    // Drag end would be handled by the drag-and-drop system.
  }
  views::LabelButton::OnMouseReleased(event);
}

void AstraBookmarksBarItemView::OnMouseEntered(
    const ui::MouseEvent& event) {
  is_hovered_ = true;
  views::LabelButton::OnMouseEntered(event);
}

void AstraBookmarksBarItemView::OnMouseExited(
    const ui::MouseEvent& event) {
  is_hovered_ = false;
  views::LabelButton::OnMouseExited(event);
}

void AstraBookmarksBarItemView::OnGestureEvent(
    ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP && delegate_) {
    delegate_->OnBookmarkClicked(bookmark_id_, false, false);
    event->SetHandled();
    return;
  }
  if (event->type() == ui::ET_GESTURE_LONG_PRESS && delegate_) {
    delegate_->OnBookmarkRightClicked(
        bookmark_id_, event->location());
    event->SetHandled();
    return;
  }
  views::LabelButton::OnGestureEvent(event);
}

void AstraBookmarksBarItemView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::LabelButton::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  node_data->SetName(title_);
  if (is_folder_) {
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kDescription, "Bookmark folder");
  } else {
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kDescription, "Bookmark");
  }
}

std::u16string AstraBookmarksBarItemView::GetTooltipText(
    const gfx::Point& p) const {
  return title_;
}

}  // namespace astra
