#include "astra/ui/views/newtab/astra_ntp_shortcut_view.h"

#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Default tile sizing (medium size).
constexpr int kTileWidth = 96;
constexpr int kTileHeight = 104;
constexpr int kIconSize = 48;
constexpr int kIconTopPadding = 14;
constexpr int kTitlePadding = 8;
constexpr int kTitleBottomPadding = 8;
constexpr int kTileCornerRadius = 12;
constexpr int kIconCornerRadius = 24;

// Small size.
constexpr int kSmallTileWidth = 72;
constexpr int kSmallTileHeight = 72;
constexpr int kSmallIconSize = 32;
constexpr int kSmallIconTopPadding = 10;

// Large size.
constexpr int kLargeTileWidth = 120;
constexpr int kLargeTileHeight = 128;
constexpr int kLargeIconSize = 64;
constexpr int kLargeIconTopPadding = 16;

// Remove button.
constexpr int kRemoveButtonSize = 20;
constexpr int kRemoveButtonTopInset = 4;
constexpr int kRemoveButtonRightInset = 4;

// Drag handle.
constexpr int kDragHandleWidth = 12;
constexpr int kDragHandleHeight = 20;
constexpr int kDragHandleLeftInset = 6;
constexpr int kDragHandleTopInset = 6;

// Title label sizing.
constexpr int kMaxTitleLines = 2;

// Shadow constants (hover state).
constexpr int kShadowBlurRadius = 8;
constexpr int kShadowYOffset = 2;

// Colors.
constexpr SkColor kTileBackgroundColor = SK_ColorWHITE;
constexpr SkColor kTileHoverBackgroundColor = SkColorSetRGB(0xFA, 0xFA, 0xFA);
constexpr SkColor kTilePressedBackgroundColor = SkColorSetRGB(0xF0, 0xF0, 0xF0);
constexpr SkColor kTileBorderColor = SkColorSetRGB(0xE0, 0xE0, 0xE0);
constexpr SkColor kTileHoverBorderColor = SkColorSetRGB(0xD0, 0xD0, 0xD0);
constexpr SkColor kIconBackgroundColor = SkColorSetRGB(0xF0, 0xF0, 0xF0);
constexpr SkColor kIconTextColor = SkColorSetRGB(0x66, 0x66, 0x66);
constexpr SkColor kTitleTextColor = SkColorSetRGB(0x33, 0x33, 0x33);
constexpr SkColor kTitlePlaceholderColor = SkColorSetRGB(0x99, 0x99, 0x99);
constexpr SkColor kFocusRingColor = SkColorSetRGB(0x5B, 0x8F, 0xF9);
constexpr SkColor kShadowColor = SkColorSetARGB(0x20, 0x00, 0x00, 0x00);
constexpr SkColor kRemoveButtonColor = SkColorSetRGB(0x99, 0x99, 0x99);
constexpr SkColor kRemoveButtonHoverColor = SkColorSetRGB(0x33, 0x33, 0x33);
constexpr SkColor kDragHandleColor = SkColorSetRGB(0xCC, 0xCC, 0xCC);
constexpr SkColor kDragHandleHoverColor = SkColorSetRGB(0x99, 0x99, 0x99);
constexpr SkColor kEditTextFieldBorderColor = SkColorSetRGB(0x5B, 0x8F, 0xF9);

// Focus ring.
constexpr int kFocusRingThickness = 2;
constexpr int kFocusRingOutset = 2;

// Drag threshold (pixels before drag starts).
constexpr int kDragThresholdPixels = 8;

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraNtpShortcutView::AstraNtpShortcutView()
    : size_(AstraNtpShortcutSize::kMedium) {
  BuildLayout();
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
}

AstraNtpShortcutView::AstraNtpShortcutView(AstraNtpShortcutSize size)
    : size_(size) {
  BuildLayout();
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
}

AstraNtpShortcutView::~AstraNtpShortcutView() = default;

// =========================================================================
// Layout construction
// =========================================================================

void AstraNtpShortcutView::BuildLayout() {
  // The tile uses a vertical layout with the icon on top and title below.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_inside_border_insets(
      gfx::Insets::TLBR(GetIconSize() / 2, 0, kTitleBottomPadding, 0));
  layout->set_between_child_spacing(kTitlePadding);

  // 1. Icon view (container for the icon + remove button + drag handle).
  icon_view_ = AddChildView(std::make_unique<views::View>());
  icon_view_->SetPreferredSize(gfx::Size(GetIconSize(), GetIconSize()));

  // Drag handle — left side of icon area.
  auto drag_handle = std::make_unique<views::View>();
  drag_handle->SetPreferredSize(
      gfx::Size(kDragHandleWidth, kDragHandleHeight));
  drag_handle->SetTooltipText(u"Drag to reorder");
  drag_handle->SetAccessibleName(u"Drag handle");
  drag_handle->SetVisible(false);
  drag_handle_ = drag_handle.get();
  icon_view_->AddChildView(std::move(drag_handle));

  // Remove button — top-right of the icon area.
  auto remove_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraNtpShortcutView::OnRemoveButtonPressed,
                          base::Unretained(this)));
  remove_button->SetPreferredSize(
      gfx::Size(kRemoveButtonSize, kRemoveButtonSize));
  remove_button->SetVisible(false);
  remove_button->SetTooltipText(u"Remove shortcut");
  remove_button->SetAccessibleName(u"Remove shortcut");
  remove_button_ = remove_button.get();
  icon_view_->AddChildView(std::move(remove_button));

  // 2. Title label below the icon (only for medium and large sizes).
  if (size_ != AstraNtpShortcutSize::kSmall) {
    title_label_ = AddChildView(std::make_unique<views::Label>());
    title_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    title_label_->SetAutoColorReadabilityEnabled(false);
    title_label_->SetEnabledColor(kTitleTextColor);
    title_label_->SetMultiLine(true);
    title_label_->SetMaxLines(kMaxTitleLines);
    title_label_->SetFontList(title_label_->font_list().Derive(
        -1, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
    title_label_->SetElideBehavior(gfx::ELIDE_TAIL);

    // Edit textfield (hidden by default, shown in edit mode).
    auto textfield = std::make_unique<views::Textfield>();
    textfield->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    textfield->SetBorder(views::CreateRoundedRectBorder(
        /*thickness=*/1, /*corner_radius=*/4, kEditTextFieldBorderColor));
    textfield->SetVisible(false);
    textfield->SetAccessibleName(u"Edit shortcut title");
    title_textfield_ = textfield.get();
    AddChildView(std::move(textfield));
  }

  // Set preferred size for the entire tile.
  SetPreferredSize(GetTileSize());

  UpdateVisualState();
}

// =========================================================================
// Data setters
// =========================================================================

void AstraNtpShortcutView::SetTitle(const std::u16string& title) {
  title_ = title;
  if (title_label_) {
    title_label_->SetText(title);
  }
  if (title_textfield_) {
    title_textfield_->SetText(title);
  }
  SetAccessibleName(title);
}

void AstraNtpShortcutView::SetURL(const GURL& url) {
  url_ = url;
  SetTooltipText(base::UTF8ToUTF16(url.spec()));
}

void AstraNtpShortcutView::SetIconURL(const GURL& icon_url) {
  icon_url_ = icon_url;
  UpdateVisualState();
}

void AstraNtpShortcutView::SetSize(AstraNtpShortcutSize size) {
  if (size_ == size) {
    return;
  }
  size_ = size;
  // Rebuild layout for the new size.
  RemoveAllChildViews();
  icon_view_ = nullptr;
  title_label_ = nullptr;
  title_textfield_ = nullptr;
  remove_button_ = nullptr;
  drag_handle_ = nullptr;
  BuildLayout();
  // Restore data.
  SetTitle(title_);
  SetURL(url_);
  SetIconURL(icon_url_);
  InvalidateLayout();
}

void AstraNtpShortcutView::SetShowDragHandle(bool show) {
  if (show_drag_handle_ == show) {
    return;
  }
  show_drag_handle_ = show;
  UpdateControlVisibility();
}

void AstraNtpShortcutView::SetEditMode(bool edit_mode) {
  if (is_edit_mode_ == edit_mode) {
    return;
  }
  is_edit_mode_ = edit_mode;

  if (title_label_ && title_textfield_) {
    title_label_->SetVisible(!edit_mode);
    title_textfield_->SetVisible(edit_mode);
    if (edit_mode) {
      title_textfield_->SetText(title_);
      title_textfield_->RequestFocus();
      title_textfield_->SelectAll(true);
    }
  }

  SchedulePaint();
}

// =========================================================================
// Callbacks
// =========================================================================

void AstraNtpShortcutView::SetClickCallback(ClickCallback callback) {
  click_callback_ = std::move(callback);
}

void AstraNtpShortcutView::SetRemoveCallback(RemoveCallback callback) {
  remove_callback_ = std::move(callback);
}

void AstraNtpShortcutView::SetContextMenuCallback(
    ContextMenuCallback callback) {
  context_menu_callback_ = std::move(callback);
}

void AstraNtpShortcutView::SetEditCallback(EditCallback callback) {
  edit_callback_ = std::move(callback);
}

void AstraNtpShortcutView::SetDragStartCallback(DragStartCallback callback) {
  drag_start_callback_ = std::move(callback);
}

// =========================================================================
// Mouse / keyboard event handling
// =========================================================================

bool AstraNtpShortcutView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    // Check if click is on the drag handle.
    if (drag_handle_ && drag_handle_->GetVisible() &&
        drag_handle_->HitTestPoint(event.location() -
                                   drag_handle_->origin())) {
      drag_start_point_ = event.location();
      is_pressed_ = true;
      UpdateVisualState();
      return true;
    }

    is_pressed_ = true;
    drag_start_point_ = event.location();
    UpdateVisualState();
    return true;
  }

  // Right click → context menu.
  if (event.IsOnlyRightMouseButton() && context_menu_callback_) {
    gfx::Point screen_point = event.location();
    ConvertPointToScreen(this, &screen_point);
    context_menu_callback_.Run(url_, screen_point);
    return true;
  }

  return views::View::OnMousePressed(event);
}

bool AstraNtpShortcutView::OnMouseDragged(const ui::MouseEvent& event) {
  if (is_pressed_ && !is_dragging_) {
    // Check if we've moved past the drag threshold.
    int dx = event.x() - drag_start_point_.x();
    int dy = event.y() - drag_start_point_.y();
    if (dx * dx + dy * dy >
        kDragThresholdPixels * kDragThresholdPixels) {
      StartDrag(event);
      return true;
    }
  }
  return views::View::OnMouseDragged(event);
}

void AstraNtpShortcutView::OnMouseReleased(const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    is_pressed_ = false;
    UpdateVisualState();
    return;
  }

  if (is_pressed_ && event.IsOnlyLeftMouseButton()) {
    is_pressed_ = false;
    // Check if the release is within the tile bounds (click, not drag-out).
    if (HitTestPoint(event.location())) {
      if (click_callback_) {
        click_callback_.Run(url_);
      }
    }
    UpdateVisualState();
  }
  views::View::OnMouseReleased(event);
}

void AstraNtpShortcutView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_ = true;
  UpdateVisualState();
  views::View::OnMouseEntered(event);
}

void AstraNtpShortcutView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_ = false;
  is_pressed_ = false;
  is_dragging_ = false;
  UpdateVisualState();
  views::View::OnMouseExited(event);
}

bool AstraNtpShortcutView::OnKeyPressed(const ui::KeyEvent& event) {
  // Space or Enter activates the shortcut (same as click).
  if (event.key_code() == ui::VKEY_SPACE ||
      event.key_code() == ui::VKEY_RETURN) {
    if (is_edit_mode_ && title_textfield_) {
      // In edit mode, Enter commits the edit.
      OnEditCommitted();
      return true;
    }
    if (click_callback_) {
      click_callback_.Run(url_);
    }
    return true;
  }

  // Escape cancels edit mode.
  if (event.key_code() == ui::VKEY_ESCAPE && is_edit_mode_) {
    SetEditMode(false);
    return true;
  }

  // Menu key or Shift+F10 shows context menu.
  if (event.key_code() == ui::VKEY_APPS ||
      (event.key_code() == ui::VKEY_F10 && event.IsShiftDown())) {
    if (context_menu_callback_) {
      gfx::Point center = GetLocalBounds().CenterPoint();
      ConvertPointToScreen(this, &center);
      context_menu_callback_.Run(url_, center);
    }
    return true;
  }

  // Delete or Backspace removes the shortcut.
  if (event.key_code() == ui::VKEY_DELETE ||
      event.key_code() == ui::VKEY_BACK) {
    if (!is_edit_mode_ && remove_callback_) {
      remove_callback_.Run(url_);
    }
    return true;
  }

  // F2 enters edit mode.
  if (event.key_code() == ui::VKEY_F2 && !is_edit_mode_) {
    SetEditMode(true);
    return true;
  }

  return views::View::OnKeyPressed(event);
}

bool AstraNtpShortcutView::OnKeyReleased(const ui::KeyEvent& event) {
  return views::View::OnKeyReleased(event);
}

void AstraNtpShortcutView::OnFocus() {
  is_focused_ = true;
  UpdateVisualState();
  views::View::OnFocus();
}

void AstraNtpShortcutView::OnBlur() {
  is_focused_ = false;
  is_pressed_ = false;
  if (is_edit_mode_) {
    SetEditMode(false);
  }
  UpdateVisualState();
  views::View::OnBlur();
}

// =========================================================================
// Gesture handling
// =========================================================================

bool AstraNtpShortcutView::OnGestureEvent(ui::GestureEvent* event) {
  switch (event->type()) {
    case ui::ET_GESTURE_TAP:
      if (click_callback_) {
        click_callback_.Run(url_);
      }
      return true;
    case ui::ET_GESTURE_LONG_PRESS:
      // Long press enters edit mode / shows context menu.
      if (context_menu_callback_) {
        gfx::Point screen_point = event->location();
        ConvertPointToScreen(this, &screen_point);
        context_menu_callback_.Run(url_, screen_point);
      }
      return true;
    default:
      break;
  }
  return views::View::OnGestureEvent(event);
}

// =========================================================================
// Painting
// =========================================================================

void AstraNtpShortcutView::OnPaintBackground(gfx::Canvas* canvas) {
  // Determine background color based on state.
  SkColor bg_color = kTileBackgroundColor;
  if (is_pressed_) {
    bg_color = kTilePressedBackgroundColor;
  } else if (is_hovered_) {
    bg_color = kTileHoverBackgroundColor;
  }

  // Draw drop shadow on hover.
  if (is_hovered_ || is_dragging_) {
    cc::PaintFlags shadow_flags;
    shadow_flags.setColor(kShadowColor);
    shadow_flags.setStyle(cc::PaintFlags::kFill_Style);
    shadow_flags.setAntiAlias(true);
    gfx::RectF shadow_rect(GetLocalBounds());
    shadow_rect.Offset(0, kShadowYOffset);
    if (is_dragging_) {
      shadow_rect.Offset(0, 2);  // More shadow when dragging
    }
    canvas->DrawRoundRect(shadow_rect, kTileCornerRadius, shadow_flags);
  }

  // Draw rounded rectangle background for the tile.
  cc::PaintFlags flags;
  flags.setColor(bg_color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  // Semi-transparent when dragging.
  if (is_dragging_) {
    flags.setAlpha(180);
  }

  canvas->DrawRoundRect(GetLocalBounds(), kTileCornerRadius, flags);

  // Draw subtle border.
  SkColor border_color =
      is_hovered_ ? kTileHoverBorderColor : kTileBorderColor;
  cc::PaintFlags border_flags;
  border_flags.setColor(border_color);
  border_flags.setStyle(cc::PaintFlags::kStroke_Style);
  border_flags.setStrokeWidth(1);
  border_flags.setAntiAlias(true);
  gfx::RectF border_rect(GetLocalBounds());
  border_rect.Inset(0.5f);
  canvas->DrawRoundRect(border_rect, kTileCornerRadius, border_flags);

  // Draw the icon (behind children, so remove button appears on top).
  if (icon_view_ && icon_url_.is_empty()) {
    PaintIcon(canvas);
  }

  // Draw drag handle if visible.
  if (drag_handle_ && drag_handle_->GetVisible()) {
    PaintDragHandle(canvas);
  }
}

void AstraNtpShortcutView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  // Paint focus ring on top of everything.
  if (is_focused_) {
    PaintFocusRing(canvas);
  }
}

void AstraNtpShortcutView::PaintIcon(gfx::Canvas* canvas) {
  // Calculate icon bounds (same as icon_view_ bounds within the tile).
  gfx::Rect icon_bounds = icon_view_->bounds();

  // Draw a rounded square / circle background for the icon.
  cc::PaintFlags icon_flags;
  icon_flags.setColor(kIconBackgroundColor);
  icon_flags.setStyle(cc::PaintFlags::kFill_Style);
  icon_flags.setAntiAlias(true);
  canvas->DrawRoundRect(icon_bounds, kIconCornerRadius, icon_flags);

  // Draw first letter of the title as a placeholder.
  if (!title_.empty()) {
    std::u16string first_char(1, title_[0]);
    gfx::FontList font_list;
    int font_size_delta = (size_ == AstraNtpShortcutSize::kLarge) ? 6 : 4;
    canvas->DrawStringRect(
        first_char,
        font_list.Derive(font_size_delta, gfx::Font::NORMAL,
                         gfx::Font::Weight::MEDIUM),
        kIconTextColor, icon_bounds, gfx::ALIGN_CENTER, gfx::ALIGN_MIDDLE);
  }
}

void AstraNtpShortcutView::PaintFocusRing(gfx::Canvas* canvas) {
  cc::PaintFlags focus_flags;
  focus_flags.setColor(kFocusRingColor);
  focus_flags.setStyle(cc::PaintFlags::kStroke_Style);
  focus_flags.setStrokeWidth(kFocusRingThickness);
  focus_flags.setAntiAlias(true);

  gfx::RectF focus_rect(GetLocalBounds());
  focus_rect.Inset(-kFocusRingOutset + kFocusRingThickness / 2.0f,
                   -kFocusRingOutset + kFocusRingThickness / 2.0f);
  canvas->DrawRoundRect(focus_rect, kTileCornerRadius + kFocusRingOutset,
                        focus_flags);
}

void AstraNtpShortcutView::PaintDragHandle(gfx::Canvas* canvas) {
  // Draw three horizontal dots as the drag handle.
  gfx::Rect handle_bounds = drag_handle_->bounds();
  gfx::Point center = handle_bounds.CenterPoint();

  SkColor dot_color =
      is_hovered_ ? kDragHandleHoverColor : kDragHandleColor;

  cc::PaintFlags dot_flags;
  dot_flags.setColor(dot_color);
  dot_flags.setStyle(cc::PaintFlags::kFill_Style);
  dot_flags.setAntiAlias(true);

  int dot_radius = 2;
  int dot_spacing = 4;

  // Three horizontal dots.
  canvas->DrawCircle(
      gfx::Point(center.x(), center.y() - dot_spacing),
      dot_radius, dot_flags);
  canvas->DrawCircle(
      gfx::Point(center.x(), center.y()),
      dot_radius, dot_flags);
  canvas->DrawCircle(
      gfx::Point(center.x(), center.y() + dot_spacing),
      dot_radius, dot_flags);
}

void AstraNtpShortcutView::OnThemeChanged() {
  views::View::OnThemeChanged();
  // TODO(astra): Read colors from ColorProvider when Astra has its own
  // color mixin.
}

void AstraNtpShortcutView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  if (!title_.empty()) {
    node_data->SetName(base::UTF16ToUTF8(title_));
  } else if (url_.is_valid()) {
    node_data->SetName(url_.spec());
  }
  if (url_.is_valid()) {
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kDescription, url_.spec());
  }
  if (show_drag_handle_) {
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kDescription,
        "Draggable shortcut: " + base::UTF16ToUTF8(title_));
  }
}

// =========================================================================
// Layout
// =========================================================================

void AstraNtpShortcutView::Layout() {
  views::View::Layout();

  // Position the drag handle in the top-left of the icon area.
  if (drag_handle_ && icon_view_) {
    gfx::Size handle_size = drag_handle_->GetPreferredSize();
    int x = kDragHandleLeftInset;
    int y = kDragHandleTopInset;
    drag_handle_->SetPosition(gfx::Point(x, y));
  }

  // Position the remove button in the top-right of the icon area.
  if (remove_button_ && icon_view_) {
    gfx::Size button_size = remove_button_->GetPreferredSize();
    int x = icon_view_->width() - button_size.width() - kRemoveButtonRightInset;
    int y = kRemoveButtonTopInset;
    remove_button_->SetPosition(gfx::Point(x, y));
  }
}

gfx::Size AstraNtpShortcutView::CalculatePreferredSize() const {
  return GetTileSize();
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraNtpShortcutView::UpdateVisualState() {
  UpdateControlVisibility();
  SchedulePaint();
}

void AstraNtpShortcutView::UpdateControlVisibility() {
  // Remove button: visible on hover or focus.
  if (remove_button_) {
    bool should_show = is_hovered_ || is_focused_;
    if (remove_button_->GetVisible() != should_show) {
      remove_button_->SetVisible(should_show);
    }
  }

  // Drag handle: visible when drag handles are enabled AND hover/focus.
  if (drag_handle_) {
    bool should_show = show_drag_handle_ && (is_hovered_ || is_focused_);
    if (drag_handle_->GetVisible() != should_show) {
      drag_handle_->SetVisible(should_show);
    }
  }
}

void AstraNtpShortcutView::OnRemoveButtonPressed() {
  if (remove_callback_) {
    remove_callback_.Run(url_);
  }
}

void AstraNtpShortcutView::OnEditCommitted() {
  if (title_textfield_) {
    std::u16string new_title = title_textfield_->GetText();
    if (!new_title.empty()) {
      SetTitle(new_title);
    }
  }
  SetEditMode(false);
  if (edit_callback_) {
    edit_callback_.Run(url_);
  }
}

void AstraNtpShortcutView::ShowContextMenu(const gfx::Point& screen_point) {
  if (context_menu_callback_) {
    context_menu_callback_.Run(url_, screen_point);
  }
}

void AstraNtpShortcutView::StartDrag(const ui::LocatedEvent& event) {
  is_dragging_ = true;
  UpdateVisualState();
  if (drag_start_callback_) {
    drag_start_callback_.Run(this);
  }
}

int AstraNtpShortcutView::GetIconSize() const {
  switch (size_) {
    case AstraNtpShortcutSize::kSmall:
      return kSmallIconSize;
    case AstraNtpShortcutSize::kMedium:
      return kIconSize;
    case AstraNtpShortcutSize::kLarge:
      return kLargeIconSize;
  }
  return kIconSize;
}

gfx::Size AstraNtpShortcutView::GetTileSize() const {
  switch (size_) {
    case AstraNtpShortcutSize::kSmall:
      return gfx::Size(kSmallTileWidth, kSmallTileHeight);
    case AstraNtpShortcutSize::kMedium:
      return gfx::Size(kTileWidth, kTileHeight);
    case AstraNtpShortcutSize::kLarge:
      return gfx::Size(kLargeTileWidth, kLargeTileHeight);
  }
  return gfx::Size(kTileWidth, kTileHeight);
}

}  // namespace astra
