#include "astra/ui/views/sidebar/astra_history_item_view.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/ui/color/astra_color_ids.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kHistoryItemHeight = 40;
constexpr int kHistoryItemVerticalPadding = 4;
constexpr int kHistoryItemIconSpacing = 8;
constexpr int kHistoryItemCornerRadius = 6;
constexpr int kHistoryItemUrlFontSizeDelta = -1;

// Separator between domain and visit count in secondary text.
const char16_t kSecondarySeparator[] = u" \u00b7 ";

// Astra color IDs for history items.
constexpr ui::ColorId kHistoryItemTitleColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kHistoryItemUrlColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kHistoryItemHoverBgColorId =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kHistoryItemSelectedBgColorId =
    kColorAstraSidebarItemSelectedBackground;

}  // namespace

AstraHistoryItemView::AstraHistoryItemView(const std::u16string& title,
                                           const GURL& url,
                                           base::Time visit_time)
    : url_(url), visit_time_(visit_time) {
  // Base class calls BuildLayout which is virtual, but our override will
  // not be called during base construction. So we call our setup here.

  SetTitle(title);
  SetSecondaryText(GetDomainText());

  // Set up icon placeholder.
  // TODO(astra): Wire up real favicon.
  icon_view()->SetVisible(true);

  // Set initial tooltip.
  UpdateTooltipText();
}

AstraHistoryItemView::~AstraHistoryItemView() = default;

void AstraHistoryItemView::BuildLayout() {
  // Call base class to build common layout.
  AstraSidebarItemView::BuildLayout();

  // History items have two lines of text (title + URL/domain).
  // The base class text container has title + secondary labels.
  // We just need to make sure the secondary label is visible.
  // (We set it visible when we set text.)
}

// =========================================================================
// History info
// =========================================================================

void AstraHistoryItemView::SetHistoryInfo(const GURL& url,
                                          const std::u16string& title,
                                          base::Time visit_time) {
  url_ = url;
  visit_time_ = visit_time;
  SetTitle(title);
  UpdateSecondaryText();
  UpdateTooltipText();
  SchedulePaint();
}

// =========================================================================
// Visit count
// =========================================================================

void AstraHistoryItemView::SetVisitCount(int count) {
  if (visit_count_ == count) {
    return;
  }
  visit_count_ = count;
  if (show_visit_count_) {
    UpdateSecondaryText();
  }
}

void AstraHistoryItemView::ShowVisitCount(bool show) {
  if (show_visit_count_ == show) {
    return;
  }
  show_visit_count_ = show;
  UpdateSecondaryText();
}

// =========================================================================
// Time grouping
// =========================================================================

void AstraHistoryItemView::SetIsToday(bool is_today) {
  if (is_today_ == is_today) {
    return;
  }
  is_today_ = is_today;
  // TODO(astra): Apply special styling for today's items (e.g., bold text).
  OnThemeChanged();
}

void AstraHistoryItemView::SetTimeGroup(const std::u16string& group_name) {
  time_group_ = group_name;
}

// =========================================================================
// Typed visit
// =========================================================================

void AstraHistoryItemView::SetTypedVisit(bool typed) {
  if (is_typed_visit_ == typed) {
    return;
  }
  is_typed_visit_ = typed;
  // TODO(astra): Show a special indicator for typed visits.
  //   Chrome's history page shows a different icon for typed URLs.
}

// =========================================================================
// Helpers
// =========================================================================

std::u16string AstraHistoryItemView::GetRelativeTimeText() const {
  // Simple relative time calculation.
  // TODO(astra): Replace with TimeFormat::Simple(TimeFormat::FORMAT_ELAPSED, ...)
  //   from ui/base/l10n/time_format.h for proper localized formatting.
  //   Chromium owner: ui/base/l10n/time_format.h
  base::TimeDelta delta = base::Time::Now() - visit_time_;

  if (delta < base::Minutes(1)) {
    return u"Just now";
  }
  if (delta < base::Hours(1)) {
    int minutes = static_cast<int>(delta.InMinutes());
    return base::NumberToString16(minutes) + u" min ago";
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
    return base::NumberToString16(days) + u" days ago";
  }

  // Older — show date (simplified).
  base::Time::Exploded exploded;
  visit_time_.LocalExplode(&exploded);
  std::u16string date =
      base::NumberToString16(exploded.month) + u"/" +
      base::NumberToString16(exploded.day);
  return date;
}

std::u16string AstraHistoryItemView::GetDomainText() const {
  if (!url_.is_valid()) {
    return std::u16string();
  }
  // Show the hostname as the secondary text.
  // TODO(astra): Use a more sophisticated domain extraction that strips
  //   common subdomains (e.g., www.) similar to Chrome's history page.
  return base::UTF8ToUTF16(url_.host());
}

void AstraHistoryItemView::UpdateSecondaryText() {
  std::u16string domain = GetDomainText();

  if (show_visit_count_ && visit_count_ > 1) {
    domain += kSecondarySeparator +
              base::NumberToString16(visit_count_) + u" visits";
  }

  SetSecondaryText(domain);
}

void AstraHistoryItemView::UpdateTooltipText() {
  // Tooltip shows full URL and relative time.
  if (!url_.is_valid()) {
    SetTooltip(std::u16string());
    return;
  }

  std::u16string tooltip = base::UTF8ToUTF16(url_.spec());
  tooltip += u"\n" + GetRelativeTimeText();

  if (visit_count_ > 1) {
    tooltip += u"\n" + base::NumberToString16(visit_count_) + u" visits";
  }

  SetTooltip(tooltip);
}

// =========================================================================
// Click handling
// =========================================================================

void AstraHistoryItemView::OnItemClicked() {
  if (delegate_) {
    delegate_->OnHistoryItemClicked(url_);
  }
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraHistoryItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = AstraSidebarItemView::CalculatePreferredSize(available_size);
  size.set_height(std::max(size.height(), kHistoryItemHeight));
  return size;
}

void AstraHistoryItemView::OnThemeChanged() {
  AstraSidebarItemView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Apply colors to text labels.
  if (title_label()) {
    title_label()->SetEnabledColor(
        color_provider->GetColor(kHistoryItemTitleColorId));
  }
  if (secondary_label()) {
    secondary_label()->SetEnabledColor(
        color_provider->GetColor(kHistoryItemUrlColorId));
    secondary_label()->SetFontList(
        secondary_label()->font_list().DeriveWithSizeDelta(
            kHistoryItemUrlFontSizeDelta));
  }

  // Today's items get bold title text.
  if (is_today_ && title_label()) {
    title_label()->SetFontList(
        title_label()->font_list().DeriveWithWeight(gfx::Font::Weight::MEDIUM));
  }
}

}  // namespace astra
