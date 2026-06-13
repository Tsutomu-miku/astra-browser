#include "astra/ui/views/settings/astra_settings_bubble.h"

#include <memory>
#include <utility>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/ui/views/settings/astra_settings_page_view.h"
#include "astra/ui/views/settings/astra_settings_search_box.h"

namespace astra {

namespace {

// Bubble sizing constants (defaults).
constexpr int kDefaultBubbleWidth = 440;
constexpr int kDefaultBubbleMinHeight = 200;
constexpr int kDefaultBubbleMaxHeight = 640;
constexpr int kHeaderBottomPadding = 12;
constexpr int kHeaderHorizontalPadding = 0;

// TODO(astra): Move these to astra/ui/color/ when we have a color system.
constexpr int kHeaderBottomSeparatorHeight = 1;

}  // namespace

// =========================================================================
// Singleton tracking
// =========================================================================

AstraSettingsBubble* AstraSettingsBubble::instance_ = nullptr;
views::Widget* AstraSettingsBubble::instance_widget_ = nullptr;

// =========================================================================
// Static show/hide API
// =========================================================================

views::Widget* AstraSettingsBubble::ShowBubble(views::View* anchor_view,
                                               Browser* browser,
                                               Delegate* delegate) {
  DCHECK(anchor_view);
  DCHECK(browser);

  // If the bubble is already open, just activate it instead of creating
  // a new one.
  if (instance_widget_ && instance_) {
    instance_widget_->Show();
    instance_widget_->Activate();
    // Focus the search box.
    if (instance_->search_box()) {
      // Use the textfield for focus if available.
      // The search box doesn't expose textfield() directly in its public
      // interface, so we request focus on the search box itself.
      instance_->search_box()->RequestFocus();
    }
    return instance_widget_;
  }

  // Create the bubble delegate.  BubbleDialogDelegateView creates its own
  // widget when shown.  The widget owns the delegate and will delete it
  // when the widget is destroyed.
  auto* bubble = new AstraSettingsBubble(anchor_view, browser, delegate);

  // Show the bubble widget.  CreateBubble returns the widget, which is
  // owned by the native widget system.
  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(bubble);
  bubble->SetArrow(views::BubbleBorder::TOP_LEFT);
  // TODO(astra): Adjust the arrow position based on where the bubble is
  // anchored.  For a toolbar settings button, TOP_RIGHT makes sense if
  // the button is on the right side of the toolbar.
  // For a sidebar settings button (left side), TOP_LEFT is appropriate.

  // Track the singleton instance.
  instance_ = bubble;
  instance_widget_ = widget;

  widget->Show();

  // Notify the delegate that the bubble is opening.
  if (delegate) {
    delegate->OnSettingsBubbleOpened();
  }

  // Focus the search box by default so users can start typing immediately.
  if (bubble->search_box()) {
    bubble->search_box()->RequestFocus();
  }

  return widget;
}

void AstraSettingsBubble::HideBubble() {
  if (instance_widget_) {
    instance_widget_->Close();
    // instance_ and instance_widget_ will be cleared in OnWidgetDestroying.
  }
}

bool AstraSettingsBubble::IsBubbleVisible() {
  return instance_widget_ != nullptr;
}

views::Widget* AstraSettingsBubble::GetBubbleWidget() {
  return instance_widget_;
}

AstraSettingsBubble* AstraSettingsBubble::GetBubble() {
  return instance_;
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSettingsBubble::AstraSettingsBubble(views::View* anchor_view,
                                         Browser* browser,
                                         Delegate* delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_LEFT,
                                       views::BubbleBorder::STANDARD_SHADOW),
      browser_(browser),
      delegate_(delegate),
      bubble_width_(kDefaultBubbleWidth),
      bubble_max_height_(kDefaultBubbleMaxHeight) {
  // Create the settings model.
  if (browser_ && browser_->profile()) {
    settings_model_ = std::make_unique<AstraSettingsModel>(
        browser_->profile()->GetPrefs());
    // Observe the model to forward notifications to the delegate.
    settings_model_->AddObserver(this);
  }

  // Set bubble properties.
  SetAcceptCallback(base::DoNothing());
  SetCancelCallback(base::DoNothing());
  // Use only a close button — settings changes are applied immediately.
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowTitle(true);
  SetTitle(u"Astra Settings");
  SetShowCloseButton(true);
  set_fixed_width(bubble_width_);
  set_min_height(kDefaultBubbleMinHeight);

  // Auto-dismiss when the widget loses activation (user clicks outside).
  // This matches the behavior of other Chrome bubbles (bookmark bubble,
  // download bubble, etc.).
  set_close_on_deactivate(true);
}

AstraSettingsBubble::~AstraSettingsBubble() {
  // Remove ourselves as an observer of the model.
  if (settings_model_) {
    settings_model_->RemoveObserver(this);
  }

  // Clear singleton tracking if this is the current instance.
  if (instance_ == this) {
    instance_ = nullptr;
    instance_widget_ = nullptr;
  }

  // Notify the delegate that the bubble is closing.
  if (delegate_) {
    delegate_->OnSettingsBubbleClosed();
  }
}

// =========================================================================
// Navigation
// =========================================================================

void AstraSettingsBubble::ShowMainPage() {
  if (settings_page_view_) {
    settings_page_view_->ShowMainPage();
    if (delegate_) {
      delegate_->OnSettingsNavigationChanged();
    }
  }
}

void AstraSettingsBubble::ShowSection(const std::string& section_id) {
  if (settings_page_view_) {
    settings_page_view_->ShowSection(section_id);
    if (delegate_) {
      delegate_->OnSettingsNavigationChanged();
    }
  }
}

void AstraSettingsBubble::ShowSearchResults(const std::u16string& query) {
  if (settings_page_view_) {
    settings_page_view_->ShowSearchResults(query);
    if (delegate_) {
      delegate_->OnSettingsNavigationChanged();
    }
  }
}

void AstraSettingsBubble::NavigateBack() {
  if (settings_page_view_) {
    settings_page_view_->NavigateBack();
    if (delegate_) {
      delegate_->OnSettingsNavigationChanged();
    }
  }
}

bool AstraSettingsBubble::CanNavigateBack() const {
  if (!settings_page_view_) {
    return false;
  }
  return settings_page_view_->CanNavigateBack();
}

// =========================================================================
// Size control
// =========================================================================

void AstraSettingsBubble::SetBubbleWidth(int width) {
  bubble_width_ = width;
  set_fixed_width(width);
  if (GetWidget()) {
    GetWidget()->SetSize(CalculatePreferredSize());
  }
}

int AstraSettingsBubble::GetBubbleWidth() const {
  return bubble_width_;
}

void AstraSettingsBubble::SetBubbleMaxHeight(int height) {
  bubble_max_height_ = height;
  if (GetWidget()) {
    GetWidget()->SetSize(CalculatePreferredSize());
  }
}

int AstraSettingsBubble::GetBubbleMaxHeight() const {
  return bubble_max_height_;
}

// =========================================================================
// BubbleDialogDelegateView overrides
// =========================================================================

void AstraSettingsBubble::Init() {
  // Vertical box layout: header (search box) + scrollable content.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(),
      kHeaderBottomPadding));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  BuildHeader();
  BuildContent();

  // Pass the settings model to the page view if available.
  if (settings_model_ && settings_page_view_) {
    settings_page_view_->SetModel(settings_model_.get());
  }
}

void AstraSettingsBubble::BuildHeader() {
  auto header = std::make_unique<views::View>();
  auto* header_layout = header->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, kHeaderHorizontalPadding),
          0));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Search box.
  auto search_box = std::make_unique<AstraSettingsSearchBox>(
      base::BindRepeating(&AstraSettingsBubble::OnSearchTextChanged,
                          base::Unretained(this)));
  search_box_ = search_box.get();
  header->AddChildView(std::move(search_box));

  // Bottom separator.
  auto separator = std::make_unique<views::Separator>();
  separator->SetColorId(ui::kColorSeparator);
  separator->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::TLBR(kHeaderBottomSeparatorHeight, 0, 0, 0)));
  header->AddChildView(std::move(separator));

  AddChildView(std::move(header));
}

void AstraSettingsBubble::BuildContent() {
  // Scroll view wrapping the settings page content.
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->SetClipHeight(true);
  scroll_view->SetDrawOverflowIndicator(true);
  // Allow vertical scrolling only.
  scroll_view->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);
  scroll_view_ = scroll_view.get();

  // Create the settings page content view inside the scroll view.
  auto settings_page = std::make_unique<AstraSettingsPageView>(browser_);
  settings_page_view_ = settings_page.get();

  scroll_view->SetContents(std::move(settings_page));

  // The scroll view expands to fill remaining space.
  auto* layout = static_cast<views::BoxLayout*>(GetLayoutManager());
  if (layout) {
    layout->SetFlexForView(scroll_view_.get(), 1);
  }

  AddChildView(std::move(scroll_view));
}

void AstraSettingsBubble::OnSearchTextChanged(const std::u16string& query) {
  // Forward to the settings page view directly.
  if (settings_page_view_) {
    settings_page_view_->OnSearchQueryChanged(query);
  }

  // Notify the delegate.
  if (delegate_) {
    delegate_->OnSettingsSearchQueryChanged(query);
  }

  // If the query is non-empty, show search results.
  // If the query is empty and we were on search results, go back to
  // the main page or previous page.
  if (!query.empty()) {
    ShowSearchResults(query);
  } else if (settings_page_view_ &&
             settings_page_view_->GetNavigationStackSize() > 0) {
    // Check if the top of the stack is a search results page.
    // If so, pop back to the previous page.
    // For simplicity, we just show the main page when query is cleared.
    settings_page_view_->ShowMainPage();
  }
}

void AstraSettingsBubble::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);
  // Clear singleton tracking.
  if (instance_widget_ == widget) {
    instance_widget_ = nullptr;
    instance_ = nullptr;
  }
  // The delegate will be notified by the destructor.
}

void AstraSettingsBubble::OnWidgetActivationChanged(views::Widget* widget,
                                                    bool active) {
  views::BubbleDialogDelegateView::OnWidgetActivationChanged(widget, active);
  // When the widget loses activation (user clicks outside), the bubble
  // auto-closes due to set_close_on_deactivate(true).  No extra work needed.
}

gfx::Size AstraSettingsBubble::CalculatePreferredSize() const {
  gfx::Size size = views::BubbleDialogDelegateView::CalculatePreferredSize();
  size.set_width(bubble_width_);
  // The max height is enforced by the scroll view and the widget's size
  // constraints.  We return a preferred height that will grow with content
  // up to the max.
  if (size.height() > bubble_max_height_) {
    size.set_height(bubble_max_height_);
  }
  return size;
}

// =========================================================================
// AstraSettingsObserver overrides
// =========================================================================

void AstraSettingsBubble::OnSettingChanged(AstraSettingsModel* model,
                                           const std::string& key) {
  if (delegate_) {
    delegate_->OnSettingChanged(key);
  }
}

void AstraSettingsBubble::OnSettingsReset(AstraSettingsModel* model) {
  // All settings reset — individual setting change notifications will
  // fire for each pref.  The delegate gets those via OnSettingChanged.
}

void AstraSettingsBubble::OnSettingsSearchResultsChanged(
    AstraSettingsModel* model) {
  // Search results changed — the page view handles updating its own UI.
  // We don't need to notify the delegate at the bubble level.
}

void AstraSettingsBubble::OnSettingsModelShutdown(AstraSettingsModel* model) {
  if (settings_model_.get() == model) {
    settings_model_.release();
  }
}

}  // namespace astra
