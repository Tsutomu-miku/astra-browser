#include "astra/ui/views/newtab/astra_ntp_workspace_card.h"

#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Card dimensions.
constexpr int kCardWidth = 200;
constexpr int kCardHeight = 88;
constexpr int kCardCornerRadius = 10;
constexpr int kAccentHeaderHeight = 4;
constexpr int kCardHPadding = 14;
constexpr int kCardVPadding = 12;
constexpr int kTextSpacing = 4;
constexpr int kMenuButtonSize = 20;

// Drag handle.
constexpr int kDragHandleWidth = 10;
constexpr int kDragHandleHeight = 18;
constexpr int kDragHandleLeftInset = 6;
constexpr int kDragHandleTopInset = 10;

// Tab count badge.
constexpr int kBadgeHPadding = 8;
constexpr int kBadgeVPadding = 2;
constexpr int kBadgeCornerRadius = 10;
constexpr int kBadgeFontSizeDelta = -2;

// New workspace card dimensions.
constexpr int kNewCardIconSize = 28;
constexpr int kNewCardSpacing = 8;

// Colors.
constexpr SkColor kCardBackgroundColor = SK_ColorWHITE;
constexpr SkColor kCardHoverBackgroundColor = SkColorSetRGB(0xFA, 0xFA, 0xFA);
constexpr SkColor kCardPressedBackgroundColor = SkColorSetRGB(0xF0, 0xF0, 0xF0);
constexpr SkColor kCardBorderColor = SkColorSetRGB(0xE0, 0xE0, 0xE0);
constexpr SkColor kCardHoverBorderColor = SkColorSetRGB(0xD0, 0xD0, 0xD0);
constexpr SkColor kActiveCardBorderColor = SkColorSetRGB(0x5B, 0x8F, 0xF9);
constexpr SkColor kNameTextColor = SkColorSetRGB(0x33, 0x33, 0x33);
constexpr SkColor kTabCountTextColor = SkColorSetRGB(0x99, 0x99, 0x99);
constexpr SkColor kMenuButtonColor = SkColorSetRGB(0x99, 0x99, 0x99);
constexpr SkColor kMenuButtonHoverColor = SkColorSetRGB(0x33, 0x33, 0x33);
constexpr SkColor kNewCardTextColor = SkColorSetRGB(0x5B, 0x8F, 0xF9);
constexpr SkColor kShadowColor = SkColorSetARGB(0x18, 0x00, 0x00, 0x00);
constexpr SkColor kFocusRingColor = SkColorSetRGB(0x5B, 0x8F, 0xF9);
constexpr SkColor kBadgeBackgroundColor = SkColorSetRGB(0xF0, 0xF4, 0xFF);
constexpr SkColor kBadgeTextColor = SkColorSetRGB(0x5B, 0x8F, 0xF9);
constexpr SkColor kDragHandleColor = SkColorSetRGB(0xCC, 0xCC, 0xCC);
constexpr SkColor kDragHandleHoverColor = SkColorSetRGB(0x99, 0x99, 0x99);

// Focus ring.
constexpr int kFocusRingThickness = 2;
constexpr int kFocusRingOutset = 2;

// Shadow.
constexpr int kShadowYOffset = 1;
constexpr int kShadowBlurRadius = 6;

// Default accent color (blue).
constexpr SkColor kDefaultAccentColor = SkColorSetRGB(0x5B, 0x8F, 0xF9);

// Drag threshold.
constexpr int kDragThresholdPixels = 8;

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraNtpWorkspaceCard::AstraNtpWorkspaceCard() {
  BuildLayout();
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
}

AstraNtpWorkspaceCard::~AstraNtpWorkspaceCard() = default;

// =========================================================================
// Layout construction
// =========================================================================

void AstraNtpWorkspaceCard::BuildLayout() {
  // Layout: accent header bar on top, then content area below.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // ---- Accent color header bar ----
  accent_header_ = AddChildView(std::make_unique<views::View>());
  accent_header_->SetPaintToLayer();
  accent_header_->layer()->SetFillsBoundsOpaquely(true);
  accent_header_->SetPreferredSize(gfx::Size(0, kAccentHeaderHeight));
  accent_header_->layer()->SetColor(kDefaultAccentColor);

  // ---- Content area ----
  auto* content = AddChildView(std::make_unique<views::View>());
  content->SetPaintToLayer();
  content->layer()->SetFillsBoundsOpaquely(false);
  auto* content_layout = content->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kCardVPadding, kCardHPadding),
          /*between_child_spacing=*/0));
  content_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  layout->SetFlexForView(content, 1);

  // Drag handle (left side).
  auto drag_handle = std::make_unique<views::View>();
  drag_handle->SetPreferredSize(
      gfx::Size(kDragHandleWidth, kDragHandleHeight));
  drag_handle->SetTooltipText(u"Drag to reorder");
  drag_handle->SetAccessibleName(u"Drag handle");
  drag_handle->SetVisible(false);
  drag_handle_ = drag_handle.get();
  content->AddChildView(std::move(drag_handle));

  // Text column: workspace name + tab count badge.
  auto* text_column = content->AddChildView(std::make_unique<views::View>());
  auto* text_layout = text_column->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  text_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  text_layout->set_between_child_spacing(kTextSpacing);
  text_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  content_layout->SetFlexForView(text_column, 1);

  name_label_ = text_column->AddChildView(std::make_unique<views::Label>());
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetEnabledColor(kNameTextColor);
  name_label_->SetFontList(name_label_->font_list().Derive(
      0, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Tab count badge (pill-shaped background with text).
  auto badge = std::make_unique<views::View>();
  badge->SetPaintToLayer();
  badge->layer()->SetFillsBoundsOpaquely(false);
  badge->SetVisible(true);
  tab_count_badge_ = badge.get();
  text_column->AddChildView(std::move(badge));

  // Tab count label inside the badge.
  tab_count_label_ =
      tab_count_badge_->AddChildView(std::make_unique<views::Label>());
  tab_count_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  tab_count_label_->SetAutoColorReadabilityEnabled(false);
  tab_count_label_->SetEnabledColor(kBadgeTextColor);
  tab_count_label_->SetFontList(tab_count_label_->font_list().Derive(
      kBadgeFontSizeDelta, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));

  // Menu button (⋮) — right side.
  auto menu_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraNtpWorkspaceCard::OnMenuButtonPressed,
                          base::Unretained(this)));
  menu_button->SetPreferredSize(
      gfx::Size(kMenuButtonSize, kMenuButtonSize));
  menu_button->SetVisible(false);
  menu_button->SetTooltipText(u"Workspace actions");
  menu_button->SetAccessibleName(u"Workspace actions menu");
  menu_button_ = menu_button.get();
  content->AddChildView(std::move(menu_button));

  SetPreferredSize(gfx::Size(kCardWidth, kCardHeight));
  accent_color_ = kDefaultAccentColor;

  UpdateVisualState();
}

void AstraNtpWorkspaceCard::BuildNewWorkspaceLayout() {
  // "New workspace" card has a centered plus icon and label.
  RemoveAllChildViews();

  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(20, kCardHPadding),
      kNewCardSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);

  // Plus icon view.
  new_card_icon_ = AddChildView(std::make_unique<views::View>());
  new_card_icon_->SetPaintToLayer();
  new_card_icon_->layer()->SetFillsBoundsOpaquely(false);
  new_card_icon_->SetPreferredSize(
      gfx::Size(kNewCardIconSize, kNewCardIconSize));

  // "New workspace" label.
  new_card_label_ = AddChildView(std::make_unique<views::Label>());
  new_card_label_->SetText(u"New workspace");
  new_card_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  new_card_label_->SetAutoColorReadabilityEnabled(false);
  new_card_label_->SetEnabledColor(kNewCardTextColor);
  new_card_label_->SetFontList(new_card_label_->font_list().Derive(
      0, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));

  // Clear references to regular card views.
  accent_header_ = nullptr;
  name_label_ = nullptr;
  tab_count_label_ = nullptr;
  tab_count_badge_ = nullptr;
  menu_button_ = nullptr;
  drag_handle_ = nullptr;

  SetPreferredSize(gfx::Size(kCardWidth, kCardHeight));
  UpdateVisualState();
}

// =========================================================================
// Data setters
// =========================================================================

void AstraNtpWorkspaceCard::SetWorkspaceId(const std::string& id) {
  workspace_id_ = id;
}

void AstraNtpWorkspaceCard::SetWorkspaceName(const std::u16string& name) {
  workspace_name_ = name;
  if (name_label_) {
    name_label_->SetText(name);
  }
  SetAccessibleName(name);
}

void AstraNtpWorkspaceCard::SetAccentColor(const std::string& color_hex) {
  accent_color_hex_ = color_hex;
  UpdateAccentColor();
}

void AstraNtpWorkspaceCard::SetTabCount(int tab_count) {
  tab_count_ = tab_count;
  UpdateTabCountBadge();
}

void AstraNtpWorkspaceCard::SetIsActive(bool is_active) {
  if (is_active_ == is_active) {
    return;
  }
  is_active_ = is_active;
  UpdateVisualState();
}

void AstraNtpWorkspaceCard::SetIsNewWorkspaceCard(bool is_new_card) {
  if (is_new_workspace_card_ == is_new_card) {
    return;
  }
  is_new_workspace_card_ = is_new_card;

  if (is_new_card) {
    BuildNewWorkspaceLayout();
    workspace_name_ = u"New workspace";
    SetAccessibleName(u"Create new workspace");
  } else {
    BuildLayout();
    if (!workspace_name_.empty() && name_label_) {
      name_label_->SetText(workspace_name_);
    }
    UpdateTabCountBadge();
    SetAccessibleName(workspace_name_);
  }

  UpdateAccentColor();
  UpdateVisualState();
}

void AstraNtpWorkspaceCard::SetShowDragHandle(bool show) {
  if (show_drag_handle_ == show) {
    return;
  }
  show_drag_handle_ = show;
  UpdateControlVisibility();
}

// =========================================================================
// Callbacks
// =========================================================================

void AstraNtpWorkspaceCard::SetClickCallback(ClickCallback callback) {
  click_callback_ = std::move(callback);
}

void AstraNtpWorkspaceCard::SetMenuCallback(MenuCallback callback) {
  menu_callback_ = std::move(callback);
}

void AstraNtpWorkspaceCard::SetDragStartCallback(DragStartCallback callback) {
  drag_start_callback_ = std::move(callback);
}

// =========================================================================
// Event handling
// =========================================================================

bool AstraNtpWorkspaceCard::OnMousePressed(const ui::MouseEvent& event) {
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

    // Check if click is on the menu button.
    if (menu_button_ && menu_button_->GetVisible() &&
        menu_button_->HitTestPoint(event.location() -
                                   menu_button_->origin())) {
      // Menu button handles its own click via its callback.
      return views::View::OnMousePressed(event);
    }

    is_pressed_ = true;
    drag_start_point_ = event.location();
    UpdateVisualState();
    return true;
  }

  // Right click → context menu (only for regular cards).
  if (event.IsOnlyRightMouseButton() && menu_callback_ &&
      !is_new_workspace_card_) {
    gfx::Point screen_point = event.location();
    ConvertPointToScreen(this, &screen_point);
    menu_callback_.Run(workspace_id_, screen_point);
    return true;
  }

  return views::View::OnMousePressed(event);
}

bool AstraNtpWorkspaceCard::OnMouseDragged(const ui::MouseEvent& event) {
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

void AstraNtpWorkspaceCard::OnMouseReleased(const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    is_pressed_ = false;
    UpdateVisualState();
    return;
  }

  if (is_pressed_ && event.IsOnlyLeftMouseButton()) {
    is_pressed_ = false;
    if (HitTestPoint(event.location())) {
      if (click_callback_) {
        click_callback_.Run(workspace_id_);
      }
    }
    UpdateVisualState();
  }
  views::View::OnMouseReleased(event);
}

void AstraNtpWorkspaceCard::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_ = true;
  UpdateVisualState();
  views::View::OnMouseEntered(event);
}

void AstraNtpWorkspaceCard::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_ = false;
  is_pressed_ = false;
  is_dragging_ = false;
  UpdateVisualState();
  views::View::OnMouseExited(event);
}

bool AstraNtpWorkspaceCard::OnKeyPressed(const ui::KeyEvent& event) {
  // Space or Enter activates the card (same as click).
  if (event.key_code() == ui::VKEY_SPACE ||
      event.key_code() == ui::VKEY_RETURN) {
    if (click_callback_) {
      click_callback_.Run(workspace_id_);
    }
    return true;
  }
  // Menu key or Shift+F10 shows the context menu.
  if ((event.key_code() == ui::VKEY_APPS ||
       (event.key_code() == ui::VKEY_F10 && event.IsShiftDown())) &&
      menu_callback_ && !is_new_workspace_card_) {
    gfx::Point center = GetLocalBounds().CenterPoint();
    ConvertPointToScreen(this, &center);
    menu_callback_.Run(workspace_id_, center);
    return true;
  }
  return views::View::OnKeyPressed(event);
}

void AstraNtpWorkspaceCard::OnFocus() {
  is_focused_ = true;
  UpdateVisualState();
  views::View::OnFocus();
}

void AstraNtpWorkspaceCard::OnBlur() {
  is_focused_ = false;
  is_pressed_ = false;
  UpdateVisualState();
  views::View::OnBlur();
}

// =========================================================================
// Painting
// =========================================================================

void AstraNtpWorkspaceCard::OnPaintBackground(gfx::Canvas* canvas) {
  // Determine background color based on state.
  SkColor bg_color = kCardBackgroundColor;
  if (is_pressed_) {
    bg_color = kCardPressedBackgroundColor;
  } else if (is_hovered_) {
    bg_color = kCardHoverBackgroundColor;
  }

  // For "new workspace" card, use a lighter background.
  if (is_new_workspace_card_) {
    bg_color = is_hovered_ ? kCardHoverBackgroundColor : kCardBackgroundColor;
  }

  // Draw subtle shadow on hover.
  if (is_hovered_ || is_dragging_) {
    cc::PaintFlags shadow_flags;
    shadow_flags.setColor(kShadowColor);
    shadow_flags.setStyle(cc::PaintFlags::kFill_Style);
    shadow_flags.setAntiAlias(true);
    gfx::RectF shadow_rect(GetLocalBounds());
    shadow_rect.Offset(0, kShadowYOffset);
    if (is_dragging_) {
      shadow_rect.Offset(0, 2);
    }
    canvas->DrawRoundRect(shadow_rect, kCardCornerRadius, shadow_flags);
  }

  // Draw rounded rectangle background.
  cc::PaintFlags flags;
  flags.setColor(bg_color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  if (is_dragging_) {
    flags.setAlpha(180);
  }
  canvas->DrawRoundRect(GetLocalBounds(), kCardCornerRadius, flags);

  // Draw border.
  SkColor border_color = kCardBorderColor;
  int border_width = 1;
  if (is_active_) {
    border_color = kActiveCardBorderColor;
    border_width = 2;
  } else if (is_hovered_) {
    border_color = kCardHoverBorderColor;
  }

  cc::PaintFlags border_flags;
  border_flags.setColor(border_color);
  border_flags.setStyle(cc::PaintFlags::kStroke_Style);
  border_flags.setStrokeWidth(border_width);
  border_flags.setAntiAlias(true);
  gfx::RectF border_rect(GetLocalBounds());
  border_rect.Inset(border_width / 2.0f);
  canvas->DrawRoundRect(border_rect, kCardCornerRadius, border_flags);

  // Draw drag handle if visible.
  if (drag_handle_ && drag_handle_->GetVisible()) {
    PaintDragHandle(canvas);
  }
}

void AstraNtpWorkspaceCard::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  // For new workspace card, paint the plus icon.
  if (is_new_workspace_card_ && new_card_icon_) {
    PaintNewWorkspaceIcon(canvas);
  }

  // Paint focus ring on top of everything.
  if (is_focused_) {
    PaintFocusRing(canvas);
  }

  // Paint menu button dots (⋮) if visible.
  if (menu_button_ && menu_button_->GetVisible() && !is_new_workspace_card_) {
    gfx::Rect menu_bounds = menu_button_->bounds();
    gfx::Point center = menu_bounds.CenterPoint();
    SkColor dot_color = is_hovered_ ? kMenuButtonHoverColor : kMenuButtonColor;

    cc::PaintFlags dot_flags;
    dot_flags.setColor(dot_color);
    dot_flags.setStyle(cc::PaintFlags::kFill_Style);
    dot_flags.setAntiAlias(true);

    int dot_radius = 2;
    int dot_spacing = 4;
    canvas->DrawCircle(gfx::Point(center.x(), center.y() - dot_spacing),
                       dot_radius, dot_flags);
    canvas->DrawCircle(gfx::Point(center.x(), center.y()),
                       dot_radius, dot_flags);
    canvas->DrawCircle(gfx::Point(center.x(), center.y() + dot_spacing),
                       dot_radius, dot_flags);
  }
}

void AstraNtpWorkspaceCard::PaintAccentHeader(gfx::Canvas* canvas) {
  // The accent header is drawn by the accent_header_ view's layer.
}

void AstraNtpWorkspaceCard::PaintFocusRing(gfx::Canvas* canvas) {
  cc::PaintFlags focus_flags;
  focus_flags.setColor(kFocusRingColor);
  focus_flags.setStyle(cc::PaintFlags::kStroke_Style);
  focus_flags.setStrokeWidth(kFocusRingThickness);
  focus_flags.setAntiAlias(true);

  gfx::RectF focus_rect(GetLocalBounds());
  focus_rect.Inset(-kFocusRingOutset + kFocusRingThickness / 2.0f,
                   -kFocusRingOutset + kFocusRingThickness / 2.0f);
  canvas->DrawRoundRect(focus_rect, kCardCornerRadius + kFocusRingOutset,
                        focus_flags);
}

void AstraNtpWorkspaceCard::PaintNewWorkspaceIcon(gfx::Canvas* canvas) {
  gfx::Rect icon_bounds = new_card_icon_->bounds();
  gfx::Point center = icon_bounds.CenterPoint();
  int line_length = 10;
  int line_thickness = 2;

  cc::PaintFlags plus_flags;
  plus_flags.setColor(accent_color_ != SK_ColorTRANSPARENT
                          ? accent_color_
                          : kDefaultAccentColor);
  plus_flags.setStyle(cc::PaintFlags::kFill_Style);
  plus_flags.setAntiAlias(true);

  // Horizontal line.
  gfx::RectF h_line(center.x() - line_length, center.y() - line_thickness / 2.0f,
                    line_length * 2, line_thickness);
  canvas->DrawRoundRect(h_line, line_thickness / 2.0f, plus_flags);

  // Vertical line.
  gfx::RectF v_line(center.x() - line_thickness / 2.0f, center.y() - line_length,
                    line_thickness, line_length * 2);
  canvas->DrawRoundRect(v_line, line_thickness / 2.0f, plus_flags);
}

void AstraNtpWorkspaceCard::PaintDragHandle(gfx::Canvas* canvas) {
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
  int dot_spacing = 3;

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

void AstraNtpWorkspaceCard::OnThemeChanged() {
  views::View::OnThemeChanged();
  // TODO(astra): Read colors from ColorProvider.
}

void AstraNtpWorkspaceCard::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  if (!workspace_name_.empty()) {
    node_data->SetName(base::UTF16ToUTF8(workspace_name_));
  }
  if (!is_new_workspace_card_) {
    node_data->AddIntAttribute(
        ax::mojom::IntAttribute::kHasPopup,
        static_cast<int>(ax::mojom::HasPopup::kMenu));
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kDescription,
        base::NumberToString(tab_count_) + " tabs");
  }
  if (show_drag_handle_) {
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kDescription,
        "Draggable workspace card: " + base::UTF16ToUTF8(workspace_name_));
  }
}

// =========================================================================
// Layout
// =========================================================================

void AstraNtpWorkspaceCard::Layout() {
  views::View::Layout();

  // Position menu button.
  if (menu_button_ && !is_new_workspace_card_) {
    gfx::Size button_size = menu_button_->GetPreferredSize();
    // The menu button is in the content area — layout is handled by BoxLayout.
  }

  // Position tab count badge.
  if (tab_count_badge_ && tab_count_label_) {
    gfx::Size label_size = tab_count_label_->GetPreferredSize();
    int badge_width = label_size.width() + kBadgeHPadding * 2;
    int badge_height = label_size.height() + kBadgeVPadding * 2;
    tab_count_badge_->SetSize(gfx::Size(badge_width, badge_height));
    tab_count_label_->SetBounds(
        kBadgeHPadding, kBadgeVPadding,
        label_size.width(), label_size.height());
  }
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraNtpWorkspaceCard::UpdateVisualState() {
  UpdateControlVisibility();
  UpdateAccentColor();
  SchedulePaint();
}

void AstraNtpWorkspaceCard::UpdateAccentColor() {
  SkColor color = kDefaultAccentColor;
  if (!accent_color_hex_.empty() && accent_color_hex_.size() == 7 &&
      accent_color_hex_[0] == '#') {
    unsigned int color_val = 0;
    if (base::HexStringToUInt(accent_color_hex_.substr(1), &color_val)) {
      color = SkColorSetRGB(SkColorGetR(color_val), SkColorGetG(color_val),
                            SkColorGetB(color_val));
    }
  }
  accent_color_ = color;

  // Update accent header bar color (regular card only).
  if (accent_header_ && accent_header_->layer()) {
    accent_header_->layer()->SetColor(color);
  }

  // Update new card label color (new workspace card only).
  if (new_card_label_) {
    new_card_label_->SetEnabledColor(color);
  }
}

void AstraNtpWorkspaceCard::UpdateControlVisibility() {
  // Menu button: visible on hover or focus (regular card only).
  if (menu_button_ && !is_new_workspace_card_) {
    bool should_show = is_hovered_ || is_focused_;
    if (menu_button_->GetVisible() != should_show) {
      menu_button_->SetVisible(should_show);
    }
  }

  // Drag handle: visible when enabled AND (hover or focus).
  if (drag_handle_ && !is_new_workspace_card_) {
    bool should_show = show_drag_handle_ && (is_hovered_ || is_focused_);
    if (drag_handle_->GetVisible() != should_show) {
      drag_handle_->SetVisible(should_show);
    }
  }
}

void AstraNtpWorkspaceCard::UpdateTabCountBadge() {
  if (!tab_count_label_) {
    return;
  }

  std::u16string count_text = base::NumberToString16(tab_count_);
  if (tab_count_ == 1) {
    count_text += u" tab";
  } else {
    count_text += u" tabs";
  }
  tab_count_label_->SetText(count_text);

  if (tab_count_badge_) {
    tab_count_badge_->SchedulePaint();
  }
}

void AstraNtpWorkspaceCard::OnMenuButtonPressed() {
  if (menu_callback_) {
    gfx::Point screen_point = menu_button_->GetMirroredBounds().CenterPoint();
    views::View::ConvertPointToScreen(menu_button_, &screen_point);
    menu_callback_.Run(workspace_id_, screen_point);
  }
}

void AstraNtpWorkspaceCard::StartDrag(const ui::LocatedEvent& event) {
  is_dragging_ = true;
  UpdateVisualState();
  if (drag_start_callback_) {
    drag_start_callback_.Run(this);
  }
}

}  // namespace astra
