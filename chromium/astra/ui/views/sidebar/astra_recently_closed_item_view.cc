#include "astra/ui/views/sidebar/astra_recently_closed_item_view.h"

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kRecentlyClosedItemHeight = 40;
constexpr int kRecentlyClosedItemVerticalPadding = 4;
constexpr int kRecentlyClosedItemCornerRadius = 6;
constexpr int kRecentlyClosedItemSecondaryFontSizeDelta = -1;

// Separator between domain and time in the secondary text.
const char16_t kSecondarySeparator[] = u" \u00b7 ";

// Astra color IDs for recently closed item styling.
constexpr ui::ColorId kRecentlyClosedItemTitleColorId =
    kColorAstraSidebarItemText;
constexpr ui::ColorId kRecentlyClosedItemSecondaryColorId =
    kColorAstraSidebarItemSecondaryText;

}  // namespace

AstraRecentlyClosedItemView::AstraRecentlyClosedItemView(
    const std::u16string& title,
    const GURL& url,
    base::Time close_time,
    int entry_id)
    : url_(url), close_time_(close_time), entry_id_(entry_id) {
  SetTitle(title);

  // Show icon placeholder.
  icon_view()->SetVisible(true);

  // Set initial secondary text.
  UpdateSecondaryText();
  UpdateTooltipText();
}

AstraRecentlyClosedItemView::~AstraRecentlyClosedItemView() = default;

void AstraRecentlyClosedItemView::BuildLayout() {
  AstraSidebarItemView::BuildLayout();

  // Two-line layout is provided by base class (title + secondary).
  if (secondary_label()) {
    secondary_label()->SetFontList(
        secondary_label()->font_list().DeriveWithSizeDelta(
            kRecentlyClosedItemSecondaryFontSizeDelta));
  }
}

// =========================================================================
// Recently closed info
// =========================================================================

void AstraRecentlyClosedItemView::SetRecentlyClosedInfo(const GURL& url,
                                                        const std::u16string& title,
                                                        base::Time close_time) {
  url_ = url;
  close_time_ = close_time;

  SetTitle(title);
  UpdateSecondaryText();
  UpdateTooltipText();
  SchedulePaint();
}

// =========================================================================
// Tab count
// =========================================================================

void AstraRecentlyClosedItemView::SetTabCount(int count) {
  if (tab_count_ == count) {
    return;
  }
  tab_count_ = count;
  UpdateSecondaryText();
  UpdateTooltipText();
}

// =========================================================================
// Window vs tab
// =========================================================================

void AstraRecentlyClosedItemView::SetIsWindow(bool is_window) {
  if (is_window_ == is_window) {
    return;
  }
  is_window_ = is_window;
  // TODO(astra): Show a window icon for window entries.
  UpdateSecondaryText();
  UpdateTooltipText();
}

// =========================================================================
// Helpers
// =========================================================================

std::u16string AstraRecentlyClosedItemView::GetRelativeTimeText() const {
  base::TimeDelta delta = base::Time::Now() - close_time_;

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

  base::Time::Exploded exploded;
  close_time_.LocalExplode(&exploded);
  std::u16string date =
      base::NumberToString16(exploded.month) + u"/" +
      base::NumberToString16(exploded.day);
  return date;
}

std::u16string AstraRecentlyClosedItemView::GetDomainText() const {
  if (!url_.is_valid()) {
    return std::u16string();
  }
  return base::UTF8ToUTF16(url_.host());
}

std::u16string AstraRecentlyClosedItemView::GetSecondaryText() const {
  std::u16string text;

  if (is_window_) {
    text = base::NumberToString16(tab_count_) + u" tabs";
  } else {
    text = GetDomainText();
  }

  std::u16string time_text = GetRelativeTimeText();
  if (!text.empty()) {
    text += kSecondarySeparator + time_text;
  } else {
    text = time_text;
  }

  return text;
}

void AstraRecentlyClosedItemView::UpdateSecondaryText() {
  SetSecondaryText(GetSecondaryText());
}

void AstraRecentlyClosedItemView::UpdateTooltipText() {
  std::u16string tooltip;

  if (is_window_) {
    tooltip = base::NumberToString16(tab_count_) + u" tabs";
  } else if (url_.is_valid()) {
    tooltip = base::UTF8ToUTF16(url_.spec());
  }

  tooltip += u"\n" + GetRelativeTimeText();

  SetTooltip(tooltip);
}

// =========================================================================
// Click handling
// =========================================================================

void AstraRecentlyClosedItemView::OnItemClicked() {
  if (delegate_) {
    delegate_->OnRecentlyClosedItemClicked(entry_id_);
  }
}

// =========================================================================
// Mouse events
// =========================================================================

bool AstraRecentlyClosedItemView::OnMousePressed(
    const ui::MouseEvent& event) {
  // Handle middle-click: restore in new tab.
  if (event.IsMiddleMouseButton() && delegate_) {
    delegate_->OnRecentlyClosedItemMiddleClicked(entry_id_);
    return true;
  }
  return AstraSidebarItemView::OnMousePressed(event);
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraRecentlyClosedItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = AstraSidebarItemView::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kRecentlyClosedItemHeight));
  return size;
}

void AstraRecentlyClosedItemView::OnThemeChanged() {
  AstraSidebarItemView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (title_label()) {
    title_label()->SetEnabledColor(
        color_provider->GetColor(kRecentlyClosedItemTitleColorId));
  }
  if (secondary_label()) {
    secondary_label()->SetEnabledColor(
        color_provider->GetColor(kRecentlyClosedItemSecondaryColorId));
  }
}

}  // namespace astra
