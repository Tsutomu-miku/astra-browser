#include "astra/ui/views/newtab/astra_new_tab_bubble.h"

#include <memory>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "ui/color/color_id.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/ui/views/newtab/astra_new_tab_view.h"

namespace astra {

namespace {

// Bubble sizing constants.
constexpr int kBubbleStandardWidth = 960;
constexpr int kBubbleStandardHeight = 720;
constexpr int kBubbleLargeWidth = 1100;
constexpr int kBubbleLargeHeight = 800;

// Search bar constants.
constexpr int kSearchBarHeight = 44;
constexpr int kSearchBarHorizontalPadding = 16;
constexpr int kSearchBarCornerRadius = 12;
constexpr int kSearchIconSize = 18;
constexpr int kSearchBarBottomMargin = 20;

// Scroll view constants.
constexpr int kScrollViewContentWidth = kBubbleStandardWidth - 48;

// Search bar character (magnifying glass) — used as a text icon.
// TODO(astra): Use a proper vector icon from Chrome's icon set.
// Chromium pattern: views::ImageButton with vector icon and vector_icon.h.
constexpr char16_t kSearchIconChar = 0x1F50D;  // 🔍

// Colors (placeholder — should use Astra color system).
// TODO(astra): Define Astra-specific color IDs in astra/ui/color/.
// Chromium pattern: Use ui::ColorId and ColorProvider mixin.
constexpr SkColor kSearchBarBackgroundColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kSearchBarBorderColor = SkColorSetRGB(0xE0, 0xE0, 0xE0);
constexpr SkColor kSearchTextColor = SkColorSetRGB(0x20, 0x20, 0x20);
constexpr SkColor kSearchPlaceholderColor = SkColorSetRGB(0x99, 0x99, 0x99);
constexpr SkColor kBubbleBackgroundColor = SkColorSetRGB(0xF8, 0xF9, 0xFA);

}  // namespace

// =========================================================================
// Static factory
// =========================================================================

views::Widget* AstraNewTabBubble::ShowBubble(views::View* anchor_view,
                                             Browser* browser,
                                             Delegate* delegate,
                                             SizeMode size_mode) {
  DCHECK(anchor_view);
  DCHECK(browser);

  // Create the bubble delegate.  BubbleDialogDelegateView creates its own
  // widget when shown.  The widget owns the delegate and will delete it
  // when the widget is destroyed.
  auto* bubble =
      new AstraNewTabBubble(anchor_view, browser, delegate, size_mode);

  // Show the bubble widget.
  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(bubble);
  bubble->SetArrow(views::BubbleBorder::TOP_LEFT);
  // TODO(astra): Decide the correct arrow position for the NTP bubble.
  // If shown from the sidebar, TOP_LEFT is appropriate.
  // If shown as a centered overlay, no arrow would be better.

  widget->Show();

  // Give focus to the search field after showing.
  bubble->RequestSearchFocus();

  // TODO(astra): Play entrance animation after widget is shown.
  // Currently the animation is a no-op stub.
  bubble->PlayEntranceAnimation();

  return widget;
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraNewTabBubble::AstraNewTabBubble(views::View* anchor_view,
                                     Browser* browser,
                                     Delegate* delegate,
                                     SizeMode size_mode)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_LEFT,
                                       views::BubbleBorder::STANDARD_SHADOW),
      browser_(browser),
      delegate_(delegate),
      size_mode_(size_mode) {
  // Set bubble properties.
  SetAcceptCallback(base::DoNothing());
  SetCancelCallback(base::DoNothing());
  // No dialog buttons — the NTP is an interactive surface, not a dialog.
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowTitle(false);
  SetShowCloseButton(true);

  // Set bubble size based on mode.
  // TODO(astra): Make the NTP bubble responsive to window size, or show it
  // as a full-content overlay instead of a fixed-size bubble.
  int width = kBubbleStandardWidth;
  int height = kBubbleStandardHeight;
  switch (size_mode_) {
    case SizeMode::kStandard:
      width = kBubbleStandardWidth;
      height = kBubbleStandardHeight;
      break;
    case SizeMode::kLarge:
      width = kBubbleLargeWidth;
      height = kBubbleLargeHeight;
      break;
    case SizeMode::kFullWindow:
      // TODO(astra): Full-window mode should size to the browser content area.
      // For now, use large size as a stand-in.
      // Chromium pattern: Use GetWidget()->GetWindowBoundsInScreen() or
      // browser->window()->GetBounds() to calculate full-window size.
      width = kBubbleLargeWidth;
      height = kBubbleLargeHeight;
      break;
  }
  set_fixed_width(width);
  set_fixed_height(height);

  // Auto-dismiss when the widget loses activation.
  // The NTP bubble is modeless — it stays open while the user interacts
  // with it, but closes when they click elsewhere.
  // TODO(astra): Consider whether the NTP should auto-dismiss.
  // Chrome's NTP is a full page, not a bubble, so it doesn't dismiss on
  // loss of focus.  For an overlay NTP, auto-dismiss may be appropriate.
  set_close_on_deactivate(true);

  // The bubble is modeless (not modal).
  set_modal_type(ui::MODAL_TYPE_NONE);
}

AstraNewTabBubble::~AstraNewTabBubble() {
  // Notify the delegate that the bubble is closing.
  if (delegate_) {
    delegate_->OnNewTabBubbleClosed();
  }
}

// =========================================================================
// BubbleDialogDelegateView overrides
// =========================================================================

void AstraNewTabBubble::Init() {
  BuildLayout();
}

void AstraNewTabBubble::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);
  // The widget is being destroyed — the delegate will be notified by
  // the destructor.
}

void AstraNewTabBubble::OnWidgetActivationChanged(views::Widget* widget,
                                                  bool active) {
  views::BubbleDialogDelegateView::OnWidgetActivationChanged(widget, active);
  // Auto-close on deactivation is handled by set_close_on_deactivate(true).
}

void AstraNewTabBubble::OnWidgetShown(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetShown(widget);
  // Widget is now visible — entrance animation was started in ShowBubble().
  // TODO(astra): If entrance animation needs to be triggered after paint,
  // do it here.
}

// =========================================================================
// TextfieldController overrides
// =========================================================================

void AstraNewTabBubble::ContentsChanged(views::Textfield* sender,
                                        const std::u16string& new_contents) {
  // TODO(astra): Provide real-time search suggestions as the user types.
  // Chromium subsystem: OmniboxView / AutocompleteController.
  // For now, we just wait for Enter to submit.
}

bool AstraNewTabBubble::HandleKeyEvent(views::Textfield* sender,
                                       const ui::KeyEvent& key_event) {
  if (key_event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }

  switch (key_event.key_code()) {
    case ui::VKEY_RETURN:
      // Submit search / navigate.
      if (delegate_ && sender) {
        delegate_->OnNewTabSearchSubmitted(sender->GetText());
      }
      // Close the bubble after submission.
      GetWidget()->Close();
      return true;

    case ui::VKEY_ESCAPE:
      // Escape closes the bubble.
      GetWidget()->Close();
      return true;

    default:
      break;
  }

  return false;
}

// =========================================================================
// Public API
// =========================================================================

void AstraNewTabBubble::RequestSearchFocus() {
  if (search_field_) {
    search_field_->RequestFocus();
  }
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraNewTabBubble::BuildLayout() {
  // Root layout: vertical stack of search bar + scrollable content.
  auto* root_layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(24, 24),
      /*between_child_spacing=*/0));
  root_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Set bubble background color.
  set_background(
      views::Background::CreateSolidBackground(kBubbleBackgroundColor));

  // ---- Search / omnibox bar ----
  auto* search_bar = AddChildView(std::make_unique<views::View>());
  search_bar->SetPreferredSize(gfx::Size(0, kSearchBarHeight));
  auto* search_layout = search_bar->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kSearchBarHorizontalPadding),
          /*between_child_spacing=*/8));
  search_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  search_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);

  // Search bar background with rounded corners.
  search_bar->SetBackground(views::Background::CreateRoundedRectBackground(
      kSearchBarBackgroundColor, kSearchBarCornerRadius));
  // Subtle border.
  search_bar->SetBorder(views::CreateRoundedRectBorder(
      /*thickness=*/1, kSearchBarCornerRadius, kSearchBarBorderColor));

  // Search icon (magnifying glass character — lightweight icon placeholder).
  auto search_icon = std::make_unique<views::Label>(
      std::u16string(1, kSearchIconChar));
  search_icon->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  search_icon->SetAutoColorReadabilityEnabled(false);
  search_icon->SetEnabledColor(kSearchPlaceholderColor);
  search_icon->SetPreferredSize(gfx::Size(kSearchIconSize, kSearchIconSize));
  search_icon->SetFontList(
      search_icon->font_list().DeriveWithSizeDelta(0));
  search_bar->AddChildView(std::move(search_icon));

  // Search textfield — takes remaining space.
  auto textfield = std::make_unique<views::Textfield>();
  textfield->SetController(this);
  textfield->SetPlaceholderText(u"Search Google or type a URL");
  textfield->SetBackgroundColor(SK_ColorTRANSPARENT);
  textfield->SetBorder(views::NullBorder());
  textfield->SetTextColor(kSearchTextColor);
  textfield->SetPlaceholderTextColor(kSearchPlaceholderColor);
  textfield->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  textfield->SetFontList(textfield->font_list().DeriveWithSizeDelta(2));
  textfield->SetAccessibleName(u"Search or enter web address");
  search_layout->SetFlexForView(textfield.get(), 1);
  search_field_ = textfield.get();
  search_bar->AddChildView(std::move(textfield));

  // Add bottom margin below search bar.
  auto* spacer = AddChildView(std::make_unique<views::View>());
  spacer->SetPreferredSize(gfx::Size(0, kSearchBarBottomMargin));

  // ---- Scrollable content area ----
  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetBackgroundColor(SK_ColorTRANSPARENT);
  scroll_view_->SetPaintToLayer();
  scroll_view_->layer()->SetFillsBoundsOpaquely(false);
  // Let the scroll view expand to fill remaining space.
  root_layout->SetFlexForView(scroll_view_, 1);

  // Content view inside scroll view — the NTP main view.
  auto content_view = std::make_unique<AstraNewTabView>(browser_);
  new_tab_view_ = content_view.get();

  // Create the NTP controller.  The controller owns the model and mediates
  // between the model and the view.
  controller_ = std::make_unique<AstraNewTabController>(
      browser_, new_tab_view_, this);

  // The controller implements AstraNewTabView::Delegate, so it receives
  // user actions directly from the view.
  new_tab_view_->SetDelegate(controller_.get());

  // Set the view's model from the controller.
  new_tab_view_->SetModel(controller_->model());

  // Refresh the view content from the model.
  new_tab_view_->RefreshContent();
  new_tab_view_->UpdateFromSettings();

  scroll_view_->SetContents(std::move(content_view));
}

void AstraNewTabBubble::PlayEntranceAnimation() {
  // TODO(astra): Implement fade-in + slight scale-up entrance animation.
  //
  // Chromium pattern:
  //   - Get the widget's layer: GetWidget()->GetLayer()
  //   - Use ui::LayerAnimator to animate opacity from 0 to 1
  //   - Use transform for scale from 0.95 to 1.0
  //   - Duration ~150ms, fast-out-slow-in easing
  //
  // Reference: chrome/browser/ui/views/bubble/bubble_dialog_delegate_view.cc
  //   has AnimateShow() / AnimateClose() methods.
  //
  // For now, this is a no-op stub.  The bubble appears instantly.
}

// =========================================================================
// AstraNewTabController::Delegate — bridge controller actions to outer delegate
// =========================================================================

void AstraNewTabBubble::OnNavigateToURL(const GURL& url) {
  if (delegate_) {
    delegate_->OnNewTabNavigateToURL(url);
  }
}

void AstraNewTabBubble::OnOpenWorkspace(const std::string& workspace_id) {
  if (delegate_) {
    delegate_->OnNewTabOpenWorkspace(workspace_id);
  }
}

void AstraNewTabBubble::OnNewWorkspace() {
  if (delegate_) {
    delegate_->OnNewTabNewWorkspace();
  }
}

void AstraNewTabBubble::OnShowAllWorkspaces() {
  if (delegate_) {
    delegate_->OnNewTabShowAllWorkspaces();
  }
}

void AstraNewTabBubble::OnQuickAction(const std::string& action_id) {
  if (delegate_) {
    delegate_->OnNewTabQuickAction(action_id);
  }
}

void AstraNewTabBubble::OnRestoreRecentlyClosed(int session_id) {
  if (delegate_) {
    delegate_->OnNewTabRestoreRecentlyClosed(session_id);
  }
}

void AstraNewTabBubble::OnShowShortcutContextMenu(
    const GURL& url,
    const gfx::Point& screen_point) {
  if (delegate_) {
    delegate_->OnNewTabShortcutContextMenu(url, screen_point);
  }
}

void AstraNewTabBubble::OnShowWorkspaceContextMenu(
    const std::string& workspace_id,
    const gfx::Point& screen_point) {
  if (delegate_) {
    delegate_->OnNewTabWorkspaceContextMenu(workspace_id, screen_point);
  }
}

void AstraNewTabBubble::OnSettingsGearPressed() {
  // TODO(astra): Show customize / settings menu for the NTP.
  // The settings gear button triggers this via the controller delegate.
}

}  // namespace astra
