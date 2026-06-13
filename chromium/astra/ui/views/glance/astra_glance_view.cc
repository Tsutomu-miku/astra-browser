#include "astra/ui/views/glance/astra_glance_view.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/ui/color/astra_color_ids.h"
#include "content/public/browser/web_contents.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/throbber.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Margins and insets.
constexpr int kHeaderHorizontalInsets = 12;
constexpr int kHeaderVerticalInsets = 8;
constexpr int kStatusBarHorizontalInsets = 12;
constexpr int kActionBarHorizontalInsets = 8;
constexpr int kActionBarVerticalInsets = 6;
constexpr int kActionButtonSpacing = 4;

// Rounded corner radius for the glance bubble.
constexpr int kGlanceCornerRadius = 12;

// The size of action bar buttons.
constexpr gfx::Size kActionBarButtonSize{36, 32};

// The size of the header close button.
constexpr gfx::Size kHeaderCloseButtonSize{24, 24};

// The size of the favicon.
constexpr gfx::Size kFaviconSize{16, 16};

}  // namespace

// =========================================================================
// Static factory
// =========================================================================

views::Widget* AstraGlanceView::ShowBubble(views::View* anchor_view,
                                            const gfx::Rect& anchor_rect,
                                            Delegate* delegate) {
  DCHECK(anchor_view);
  DCHECK(delegate);

  // Create the bubble delegate.  BubbleDialogDelegateView creates its own
  // widget when shown via CreateBubble().  The widget owns the delegate
  // and deletes it when closed.
  auto* bubble = new AstraGlanceView(anchor_view, anchor_rect, delegate);

  // Create the bubble widget.  Ownership transfers to the widget system.
  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(bubble);

  // Arrow position: the glance typically appears to the right of the sidebar
  // item or below a link.  We use LEFT_TOP so the arrow points to the
  // top-left of the anchor (for sidebar-triggered glance).
  // TODO(astra): Adjust arrow position based on source type.
  //   For sidebar items, arrow on the left pointing right.
  //   For link previews, arrow on top pointing down.
  bubble->SetArrow(views::BubbleBorder::LEFT_TOP);

  // Apply entrance animation.
  bubble->PlayEntranceAnimation();

  widget->Show();

  return widget;
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraGlanceView::AstraGlanceView(views::View* anchor_view,
                                 const gfx::Rect& anchor_rect,
                                 Delegate* delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::LEFT_TOP,
                                       views::BubbleBorder::STANDARD_SHADOW),
      delegate_(delegate) {
  // Set the anchor rect if provided — this allows anchoring to a specific
  // sub-rect of the anchor view (e.g., a sidebar item row).
  if (!anchor_rect.IsEmpty()) {
    SetAnchorRect(anchor_rect);
  }

  // Bubble configuration.
  SetAcceptCallback(base::DoNothing());
  SetCancelCallback(base::DoNothing());
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowTitle(false);
  SetShowCloseButton(false);  // We use our own close button in the header.

  // Start with expanded size (the default).
  set_fixed_width(kExpandedSize.width());
  set_fixed_height(kExpandedSize.height());

  // Auto-dismiss when the widget loses activation (user clicks outside).
  set_close_on_deactivate(true);

  // Register keyboard accelerators.
  AddAccelerator(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_RETURN, ui::EF_CONTROL_DOWN));
  AddAccelerator(ui::Accelerator(ui::VKEY_P, ui::EF_CONTROL_DOWN));
  AddAccelerator(ui::Accelerator(ui::VKEY_S, ui::EF_CONTROL_DOWN));
  AddAccelerator(ui::Accelerator(ui::VKEY_OEM_PLUS, ui::EF_CONTROL_DOWN));
  AddAccelerator(ui::Accelerator(ui::VKEY_OEM_MINUS, ui::EF_CONTROL_DOWN));
  AddAccelerator(ui::Accelerator(ui::VKEY_1, ui::EF_CONTROL_DOWN));
  AddAccelerator(ui::Accelerator(ui::VKEY_2, ui::EF_CONTROL_DOWN));
  AddAccelerator(ui::Accelerator(ui::VKEY_3, ui::EF_CONTROL_DOWN));

  // Set bubble corner radius.
  // TODO(astra): BubbleBorder may not support custom radius directly.
  //   Consider using a custom border or background with rounded corners.
  //   Chromium pattern: views::BubbleBorder::SetCornerRadius().
}

AstraGlanceView::~AstraGlanceView() {
  // Notify the delegate that the view is being destroyed.
  if (delegate_) {
    delegate_->OnGlanceViewDestroyed();
  }
}

// =========================================================================
// Init — build the child views
// =========================================================================

void AstraGlanceView::Init() {
  // Use a vertical flex layout for the whole bubble content:
  //   header bar
  //   preview area (flexes to fill remaining space)
  //   status bar
  //   action bar
  //   (resize handle floats at bottom-right)
  auto* layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);

  BuildHeader();
  BuildPreviewArea();
  BuildStatusBar();
  BuildActionBar();
  BuildResizeHandle();

  // Initialize colors.
  UpdateColors();

  // Initialize state views.
  UpdateStateViews();

  // Initialize layout for the default display mode.
  UpdateLayoutForDisplayMode();

  // If a WebContents is already set, attach its view.
  if (web_contents_) {
    AttachWebContentsView();
  }
}

// =========================================================================
// Header bar
// =========================================================================

void AstraGlanceView::BuildHeader() {
  auto header = std::make_unique<views::View>();
  header->SetPreferredSize(gfx::Size(0, kHeaderHeight));
  header->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      kHeaderVerticalInsets, kHeaderHorizontalInsets)));

  auto* layout = header->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetDefault(views::kMarginsKey, gfx::Insets::VH(0, 8));

  // Favicon.
  auto favicon = std::make_unique<views::ImageView>();
  favicon->SetPreferredSize(kFaviconSize);
  favicon->SetAccessibleName(u"Site favicon");
  favicon_view_ = favicon.get();
  header->AddChildView(std::move(favicon));

  // Title + URL text container (vertical stack).
  auto text_container = std::make_unique<views::View>();
  auto* text_layout =
      text_container->SetLayoutManager(std::make_unique<views::FlexLayout>());
  text_layout->SetOrientation(views::LayoutOrientation::kVertical);
  text_layout->SetMainAxisAlignment(views::LayoutAlignment::kCenter);
  text_layout->SetFlexForcedCrossAxisAlignment(true);
  text_container->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded));
  header_text_container_ = text_container.get();

  // Title label.
  auto title_label = std::make_unique<views::Label>(std::u16string());
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label->SetFontList(title_label->font_list().Derive(
      0, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
  title_label_ = title_label.get();
  text_container->AddChildView(std::move(title_label));

  // URL label (smaller, secondary text).
  auto url_label = std::make_unique<views::Label>(std::u16string());
  url_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  url_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  url_label->SetFontList(url_label->font_list().Derive(
      -2, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
  url_label_ = url_label.get();
  text_container->AddChildView(std::move(url_label));

  header->AddChildView(std::move(text_container));

  // Size preset buttons (small, medium, large).
  auto size_preset_container = std::make_unique<views::View>();
  auto* preset_layout =
      size_preset_container->SetLayoutManager(std::make_unique<views::FlexLayout>());
  preset_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  preset_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  preset_layout->SetDefault(views::kMarginsKey, gfx::Insets::VH(0, 2));

  auto size_small = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnSizeSmallButtonPressed,
                          base::Unretained(this)));
  size_small->SetPreferredSize(gfx::Size(20, 20));
  size_small->SetTooltipText(u"Small size");
  size_small->SetAccessibleName(u"Small glance size");
  size_small_button_ = size_small.get();
  size_preset_container->AddChildView(std::move(size_small));

  auto size_medium = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnSizeMediumButtonPressed,
                          base::Unretained(this)));
  size_medium->SetPreferredSize(gfx::Size(20, 20));
  size_medium->SetTooltipText(u"Medium size");
  size_medium->SetAccessibleName(u"Medium glance size");
  size_medium_button_ = size_medium.get();
  size_preset_container->AddChildView(std::move(size_medium));

  auto size_large = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnSizeLargeButtonPressed,
                          base::Unretained(this)));
  size_large->SetPreferredSize(gfx::Size(20, 20));
  size_large->SetTooltipText(u"Large size");
  size_large->SetAccessibleName(u"Large glance size");
  size_large_button_ = size_large.get();
  size_preset_container->AddChildView(std::move(size_large));

  size_preset_container_ = header->AddChildView(std::move(size_preset_container));

  // Pin button (pins the glance window open).
  auto pin_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnPinButtonPressed,
                          base::Unretained(this)));
  pin_button->SetPreferredSize(kHeaderCloseButtonSize);
  pin_button->SetTooltipText(u"Pin glance");
  pin_button->SetAccessibleName(u"Pin glance preview open");
  pin_button_ = pin_button.get();
  header->AddChildView(std::move(pin_button));

  // Position cycle button.
  auto position_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnPositionButtonPressed,
                          base::Unretained(this)));
  position_button->SetPreferredSize(kHeaderCloseButtonSize);
  position_button->SetTooltipText(u"Change position");
  position_button->SetAccessibleName(u"Cycle glance position");
  position_button_ = position_button.get();
  header->AddChildView(std::move(position_button));

  // Settings button.
  auto settings_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnSettingsButtonPressed,
                          base::Unretained(this)));
  settings_button->SetPreferredSize(kHeaderCloseButtonSize);
  settings_button->SetTooltipText(u"Glance settings");
  settings_button->SetAccessibleName(u"Open glance settings");
  settings_button_ = settings_button.get();
  header->AddChildView(std::move(settings_button));

  // Expand button (toggles compact/expanded mode).
  auto expand_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnExpandButtonPressed,
                          base::Unretained(this)));
  expand_button->SetPreferredSize(kHeaderCloseButtonSize);
  expand_button->SetTooltipText(u"Expand");
  expand_button->SetAccessibleName(u"Expand glance preview");
  expand_button_ = expand_button.get();
  header->AddChildView(std::move(expand_button));

  // Close button.
  auto close_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnCloseButtonPressed,
                          base::Unretained(this)));
  close_button->SetPreferredSize(kHeaderCloseButtonSize);
  close_button->SetTooltipText(u"Close glance");
  close_button->SetAccessibleName(u"Close glance preview");
  // TODO(astra): Set the actual close icon from Chromium vector icons.
  //   Use views::kCloseIcon or chrome/browser/ui/views vector icons.
  //   Chromium pattern: ui/views/vector_icons/close_icon.h.
  close_button_ = close_button.get();
  header->AddChildView(std::move(close_button));

  header_ = AddChildView(std::move(header));
}

// =========================================================================
// Preview area
// =========================================================================

void AstraGlanceView::BuildPreviewArea() {
  auto preview_area = std::make_unique<views::View>();
  preview_area->SetLayoutManager(std::make_unique<views::FillLayout>());
  preview_area->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kUnbounded));
  preview_area_ = AddChildView(std::move(preview_area));

  // Build state subviews (loading and error overlays).
  BuildLoadingView();
  BuildErrorView();
  BuildPreviewImageView();
}

void AstraGlanceView::BuildLoadingView() {
  auto loading_view = std::make_unique<views::View>();
  auto* layout =
      loading_view->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetDefault(views::kMarginsKey, gfx::Insets::VH(4, 0));

  // Throbber (spinner).
  auto throbber = std::make_unique<views::Throbber>();
  throbber->SetPreferredSize(gfx::Size(32, 32));
  throbber->Start();
  loading_throbber_ = throbber.get();
  loading_view->AddChildView(std::move(throbber));

  // Loading text.
  auto loading_label = std::make_unique<views::Label>(u"Loading...");
  loading_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  loading_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  loading_label_ = loading_label.get();
  loading_view->AddChildView(std::move(loading_label));

  loading_view_ = preview_area_->AddChildView(std::move(loading_view));
}

void AstraGlanceView::BuildErrorView() {
  auto error_view = std::make_unique<views::View>();
  auto* layout =
      error_view->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetDefault(views::kMarginsKey, gfx::Insets::VH(4, 0));

  // Error icon placeholder.
  // TODO(astra): Use a real error icon from Chromium's vector icons.
  //   Chromium pattern: views::kErrorIcon or a custom Astra icon.
  auto error_icon = std::make_unique<views::ImageView>();
  error_icon->SetPreferredSize(gfx::Size(48, 48));
  error_icon->SetAccessibleName(u"Error");
  error_view->AddChildView(std::move(error_icon));

  // Error message.
  auto error_label = std::make_unique<views::Label>(
      u"Failed to load preview");
  error_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  error_label->SetMultiLine(true);
  error_label->SetElideBehavior(gfx::ELIDE_TAIL);
  error_label->SetFontList(error_label->font_list().Derive(
      0, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
  error_label_ = error_label.get();
  error_view->AddChildView(std::move(error_label));

  error_view_ = preview_area_->AddChildView(std::move(error_view));
}

// =========================================================================
// Preview image view (screenshot / thumbnail)
// =========================================================================

void AstraGlanceView::BuildPreviewImageView() {
  auto preview_image = std::make_unique<views::ImageView>();
  preview_image->SetVisible(false);
  preview_image->SetAccessibleName(u"Tab preview image");
  preview_image->SetImageSize(gfx::Size(0, 0));  // Use image's intrinsic size
  preview_image_view_ = preview_area_->AddChildView(std::move(preview_image));
}

// =========================================================================
// Status bar
// =========================================================================

void AstraGlanceView::BuildStatusBar() {
  auto status_bar = std::make_unique<views::View>();
  status_bar->SetPreferredSize(gfx::Size(0, kStatusBarHeight));
  status_bar->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kStatusBarHorizontalInsets)));

  auto* layout =
      status_bar->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetDefault(views::kMarginsKey, gfx::Insets::VH(0, 6));

  // Security icon.
  auto security_icon = std::make_unique<views::ImageView>();
  security_icon->SetPreferredSize(gfx::Size(14, 14));
  security_icon->SetAccessibleName(u"Security status");
  security_icon_ = security_icon.get();
  status_bar->AddChildView(std::move(security_icon));

  // Status text.
  auto status_label = std::make_unique<views::Label>(std::u16string());
  status_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  status_label->SetFontList(status_label->font_list().Derive(
      -2, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
  status_label->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded));
  status_label_ = status_label.get();
  status_bar->AddChildView(std::move(status_label));

  status_bar_ = AddChildView(std::move(status_bar));
}

// =========================================================================
// Action bar
// =========================================================================

void AstraGlanceView::BuildActionBar() {
  auto action_bar = std::make_unique<views::View>();
  action_bar->SetPreferredSize(gfx::Size(0, kActionBarHeight));
  action_bar->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      kActionBarVerticalInsets, kActionBarHorizontalInsets)));

  auto* layout =
      action_bar->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kSpaceAround);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetDefault(views::kMarginsKey,
                      gfx::Insets::VH(0, kActionButtonSpacing));

  // Open in new tab button.
  auto promote_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnPromoteButtonPressed,
                          base::Unretained(this)));
  promote_button->SetPreferredSize(kActionBarButtonSize);
  promote_button->SetTooltipText(u"Open in new tab");
  promote_button->SetAccessibleName(u"Open in new tab");
  // TODO(astra): Set actual icon.  Use Chromium vector icon for "open in new".
  promote_action_button_ = promote_button.get();
  action_bar->AddChildView(std::move(promote_button));

  // Add to favorites button.
  auto favorite_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnFavoriteButtonPressed,
                          base::Unretained(this)));
  favorite_button->SetPreferredSize(kActionBarButtonSize);
  favorite_button->SetTooltipText(u"Add to favorites");
  favorite_button->SetAccessibleName(u"Add to favorites");
  // TODO(astra): Set actual icon.  Use Chromium vector icon for bookmark star.
  favorite_button_ = favorite_button.get();
  action_bar->AddChildView(std::move(favorite_button));

  // Copy URL button.
  auto copy_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnCopyURLButtonPressed,
                          base::Unretained(this)));
  copy_button->SetPreferredSize(kActionBarButtonSize);
  copy_button->SetTooltipText(u"Copy URL");
  copy_button->SetAccessibleName(u"Copy URL to clipboard");
  // TODO(astra): Set actual icon.  Use Chromium vector icon for copy/link.
  copy_url_button_ = copy_button.get();
  action_bar->AddChildView(std::move(copy_button));

  // Pin tab button.
  auto pin_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnPinTabButtonPressed,
                          base::Unretained(this)));
  pin_button->SetPreferredSize(kActionBarButtonSize);
  pin_button->SetTooltipText(u"Pin tab");
  pin_button->SetAccessibleName(u"Pin tab");
  // TODO(astra): Set actual icon.  Use Chromium vector icon for pin.
  pin_tab_button_ = pin_button.get();
  action_bar->AddChildView(std::move(pin_button));

  // Close tab button.
  auto close_tab_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraGlanceView::OnCloseTabButtonPressed,
                          base::Unretained(this)));
  close_tab_button->SetPreferredSize(kActionBarButtonSize);
  close_tab_button->SetTooltipText(u"Close tab");
  close_tab_button->SetAccessibleName(u"Close tab");
  // TODO(astra): Set actual icon.  Use Chromium vector icon for close tab.
  close_tab_button_ = close_tab_button.get();
  action_bar->AddChildView(std::move(close_tab_button));

  action_bar_ = AddChildView(std::move(action_bar));
}

// =========================================================================
// Resize handle
// =========================================================================

void AstraGlanceView::BuildResizeHandle() {
  // The resize handle sits at the bottom-right corner of the bubble.
  // It's a small view that captures mouse events for resizing.
  auto resize_handle = std::make_unique<views::View>();
  resize_handle->SetPreferredSize(
      gfx::Size(kResizeHandleSize, kResizeHandleSize));
  // TODO(astra): Add a visual grip indicator (e.g. three diagonal lines).
  //   Chromium pattern: views::ResizeCorner or a custom SVG icon.

  // Install mouse handlers for the resize handle.
  resize_handle->SetOnMousePressedCallback(base::BindRepeating(
      &AstraGlanceView::OnResizeHandleMousePressed, base::Unretained(this)));
  resize_handle->SetOnMouseDraggedCallback(base::BindRepeating(
      &AstraGlanceView::OnResizeHandleMouseDragged, base::Unretained(this)));
  resize_handle->SetOnMouseReleasedCallback(base::BindRepeating(
      &AstraGlanceView::OnResizeHandleMouseReleased, base::Unretained(this)));
  resize_handle->SetCursor(ui::mojom::CursorType::kNorthWestSouthEastResize);

  // Position the handle at bottom-right using flex layout end alignment.
  // We add it as a floating view positioned at the bottom-right corner.
  resize_handle_ = AddChildView(std::move(resize_handle));

  // TODO(astra): Position the resize handle properly at the bottom-right
  //   corner using a layout manager or explicit positioning.
  //   For now, it sits in the vertical flow; we should use a separate
  //   overlay layout or absolute positioning.
  //   Chromium pattern: views::OverlayLayout or views::GridLayout.
}

bool AstraGlanceView::OnResizeHandleMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    is_resizing_ = true;
    resize_start_mouse_ = event.location();
    views::Widget* widget = GetWidget();
    if (widget) {
      resize_start_size_ = widget->GetWindowBoundsInScreen().size();
    }
    return true;
  }
  return false;
}

bool AstraGlanceView::OnResizeHandleMouseDragged(const ui::MouseEvent& event) {
  if (!is_resizing_) {
    return false;
  }

  views::Widget* widget = GetWidget();
  if (!widget) {
    return false;
  }

  // Calculate delta from start position.
  int delta_x = event.x() - resize_start_mouse_.x();
  int delta_y = event.y() - resize_start_mouse_.y();

  // Apply minimum size constraints.
  int new_width = std::max(resize_start_size_.width() + delta_x,
                           kCompactSize.width());
  int new_height = std::max(resize_start_size_.height() + delta_y,
                            kCompactSize.height());

  gfx::Rect bounds = widget->GetWindowBoundsInScreen();
  bounds.set_width(new_width);
  bounds.set_height(new_height);
  widget->SetBounds(bounds);

  return true;
}

void AstraGlanceView::OnResizeHandleMouseReleased(const ui::MouseEvent& event) {
  if (is_resizing_) {
    is_resizing_ = false;
    // Notify delegate that the glance was resized.
    views::Widget* widget = GetWidget();
    if (widget && delegate_) {
      delegate_->OnGlanceResized(widget->GetWindowBoundsInScreen().size());
    }
  }
}

// =========================================================================
// WebContents embedding
// =========================================================================

void AstraGlanceView::SetWebContents(content::WebContents* web_contents) {
  if (web_contents_ == web_contents) {
    return;
  }

  // Detach the old WebContents view if there was one.
  if (web_contents_ && preview_area_) {
    DetachWebContentsView();
  }

  web_contents_ = web_contents;

  // Attach the new WebContents view if we're already initialized.
  if (web_contents_ && preview_area_) {
    AttachWebContentsView();
  }

  // Update the title and URL from the new WebContents.
  if (web_contents_) {
    // TODO(astra): Subscribe to WebContentsObserver for title/URL/favicon
    //   changes and update dynamically.  For now, set once.
    SetTitleText(web_contents_->GetTitle());
    SetURLText(base::UTF8ToUTF16(web_contents_->GetVisibleURL().spec()));
  } else {
    SetTitleText(std::u16string());
    SetURLText(std::u16string());
  }
}

void AstraGlanceView::AttachWebContentsView() {
  // TODO(astra): This is where the real WebContents view embedding happens.
  //
  // The exact approach depends on the Chromium API:
  //
  // Option 1 — views::WebView (if available):
  //   auto web_view = std::make_unique<views::WebView>(nullptr);
  //   web_view->SetWebContents(web_contents_);
  //   preview_area_->AddChildView(std::move(web_view));
  //
  // Option 2 — NativeViewHost wrapping WebContents::GetNativeView():
  //   gfx::NativeView native_view = web_contents_->GetNativeView();
  //   auto native_host = std::make_unique<views::NativeViewHost>();
  //   native_host->Attach(native_view);
  //   preview_area_->AddChildView(std::move(native_host));
  //
  // Option 3 — Direct WebContentsView access:
  //   content::WebContentsView* wcv = web_contents_->GetView();
  //   views::View* wcv_view = wcv->GetView();  // If available on views port
  //   preview_area_->AddChildView(wcv_view);
  //
  // Reference: Chrome side panel (chrome/browser/ui/views/side_panel/)
  // hosts WebContents for side panel entries.  The side panel uses a
  // SidePanelContentView which wraps WebContentsView.
  //
  // Chromium owner: content::WebContentsView (content/public/browser/),
  // and chrome/browser/ui/views/side_panel/side_panel_content_view.h.
  //
  // Patch point: May need a tiny patch to expose the views layer of
  // WebContentsView, or to create a views::WebView wrapper.

  // For the skeleton, we update the state views — the loading/error
  // overlays show on top of the (future) WebContents, and the WebContents
  // itself is the base layer.
  UpdateStateViews();
}

void AstraGlanceView::DetachWebContentsView() {
  // TODO(astra): Remove the WebContents view from preview_area_.
  //   Corresponding to the AttachWebContentsView() TODO(astra) — whichever
  //   embedding approach we use, this should undo it and restore the
  //   WebContents to its original host.
  //
  // For tab glance mode, the WebContents must be returned to its normal
  // position in the tab strip's content area.
  // For URL glance mode, the WebContents will be destroyed by the controller.
  //
  // Chromium subsystem: TabStripModel active tab handling.

  // Remove all child views except the state overlays (loading, error).
  // The state overlays are part of the view and persist.
  if (preview_area_) {
    // TODO(astra): Only remove the WebContents host view, not all children.
    //   For now, we keep the state views and assume the WebContents is
    //   added separately.
  }
}

// =========================================================================
// Content metadata setters
// =========================================================================

void AstraGlanceView::SetTitleText(const std::u16string& title) {
  if (title_label_) {
    title_label_->SetText(title);
  }
}

void AstraGlanceView::SetURLText(const std::u16string& url) {
  if (url_label_) {
    url_label_->SetText(url);
  }
  if (status_label_) {
    status_label_->SetText(url);
  }
}

void AstraGlanceView::SetFavicon(const gfx::ImageSkia& icon) {
  if (favicon_view_) {
    favicon_view_->SetImage(icon);
  }
}

// =========================================================================
// Loading state
// =========================================================================

void AstraGlanceView::SetLoadingState(LoadingState state) {
  if (loading_state_ == state) {
    return;
  }

  loading_state_ = state;
  UpdateStateViews();
}

void AstraGlanceView::SetErrorMessage(const std::u16string& message) {
  error_message_ = message;
  if (error_label_) {
    error_label_->SetText(message.empty() ? u"Failed to load preview" : message);
  }
}

void AstraGlanceView::UpdateStateViews() {
  if (!preview_area_) {
    return;
  }

  bool show_loading = (loading_state_ == LoadingState::kLoading);
  bool show_error = (loading_state_ == LoadingState::kError);
  bool show_web = (loading_state_ == LoadingState::kLoaded);

  if (loading_view_) {
    loading_view_->SetVisible(show_loading);
    if (loading_throbber_) {
      if (show_loading) {
        loading_throbber_->Start();
      } else {
        loading_throbber_->Stop();
      }
    }
  }

  if (error_view_) {
    error_view_->SetVisible(show_error);
  }

  // TODO(astra): Show/hide the actual WebContents host view.
  //   For now, the preview area background serves as the "web content"
  //   placeholder when loaded.

  // Update status bar text based on loading state.
  if (status_label_) {
    if (loading_state_ == LoadingState::kLoading) {
      status_label_->SetText(u"Loading...");
    } else if (loading_state_ == LoadingState::kError) {
      status_label_->SetText(u"Error");
    }
    // When loaded, the status label shows the URL (set via SetURLText).
  }
}

// =========================================================================
// Security state
// =========================================================================

void AstraGlanceView::SetSecurityState(SecurityState state) {
  if (security_state_ == state) {
    return;
  }

  security_state_ = state;

  // TODO(astra): Update the security icon based on state.
  //   - kSecure: lock icon, green color.
  //   - kNonSecure: info/warning icon.
  //   - kUnknown: no icon or default icon.
  //   Chromium pattern: Chrome's omnibox security icons (LocationBarView).
  //   Chromium owner: chrome/browser/ui/views/location_bar/location_bar_view.h.
}

// =========================================================================
// Display mode
// =========================================================================

void AstraGlanceView::SetDisplayMode(DisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }

  display_mode_ = mode;
  UpdateLayoutForDisplayMode();

  // Notify the delegate.
  if (delegate_ && mode == DisplayMode::kExpanded) {
    delegate_->OnGlanceToggleExpanded();
  }
}

void AstraGlanceView::ToggleDisplayMode() {
  SetDisplayMode(display_mode_ == DisplayMode::kExpanded
                     ? DisplayMode::kCompact
                     : DisplayMode::kExpanded);
}

void AstraGlanceView::UpdateLayoutForDisplayMode() {
  const bool is_expanded = (display_mode_ == DisplayMode::kExpanded);

  // Show/hide sections that are only in expanded mode.
  if (status_bar_) {
    status_bar_->SetVisible(is_expanded);
  }
  if (action_bar_) {
    action_bar_->SetVisible(is_expanded);
  }

  // URL label is only shown in expanded mode.
  if (url_label_) {
    url_label_->SetVisible(is_expanded);
  }

  // Resize handle is only available in expanded mode.
  if (resize_handle_) {
    resize_handle_->SetVisible(is_expanded);
  }

  // Update the widget size.
  views::Widget* widget = GetWidget();
  if (widget) {
    gfx::Size size = is_expanded ? kExpandedSize : kCompactSize;
    // TODO(astra): Animate the size change.
    //   Use views::Widget::SetBounds() with animation or a layout animation.
    gfx::Rect bounds = widget->GetWindowBoundsInScreen();
    bounds.set_width(size.width());
    bounds.set_height(size.height());
    widget->SetBounds(bounds);
  }

  InvalidateLayout();
}

// =========================================================================
// Action button states
// =========================================================================

void AstraGlanceView::SetIsFavorite(bool is_favorite) {
  if (is_favorite_ == is_favorite) {
    return;
  }

  is_favorite_ = is_favorite;

  // Update tooltip and accessible name.
  if (favorite_button_) {
    favorite_button_->SetTooltipText(
        is_favorite ? u"Remove from favorites" : u"Add to favorites");
    favorite_button_->SetAccessibleName(
        is_favorite ? u"Remove from favorites" : u"Add to favorites");
    // TODO(astra): Toggle the icon (filled vs outline star).
  }
}

void AstraGlanceView::SetIsPinned(bool is_pinned) {
  if (is_pinned_ == is_pinned) {
    return;
  }

  is_pinned_ = is_pinned;

  if (pin_tab_button_) {
    pin_tab_button_->SetTooltipText(is_pinned ? u"Unpin tab" : u"Pin tab");
    pin_tab_button_->SetAccessibleName(is_pinned ? u"Unpin tab" : u"Pin tab");
    // TODO(astra): Toggle the icon (pinned vs unpinned).
  }
}

void AstraGlanceView::SetActionButtonEnabled(bool open_in_tab_enabled,
                                              bool favorite_enabled,
                                              bool copy_url_enabled,
                                              bool pin_tab_enabled,
                                              bool close_tab_enabled) {
  if (promote_action_button_) {
    promote_action_button_->SetEnabled(open_in_tab_enabled);
  }
  if (favorite_button_) {
    favorite_button_->SetEnabled(favorite_enabled);
  }
  if (copy_url_button_) {
    copy_url_button_->SetEnabled(copy_url_enabled);
  }
  if (pin_tab_button_) {
    pin_tab_button_->SetEnabled(pin_tab_enabled);
  }
  if (close_tab_button_) {
    close_tab_button_->SetEnabled(close_tab_enabled);
  }
}

void AstraGlanceView::UpdateActionButtonLabels() {
  // Called when language or state changes to refresh all button labels.
  SetIsFavorite(is_favorite_);
  SetIsPinned(is_pinned_);
}

// =========================================================================
// Animation stubs
// =========================================================================

void AstraGlanceView::PlayEntranceAnimation() {
  // TODO(astra): Implement real entrance animation for the glance bubble.
  //   A good entrance would be a fast fade-in + slight scale-up from the
  //   anchor point (like a pop-in effect).
  //
  //   Chromium pattern: views::SlideAnimation or
  //   views::Widget::SetVisibilityAnimationTransition().
  //
  //   Chromium owner: ui/compositor/layer_animation.h, views::View layer
  //   animations.  We could use View::SetPaintToLayer() and animate the
  //   layer's opacity and transform.
  //
  //   For now, the bubble appears instantly.
  //
  // Implementation pattern (for future real implementation):
  //   SetPaintToLayer();
  //   layer()->SetOpacity(0.0f);
  //   layer()->SetTransform(gfx::Transform::MakeScale(0.9f));
  //
  //   auto* sequence = ui::LayerAnimator::CreateImplicitAnimator();
  //   layer()->SetAnimator(sequence);
  //
  //   // Animate opacity to 1.0 and scale to 1.0.
  //   layer()->SetOpacity(1.0f);
  //   layer()->SetTransform(gfx::Transform());
  //
  //   // After animation, stop painting to layer if not needed.
  //   // base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
  //   //     FROM_HERE,
  //   //     base::BindOnce([](views::View* view) {
  //   //       view->SetPaintToLayer(ui::LAYER_NOT_DRAWN);
  //   //     },
  //   //     base::Unretained(this)),
  //   //     base::Milliseconds(200));
  //
  // Chromium reference: views::BubbleDialogDelegateView uses a show animation
  // via Widget::Show() with SetVisibilityAnimationTransition().
  // The bubble's native show animation can be configured via:
  //   widget->SetVisibilityAnimationTransition(views::Widget::ANIMATE_SHOW);
}

void AstraGlanceView::PlayExitAnimation(base::OnceClosure on_done) {
  // TODO(astra): Implement real exit animation (fade-out + scale-down).
  //   See PlayEntranceAnimation() for the animation approach.
  //
  //   The |on_done| callback should be called when the animation completes,
  //   so the controller knows when it's safe to destroy the widget.
  //
  // Implementation pattern (for future real implementation):
  //   if (!GetWidget() || !layer()) {
  //     if (!on_done.is_null())
  //       std::move(on_done).Run();
  //     return;
  //   }
  //
  //   auto* animator = layer()->GetAnimator();
  //   animator->set_preemption_strategy(
  //       ui::LayerAnimator::REPLACE_QUEUED_ANIMATIONS);
  //
  //   // Fade out and scale down.
  //   layer()->SetOpacity(0.0f);
  //   layer()->SetTransform(gfx::Transform::MakeScale(0.95f));
  //
  //   // When animation completes, call the callback.
  //   animator->AddObserver(animation_observer_.get());
  //   // Or use base::SequencedTaskRunner for a fixed duration fallback.
  //
  //   base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
  //       FROM_HERE, std::move(on_done), base::Milliseconds(150));
  //
  // For now, call the callback immediately (no animation).
  if (!on_done.is_null()) {
    std::move(on_done).Run();
  }
}

// =========================================================================
// BubbleDialogDelegateView overrides
// =========================================================================

void AstraGlanceView::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);

  // Stop any animations.
  if (loading_throbber_) {
    loading_throbber_->Stop();
  }

  // Detach the WebContents before the widget is destroyed.
  // This prevents the WebContents view from being destroyed along with
  // the bubble — for tab glance, the WebContents must survive and return
  // to the tab strip.
  DetachWebContentsView();
  web_contents_ = nullptr;
}

void AstraGlanceView::OnWidgetActivationChanged(views::Widget* widget,
                                                bool active) {
  views::BubbleDialogDelegateView::OnWidgetActivationChanged(widget, active);

  // When the widget loses activation, it auto-closes due to
  // set_close_on_deactivate(true).  No extra work needed here.
  //
  // TODO(astra): Consider whether auto-hide should be delayed or
  //   have hysteresis for hover-triggered glances.
  //   The controller should handle the auto-hide timer.
}

bool AstraGlanceView::AcceleratorPressed(const ui::Accelerator& accelerator) {
  if (accelerator.key_code() == ui::VKEY_ESCAPE) {
    OnCloseButtonPressed();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_RETURN &&
      accelerator.IsCtrlDown()) {
    // Ctrl+Enter: promote to full tab.
    OnPromoteButtonPressed();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_P && accelerator.IsCtrlDown()) {
    // Ctrl+P: toggle pin glance open.
    OnPinButtonPressed();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_S && accelerator.IsCtrlDown()) {
    // Ctrl+S: toggle settings button visibility / settings menu.
    OnSettingsButtonPressed();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_OEM_PLUS && accelerator.IsCtrlDown()) {
    // Ctrl++: increase size (next preset).
    SizePreset current = GetCurrentSizePreset();
    if (current == SizePreset::kSmall) {
      ApplySizePreset(SizePreset::kMedium);
    } else if (current == SizePreset::kMedium) {
      ApplySizePreset(SizePreset::kLarge);
    }
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_OEM_MINUS && accelerator.IsCtrlDown()) {
    // Ctrl+-: decrease size (previous preset).
    SizePreset current = GetCurrentSizePreset();
    if (current == SizePreset::kLarge) {
      ApplySizePreset(SizePreset::kMedium);
    } else if (current == SizePreset::kMedium) {
      ApplySizePreset(SizePreset::kSmall);
    }
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_1 && accelerator.IsCtrlDown()) {
    // Ctrl+1: small size preset.
    ApplySizePreset(SizePreset::kSmall);
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_2 && accelerator.IsCtrlDown()) {
    // Ctrl+2: medium size preset.
    ApplySizePreset(SizePreset::kMedium);
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_3 && accelerator.IsCtrlDown()) {
    // Ctrl+3: large size preset.
    ApplySizePreset(SizePreset::kLarge);
    return true;
  }
  return views::BubbleDialogDelegateView::AcceleratorPressed(accelerator);
}

gfx::Size AstraGlanceView::CalculatePreferredSize() const {
  // Return the size appropriate for the current display mode.
  // The bubble also respects set_fixed_width/set_fixed_height, but
  // CalculatePreferredSize is used by some layout paths.
  return (display_mode_ == DisplayMode::kExpanded) ? kExpandedSize
                                                   : kCompactSize;
}

void AstraGlanceView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  UpdateColors();
}

void AstraGlanceView::UpdateColors() {
  const ui::ColorProvider* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor bg_color = color_provider->GetColor(kColorAstraGlanceBackground);
  SkColor border_color = color_provider->GetColor(kColorAstraGlanceBorder);

  // Apply background to the main view.
  // TODO(astra): BubbleDialogDelegateView uses BubbleBorder for the
  //   background and border.  To customize colors, we may need to:
  //     a) Override OnThemeChanged and call SetColor() on the BubbleBorder.
  //     b) Use a custom BubbleBorder with Astra colors.
  //     c) Patch BubbleDialogDelegateView to support color IDs.
  //
  //   Reference: chrome/browser/ui/views/bubble/webui_bubble_dialog_view.h
  //   uses SetBackground with a rounded rect background.
  //
  //   Patch point: ui/views/bubble/bubble_border.cc — OnThemeChanged method
  //   or color support.  Or we can set a custom background on the content
  //   area and let the bubble border be transparent.
  //
  //   For now, we set the background color on child views.

  // Header background.
  if (header_) {
    header_->SetBackground(views::CreateSolidBackground(bg_color));
  }

  // Preview area background.
  if (preview_area_) {
    preview_area_->SetBackground(views::CreateSolidBackground(bg_color));
  }

  // Status bar background.
  if (status_bar_) {
    status_bar_->SetBackground(views::CreateSolidBackground(bg_color));
  }

  // Action bar background.
  if (action_bar_) {
    action_bar_->SetBackground(views::CreateSolidBackground(bg_color));
  }

  // TODO(astra): Apply border color to the bubble border.
  //   The BubbleBorder owns the border and shadow; we'd need to either
  //   set its color or use a custom border.
  //   Chromium pattern: BubbleBorder::SetColor().

  // Update text colors for labels.
  // TODO(astra): Use proper Chromium color IDs for text (e.g., kColorTextPrimary,
  //   kColorTextSecondary).  Astra should define its own text colors or
  //   reuse Chromium's.
  if (title_label_) {
    title_label_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForeground));
  }
  if (url_label_) {
    url_label_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForegroundSecondary));
  }
  if (status_label_) {
    status_label_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForegroundSecondary));
  }
  if (loading_label_) {
    loading_label_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForegroundSecondary));
  }
  if (error_label_) {
    error_label_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForeground));
  }

  // Update the border color reference (for documentation).
  // TODO(astra): Use kColorAstraGlanceBorder for the bubble border.
  ALLOW_UNUSED_LOCAL(border_color);
}

void AstraGlanceView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::BubbleDialogDelegateView::GetAccessibleNodeData(node_data);

  // The glance is a dialog with preview content.
  node_data->role = ax::mojom::Role::kDialog;

  // Set a descriptive name from the title.
  if (title_label_ && !title_label_->GetText().empty()) {
    node_data->SetName(title_label_->GetText());
  } else {
    node_data->SetName(u"Tab preview");
  }

  // Mark as modal? No — glance is a non-modal popup.
  node_data->AddState(ax::mojom::State::kFocusable);
}

// =========================================================================
// Button handlers
// =========================================================================

void AstraGlanceView::OnCloseButtonPressed() {
  if (delegate_) {
    delegate_->OnGlanceCloseRequested();
  } else {
    // Fallback: close the widget directly.
    if (GetWidget()) {
      GetWidget()->Close();
    }
  }
}

void AstraGlanceView::OnPromoteButtonPressed() {
  if (delegate_) {
    delegate_->OnGlancePromoteToTab();
  }
}

void AstraGlanceView::OnFavoriteButtonPressed() {
  if (delegate_) {
    delegate_->OnGlanceAddToFavorites();
  }
}

void AstraGlanceView::OnCopyURLButtonPressed() {
  if (delegate_) {
    delegate_->OnGlanceCopyURL();
  }
}

void AstraGlanceView::OnPinTabButtonPressed() {
  if (delegate_) {
    delegate_->OnGlancePinTab();
  }
}

void AstraGlanceView::OnCloseTabButtonPressed() {
  if (delegate_) {
    delegate_->OnGlanceCloseTab();
  }
}

void AstraGlanceView::OnExpandButtonPressed() {
  ToggleDisplayMode();
}

// =========================================================================
// Glance pinning (window pin)
// =========================================================================

void AstraGlanceView::SetGlancePinned(bool pinned) {
  if (glance_pinned_ == pinned) {
    return;
  }
  glance_pinned_ = pinned;

  if (pin_button_) {
    pin_button_->SetTooltipText(pinned ? u"Unpin glance" : u"Pin glance");
    pin_button_->SetAccessibleName(
        pinned ? u"Unpin glance preview" : u"Pin glance preview open");
    // TODO(astra): Toggle the icon (filled pin vs outline pin).
  }

  // Update close-on-deactivate behavior.
  // When pinned, the glance stays open even when losing activation is lost.
  set_close_on_deactivate(!pinned);
}

// =========================================================================
// Size presets
// =========================================================================

void AstraGlanceView::ApplySizePreset(SizePreset preset) {
  gfx::Size new_size;
  switch (preset) {
    case SizePreset::kSmall:
      new_size = gfx::Size(
          kSmallSize.width(), kSmallSize.height());
      break;
    case SizePreset::kMedium:
      new_size = gfx::Size(
          kExpandedSize.width(), kExpandedSize.height());
      break;
    case SizePreset::kLarge:
      new_size = gfx::Size(
          kLargeSize.width(), kLargeSize.height());
      break;
  }

  views::Widget* widget = GetWidget();
  if (!widget) {
    return;
  }

  gfx::Rect bounds = widget->GetWindowBoundsInScreen();
  if (bounds.size() == new_size) {
    return;
  }

  bounds.set_size(new_size);
  widget->SetBounds(bounds);

  // Notify delegate.
  if (delegate_) {
    delegate_->OnGlanceResized(new_size);
  }
}

AstraGlanceView::SizePreset AstraGlanceView::GetCurrentSizePreset() const {
  views::Widget* widget = GetWidget();
  if (!widget) {
    return SizePreset::kMedium;
  }

  gfx::Size current_size = widget->GetWindowBoundsInScreen().size();

  // Find the closest preset by area difference.
  int small_area = kSmallSize.GetArea();
  int medium_area = kExpandedSize.GetArea();
  int large_area = kLargeSize.GetArea();
  int current_area = current_size.GetArea();

  int small_diff = std::abs(current_area - small_area);
  int medium_diff = std::abs(current_area - medium_area);
  int large_diff = std::abs(current_area - large_area);

  if (small_diff <= medium_diff && small_diff <= large_diff) {
    return SizePreset::kSmall;
  } else if (medium_diff <= large_diff) {
    return SizePreset::kMedium;
  } else {
    return SizePreset::kLarge;
  }
}

// =========================================================================
// Settings button visibility
// =========================================================================

void AstraGlanceView::SetSettingsButtonVisible(bool visible) {
  if (settings_button_visible_ == visible) {
    return;
  }
  settings_button_visible_ = visible;
  if (settings_button_) {
    settings_button_->SetVisible(visible);
  }
}

// =========================================================================
// Section visibility controls
// =========================================================================

bool AstraGlanceView::IsStatusBarVisible() const {
  return status_bar_ && status_bar_->GetVisible();
}

void AstraGlanceView::SetStatusBarVisible(bool visible) {
  if (status_bar_) {
    status_bar_->SetVisible(visible);
  }
}

bool AstraGlanceView::IsActionBarVisible() const {
  return action_bar_ && action_bar_->GetVisible();
}

void AstraGlanceView::SetActionBarVisible(bool visible) {
  if (action_bar_) {
    action_bar_->SetVisible(visible);
  }
}

bool AstraGlanceView::IsResizeHandleVisible() const {
  return resize_handle_ && resize_handle_->GetVisible();
}

void AstraGlanceView::SetResizeHandleVisible(bool visible) {
  if (resize_handle_) {
    resize_handle_->SetVisible(visible);
  }
}

// =========================================================================
// New button handlers
// =========================================================================

void AstraGlanceView::OnPinButtonPressed() {
  SetGlancePinned(!glance_pinned_);
  if (delegate_) {
    delegate_->OnGlanceTogglePinned();
  }
}

void AstraGlanceView::OnSettingsButtonPressed() {
  if (delegate_) {
    delegate_->OnGlanceSettingsRequested();
  }
}

void AstraGlanceView::OnPositionButtonPressed() {
  if (delegate_) {
    delegate_->OnGlanceCyclePosition();
  }
}

void AstraGlanceView::OnSizeSmallButtonPressed() {
  ApplySizePreset(SizePreset::kSmall);
  if (delegate_) {
    delegate_->OnGlanceSizePresetChanged(SizePreset::kSmall);
  }
}

void AstraGlanceView::OnSizeMediumButtonPressed() {
  ApplySizePreset(SizePreset::kMedium);
  if (delegate_) {
    delegate_->OnGlanceSizePresetChanged(SizePreset::kMedium);
  }
}

void AstraGlanceView::OnSizeLargeButtonPressed() {
  ApplySizePreset(SizePreset::kLarge);
  if (delegate_) {
    delegate_->OnGlanceSizePresetChanged(SizePreset::kLarge);
  }
}

// =========================================================================
// Unified content API
// =========================================================================

void AstraGlanceView::SetContent(AstraGlanceContentType type,
                                  const std::u16string& title,
                                  const GURL& url,
                                  const gfx::ImageSkia& preview_image) {
  SetContentType(type);
  SetTitle(title);
  SetUrl(url);
  SetPreviewImage(preview_image);
}

void AstraGlanceView::SetTitle(const std::u16string& title) {
  SetTitleText(title);
}

std::u16string AstraGlanceView::GetTitle() const {
  if (title_label_) {
    return title_label_->GetText();
  }
  return std::u16string();
}

void AstraGlanceView::SetUrl(const GURL& url) {
  url_ = url;
  if (url.is_valid()) {
    SetURLText(base::UTF8ToUTF16(url.spec()));
  } else {
    SetURLText(std::u16string());
  }
}

GURL AstraGlanceView::GetUrl() const {
  return url_;
}

void AstraGlanceView::SetPreviewImage(const gfx::ImageSkia& image) {
  if (preview_image_view_) {
    preview_image_view_->SetImage(image);
    preview_image_view_->SetVisible(!image.isNull());
  }
}

void AstraGlanceView::ClearPreviewImage() {
  if (preview_image_view_) {
    preview_image_view_->SetImage(gfx::ImageSkia());
    preview_image_view_->SetVisible(false);
  }
}

void AstraGlanceView::SetContentType(AstraGlanceContentType type) {
  if (content_type_ == type) {
    return;
  }
  content_type_ = type;

  // Update visibility of UI elements based on content type.
  const bool has_web_content = (type == AstraGlanceContentType::kTabPreview);
  const bool has_preview_image = (type == AstraGlanceContentType::kScreenshot ||
                                type == AstraGlanceContentType::kTabInfo ||
                                type == AstraGlanceContentType::kReadingList ||
                                type == AstraGlanceContentType::kNote ||
                                type == AstraGlanceContentType::kBookmark);

  // For tab preview, the WebContents (or placeholder) is visible.
  // For other types, the preview image view may be visible.
  // TODO(astra): Wire up actual WebContents visibility based on content type.
  //   For now, we update the preview image view visibility.
  if (preview_image_view_) {
    preview_image_view_->SetVisible(
        has_preview_image && preview_image_view_->GetImage() &&
        !preview_image_view_->GetImage().isNull());
  }

  // Update loading state visibility.
  UpdateStateViews();
}

AstraGlanceContentType AstraGlanceView::GetContentType() const {
  return content_type_;
}

// =========================================================================
// Pin state (glance pinned open)
// =========================================================================

void AstraGlanceView::SetPinned(bool pinned) {
  SetGlancePinned(pinned);
}

bool AstraGlanceView::IsPinned() const {
  return IsGlancePinned();
}

// =========================================================================
// Expanded state
// =========================================================================

void AstraGlanceView::SetExpanded(bool expanded) {
  SetDisplayMode(expanded ? DisplayMode::kExpanded : DisplayMode::kCompact);
}

bool AstraGlanceView::IsExpanded() const {
  return display_mode_ == DisplayMode::kExpanded;
}

// =========================================================================
// Action button visibility
// =========================================================================

void AstraGlanceView::SetShowCloseButton(bool show) {
  if (close_button_) {
    close_button_->SetVisible(show);
  }
}

void AstraGlanceView::SetShowPinButton(bool show) {
  if (pin_button_) {
    pin_button_->SetVisible(show);
  }
}

void AstraGlanceView::SetShowOpenButton(bool show) {
  if (promote_action_button_) {
    promote_action_button_->SetVisible(show);
  }
}

// =========================================================================
// Domain text
// =========================================================================

void AstraGlanceView::SetDomainText(const std::u16string& domain) {
  domain_text_ = domain;
  // TODO(astra): Add a dedicated domain label in the header.  For now,
  // we store the domain text and use it as the URL label in compact mode
  // or as the status text.
  if (status_label_) {
    status_label_->SetText(domain);
  }
}

// =========================================================================
// Loading state (boolean API)
// =========================================================================

void AstraGlanceView::SetLoading(bool is_loading) {
  if (is_loading) {
    SetLoadingState(LoadingState::kLoading);
  } else {
    SetLoadingState(LoadingState::kLoaded);
  }
}

bool AstraGlanceView::IsLoading() const {
  return loading_state_ == LoadingState::kLoading;
}

}  // namespace astra
