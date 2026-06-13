#include "astra/ui/views/sidebar/astra_note_item_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kNoteItemHeight = 56;
constexpr int kNoteItemHorizontalPadding = 12;
constexpr int kNoteItemVerticalPadding = 6;
constexpr int kColorBarWidth = 3;
constexpr int kColorBarSpacing = 8;
constexpr int kNoteItemCornerRadius = 6;
constexpr int kNoteTitleLineHeight = 16;
constexpr int kNoteSnippetFontSizeDelta = -1;
constexpr int kNoteSnippetLineHeight = 14;
constexpr int kNoteTimeFontSizeDelta = -2;
constexpr int kActionButtonSize = 16;

// Fallback default note color (amber).
constexpr SkColor kDefaultNoteColor = SkColorSetRGB(0xFF, 0xD9, 0x3D);

// Astra color IDs for note item styling.
constexpr ui::ColorId kNoteItemTitleColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kNoteItemSnippetColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kNoteItemTimeColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kNoteItemHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;

}  // namespace

AstraNoteItemView::AstraNoteItemView(const std::string& note_id,
                                     const std::u16string& title,
                                     const std::u16string& preview,
                                     const std::u16string& modified_time,
                                     SkColor color)
    : note_id_(note_id),
      preview_text_(preview),
      note_color_(color == 0 ? kDefaultNoteColor : color) {
  SetTitle(title);
  SetSecondaryText(preview);

  // Note items don't show the leading icon by default.
  icon_view()->SetVisible(false);
}

AstraNoteItemView::~AstraNoteItemView() = default;

void AstraNoteItemView::BuildLayout() {
  // Start with base class layout.
  AstraSidebarItemView::BuildLayout();

  // Add color bar at the very beginning (before icon).
  // We need to insert it at the beginning of the layout.
  // Since BoxLayout adds children in order, we'll create the color bar
  // and manually position it in Layout().

  color_bar_ = AddChildView(std::make_unique<views::View>());
  color_bar_->SetPreferredSize(gfx::Size(kColorBarWidth, 0));
  color_bar_->SetPaintToLayer();
  color_bar_->layer()->SetFillsBoundsOpaquely(true);
  color_bar_->layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(kColorBarWidth / 2.0f));
  UpdateColorIndicator();

  // Delete button (shown on hover) — add to trailing container.
  delete_button_ = trailing_container()->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(&AstraNoteItemView::OnDeleteButtonPressed,
                              base::Unretained(this))));
  delete_button_->SetPreferredSize(
      gfx::Size(kActionButtonSize, kActionButtonSize));
  delete_button_->SetTooltipText(u"Delete note");
  delete_button_->SetVisible(false);

  // Time label — add to text container, above or with title.
  // Actually, the title row has title + time. We'll handle this in Layout().
  // For now, add time label to the text container and adjust.
  //
  // The base text_container has title_label_ and secondary_label_.
  // We want: title row (title + time) and snippet row.
  // So we add time_label and rework the layout a bit.

  // We'll override Layout() to position the time label in the title row.
  // For now, just create it.
  time_label_ = text_container()->AddChildView(
      std::make_unique<views::Label>());
  time_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  time_label_->SetAutoColorReadabilityEnabled(false);
  time_label_->SetFontList(
      time_label_->font_list().DeriveWithSizeDelta(kNoteTimeFontSizeDelta));

  // Initial state.
  UpdateTimeLabel();
  UpdateActionButtonsVisibility();
  UpdatePreviewText();
}

// =========================================================================
// Note info
// =========================================================================

void AstraNoteItemView::SetNoteInfo(const std::string& note_id,
                                   const std::u16string& title,
                                   const std::u16string& preview) {
  note_id_ = note_id;
  preview_text_ = preview;

  SetTitle(title);
  UpdatePreviewText();
  SchedulePaint();
}

// =========================================================================
// Preview text
// =========================================================================

void AstraNoteItemView::SetPreviewText(const std::u16string& preview) {
  preview_text_ = preview;
  UpdatePreviewText();
}

void AstraNoteItemView::UpdatePreviewText() {
  if (show_preview_) {
    SetSecondaryText(preview_text_);
  }
}

void AstraNoteItemView::ShowPreview(bool show) {
  if (show_preview_ == show) {
    return;
  }
  show_preview_ = show;
  ShowSecondaryText(show);
  if (show) {
    UpdatePreviewText();
  }
  InvalidateLayout();
}

// =========================================================================
// Note color
// =========================================================================

void AstraNoteItemView::SetNoteColor(SkColor color) {
  if (note_color_ == color) {
    return;
  }
  note_color_ = color;
  UpdateColorIndicator();
}

void AstraNoteItemView::UpdateColorIndicator() {
  if (color_bar_ && color_bar_->layer()) {
    color_bar_->layer()->SetColor(note_color_);
  }
}

// =========================================================================
// Tag count
// =========================================================================

void AstraNoteItemView::SetTagCount(int count) {
  if (tag_count_ == count) {
    return;
  }
  tag_count_ = count;

  if (show_tag_count_) {
    SetBadgeText(base::NumberToString16(count));
  }
}

void AstraNoteItemView::ShowTagCount(bool show) {
  if (show_tag_count_ == show) {
    return;
  }
  show_tag_count_ = show;

  if (show && tag_count_ > 0) {
    SetBadgeText(base::NumberToString16(tag_count_));
  } else {
    ShowBadge(false);
  }
}

// =========================================================================
// Pinned state
// =========================================================================

void AstraNoteItemView::SetPinned(bool pinned) {
  if (is_pinned_ == pinned) {
    return;
  }
  is_pinned_ = pinned;
  // TODO(astra): Show a pin icon indicator for pinned notes.
  //   Could use the trailing icon slot.
}

// =========================================================================
// Modified time
// =========================================================================

void AstraNoteItemView::SetModifiedTime(base::Time time) {
  if (modified_time_ == time) {
    return;
  }
  modified_time_ = time;
  UpdateTimeLabel();
}

void AstraNoteItemView::UpdateTimeLabel() {
  if (time_label_) {
    time_label_->SetText(FormatModifiedTime());
  }
}

std::u16string AstraNoteItemView::FormatModifiedTime() const {
  if (modified_time_.is_null()) {
    return std::u16string();
  }

  base::TimeDelta delta = base::Time::Now() - modified_time_;

  if (delta < base::Minutes(1)) {
    return u"Just now";
  }
  if (delta < base::Hours(1)) {
    int minutes = static_cast<int>(delta.InMinutes());
    return base::NumberToString16(minutes) + u"m ago";
  }
  if (delta < base::Days(1)) {
    int hours = static_cast<int>(delta.InHours());
    return base::NumberToString16(hours) + u"h ago";
  }
  if (delta < base::Days(2)) {
    return u"Yesterday";
  }
  if (delta < base::Days(7)) {
    int days = static_cast<int>(delta.InDays());
    return base::NumberToString16(days) + u"d ago";
  }

  // Older — show date.
  base::Time::Exploded exploded;
  modified_time_.LocalExplode(&exploded);
  return base::NumberToString16(exploded.month) + u"/" +
         base::NumberToString16(exploded.day);
}

// =========================================================================
// Action buttons
// =========================================================================

void AstraNoteItemView::UpdateActionButtonsVisibility() {
  if (delete_button_) {
    delete_button_->SetVisible(is_hovered_internal_);
  }
}

void AstraNoteItemView::OnDeleteButtonPressed() {
  if (delegate_) {
    delegate_->OnNoteDeleteRequested(note_id_);
  }
}

// =========================================================================
// Click handling
// =========================================================================

void AstraNoteItemView::OnItemClicked() {
  if (delegate_) {
    delegate_->OnNoteItemClicked(note_id_);
  }
}

// =========================================================================
// Hover handling
// =========================================================================

void AstraNoteItemView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_internal_ = true;
  UpdateActionButtonsVisibility();
  AstraSidebarItemView::OnMouseEntered(event);
}

void AstraNoteItemView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_internal_ = false;
  UpdateActionButtonsVisibility();
  AstraSidebarItemView::OnMouseExited(event);
}

// =========================================================================
// Layout
// =========================================================================

void AstraNoteItemView::Layout() {
  // Let base class do its layout first.
  AstraSidebarItemView::Layout();

  // Position the color bar on the left edge.
  if (color_bar_) {
    int y = kNoteItemVerticalPadding;
    int height = height() - 2 * kNoteItemVerticalPadding;
    color_bar_->SetBounds(kNoteItemHorizontalPadding, y, kColorBarWidth,
                          height);
  }

  // Position the time label on the same line as the title, on the right.
  // We need to adjust the title and time layout.
  // For simplicity, we rely on the BoxLayout in the text container,
  // but the time label needs to be on the same row as the title.
  //
  // TODO(astra): Implement proper title + time row layout.
  //   For now, we position the time label manually.
  if (time_label_ && title_label()) {
    gfx::Size time_size = time_label_->GetPreferredSize();
    int text_right = text_container()->bounds().right();
    int title_top = title_label()->bounds().y();
    time_label_->SetBounds(text_right - time_size.width(),
                           title_top,
                           time_size.width(),
                           time_size.height());
  }
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraNoteItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = AstraSidebarItemView::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kNoteItemHeight));
  return size;
}

void AstraNoteItemView::OnThemeChanged() {
  AstraSidebarItemView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (title_label()) {
    title_label()->SetEnabledColor(
        color_provider->GetColor(kNoteItemTitleColorId));
    title_label()->SetLineHeight(kNoteTitleLineHeight);
    title_label()->SetFontList(
        title_label()->font_list().DeriveWithWeight(gfx::Font::Weight::MEDIUM));
  }
  if (secondary_label()) {
    secondary_label()->SetEnabledColor(
        color_provider->GetColor(kNoteItemSnippetColorId));
    secondary_label()->SetLineHeight(kNoteSnippetLineHeight);
    secondary_label()->SetFontList(
        secondary_label()->font_list().DeriveWithSizeDelta(
            kNoteSnippetFontSizeDelta));
  }
  if (time_label_) {
    time_label_->SetEnabledColor(
        color_provider->GetColor(kNoteItemTimeColorId));
  }
}

}  // namespace astra
