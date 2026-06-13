#include "astra/ui/views/tab_hover/astra_tab_hover_preview_view.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/ui/color/astra_color_ids.h"
#include "content/public/browser/web_contents.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/color/color_provider.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// A simple circular colored dot view, used for the workspace indicator.
class AstraWorkspaceDotView : public views::View {
 public:
  explicit AstraWorkspaceDotView(SkColor color = SK_ColorBLUE)
      : color_(color) {
    SetPreferredSize(
        gfx::Size(AstraTabHoverPreviewView::kWorkspaceDotSize,
                  AstraTabHoverPreviewView::kWorkspaceDotSize));
  }

  void SetDotColor(SkColor color) {
    color_ = color;
    SchedulePaint();
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override {
    views::View::OnPaint(canvas);
    gfx::Rect bounds = GetLocalBounds();
    // Draw a filled circle.
    // TODO(astra): Use cc::PaintFlags for proper antialiased circle drawing.
    // For the skeleton, we draw a filled rect that appears as a small dot.
    canvas->FillRect(bounds, color_);
  }

  void OnThemeChanged() override {
    views::View::OnThemeChanged();
    SchedulePaint();
  }

 private:
  SkColor color_;
};

// A small rounded badge view showing a tab index number.
class AstraTabIndexBadgeView : public views::View {
 public:
  explicit AstraTabIndexBadgeView(int index = 0) : index_(index) {
    SetPreferredSize(
        gfx::Size(AstraTabHoverPreviewView::kTabIndexBadgeSize,
                  AstraTabHoverPreviewView::kTabIndexBadgeSize));
  }

  void SetIndex(int index) {
    index_ = index;
    SchedulePaint();
  }

  void SetBadgeColor(SkColor bg_color, SkColor fg_color) {
    bg_color_ = bg_color;
    fg_color_ = fg_color;
    SchedulePaint();
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override {
    views::View::OnPaint(canvas);
    gfx::Rect bounds = GetLocalBounds();
    // Draw background as rounded rect.
    // TODO(astra): Use proper rounded rect drawing with cc::PaintFlags.
    canvas->FillRect(bounds, bg_color_);
  }

 private:
  int index_ = 0;
  SkColor bg_color_ = SK_ColorGRAY;
  SkColor fg_color_ = SK_ColorWHITE;
};

// Get a media indicator tooltip based on media state.
std::u16string GetMediaIndicatorTooltip(AstraTabHoverMediaState state) {
  switch (state) {
    case AstraTabHoverMediaState::kNone:
      return std::u16string();
    case AstraTabHoverMediaState::kPlaying:
      return u"Media playing";
    case AstraTabHoverMediaState::kPaused:
      return u"Media paused";
    case AstraTabHoverMediaState::kMuted:
      return u"Tab muted";
  }
  return std::u16string();
}

}  // namespace

// =========================================================================
// Static factory
// =========================================================================

views::Widget* AstraTabHoverPreviewView::ShowBubble(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    content::WebContents* web_contents,
    Delegate* delegate) {
  DCHECK(anchor_view);

  // Create the bubble delegate.
  auto* bubble = new AstraTabHoverPreviewView(
      anchor_view, anchor_rect, web_contents, delegate);

  // Create the bubble widget.  Ownership transfers to the widget system.
  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(bubble);

  // Arrow position: defaults to left-top for sidebar items.
  bubble->SetArrow(views::BubbleBorder::LEFT_TOP);

  widget->Show();

  return widget;
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraTabHoverPreviewView::AstraTabHoverPreviewView(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    content::WebContents* web_contents,
    Delegate* delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::LEFT_TOP,
                                       views::BubbleBorder::STANDARD_SHADOW),
      web_contents_(web_contents),
      delegate_(delegate) {
  // Set the anchor rect if provided.
  if (!anchor_rect.IsEmpty()) {
    SetAnchorRect(anchor_rect);
  }

  // Bubble configuration.
  SetAcceptCallback(base::DoNothing());
  SetCancelCallback(base::DoNothing());
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowTitle(false);
  SetShowCloseButton(false);
  set_fixed_width(kCompactSize.width());
  set_fixed_height(kCompactSize.height());

  // Auto-dismiss when the widget loses activation.
  set_close_on_deactivate(true);

  // Register keyboard accelerators.
  AddAccelerator(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_SPACE, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_M, ui::EF_CONTROL_DOWN));
}

AstraTabHoverPreviewView::~AstraTabHoverPreviewView() {
  // Notify the delegate that the view is being destroyed.
  if (delegate_) {
    delegate_->OnPreviewViewDestroyed();
  }
}

// =========================================================================
// Init — build the child views
// =========================================================================

void AstraTabHoverPreviewView::Init() {
  // Use a vertical flex layout for the whole card:
  //   header row (index + favicon + text + media + mute + close)
  //   thumbnail area
  //   footer row (workspace dot + peek button)
  auto* layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);
  layout->SetDefault(views::kMarginsKey, gfx::Insets::VH(0, 0));

  SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kVerticalPadding, kHorizontalPadding)));

  BuildHeaderRow();
  BuildThumbnailArea();
  BuildFooterRow();

  // Start in compact mode — thumbnail is hidden by default.
  SetThumbnailVisible(false);

  // Populate from WebContents if available.
  if (web_contents_) {
    UpdateFromWebContents(web_contents_);
  }

  // Apply initial theme colors.
  UpdateColors();
}

void AstraTabHoverPreviewView::BuildHeaderRow() {
  auto header_row = std::make_unique<views::View>();
  auto* layout = header_row->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetDefault(views::kMarginsKey,
                     gfx::Insets::VH(0, kHeaderElementSpacing));

  // Tab index badge (leftmost).
  BuildTabIndexBadge();
  if (tab_index_badge_) {
    header_row->AddChildView(
        std::unique_ptr<views::View>(tab_index_badge_));
  }

  // Favicon.
  auto favicon_view = std::make_unique<views::ImageView>();
  favicon_view->SetPreferredSize(gfx::Size(kFaviconSize, kFaviconSize));
  favicon_view->SetImageSize(gfx::Size(kFaviconSize, kFaviconSize));
  favicon_view_ = favicon_view.get();
  header_row->AddChildView(std::move(favicon_view));

  // Text area (middle, flexes to fill).
  auto text_container = std::make_unique<views::View>();
  BuildTextArea(text_container.get());
  text_container->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kUnbounded));
  text_container_ = text_container.get();
  header_row->AddChildView(std::move(text_container));

  // Media indicator (right side, before mute button).
  BuildMediaIndicator();
  if (media_indicator_) {
    header_row->AddChildView(
        std::unique_ptr<views::View>(media_indicator_));
  }

  // Mute toggle button (right side, before close button).
  BuildMuteButton();
  if (mute_button_) {
    header_row->AddChildView(
        std::unique_ptr<views::View>(mute_button_));
  }

  // Close button (rightmost).
  BuildCloseButton();
  if (close_button_) {
    header_row->AddChildView(
        std::unique_ptr<views::View>(close_button_));
  }

  header_row_ = AddChildView(std::move(header_row));
}

void AstraTabHoverPreviewView::BuildTextArea(views::View* text_container) {
  auto* layout = text_container->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kStart);
  layout->SetDefault(views::kMarginsKey, gfx::Insets::VH(1, 0));

  // Title label.
  auto title_label = std::make_unique<views::Label>(std::u16string());
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label->SetEnabledColor(SK_ColorBLACK);
  title_label->SetFontList(
      title_label->font_list().Derive(0, gfx::Font::FontStyle::NORMAL,
                                       gfx::Font::Weight::MEDIUM));
  title_label_ = title_label.get();
  text_container->AddChildView(std::move(title_label));

  // Domain label (smaller, lighter color — shown below the title).
  auto domain_label = std::make_unique<views::Label>(std::u16string());
  domain_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  domain_label->SetElideBehavior(gfx::ELIDE_MIDDLE);
  domain_label->SetEnabledColor(SK_ColorGRAY);
  domain_label_ = domain_label.get();
  text_container->AddChildView(std::move(domain_label));
}

void AstraTabHoverPreviewView::BuildTabIndexBadge() {
  auto badge = std::make_unique<AstraTabIndexBadgeView>(0);
  badge->SetVisible(false);
  badge->SetTooltipText(u"Tab index");
  tab_index_badge_ = badge.release();
}

void AstraTabHoverPreviewView::BuildMediaIndicator() {
  auto indicator = std::make_unique<views::ImageView>();
  indicator->SetPreferredSize(
      gfx::Size(kMediaIndicatorSize, kMediaIndicatorSize));
  indicator->SetImageSize(gfx::Size(kMediaIndicatorSize, kMediaIndicatorSize));
  indicator->SetVisible(false);
  indicator->SetTooltipText(u"");
  media_indicator_ = indicator.release();
}

void AstraTabHoverPreviewView::BuildMuteButton() {
  auto mute_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraTabHoverPreviewView::OnMuteButtonPressed,
                          base::Unretained(this)));
  mute_button->SetPreferredSize(
      gfx::Size(kMuteButtonSize, kMuteButtonSize));
  mute_button->SetTooltipText(u"Mute / unmute tab");
  mute_button->SetVisible(false);
  mute_button_ = mute_button.release();
}

void AstraTabHoverPreviewView::BuildThumbnailArea() {
  auto thumbnail_container = std::make_unique<views::View>();
  thumbnail_container->SetLayoutManager(
      std::make_unique<views::FillLayout>());
  thumbnail_container->SetPreferredSize(kMediumThumbnailSize);

  // The thumbnail image view fills the container.
  auto thumbnail_view = std::make_unique<views::ImageView>();
  thumbnail_view->SetImageSize(kMediumThumbnailSize);
  thumbnail_view->SetVisible(true);
  thumbnail_view_ = thumbnail_view.get();

  // Set a placeholder background for the thumbnail area.
  thumbnail_view->SetBackground(
      views::CreateSolidBackground(SK_ColorLTGRAY));

  // Loading indicator overlay.
  // TODO(astra): Use a proper animated loading indicator.
  auto loading_indicator = std::make_unique<views::View>();
  loading_indicator->SetVisible(false);
  loading_indicator->SetBackground(
      views::CreateSolidBackground(SkColorSetA(SK_ColorBLACK, 128)));
  thumbnail_loading_indicator_ = loading_indicator.get();

  thumbnail_container->AddChildView(std::move(thumbnail_view));
  thumbnail_container->AddChildView(std::move(loading_indicator));

  thumbnail_container_ = AddChildView(std::move(thumbnail_container));
}

void AstraTabHoverPreviewView::BuildFooterRow() {
  auto footer_row = std::make_unique<views::View>();
  auto* layout = footer_row->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kSpaceBetween);

  // Workspace indicator dot (left side).
  auto workspace_dot = std::make_unique<AstraWorkspaceDotView>();
  workspace_dot->SetVisible(false);
  workspace_dot_ = workspace_dot.get();
  footer_row->AddChildView(std::move(workspace_dot));

  // Spacer that flexes to push the peek button to the right.
  auto spacer = std::make_unique<views::View>();
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
  footer_row->AddChildView(std::move(spacer));

  // Peek button (right side).
  auto peek_button = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraTabHoverPreviewView::OnPeekButtonPressed,
                          base::Unretained(this)),
      u"Peek");
  peek_button->SetPreferredSize(gfx::Size(kPeekButtonWidth, kPeekButtonHeight));
  peek_button_ = peek_button.get();
  peek_button_->SetVisible(true);
  footer_row->AddChildView(std::move(peek_button));

  footer_row_ = AddChildView(std::move(footer_row));
}

void AstraTabHoverPreviewView::BuildWorkspaceDot() {
  // Already built in BuildFooterRow.
}

void AstraTabHoverPreviewView::BuildPeekButton() {
  // Already built in BuildFooterRow.
}

void AstraTabHoverPreviewView::BuildCloseButton() {
  auto close_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraTabHoverPreviewView::OnCloseButtonPressed,
                          base::Unretained(this)));
  close_button->SetPreferredSize(
      gfx::Size(kCloseButtonSize, kCloseButtonSize));
  close_button->SetTooltipText(u"Close preview");
  close_button->SetVisible(true);
  close_button_ = close_button.release();
}

// =========================================================================
// Content updates
// =========================================================================

void AstraTabHoverPreviewView::UpdateFromWebContents(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  web_contents_ = web_contents;

  // Title.
  std::u16string title = web_contents->GetTitle();
  GURL url = web_contents->GetVisibleURL();
  if (title.empty() && url.is_valid()) {
    title = base::UTF8ToUTF16(url.host());
  }
  SetTitleText(title);

  // URL.
  SetUrlText(base::UTF8ToUTF16(url.spec()));

  // Domain.
  SetDomainText(AstraTabHoverModel::FormatDomainFromUrl(url));

  // Favicon.
  SetFaviconPlaceholder();

  // Thumbnail.
  SetThumbnailPlaceholder();
  SetThumbnailLoadingState(AstraTabHoverImageLoadingState::kNotLoaded);

  // Media state.
  bool is_media = web_contents->IsCurrentlyAudible();
  // TODO(astra): Get actual media state from WebContents (playing/paused/muted).
  SetMediaState(is_media ? AstraTabHoverMediaState::kPlaying
                        : AstraTabHoverMediaState::kNone);

  // Workspace indicator.
  SetWorkspaceIndicatorVisible(false);
}

void AstraTabHoverPreviewView::UpdateFromModel(const AstraTabHoverModel& model) {
  // Tab data.
  const auto& tab_data = model.tab_data();
  SetTitleText(AstraTabHoverModel::FormatTabTitle(tab_data.title));
  SetDomainText(AstraTabHoverModel::FormatDomainFromUrl(tab_data.url));
  SetUrlText(base::UTF8ToUTF16(tab_data.url.spec()));
  if (!tab_data.favicon.isNull()) {
    SetFavicon(tab_data.favicon);
  } else {
    SetFaviconPlaceholder();
  }
  SetMediaState(tab_data.media_state);
  SetMuted(tab_data.is_muted);
  SetTabIndex(tab_data.tab_index);

  // Settings-driven visibility.
  const auto& settings = model.settings();
  SetTitleVisible(settings.show_tab_title);
  SetDomainVisible(settings.show_tab_url);
  SetFaviconVisible(settings.show_favicon);

  // Preview image.
  const auto& img_state = model.preview_image_state();
  SetThumbnailVisible(settings.show_preview_image && img_state.has_image);
  SetThumbnailLoadingState(img_state.loading_state);

  // Peek mode.
  const auto& peek_state = model.peek_state();
  SetPeekMode(peek_state.is_peeking);
  SetPreviewSize(peek_state.peek_size);
  SetPeekButtonVisible(settings.enable_peek_mode);

  // Visibility toggles from settings.
  SetFaviconVisible(settings.show_favicon);
  SetCloseButtonVisible(settings.show_close_button);
  SetTabIndexVisible(settings.show_tab_index);
  SetMediaIndicatorVisible(settings.show_media_indicator);
  SetMuteButtonVisible(settings.show_mute_button);

  // Card position.
  SetCardPosition(settings.card_position);
}

void AstraTabHoverPreviewView::SetTitleText(const std::u16string& title) {
  if (title_label_) {
    title_label_->SetText(title);
  }
}

void AstraTabHoverPreviewView::SetUrlText(const std::u16string& url) {
  // URL is not directly displayed in the current layout (domain is shown
  // instead).  Keep the method for API compatibility.
  // TODO(astra): Consider showing full URL in tooltip or on hover.
}

void AstraTabHoverPreviewView::SetDomainText(const std::u16string& domain) {
  if (domain_label_) {
    domain_label_->SetText(domain);
  }
}

void AstraTabHoverPreviewView::SetFavicon(const gfx::ImageSkia& favicon) {
  if (favicon_view_) {
    favicon_view_->SetImage(favicon);
    favicon_view_->SetVisible(true);
  }
}

void AstraTabHoverPreviewView::SetFaviconPlaceholder() {
  if (favicon_view_) {
    favicon_view_->SetVisible(true);
  }
}

void AstraTabHoverPreviewView::SetThumbnail(const gfx::ImageSkia& thumbnail) {
  if (thumbnail_view_) {
    thumbnail_view_->SetImage(thumbnail);
  }
}

void AstraTabHoverPreviewView::SetThumbnailPlaceholder() {
  if (thumbnail_view_) {
    SkColor bg_color = SK_ColorLTGRAY;
    if (GetColorProvider()) {
      bg_color = GetColorProvider()->GetColor(kColorAstraPeekBackground);
    }
    thumbnail_view_->SetBackground(
        views::CreateSolidBackground(bg_color));
  }
}

void AstraTabHoverPreviewView::SetThumbnailLoadingState(
    AstraTabHoverImageLoadingState state) {
  thumbnail_loading_state_ = state;
  if (thumbnail_loading_indicator_) {
    thumbnail_loading_indicator_->SetVisible(
        state == AstraTabHoverImageLoadingState::kLoading);
  }
}

// =========================================================================
// Media indicator
// =========================================================================

void AstraTabHoverPreviewView::SetMediaState(AstraTabHoverMediaState state) {
  media_state_ = state;
  if (media_indicator_) {
    bool visible = state != AstraTabHoverMediaState::kNone;
    media_indicator_->SetVisible(visible);
    if (visible) {
      media_indicator_->SetTooltipText(GetMediaIndicatorTooltip(state));
    }
  }
  // Update mute button state based on media state.
  if (state == AstraTabHoverMediaState::kMuted) {
    is_muted_ = true;
  }
}

void AstraTabHoverPreviewView::SetMediaIndicatorVisible(bool visible) {
  if (media_indicator_) {
    if (visible && media_state_ != AstraTabHoverMediaState::kNone) {
      media_indicator_->SetVisible(true);
    } else if (!visible) {
      media_indicator_->SetVisible(false);
    }
  }
}

void AstraTabHoverPreviewView::SetMuteButtonVisible(bool visible) {
  if (mute_button_) {
    mute_button_->SetVisible(visible);
  }
}

void AstraTabHoverPreviewView::SetMuted(bool muted) {
  is_muted_ = muted;
  if (muted) {
    SetMediaState(AstraTabHoverMediaState::kMuted);
  }
}

// =========================================================================
// Tab index badge
// =========================================================================

void AstraTabHoverPreviewView::SetTabIndex(int index) {
  if (tab_index_badge_) {
    if (index >= 0) {
      // Update the badge text.
      // TODO(astra): Use a proper label inside the badge view.
      tab_index_badge_->SetVisible(true);
      tab_index_badge_->SetTooltipText(
          base::UTF8ToUTF16(base::NumberToString(index + 1)));
    } else {
      tab_index_badge_->SetVisible(false);
    }
  }
}

void AstraTabHoverPreviewView::SetTabIndexVisible(bool visible) {
  if (tab_index_badge_) {
    tab_index_badge_->SetVisible(visible);
  }
}

// =========================================================================
// Peek / expand
// =========================================================================

void AstraTabHoverPreviewView::SetPeekButtonVisible(bool visible) {
  if (peek_button_) {
    peek_button_->SetVisible(visible);
    InvalidateLayout();
  }
}

void AstraTabHoverPreviewView::SetPeekButtonText(const std::u16string& text) {
  if (peek_button_) {
    peek_button_->SetText(text);
  }
}

void AstraTabHoverPreviewView::SetPeekMode(bool is_peeking) {
  if (is_peek_mode_ == is_peeking) {
    return;
  }
  is_peek_mode_ = is_peeking;

  if (is_peeking) {
    // In peek mode, show thumbnail with larger size.
    SetThumbnailVisible(true);
    StartPeekExpandAnimation();
  } else {
    // Exiting peek mode — revert to normal size.
    SetThumbnailVisible(false);
  }
}

// =========================================================================
// Workspace indicator
// =========================================================================

void AstraTabHoverPreviewView::SetWorkspaceIndicatorVisible(bool visible) {
  if (workspace_dot_) {
    workspace_dot_->SetVisible(visible);
  }
}

void AstraTabHoverPreviewView::SetWorkspaceIndicatorColor(SkColor color) {
  if (workspace_dot_) {
    AstraWorkspaceDotView* dot =
        static_cast<AstraWorkspaceDotView*>(workspace_dot_);
    dot->SetDotColor(color);
  }
}

// =========================================================================
// Close button
// =========================================================================

void AstraTabHoverPreviewView::SetCloseButtonVisible(bool visible) {
  if (close_button_) {
    close_button_->SetVisible(visible);
  }
}

// =========================================================================
// Element visibility toggles
// =========================================================================

void AstraTabHoverPreviewView::SetTitleVisible(bool visible) {
  if (title_label_) {
    title_label_->SetVisible(visible);
  }
}

void AstraTabHoverPreviewView::SetFaviconVisible(bool visible) {
  if (favicon_view_) {
    favicon_view_->SetVisible(visible);
  }
}

void AstraTabHoverPreviewView::SetDomainVisible(bool visible) {
  if (domain_label_) {
    domain_label_->SetVisible(visible);
  }
}

// =========================================================================
// Size configuration
// =========================================================================

void AstraTabHoverPreviewView::SetThumbnailVisible(bool visible) {
  if (thumbnail_visible_ == visible) {
    return;
  }
  thumbnail_visible_ = visible;

  if (thumbnail_container_) {
    thumbnail_container_->SetVisible(visible);
  }

  UpdateWidgetSize();
  InvalidateLayout();
}

void AstraTabHoverPreviewView::SetPreviewSize(AstraTabHoverPreviewSize size) {
  if (preview_size_ == size) {
    return;
  }
  preview_size_ = size;
  UpdateThumbnailSize();
  UpdateWidgetSize();
  InvalidateLayout();
}

void AstraTabHoverPreviewView::SetCardPosition(AstraTabHoverCardPosition position) {
  if (card_position_ == position) {
    return;
  }
  card_position_ = position;
  UpdateArrowPosition();
}

// =========================================================================
// Accessibility
// =========================================================================

void AstraTabHoverPreviewView::SetAccessibleName(const std::u16string& name) {
  // TODO(astra): Set accessible name properly on the bubble widget.
  // For now, we store it for use in GetAccessibleNodeData.
  accessible_name_ = name;
}

// =========================================================================
// Peek expansion animation (stub)
// =========================================================================

void AstraTabHoverPreviewView::StartPeekExpandAnimation() {
  // TODO(astra): Implement real peek expansion animation.
  // This is a stub — the view just switches to the larger size immediately.
  // Real implementation would use gfx::Animation or ui::Compositor
  // to animate the size transition smoothly.

  if (is_peek_mode_) {
    // Ensure thumbnail is visible and sized appropriately.
    SetThumbnailVisible(true);
  }
}

// =========================================================================
// Button handlers
// =========================================================================

void AstraTabHoverPreviewView::OnPeekButtonPressed() {
  if (delegate_) {
    delegate_->OnPeekRequested();
  }
}

void AstraTabHoverPreviewView::OnCloseButtonPressed() {
  if (delegate_) {
    delegate_->OnCloseRequested();
  } else if (GetWidget()) {
    GetWidget()->Close();
  }
}

void AstraTabHoverPreviewView::OnMuteButtonPressed() {
  if (delegate_) {
    delegate_->OnMuteToggled();
  }
  // Toggle local state immediately for responsive feel.
  SetMuted(!is_muted_);
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraTabHoverPreviewView::UpdateWidgetSize() {
  if (!GetWidget()) {
    return;
  }

  gfx::Size size = thumbnail_visible_ ? kExpandedSize : kCompactSize;
  set_fixed_width(size.width());
  set_fixed_height(size.height());

  SizeToContents();
}

void AstraTabHoverPreviewView::UpdateThumbnailSize() {
  if (!thumbnail_container_ || !thumbnail_view_) {
    return;
  }

  gfx::Size size = AstraTabHoverModel::GetPreviewSizePixels(preview_size_);
  thumbnail_container_->SetPreferredSize(size);
  thumbnail_view_->SetImageSize(size);
}

void AstraTabHoverPreviewView::UpdateArrowPosition() {
  // TODO(astra): Adjust bubble arrow position based on card_position_.
  // For now, we always use LEFT_TOP (sidebar-style).
  // Chromium's BubbleDialogDelegateView supports different arrow positions.

  views::BubbleBorder::Arrow arrow = views::BubbleBorder::LEFT_TOP;

  switch (card_position_) {
    case AstraTabHoverCardPosition::kAbove:
      // Would be TOP_CENTER or similar for above-tab positioning.
      arrow = views::BubbleBorder::TOP_LEFT;
      break;
    case AstraTabHoverCardPosition::kBelow:
      arrow = views::BubbleBorder::BOTTOM_LEFT;
      break;
    case AstraTabHoverCardPosition::kAuto:
      // Default — let the bubble framework decide.
      arrow = views::BubbleBorder::LEFT_TOP;
      break;
  }

  SetArrow(arrow);
}

void AstraTabHoverPreviewView::UpdateColors() {
  if (!GetColorProvider()) {
    return;
  }

  // Update title text color.
  if (title_label_) {
    title_label_->SetEnabledColor(
        GetColorProvider()->GetColor(ui::kColorLabelForeground));
  }

  // Update domain text color.
  if (domain_label_) {
    domain_label_->SetEnabledColor(
        GetColorProvider()->GetColor(ui::kColorLabelForegroundSecondary));
  }

  // Update thumbnail placeholder.
  SetThumbnailPlaceholder();
}

// =========================================================================
// BubbleDialogDelegateView overrides
// =========================================================================

void AstraTabHoverPreviewView::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);

  // Clear our pointer to the WebContents — we don't own it.
  web_contents_ = nullptr;
  delegate_ = nullptr;
}

bool AstraTabHoverPreviewView::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  if (accelerator.key_code() == ui::VKEY_ESCAPE) {
    if (GetWidget()) {
      GetWidget()->Close();
    }
    return true;
  }

  if (accelerator.key_code() == ui::VKEY_RETURN ||
      accelerator.key_code() == ui::VKEY_SPACE) {
    if (delegate_) {
      delegate_->OnPeekRequested();
    }
    return true;
  }

  if (accelerator.key_code() == ui::VKEY_M &&
      accelerator.IsCtrlDown()) {
    if (delegate_) {
      delegate_->OnMuteToggled();
    }
    return true;
  }

  return views::BubbleDialogDelegateView::AcceleratorPressed(accelerator);
}

void AstraTabHoverPreviewView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  UpdateColors();
}

void AstraTabHoverPreviewView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::BubbleDialogDelegateView::GetAccessibleNodeData(node_data);

  // Set accessible role and name.
  node_data->role = ax::mojom::Role::kDialog;
  if (!accessible_name_.empty()) {
    node_data->SetName(accessible_name_);
  } else if (title_label_ && !title_label_->GetText().empty()) {
    node_data->SetName(title_label_->GetText());
  } else {
    node_data->SetName(u"Tab preview");
  }

  // Add description for screen readers.
  std::u16string description = u"Tab hover preview";
  if (domain_label_ && !domain_label_->GetText().empty()) {
    description += u" — " + domain_label_->GetText();
  }
  node_data->SetDescription(description);
}

}  // namespace astra
