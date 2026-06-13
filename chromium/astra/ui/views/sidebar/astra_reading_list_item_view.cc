#include "astra/ui/views/sidebar/astra_reading_list_item_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kReadingListItemHeight = 48;
constexpr int kReadingListItemVerticalPadding = 6;
constexpr int kReadingListItemIconSize = 16;
constexpr int kReadingListItemCornerRadius = 6;
constexpr int kReadingListItemTitleLineHeight = 18;
constexpr int kReadingListItemDomainFontSizeDelta = -1;
constexpr int kActionButtonSize = 16;
constexpr int kActionButtonSpacing = 4;

// Separator between domain and read time.
const char16_t kSecondarySeparator[] = u" \u00b7 ";

// Astra color IDs for reading list item styling.
constexpr ui::ColorId kReadingListItemTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kReadingListItemSecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kReadingListItemHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kReadingListItemReadTextColorId =
    kColorAstraSidebarItemSecondaryText;

}  // namespace

AstraReadingListItemView::AstraReadingListItemView(const GURL& url,
                                                   const std::u16string& title,
                                                   const std::u16string& domain,
                                                   bool is_read)
    : url_(url), domain_(domain), is_read_(is_read) {
  SetTitle(title);
  SetSecondaryText(domain);

  // Show icon placeholder (read indicator).
  icon_view()->SetVisible(true);
  UpdateReadIndicator();

  // Initial secondary text.
  UpdateSecondaryText();
}

AstraReadingListItemView::~AstraReadingListItemView() = default;

void AstraReadingListItemView::BuildLayout() {
  AstraSidebarItemView::BuildLayout();

  // Action buttons (shown on hover) — add to trailing container.
  // Toggle read button.
  toggle_read_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraReadingListItemView::OnToggleReadButtonPressed,
              base::Unretained(this))));
  toggle_read_button_->SetPreferredSize(
      gfx::Size(kActionButtonSize, kActionButtonSize));
  toggle_read_button_->SetTooltipText(u"Mark as read");
  toggle_read_button_->SetVisible(false);

  // Remove button.
  remove_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraReadingListItemView::OnRemoveButtonPressed,
              base::Unretained(this))));
  remove_button_->SetPreferredSize(
      gfx::Size(kActionButtonSize, kActionButtonSize));
  remove_button_->SetTooltipText(u"Remove from reading list");
  remove_button_->SetVisible(false);

  // Initial state: action buttons hidden by default, shown on hover.
  UpdateActionButtonsVisibility();
}

// =========================================================================
// Reading list item info
// =========================================================================

void AstraReadingListItemView::SetReadingListItem(const GURL& url,
                                                  const std::u16string& title,
                                                  bool is_read) {
  url_ = url;
  is_read_ = is_read;

  SetTitle(title);
  UpdateReadIndicator();
  UpdateSecondaryText();
  UpdateActionButtonsVisibility();
  SchedulePaint();
}

// =========================================================================
// Read state
// =========================================================================

void AstraReadingListItemView::SetRead(bool read) {
  if (is_read_ == read) {
    return;
  }
  is_read_ = read;
  UpdateReadIndicator();
  UpdateActionButtonsVisibility();
  OnThemeChanged();  // Update text colors for read state.
}

void AstraReadingListItemView::UpdateReadIndicator() {
  // TODO(astra): Swap the icon based on read state.
  //   - Unread: filled bookmark icon or eye icon
  //   - Read: checkmark or outline bookmark icon
  // For now, we just rely on color changes from OnThemeChanged.
  if (show_read_indicator_ && icon_view()) {
    // Placeholder: the icon appearance will be updated via OnThemeChanged.
  }
}

// =========================================================================
// Estimated read time
// =========================================================================

void AstraReadingListItemView::SetEstimatedReadTime(base::TimeDelta read_time) {
  if (estimated_read_time_ == read_time) {
    return;
  }
  estimated_read_time_ = read_time;
  UpdateSecondaryText();
}

// =========================================================================
// Word count
// =========================================================================

void AstraReadingListItemView::SetWordCount(int count) {
  if (word_count_ == count) {
    return;
  }
  word_count_ = count;
  UpdateSecondaryText();
}

// =========================================================================
// Domain
// =========================================================================

void AstraReadingListItemView::SetDomain(const std::u16string& domain) {
  domain_ = domain;
  UpdateSecondaryText();
}

// =========================================================================
// Read indicator visibility
// =========================================================================

void AstraReadingListItemView::ShowReadIndicator(bool show) {
  if (show_read_indicator_ == show) {
    return;
  }
  show_read_indicator_ = show;
  if (icon_view()) {
    icon_view()->SetVisible(show);
  }
  InvalidateLayout();
}

// =========================================================================
// Distilled version
// =========================================================================

void AstraReadingListItemView::SetHasDistilledVersion(bool has_distilled) {
  if (has_distilled_version_ == has_distilled) {
    return;
  }
  has_distilled_version_ = has_distilled;
  // TODO(astra): Show a distilled/readability indicator icon.
}

// =========================================================================
// Favorite
// =========================================================================

void AstraReadingListItemView::SetIsFavorite(bool favorite) {
  if (is_favorite_ == favorite) {
    return;
  }
  is_favorite_ = favorite;
  // TODO(astra): Show a star/favorite badge or icon.
}

// =========================================================================
// Secondary text
// =========================================================================

void AstraReadingListItemView::UpdateSecondaryText() {
  std::u16string text = domain_;

  // Add estimated read time if available.
  if (estimated_read_time_ > base::TimeDelta()) {
    int minutes = std::max(1, static_cast<int>(estimated_read_time_.InMinutes()));
    text += kSecondarySeparator + base::NumberToString16(minutes) + u" min read";
  } else if (word_count_ > 0) {
    // Or show word count as a fallback.
    text += kSecondarySeparator + base::NumberToString16(word_count_) + u" words";
  }

  SetSecondaryText(text);
}

// =========================================================================
// Action buttons
// =========================================================================

void AstraReadingListItemView::UpdateActionButtonsVisibility() {
  if (toggle_read_button_) {
    toggle_read_button_->SetVisible(is_hovered_internal_);
    // Update tooltip based on current state.
    toggle_read_button_->SetTooltipText(
        is_read_ ? u"Mark as unread" : u"Mark as read");
  }
  if (remove_button_) {
    remove_button_->SetVisible(is_hovered_internal_);
  }
}

void AstraReadingListItemView::OnToggleReadButtonPressed() {
  if (delegate_) {
    delegate_->OnReadingListToggleRead(url_);
  }
}

void AstraReadingListItemView::OnRemoveButtonPressed() {
  if (delegate_) {
    delegate_->OnReadingListRemove(url_);
  }
}

// =========================================================================
// Click handling
// =========================================================================

void AstraReadingListItemView::OnItemClicked() {
  if (delegate_) {
    delegate_->OnReadingListItemClicked(url_);
  }
}

// =========================================================================
// Hover handling
// =========================================================================

void AstraReadingListItemView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_internal_ = true;
  UpdateActionButtonsVisibility();
  AstraSidebarItemView::OnMouseEntered(event);
}

void AstraReadingListItemView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_internal_ = false;
  UpdateActionButtonsVisibility();
  AstraSidebarItemView::OnMouseExited(event);
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraReadingListItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = AstraSidebarItemView::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kReadingListItemHeight));
  return size;
}

void AstraReadingListItemView::OnThemeChanged() {
  AstraSidebarItemView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Title text: regular color for unread, secondary (dimmed) for read.
  if (title_label()) {
    SkColor title_color = color_provider->GetColor(
        is_read_ ? kReadingListItemReadTextColorId
                 : kReadingListItemTextColorId);
    title_label()->SetEnabledColor(title_color);
    title_label()->SetLineHeight(kReadingListItemTitleLineHeight);
  }

  // Domain text: always secondary.
  if (secondary_label()) {
    secondary_label()->SetEnabledColor(
        color_provider->GetColor(kReadingListItemSecondaryTextColorId));
    secondary_label()->SetFontList(
        secondary_label()->font_list().DeriveWithSizeDelta(
            kReadingListItemDomainFontSizeDelta));
  }
}

}  // namespace astra
