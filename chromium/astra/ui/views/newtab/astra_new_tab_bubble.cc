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

// =========================================================================
// AstraNewTabCustomizeBubble — customize/settings bubble implementation
// =========================================================================

namespace {

// Customize bubble constants.
constexpr int kCustomizeBubbleWidth = 320;
constexpr int kCustomizeBubbleMaxHeight = 520;
constexpr int kSectionSpacing = 16;
constexpr int kSectionHeaderSpacing = 12;
constexpr int kRowHeight = 40;
constexpr int kRowSpacing = 8;
constexpr int kResetButtonHeight = 36;
constexpr SkColor kSectionBackgroundColor = SK_ColorWHITE;
constexpr SkColor kSectionBorderColor = SkColorSetRGB(0xE0, 0xE0, 0xE0);
constexpr SkColor kHeaderTextColor = SkColorSetRGB(0x33, 0x33, 0x33);
constexpr SkColor kLabelTextColor = SkColorSetRGB(0x55, 0x55, 0x55);
constexpr SkColor kResetButtonColor = SkColorSetRGB(0xEA, 0x43, 0x35);

// A simple toggle switch view (custom-drawn for testability).
class ToggleSwitchView : public views::View {
 public:
  using ToggleCallback = base::RepeatingCallback<void(bool)>;

  ToggleSwitch(bool initial_value, ToggleCallback callback)
      : is_on_(initial_value), callback_(std::move(callback)) {
    SetPreferredSize(gfx::Size(36, 20));
    SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    SetAccessibleName(is_on_ ? u"Toggle switch: on" : u"Toggle switch: off");
  }

  ~ToggleSwitchView() override = default;

  void SetState(bool state) {
    if (is_on_ == state) return;
    is_on_ = state;
    SetAccessibleName(is_on_ ? u"Toggle switch: on" : u"Toggle switch: off");
    SchedulePaint();
  }

  bool is_on() const { return is_on_; }

  // views::View:
  bool OnMousePressed(const ui::MouseEvent& event) override {
    if (event.IsOnlyLeftMouseButton()) {
      Toggle();
      return true;
    }
    return views::View::OnMousePressed(event);
  }

  bool OnKeyPressed(const ui::KeyEvent& event) override {
    if (event.key_code() == ui::VKEY_SPACE ||
        event.key_code() == ui::VKEY_RETURN) {
      Toggle();
      return true;
    }
    return views::View::OnKeyPressed(event);
  }

  void OnPaint(gfx::Canvas* canvas) override {
    views::View::OnPaint(canvas);

    // Draw track.
    cc::PaintFlags track_flags;
    track_flags.setColor(is_on_ ? SkColorSetRGB(0x5B, 0x8F, 0xF9)
                                 : SkColorSetRGB(0xCC, 0xCC, 0xCC));
    track_flags.setStyle(cc::PaintFlags::kFill_Style);
    track_flags.setAntiAlias(true);
    gfx::RectF track_rect(GetLocalBounds());
    canvas->DrawRoundRect(track_rect, 10, track_flags);

    // Draw thumb.
    cc::PaintFlags thumb_flags;
    thumb_flags.setColor(SK_ColorWHITE);
    thumb_flags.setStyle(cc::PaintFlags::kFill_Style);
    thumb_flags.setAntiAlias(true);
    int thumb_size = 16;
    int thumb_x = is_on_ ? width() - thumb_size - 2 : 2;
    int thumb_y = (height() - thumb_size) / 2;
    gfx::RectF thumb_rect(thumb_x, thumb_y, thumb_size, thumb_size);
    canvas->DrawRoundRect(thumb_rect, thumb_size / 2.0f, thumb_flags);
  }

  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    views::View::GetAccessibleNodeData(node_data);
    node_data->role = ax::mojom::Role::kToggleButton;
    node_data->SetCheckedState(is_on_ ? ax::mojom::CheckedState::kTrue
                                      : ax::mojom::CheckedState::kFalse);
  }

 private:
  void Toggle() {
    is_on_ = !is_on_;
    SetAccessibleName(is_on_ ? u"Toggle switch: on" : u"Toggle switch: off");
    SchedulePaint();
    if (callback_) {
      callback_.Run(is_on_);
    }
  }

  bool is_on_;
  ToggleCallback callback_;
};

// A simple segmented control / selector view.
class SelectorView : public views::View {
 public:
  using SelectCallback = base::RepeatingCallback<void(int)>;

  SelectorView(const std::vector<std::u16string>& options,
               int selected_index,
               SelectCallback callback)
      : options_(options),
        selected_index_(selected_index),
        callback_(std::move(callback)) {
    DCHECK(!options_.empty());
    DCHECK_GE(selected_index_, 0);
    DCHECK_LT(selected_index_, static_cast<int>(options_.size()));
    SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    SetPreferredSize(gfx::Size(120, 28));
  }

  ~SelectorView() override = default;

  int selected_index() const { return selected_index_; }

  void SetSelectedIndex(int index) {
    if (index == selected_index_ || index < 0 ||
        index >= static_cast<int>(options_.size())) {
      return;
    }
    selected_index_ = index;
    SchedulePaint();
  }

  // views::View:
  bool OnMousePressed(const ui::MouseEvent& event) override {
    if (event.IsOnlyLeftMouseButton()) {
      // Calculate which segment was clicked.
      int segment_width = width() / static_cast<int>(options_.size());
      int clicked_index = event.x() / segment_width;
      if (clicked_index >= static_cast<int>(options_.size())) {
        clicked_index = static_cast<int>(options_.size()) - 1;
      }
      if (clicked_index != selected_index_) {
        selected_index_ = clicked_index;
        SchedulePaint();
        if (callback_) {
          callback_.Run(selected_index_);
        }
      }
      return true;
    }
    return views::View::OnMousePressed(event);
  }

  bool OnKeyPressed(const ui::KeyEvent& event) override {
    if (event.key_code() == ui::VKEY_LEFT || event.key_code() == ui::VKEY_UP) {
      if (selected_index_ > 0) {
        selected_index_--;
        SchedulePaint();
        if (callback_) callback_.Run(selected_index_);
      }
      return true;
    }
    if (event.key_code() == ui::VKEY_RIGHT ||
        event.key_code() == ui::VKEY_DOWN) {
      if (selected_index_ < static_cast<int>(options_.size()) - 1) {
        selected_index_++;
        SchedulePaint();
        if (callback_) callback_.Run(selected_index_);
      }
      return true;
    }
    return views::View::OnKeyPressed(event);
  }

  void OnPaint(gfx::Canvas* canvas) override {
    views::View::OnPaint(canvas);

    int count = static_cast<int>(options_.size());
    int segment_width = width() / count;

    cc::PaintFlags bg_flags;
    bg_flags.setColor(SkColorSetRGB(0xF0, 0xF0, 0xF0));
    bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    bg_flags.setAntiAlias(true);
    canvas->DrawRoundRect(GetLocalBounds(), 6, bg_flags);

    // Draw selected segment highlight.
    cc::PaintFlags sel_flags;
    sel_flags.setColor(SK_ColorWHITE);
    sel_flags.setStyle(cc::PaintFlags::kFill_Style);
    sel_flags.setAntiAlias(true);
    gfx::Rect sel_rect(selected_index_ * segment_width + 1, 1,
                        segment_width - 2, height() - 2);
    canvas->DrawRoundRect(gfx::RectF(sel_rect), 5, sel_flags);

    // Draw labels.
    gfx::FontList font_list;
    font_list = font_list.Derive(-1, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM);
    for (int i = 0; i < count; ++i) {
      gfx::Rect text_rect(i * segment_width, 0, segment_width, height());
      SkColor text_color = (i == selected_index_)
                               ? SkColorSetRGB(0x33, 0x33, 0x33)
                               : SkColorSetRGB(0x88, 0x88, 0x88);
      canvas->DrawStringRect(options_[i], font_list, text_color, text_rect,
                             gfx::ALIGN_CENTER, gfx::ALIGN_MIDDLE);
    }
  }

  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    views::View::GetAccessibleNodeData(node_data);
    node_data->role = ax::mojom::Role::kRadioGroup;
  }

 private:
  std::vector<std::u16string> options_;
  int selected_index_;
  SelectCallback callback_;
};

}  // namespace

// =========================================================================
// Customize bubble — static factory
// =========================================================================

views::Widget* AstraNewTabCustomizeBubble::ShowBubble(
    views::View* anchor_view,
    AstraNewTabModel* model,
    Delegate* delegate) {
  DCHECK(anchor_view);
  DCHECK(model);

  auto* bubble =
      new AstraNewTabCustomizeBubble(anchor_view, model, delegate);
  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(bubble);
  bubble->SetArrow(views::BubbleBorder::TOP_RIGHT);
  widget->Show();
  return widget;
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraNewTabCustomizeBubble::AstraNewTabCustomizeBubble(
    views::View* anchor_view,
    AstraNewTabModel* model,
    Delegate* delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT,
                                       views::BubbleBorder::STANDARD_SHADOW),
      model_(model),
      delegate_(delegate) {
  SetAcceptCallback(base::DoNothing());
  SetCancelCallback(base::DoNothing());
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowTitle(false);
  SetShowCloseButton(true);

  set_fixed_width(kCustomizeBubbleWidth);
  set_close_on_deactivate(true);
  set_modal_type(ui::MODAL_TYPE_NONE);
}

AstraNewTabCustomizeBubble::~AstraNewTabCustomizeBubble() {
  if (delegate_) {
    delegate_->OnCustomizeBubbleClosed();
  }
}

// =========================================================================
// Init / layout
// =========================================================================

void AstraNewTabCustomizeBubble::Init() {
  auto* root_layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(16, 16),
      /*between_child_spacing=*/kSectionSpacing));
  root_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  set_background(
      views::Background::CreateSolidBackground(SK_ColorWHITE));

  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetBackgroundColor(SK_ColorTRANSPARENT);
  scroll_view_->SetPaintToLayer();
  scroll_view_->layer()->SetFillsBoundsOpaquely(false);
  root_layout->SetFlexForView(scroll_view_, 1);

  auto content_view = std::make_unique<views::View>();
  auto* content_layout = content_view->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, 0),
          /*between_child_spacing=*/kSectionSpacing));
  content_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  BuildSections();

  // Move sections to the scroll view content.
  for (auto& section : sections_) {
    content_view->AddChildView(
        section->parent()->RemoveChildViewT(section));
  }
  sections_.clear();

  scroll_view_->SetContents(std::move(content_view));

  // Reset button at the bottom.
  auto reset_button = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnResetButtonPressed,
                          base::Unretained(this)),
      u"Reset to defaults");
  reset_button->SetPreferredSize(gfx::Size(0, kResetButtonHeight));
  reset_button->SetEnabledColor(kResetButtonColor);
  reset_button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  reset_button->SetAccessibleName(u"Reset to defaults");
  AddChildView(std::move(reset_button));
}

void AstraNewTabCustomizeBubble::BuildSections() {
  // ---- Appearance section ----
  auto appearance_section = BuildSectionHeader(u"Appearance");
  auto* appearance_ptr = appearance_section.get();
  AddChildView(std::move(appearance_section));
  sections_.push_back(appearance_ptr);

  // Theme mode selector.
  AddChildView(BuildSelectorRow(
      u"Theme",
      {u"System", u"Light", u"Dark"},
      static_cast<int>(model_->theme_mode()),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnThemeModeChanged,
                          base::Unretained(this))));

  // Layout density selector.
  AddChildView(BuildSelectorRow(
      u"Density",
      {u"Comfortable", u"Cozy", u"Compact"},
      static_cast<int>(model_->layout_density()),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnLayoutDensityChanged,
                          base::Unretained(this))));

  // Greeting style selector.
  AddChildView(BuildSelectorRow(
      u"Greeting",
      {u"Formal", u"Casual", u"Minimal"},
      static_cast<int>(model_->greeting_style()),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnGreetingStyleChanged,
                          base::Unretained(this))));

  // Clock format selector.
  AddChildView(BuildSelectorRow(
      u"Clock",
      {u"12h", u"24h", u"System"},
      static_cast<int>(model_->clock_format()),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnClockFormatChanged,
                          base::Unretained(this))));

  // Toggle show seconds.
  AddChildView(BuildToggleRow(
      u"Show seconds",
      model_->show_seconds(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleSeconds,
                          base::Unretained(this))));

  // Toggle show date.
  AddChildView(BuildToggleRow(
      u"Show date",
      model_->show_date(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleDate,
                          base::Unretained(this))));

  // ---- Background section ----
  auto bg_section = BuildSectionHeader(u"Background");
  auto* bg_ptr = bg_section.get();
  AddChildView(std::move(bg_section));
  sections_.push_back(bg_ptr);

  AddChildView(BuildSelectorRow(
      u"Style",
      {u"Solid", u"Gradient", u"Image", u"Daily"},
      static_cast<int>(model_->background_style()),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnBackgroundStyleChanged,
                          base::Unretained(this))));

  // ---- Shortcuts section ----
  auto sc_section = BuildSectionHeader(u"Shortcuts");
  auto* sc_ptr = sc_section.get();
  AddChildView(std::move(sc_section));
  sections_.push_back(sc_ptr);

  AddChildView(BuildToggleRow(
      u"Show shortcuts",
      model_->show_shortcuts(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleShortcuts,
                          base::Unretained(this))));

  AddChildView(BuildToggleRow(
      u"Show titles",
      model_->show_shortcut_titles(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleShortcutTitles,
                          base::Unretained(this))));

  AddChildView(BuildToggleRow(
      u"Most visited",
      model_->show_most_visited(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleMostVisited,
                          base::Unretained(this))));

  AddChildView(BuildSelectorRow(
      u"Layout",
      {u"Grid", u"List"},
      static_cast<int>(model_->shortcut_layout_mode()),
      base::BindRepeating(
          &AstraNewTabCustomizeBubble::OnShortcutLayoutModeChanged,
          base::Unretained(this))));

  AddChildView(BuildSelectorRow(
      u"Icon size",
      {u"Small", u"Medium", u"Large"},
      static_cast<int>(model_->shortcut_icon_size()),
      base::BindRepeating(
          &AstraNewTabCustomizeBubble::OnShortcutIconSizeChanged,
          base::Unretained(this))));

  AddChildView(BuildSliderRow(
      u"Columns", AstraNewTabModel::kMinShortcutColumns,
      AstraNewTabModel::kMaxShortcutColumns, model_->shortcut_columns(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnShortcutColumnsChanged,
                          base::Unretained(this))));

  // ---- Workspace section ----
  auto ws_section = BuildSectionHeader(u"Workspaces");
  auto* ws_ptr = ws_section.get();
  AddChildView(std::move(ws_section));
  sections_.push_back(ws_ptr);

  AddChildView(BuildToggleRow(
      u"Show workspace cards",
      model_->show_workspace_cards(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleWorkspaces,
                          base::Unretained(this))));

  AddChildView(BuildSelectorRow(
      u"Card style",
      {u"Compact", u"Full", u"Grid"},
      static_cast<int>(model_->workspace_card_style()),
      base::BindRepeating(
          &AstraNewTabCustomizeBubble::OnWorkspaceCardStyleChanged,
          base::Unretained(this))));

  // ---- Content section ----
  auto content_section = BuildSectionHeader(u"Content");
  auto* content_ptr = content_section.get();
  AddChildView(std::move(content_section));
  sections_.push_back(content_ptr);

  AddChildView(BuildToggleRow(
      u"Recently closed",
      model_->show_recently_closed(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleRecentlyClosed,
                          base::Unretained(this))));

  AddChildView(BuildToggleRow(
      u"Quick actions",
      model_->show_quick_actions(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleQuickActions,
                          base::Unretained(this))));

  AddChildView(BuildToggleRow(
      u"Suggested content",
      model_->show_suggested_content(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleSuggestedContent,
                          base::Unretained(this))));

  AddChildView(BuildToggleRow(
      u"Greeting",
      model_->show_greeting(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleGreeting,
                          base::Unretained(this))));

  AddChildView(BuildToggleRow(
      u"Search bar",
      model_->show_search_bar(),
      base::BindRepeating(&AstraNewTabCustomizeBubble::OnToggleSearchBar,
                          base::Unretained(this))));
}

std::unique_ptr<views::View> AstraNewTabCustomizeBubble::BuildSectionHeader(
    const std::u16string& title) {
  auto header = std::make_unique<views::Label>(title);
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetEnabledColor(kHeaderTextColor);
  header->SetFontList(header->font_list().Derive(
      1, gfx::Font::NORMAL, gfx::Font::Weight::BOLD));
  header->SetPreferredSize(gfx::Size(0, 24));
  return header;
}

std::unique_ptr<views::View> AstraNewTabCustomizeBubble::BuildToggleRow(
    const std::u16string& label,
    bool initial_value,
    base::RepeatingCallback<void(bool)> callback) {
  auto row = std::make_unique<views::View>();
  auto* row_layout = row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(8, 4),
      /*between_child_spacing=*/8));
  row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);

  auto label_view = std::make_unique<views::Label>(label);
  label_view->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label_view->SetAutoColorReadabilityEnabled(false);
  label_view->SetEnabledColor(kLabelTextColor);
  label_view->SetFontList(label_view->font_list().Derive(
      0, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
  row_layout->SetFlexForView(label_view.get(), 1);
  row->AddChildView(std::move(label_view));

  auto toggle = std::make_unique<ToggleSwitchView>(
      initial_value, std::move(callback));
  row->AddChildView(std::move(toggle));

  return row;
}

std::unique_ptr<views::View> AstraNewTabCustomizeBubble::BuildSelectorRow(
    const std::u16string& label,
    const std::vector<std::u16string>& options,
    int selected_index,
    base::RepeatingCallback<void(int)> callback) {
  auto row = std::make_unique<views::View>();
  auto* row_layout = row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(8, 4),
      /*between_child_spacing=*/8));
  row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);

  auto label_view = std::make_unique<views::Label>(label);
  label_view->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label_view->SetAutoColorReadabilityEnabled(false);
  label_view->SetEnabledColor(kLabelTextColor);
  label_view->SetFontList(label_view->font_list().Derive(
      0, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
  row_layout->SetFlexForView(label_view.get(), 1);
  row->AddChildView(std::move(label_view));

  auto selector = std::make_unique<SelectorView>(
      options, selected_index, std::move(callback));
  row->AddChildView(std::move(selector));

  return row;
}

std::unique_ptr<views::View> AstraNewTabCustomizeBubble::BuildSliderRow(
    const std::u16string& label,
    int min_value,
    int max_value,
    int current_value,
    base::RepeatingCallback<void(int)> callback) {
  // Simple slider using an integer text display (for testing).
  // In production, this would use views::Slider.
  auto row = std::make_unique<views::View>();
  auto* row_layout = row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(8, 4),
      /*between_child_spacing=*/8));
  row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);

  auto label_view = std::make_unique<views::Label>(label);
  label_view->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label_view->SetAutoColorReadabilityEnabled(false);
  label_view->SetEnabledColor(kLabelTextColor);
  row_layout->SetFlexForView(label_view.get(), 1);
  row->AddChildView(std::move(label_view));

  // Simple numeric display + buttons.
  auto value_label = std::make_unique<views::Label>(
      base::NumberToString16(current_value));
  value_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  value_label->SetAutoColorReadabilityEnabled(false);
  value_label->SetEnabledColor(kLabelTextColor);
  value_label->SetPreferredSize(gfx::Size(32, 20));
  row->AddChildView(std::move(value_label));

  return row;
}

// =========================================================================
// Widget callbacks
// =========================================================================

void AstraNewTabCustomizeBubble::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);
}

// =========================================================================
// Settings change handlers
// =========================================================================

void AstraNewTabCustomizeBubble::OnBackgroundStyleChanged(int index) {
  model_->set_background_style(
      static_cast<AstraNtpBackgroundStyle>(index));
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnShortcutColumnsChanged(int value) {
  model_->set_shortcut_columns(value);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnShortcutLayoutModeChanged(int index) {
  model_->set_shortcut_layout_mode(
      static_cast<AstraNtpShortcutLayoutMode>(index));
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnShortcutIconSizeChanged(int index) {
  model_->set_shortcut_icon_size(
      static_cast<AstraNtpShortcutIconSize>(index));
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnWorkspaceCardStyleChanged(int index) {
  model_->set_workspace_card_style(
      static_cast<AstraNtpWorkspaceCardStyle>(index));
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnThemeModeChanged(int index) {
  model_->set_theme_mode(static_cast<AstraNtpThemeMode>(index));
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnLayoutDensityChanged(int index) {
  model_->set_layout_density(
      static_cast<AstraNtpLayoutDensity>(index));
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnGreetingStyleChanged(int index) {
  model_->set_greeting_style(
      static_cast<AstraNtpGreetingStyle>(index));
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnClockFormatChanged(int index) {
  model_->set_clock_format(static_cast<AstraNtpClockFormat>(index));
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnSearchBarStyleChanged(int index) {
  model_->set_search_bar_style(
      static_cast<AstraNtpSearchBarStyle>(index));
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleShortcuts(bool enabled) {
  model_->set_show_shortcuts(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleWorkspaces(bool enabled) {
  model_->set_show_workspace_cards(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleRecentlyClosed(bool enabled) {
  model_->set_show_recently_closed(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleQuickActions(bool enabled) {
  model_->set_show_quick_actions(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleGreeting(bool enabled) {
  model_->set_show_greeting(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleSearchBar(bool enabled) {
  model_->set_show_search_bar(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleSuggestedContent(bool enabled) {
  model_->set_show_suggested_content(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleShortcutTitles(bool enabled) {
  model_->set_show_shortcut_titles(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleSeconds(bool enabled) {
  model_->set_show_seconds(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleDate(bool enabled) {
  model_->set_show_date(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnToggleMostVisited(bool enabled) {
  model_->set_show_most_visited(enabled);
  if (delegate_) delegate_->OnCustomizeSettingsChanged();
}

void AstraNewTabCustomizeBubble::OnResetButtonPressed() {
  if (model_) {
    model_->ResetSettingsToDefaults();
  }
  if (delegate_) {
    delegate_->OnCustomizeResetToDefaults();
    delegate_->OnCustomizeSettingsChanged();
  }
}

// =========================================================================
// Public test helpers
// =========================================================================

void AstraNewTabCustomizeBubble::ResetToDefaults() {
  if (model_) {
    model_->ResetSettingsToDefaults();
  }
}

void AstraNewTabCustomizeBubble::ToggleSetting(const std::string& setting_key) {
  // Handle boolean settings by key (for testing).
  if (setting_key == "show_shortcuts") {
    OnToggleShortcuts(!model_->show_shortcuts());
  } else if (setting_key == "show_workspaces") {
    OnToggleWorkspaces(!model_->show_workspace_cards());
  } else if (setting_key == "show_recently_closed") {
    OnToggleRecentlyClosed(!model_->show_recently_closed());
  } else if (setting_key == "show_quick_actions") {
    OnToggleQuickActions(!model_->show_quick_actions());
  } else if (setting_key == "show_greeting") {
    OnToggleGreeting(!model_->show_greeting());
  } else if (setting_key == "show_search_bar") {
    OnToggleSearchBar(!model_->show_search_bar());
  } else if (setting_key == "show_suggested_content") {
    OnToggleSuggestedContent(!model_->show_suggested_content());
  } else if (setting_key == "show_shortcut_titles") {
    OnToggleShortcutTitles(!model_->show_shortcut_titles());
  } else if (setting_key == "show_seconds") {
    OnToggleSeconds(!model_->show_seconds());
  } else if (setting_key == "show_date") {
    OnToggleDate(!model_->show_date());
  } else if (setting_key == "show_most_visited") {
    OnToggleMostVisited(!model_->show_most_visited());
  }
}

bool AstraNewTabCustomizeBubble::GetBooleanSetting(
    const std::string& setting_key) const {
  if (!model_) return false;
  if (setting_key == "show_shortcuts") return model_->show_shortcuts();
  if (setting_key == "show_workspaces") return model_->show_workspace_cards();
  if (setting_key == "show_recently_closed") return model_->show_recently_closed();
  if (setting_key == "show_quick_actions") return model_->show_quick_actions();
  if (setting_key == "show_greeting") return model_->show_greeting();
  if (setting_key == "show_search_bar") return model_->show_search_bar();
  if (setting_key == "show_suggested_content")
    return model_->show_suggested_content();
  if (setting_key == "show_shortcut_titles")
    return model_->show_shortcut_titles();
  if (setting_key == "show_seconds") return model_->show_seconds();
  if (setting_key == "show_date") return model_->show_date();
  if (setting_key == "show_most_visited") return model_->show_most_visited();
  return false;
}

int AstraNewTabCustomizeBubble::GetIntSetting(
    const std::string& setting_key) const {
  if (!model_) return 0;
  if (setting_key == "shortcut_columns") return model_->shortcut_columns();
  if (setting_key == "theme_mode") return static_cast<int>(model_->theme_mode());
  if (setting_key == "layout_density")
    return static_cast<int>(model_->layout_density());
  if (setting_key == "greeting_style")
    return static_cast<int>(model_->greeting_style());
  if (setting_key == "clock_format")
    return static_cast<int>(model_->clock_format());
  if (setting_key == "shortcut_layout_mode")
    return static_cast<int>(model_->shortcut_layout_mode());
  if (setting_key == "shortcut_icon_size")
    return static_cast<int>(model_->shortcut_icon_size());
  if (setting_key == "workspace_card_style")
    return static_cast<int>(model_->workspace_card_style());
  if (setting_key == "background_style")
    return static_cast<int>(model_->background_style());
  return 0;
}

}  // namespace astra
