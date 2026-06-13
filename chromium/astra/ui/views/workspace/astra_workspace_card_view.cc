#include "astra/ui/views/workspace/astra_workspace_card_view.h"

#include <algorithm>
#include <string>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Card dimensions (medium size, default).
constexpr int kCardWidthMedium = 280;
constexpr int kCardHeightMedium = 220;
constexpr int kCardWidthSmall = 200;
constexpr int kCardHeightSmall = 170;
constexpr int kCardWidthLarge = 360;
constexpr int kCardHeightLarge = 280;
constexpr int kAccentBarHeight = 6;
constexpr int kThumbnailAreaHeight = 120;
constexpr int kInfoAreaHeight = 80;
constexpr int kThumbnailSpacing = 4;
constexpr int kCardCornerRadius = 12;

// List view dimensions.
constexpr int kListRowHeight = 56;
constexpr int kListIconSize = 32;

// Info area layout.
constexpr int kInfoHorizontalPadding = 14;
constexpr int kInfoVerticalPadding = 10;
constexpr int kNameLineHeight = 20;
constexpr int kSubLineHeight = 18;
constexpr int kInfoLineSpacing = 2;

// Thumbnail grid: 2 columns, up to 2 rows (4 thumbnails max).
constexpr int kMaxThumbnails = 4;
constexpr int kThumbnailColumns = 2;

// Menu button size.
constexpr int kMenuButtonSize = 24;

// Shadow elevation values.
constexpr int kShadowElevationDefault = 1;
constexpr int kShadowElevationHover = 3;
constexpr int kShadowElevationSelected = 2;

// Border width.
constexpr int kActiveBorderWidth = 2;
constexpr int kDefaultBorderWidth = 1;

// Focus ring inset from card edge.
constexpr int kFocusRingInset = 2;

// Helper: format a relative time string like "3 days ago" or "just now".
std::u16string FormatRelativeTime(base::Time time) {
  if (time.is_null()) {
    return std::u16string();
  }

  base::TimeDelta delta = base::Time::Now() - time;

  if (delta < base::Minutes(1)) {
    return u"just now";
  }
  if (delta < base::Hours(1)) {
    int minutes = static_cast<int>(delta.InMinutes());
    return base::NumberToString16(minutes) +
           (minutes == 1 ? u" minute ago" : u" minutes ago");
  }
  if (delta < base::Days(1)) {
    int hours = static_cast<int>(delta.InHours());
    return base::NumberToString16(hours) +
           (hours == 1 ? u" hour ago" : u" hours ago");
  }
  if (delta < base::Days(7)) {
    int days = static_cast<int>(delta.InDays());
    return base::NumberToString16(days) +
           (days == 1 ? u" day ago" : u" days ago");
  }
  if (delta < base::Days(30)) {
    int weeks = static_cast<int>(delta.InDays()) / 7;
    return base::NumberToString16(weeks) +
           (weeks == 1 ? u" week ago" : u" weeks ago");
  }
  if (delta < base::Days(365)) {
    int months = static_cast<int>(delta.InDays()) / 30;
    return base::NumberToString16(months) +
           (months == 1 ? u" month ago" : u" months ago");
  }
  int years = static_cast<int>(delta.InDays()) / 365;
  return base::NumberToString16(years) +
         (years == 1 ? u" year ago" : u" years ago");
}

}  // namespace

AstraWorkspaceCardView::AstraWorkspaceCardView() {
  BuildLayout();
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetAccessibleRole(ax::mojom::Role::kListItem);

  // Initial shadow.
  UpdateShadow();
}

AstraWorkspaceCardView::~AstraWorkspaceCardView() = default;

void AstraWorkspaceCardView::BuildLayout() {
  // Vertical stack: accent bar + thumbnail area + info area.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // 1. Accent color bar at the top.
  accent_bar_ = AddChildView(std::make_unique<views::View>());
  accent_bar_->SetPaintToLayer();
  accent_bar_->layer()->SetFillsBoundsOpaquely(true);
  accent_bar_->SetPreferredSize(gfx::Size(kCardWidthMedium, kAccentBarHeight));
  // Default accent color — will be updated by SetAccentColor().
  accent_bar_->layer()->SetColor(SK_ColorBLUE);

  // 2. Thumbnail area — holds a 2x2 grid of placeholder thumbnails.
  thumbnail_grid_ = AddChildView(std::make_unique<views::View>());
  thumbnail_grid_->SetPreferredSize(
      gfx::Size(kCardWidthMedium, kThumbnailAreaHeight));
  thumbnail_grid_->SetPaintToLayer();
  thumbnail_grid_->layer()->SetFillsBoundsOpaquely(false);

  auto* grid_layout =
      thumbnail_grid_->SetLayoutManager(std::make_unique<views::GridLayout>());
  views::ColumnSet* columns = grid_layout->AddColumnSet(0);
  columns->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1.0,
                     views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  columns->AddPaddingColumn(0, kThumbnailSpacing);
  columns->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1.0,
                     views::GridLayout::ColumnSize::kUsePreferred, 0, 0);

  // Create 4 placeholder thumbnail views (2 rows x 2 columns).
  for (int i = 0; i < kMaxThumbnails; ++i) {
    int col = i % kThumbnailColumns;
    int row = i / kThumbnailColumns;
    if (col == 0) {
      if (row > 0) {
        grid_layout->AddPaddingRow(0, kThumbnailSpacing);
      }
      grid_layout->StartRow(views::GridLayout::kFixedSize, 0);
    }

    auto placeholder = std::make_unique<views::View>();
    placeholder->SetPaintToLayer();
    placeholder->layer()->SetFillsBoundsOpaquely(true);
    // TODO(astra): Replace placeholder colored rectangles with real tab
    // thumbnails from Chromium's thumbnail subsystem.
    // Chromium subsystem: chrome/browser/ui/thumbnails/ or
    //   TabThumbnailTracker.  Alternative: content::WebContents::
    //   GetCaptureUpdater().
    // Patch point: integrate thumbnail capture for Astra workspace overview.
    raw_ptr<views::View> ptr =
        grid_layout->AddView(std::move(placeholder));
    thumbnail_placeholders_.push_back(ptr);
  }

  // 3. Info area with two rows:
  //    Row 1: workspace name (left) + menu button (right)
  //    Row 2: secondary info (windows · tabs · created time)
  auto* info_area = AddChildView(std::make_unique<views::View>());
  info_area->SetPreferredSize(gfx::Size(kCardWidthMedium, kInfoAreaHeight));

  auto* info_layout = info_area->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(kInfoVerticalPadding, kInfoHorizontalPadding),
          kInfoLineSpacing));
  info_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Row 1: name + menu button
  auto* name_row = info_area->AddChildView(std::make_unique<views::View>());
  auto* name_row_layout = name_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          /*between_child_spacing=*/0));
  name_row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  name_row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);
  name_row->SetPreferredSize(gfx::Size(kCardWidthMedium - 2 * kInfoHorizontalPadding,
                                       kNameLineHeight));

  name_label_ = name_row->AddChildView(std::make_unique<views::Label>());
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(name_label_->font_list().Derive(
      0, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Hibernation badge (shown when workspace is hibernated).
  hibernation_badge_ = name_row->AddChildView(std::make_unique<views::Label>(u"Hibernated"));
  hibernation_badge_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  hibernation_badge_->SetAutoColorReadabilityEnabled(false);
  hibernation_badge_->SetFontList(
      hibernation_badge_->font_list().Derive(-1, gfx::Font::NORMAL,
                                              gfx::Font::Weight::MEDIUM));
  hibernation_badge_->SetVisible(false);  // Hidden by default.

  // Menu button (⋮)
  // TODO(astra): Use a proper vector icon for the menu (more_horiz or
  //   similar).  Chromium pattern: ui/gfx/vector_icon_types.h with
  //   views::ImageButton::SetImageFromVectorIcon.
  //   For now, use a text label as a visual placeholder.
  auto menu_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(
          [](AstraWorkspaceCardView* card, const ui::Event& event) {
            if (card->menu_action_callback_) {
              // Convert to screen coordinates for menu anchoring.
              gfx::Point screen_pos = event.location();
              views::View::ConvertPointToScreen(card, &screen_pos);
              card->menu_action_callback_.Run(screen_pos);
            }
          },
          base::Unretained(this)));
  menu_button->SetPreferredSize(gfx::Size(kMenuButtonSize, kMenuButtonSize));
  menu_button->SetTooltipText(u"Workspace actions");
  menu_button->SetFocusBehavior(FocusBehavior::ALWAYS);
  menu_button_ = name_row->AddChildView(std::move(menu_button));

  // Row 2: secondary info (window count + tab count + created time)
  auto* sub_row = info_area->AddChildView(std::make_unique<views::View>());
  auto* sub_row_layout = sub_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          /*between_child_spacing=*/0));
  sub_row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  sub_row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  sub_row->SetPreferredSize(gfx::Size(kCardWidthMedium - 2 * kInfoHorizontalPadding,
                                      kSubLineHeight));

  // Window count label.
  window_count_label_ = sub_row->AddChildView(std::make_unique<views::Label>());
  window_count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  window_count_label_->SetAutoColorReadabilityEnabled(false);
  window_count_label_->SetFontList(
      window_count_label_->font_list().Derive(-1, gfx::Font::NORMAL,
                                              gfx::Font::Weight::NORMAL));

  // Separator dot.
  auto* dot1 = sub_row->AddChildView(std::make_unique<views::Label>(u" · "));
  dot1->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  dot1->SetAutoColorReadabilityEnabled(false);
  dot1->SetFontList(
      dot1->font_list().Derive(-1, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));

  // Tab count label.
  tab_count_label_ = sub_row->AddChildView(std::make_unique<views::Label>());
  tab_count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tab_count_label_->SetAutoColorReadabilityEnabled(false);
  tab_count_label_->SetFontList(
      tab_count_label_->font_list().Derive(-1, gfx::Font::NORMAL,
                                           gfx::Font::Weight::NORMAL));

  // Separator dot 2.
  auto* dot2 = sub_row->AddChildView(std::make_unique<views::Label>(u" · "));
  dot2->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  dot2->SetAutoColorReadabilityEnabled(false);
  dot2->SetFontList(
      dot2->font_list().Derive(-1, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));

  // Created time label.
  created_time_label_ = sub_row->AddChildView(std::make_unique<views::Label>());
  created_time_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  created_time_label_->SetAutoColorReadabilityEnabled(false);
  created_time_label_->SetFontList(
      created_time_label_->font_list().Derive(-1, gfx::Font::NORMAL,
                                              gfx::Font::Weight::NORMAL));
  created_time_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Last used time label (shown when show_statistics is true).
  last_used_time_label_ = sub_row->AddChildView(std::make_unique<views::Label>());
  last_used_time_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  last_used_time_label_->SetAutoColorReadabilityEnabled(false);
  last_used_time_label_->SetFontList(
      last_used_time_label_->font_list().Derive(-1, gfx::Font::NORMAL,
                                                  gfx::Font::Weight::NORMAL));
  last_used_time_label_->SetVisible(false);  // Hidden by default.

  // Initial update of visual state.
  UpdateVisualState();

  // Initialize default labels.
  if (window_count_label_) {
    window_count_label_->SetText(u"1 window");
  }
  if (tab_count_label_) {
    tab_count_label_->SetText(u"0 tabs");
  }
}

void AstraWorkspaceCardView::SetWorkspaceName(const std::u16string& name) {
  workspace_name_ = name;
  if (name_label_) {
    name_label_->SetText(name);
  }
  GetViewAccessibility().SetName(name);
  // Accessible name is the workspace name.
  SetAccessibleName(name);
}

void AstraWorkspaceCardView::SetWorkspaceInfo(const AstraWorkspace& workspace) {
  SetWorkspaceName(base::UTF8ToUTF16(workspace.name));
  SetAccentColor(workspace.accent_color);
  SetCreatedTime(workspace.created_time);
  SetLastUsedTime(workspace.last_used_time);
  SetIcon(workspace.icon);
  SetIsActive(workspace.id == "");  // Caller should set active separately.
  SetIsHibernated(workspace.is_hibernated);
  // TODO(astra): Set pinned state from workspace metadata when available.
}

AstraWorkspace AstraWorkspaceCardView::GetWorkspaceInfo() const {
  AstraWorkspace workspace;
  workspace.name = base::UTF16ToUTF8(workspace_name_);
  workspace.accent_color = accent_color_hex_;
  workspace.created_time = created_time_;
  workspace.last_used_time = last_used_time_;
  workspace.icon = icon_;
  workspace.is_hibernated = is_hibernated_;
  return workspace;
}

void AstraWorkspaceCardView::SetAccentColor(const std::string& color_hex) {
  accent_color_hex_ = color_hex;
  // Parse hex to SkColor.
  //
  // TODO(astra): Use a proper color parsing utility.
  //   Chromium component: SkColorParse or similar.
  //   For now, use a simple approach with default fallback.
  SkColor color = gfx::kPlaceholderColor;
  if (!color_hex.empty() && color_hex[0] == '#') {
    // Try to parse #RRGGBB format.
    if (color_hex.size() == 7) {
      std::string hex = color_hex.substr(1);
      uint32_t rgb = 0;
      if (base::HexStringToUInt(hex, &rgb)) {
        color = SkColorSetRGB(
            static_cast<unsigned char>((rgb >> 16) & 0xFF),
            static_cast<unsigned char>((rgb >> 8) & 0xFF),
            static_cast<unsigned char>(rgb & 0xFF));
      }
    }
  }
  accent_color_ = color;
  UpdateAccentColorBar();
}

void AstraWorkspaceCardView::SetAccentColor(SkColor color) {
  accent_color_ = color;
  // Also update the hex string for consistency.
  // Format as #RRGGBB.
  std::string hex = base::StringPrintf(
      "#%02X%02X%02X",
      SkColorGetR(color),
      SkColorGetG(color),
      SkColorGetB(color));
  accent_color_hex_ = hex;
  UpdateAccentColorBar();
}

SkColor AstraWorkspaceCardView::GetAccentColor() const {
  return accent_color_;
}

void AstraWorkspaceCardView::SetTabCount(int tab_count) {
  tab_count_ = std::max(0, tab_count);
  if (tab_count_label_) {
    tab_count_label_->SetText(
        base::NumberToString16(tab_count_) +
        (tab_count_ == 1 ? u" tab" : u" tabs"));
  }
  // Update how many thumbnail placeholders are visible.
  SetThumbnailCount(tab_count);
}

void AstraWorkspaceCardView::SetWindowCount(int window_count) {
  window_count_ = std::max(1, window_count);
  if (window_count_label_) {
    window_count_label_->SetText(
        base::NumberToString16(window_count_) +
        (window_count_ == 1 ? u" window" : u" windows"));
  }
}

void AstraWorkspaceCardView::SetCreatedTime(base::Time created_time) {
  created_time_ = created_time;
  UpdateCreatedTimeLabel();
}

void AstraWorkspaceCardView::SetThumbnailCount(int count) {
  thumbnail_count_ = std::max(0, std::min(count, kMaxThumbnails));

  // Show/hide thumbnail placeholders based on count.
  SkColor placeholder_color = SK_ColorLTGRAY;
  if (GetColorProvider()) {
    // Use a subtle color from the color provider.
    placeholder_color = GetColorProvider()->GetColor(
        ui::kColorSidebarBackground);
  }

  for (size_t i = 0; i < thumbnail_placeholders_.size(); ++i) {
    bool visible = static_cast<int>(i) < thumbnail_count_;
    thumbnail_placeholders_[i]->SetVisible(visible);
    if (visible && thumbnail_placeholders_[i]->layer()) {
      thumbnail_placeholders_[i]->layer()->SetColor(placeholder_color);
    }
  }

  // TODO(astra): When real thumbnails are available, each placeholder will
  // map to an actual tab in the workspace.  The mapping should come from
  // TabStripModel filtered by workspace_id (via AstraTabFeatures).
  // Chromium subsystem: TabStripModel + AstraTabFeatures.
}

void AstraWorkspaceCardView::SetLastUsedTime(base::Time last_used_time) {
  last_used_time_ = last_used_time;
  UpdateLastUsedTimeLabel();
}

void AstraWorkspaceCardView::SetIcon(const std::optional<std::string>& icon) {
  icon_ = icon;
  // TODO(astra): Update icon display when icon system is implemented.
  // For now, the icon is stored but not visually rendered.
  // Chromium pattern: views::ImageView with vector icon.
}

void AstraWorkspaceCardView::SetIsHibernated(bool is_hibernated) {
  if (is_hibernated_ == is_hibernated) {
    return;
  }
  is_hibernated_ = is_hibernated;
  UpdateHibernationBadge();
  UpdateVisualState();
}

void AstraWorkspaceCardView::SetDisplayMode(DisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;

  // Show/hide elements based on mode.
  if (accent_bar_) {
    accent_bar_->SetVisible(mode == DisplayMode::kCard);
  }
  if (thumbnail_grid_) {
    thumbnail_grid_->SetVisible(mode == DisplayMode::kCard);
  }

  UpdateStatisticsVisibility();
  InvalidateLayout();
  SchedulePaint();
}

void AstraWorkspaceCardView::SetShowStatistics(bool show) {
  if (show_statistics_ == show) {
    return;
  }
  show_statistics_ = show;
  UpdateStatisticsVisibility();
}

void AstraWorkspaceCardView::SetSizeVariant(AstraWorkspaceOverviewCardSize size) {
  if (size_variant_ == size) {
    return;
  }
  size_variant_ = size;
  InvalidateLayout();
  PreferredSizeChanged();
}

void AstraWorkspaceCardView::SetIsActive(bool is_active) {
  if (is_active_ == is_active) {
    return;
  }
  is_active_ = is_active;
  UpdateVisualState();
}

void AstraWorkspaceCardView::SetIsSelected(bool is_selected) {
  if (is_selected_ == is_selected) {
    return;
  }
  is_selected_ = is_selected;
  UpdateVisualState();
}

void AstraWorkspaceCardView::SetHovered(bool is_hovered) {
  if (is_hovered_ == is_hovered) {
    return;
  }
  is_hovered_ = is_hovered;
  UpdateVisualState();
  UpdateShadow();
}

// -- Tab / window count visibility ------------------------------------------

void AstraWorkspaceCardView::ShowTabCount(bool show) {
  if (show_tab_count_ == show) {
    return;
  }
  show_tab_count_ = show;
  if (tab_count_label_) {
    tab_count_label_->SetVisible(show);
  }
}

bool AstraWorkspaceCardView::IsTabCountVisible() const {
  return show_tab_count_;
}

void AstraWorkspaceCardView::ShowWindowCount(bool show) {
  if (show_window_count_ == show) {
    return;
  }
  show_window_count_ = show;
  if (window_count_label_) {
    window_count_label_->SetVisible(show);
  }
}

bool AstraWorkspaceCardView::IsWindowCountVisible() const {
  return show_window_count_;
}

// -- Menu button visibility -------------------------------------------------

void AstraWorkspaceCardView::ShowMenuButton(bool show) {
  if (show_menu_button_ == show) {
    return;
  }
  show_menu_button_ = show;
  if (menu_button_) {
    menu_button_->SetVisible(show);
  }
}

bool AstraWorkspaceCardView::IsMenuButtonVisible() const {
  return show_menu_button_;
}

// -- Pinned state -----------------------------------------------------------

void AstraWorkspaceCardView::SetPinned(bool pinned) {
  if (is_pinned_ == pinned) {
    return;
  }
  is_pinned_ = pinned;
  // TODO(astra): Update visual state to show pinned indicator.
  //   For now, just store the state.
  SchedulePaint();
}

void AstraWorkspaceCardView::SetClickCallback(ClickCallback callback) {
  click_callback_ = std::move(callback);
}

void AstraWorkspaceCardView::SetRenameCallback(RenameCallback callback) {
  rename_callback_ = std::move(callback);
}

void AstraWorkspaceCardView::SetMenuActionCallback(MenuActionCallback callback) {
  menu_action_callback_ = std::move(callback);
}

void AstraWorkspaceCardView::SetDeleteCallback(DeleteCallback callback) {
  delete_callback_ = std::move(callback);
}

bool AstraWorkspaceCardView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    is_pressed_ = true;
    UpdateVisualState();
    RequestFocus();
    return true;
  }
  return views::View::OnMousePressed(event);
}

bool AstraWorkspaceCardView::OnMouseDragged(const ui::MouseEvent& event) {
  // TODO(astra): Implement drag and drop reordering for workspace cards.
  // Chromium pattern: views::View::OnMouseDragged with drag source / drop
  // target, or ui::OSExchangeData.
  // For now, just consume the drag to maintain pressed visual state.
  return true;
}

void AstraWorkspaceCardView::OnMouseReleased(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton() && is_pressed_) {
    is_pressed_ = false;
    UpdateVisualState();

    // Only trigger click if the release is within the card bounds.
    if (GetLocalBounds().Contains(event.location())) {
      if (click_callback_) {
        click_callback_.Run();
      }
    }
  }
  views::View::OnMouseReleased(event);
}

bool AstraWorkspaceCardView::OnMouseDoubleClicked(
    const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    // Double-click triggers rename.
    if (rename_callback_) {
      rename_callback_.Run();
    }
    return true;
  }
  return views::View::OnMouseDoubleClicked(event);
}

void AstraWorkspaceCardView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_ = true;
  UpdateVisualState();
  views::View::OnMouseEntered(event);
}

void AstraWorkspaceCardView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_ = false;
  is_pressed_ = false;
  UpdateVisualState();
  views::View::OnMouseExited(event);
}

bool AstraWorkspaceCardView::OnKeyPressed(const ui::KeyEvent& event) {
  switch (event.key_code()) {
    case ui::VKEY_SPACE:
    case ui::VKEY_RETURN:
      if (click_callback_) {
        click_callback_.Run();
      }
      return true;

    case ui::VKEY_DELETE:
    case ui::VKEY_BACK:
      if (delete_callback_ && !is_active_) {
        // Can't delete the active workspace.
        delete_callback_.Run();
      }
      return true;

    case ui::VKEY_F2:
      if (rename_callback_) {
        rename_callback_.Run();
      }
      return true;

    case ui::VKEY_APPS:
      if (menu_action_callback_) {
        gfx::Point screen_pos;
        views::View::ConvertPointToScreen(this, &screen_pos);
        menu_action_callback_.Run(screen_pos);
      }
      return true;

    default:
      break;
  }
  return views::View::OnKeyPressed(event);
}

void AstraWorkspaceCardView::OnFocus() {
  is_selected_ = true;
  UpdateVisualState();
  views::View::OnFocus();
}

void AstraWorkspaceCardView::OnBlur() {
  is_selected_ = false;
  UpdateVisualState();
  views::View::OnBlur();
}

void AstraWorkspaceCardView::OnThemeChanged() {
  views::View::OnThemeChanged();

  // Update text colors from the color provider.
  const ui::ColorProvider* cp = GetColorProvider();
  if (!cp) {
    return;
  }

  if (name_label_) {
    name_label_->SetEnabledColor(cp->GetColor(ui::kColorLabelForeground));
  }
  if (window_count_label_) {
    window_count_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
  if (tab_count_label_) {
    tab_count_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
  if (created_time_label_) {
    created_time_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
  if (last_used_time_label_) {
    last_used_time_label_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }
  if (hibernation_badge_) {
    hibernation_badge_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }

  // Update thumbnail placeholder colors.
  for (auto* placeholder : thumbnail_placeholders_) {
    if (placeholder && placeholder->layer()) {
      placeholder->layer()->SetColor(
          cp->GetColor(ui::kColorSidebarBackground));
    }
  }

  // Refresh visual state (background, border) with new colors.
  UpdateVisualState();
}

void AstraWorkspaceCardView::OnPaintBackground(gfx::Canvas* canvas) {
  const ui::ColorProvider* cp = GetColorProvider();

  // Determine background color.
  SkColor bg_color = SK_ColorWHITE;
  if (cp) {
    bg_color = cp->GetColor(ui::kColorDialogBackground);
    if (is_hovered_) {
      // Slightly lighter on hover.
      bg_color = cp->GetColor(ui::kColorButtonBackgroundHovered);
    }
    if (is_selected_) {
      // Subtle tint for keyboard selection.
      bg_color = cp->GetColor(kColorAstraWorkspaceAccentSubtle);
    }
  }

  cc::PaintFlags flags;
  flags.setColor(bg_color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  canvas->DrawRoundRect(GetLocalBounds(), kCardCornerRadius, flags);
}

void AstraWorkspaceCardView::OnPaintBorder(gfx::Canvas* canvas) {
  const ui::ColorProvider* cp = GetColorProvider();

  SkColor border_color = SK_ColorLTGRAY;
  int border_width = kDefaultBorderWidth;

  if (cp) {
    if (is_active_) {
      border_color = cp->GetColor(kColorAstraWorkspaceAccent);
      border_width = kActiveBorderWidth;
    } else {
      border_color = cp->GetColor(ui::kColorSeparator);
    }
  }

  cc::PaintFlags border_flags;
  border_flags.setColor(border_color);
  border_flags.setStyle(cc::PaintFlags::kStroke_Style);
  border_flags.setStrokeWidth(border_width);
  border_flags.setAntiAlias(true);

  gfx::RectF border_rect(GetLocalBounds());
  border_rect.Inset(border_width / 2.0f);
  canvas->DrawRoundRect(border_rect, kCardCornerRadius, border_flags);

  // Draw focus ring if selected (keyboard focus).
  if (is_selected_ && cp) {
    SkColor focus_color = cp->GetColor(kColorAstraWorkspaceAccent);
    cc::PaintFlags focus_flags;
    focus_flags.setColor(focus_color);
    focus_flags.setStyle(cc::PaintFlags::kStroke_Style);
    focus_flags.setStrokeWidth(2);
    focus_flags.setAntiAlias(true);

    gfx::RectF focus_rect(GetLocalBounds());
    focus_rect.Inset(kFocusRingInset);
    canvas->DrawRoundRect(focus_rect, kCardCornerRadius - kFocusRingInset,
                          focus_flags);
  }
}

void AstraWorkspaceCardView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);

  node_data->role = ax::mojom::Role::kListItem;
  if (!workspace_name_.empty()) {
    node_data->SetName(workspace_name_);
  }

  // Add state description.
  std::u16string desc;
  if (is_active_) {
    desc += u"Active workspace. ";
  }
  if (is_hibernated_) {
    desc += u"Hibernated. ";
  }
  desc += base::NumberToString16(tab_count_) +
          (tab_count_ == 1 ? u" tab. " : u" tabs. ");
  desc += base::NumberToString16(window_count_) +
          (window_count_ == 1 ? u" window. " : u" windows. ");
  if (!created_time_.is_null()) {
    desc += u"Created " + FormatRelativeTime(created_time_) + u". ";
  }
  if (!last_used_time_.is_null() && show_statistics_) {
    desc += u"Last used " + FormatRelativeTime(last_used_time_) + u".";
  }
  node_data->SetDescription(desc);

  if (is_active_) {
    node_data->AddState(ax::mojom::State::kSelected);
  }
}

gfx::Size AstraWorkspaceCardView::CalculatePreferredSize() const {
  if (display_mode_ == DisplayMode::kList) {
    // In list mode, expand to full width with fixed row height.
    return gfx::Size(0, GetListRowHeight());
  }
  return gfx::Size(GetCardWidth(), GetCardHeight());
}

void AstraWorkspaceCardView::UpdateAccentColorBar() {
  if (!accent_bar_ || accent_color_hex_.empty()) {
    return;
  }

  // Parse hex color string like "#RRGGBB".
  // TODO(astra): Use SkColorParseName or a proper color parsing utility
  // once we have a color system in place.  This simple parser handles the
  // common #RRGGBB format.
  if (accent_color_hex_.size() == 7 && accent_color_hex_[0] == '#') {
    unsigned int color_val = 0;
    if (base::HexStringToUInt(accent_color_hex_.substr(1), &color_val)) {
      SkColor color =
          SkColorSetRGB(SkColorGetR(color_val), SkColorGetG(color_val),
                        SkColorGetB(color_val));
      accent_bar_->layer()->SetColor(color);
    }
  }
}

void AstraWorkspaceCardView::UpdateVisualState() {
  // Trigger repaints for background and border.
  SchedulePaint();

  // Update shadow based on state.
  UpdateShadow();
}

void AstraWorkspaceCardView::UpdateCreatedTimeLabel() {
  if (!created_time_label_) {
    return;
  }
  created_time_label_->SetText(FormatRelativeTime(created_time_));
}

void AstraWorkspaceCardView::UpdateShadow() {
  if (!layer()) {
    return;
  }

  int elevation = kShadowElevationDefault;
  if (is_selected_) {
    elevation = kShadowElevationSelected;
  }
  if (is_hovered_) {
    elevation = kShadowElevationHover;
  }

  if (elevation > 0) {
    gfx::ShadowValues shadows =
        gfx::ShadowValue::MakeMdShadowValues(elevation);
    layer()->SetShadowValues(shadows);
  } else {
    layer()->SetShadowValues(gfx::ShadowValues());
  }

  // Apply hibernation opacity.
  if (layer()) {
    float opacity = is_hibernated_ ? 0.6f : 1.0f;
    layer()->SetOpacity(opacity);
  }
}

void AstraWorkspaceCardView::UpdateStatisticsVisibility() {
  if (last_used_time_label_) {
    last_used_time_label_->SetVisible(show_statistics_ &&
                                       !last_used_time_.is_null());
  }
  if (created_time_label_) {
    // In list mode, hide created time to save space.
    created_time_label_->SetVisible(display_mode_ == DisplayMode::kCard);
  }
}

void AstraWorkspaceCardView::UpdateHibernationBadge() {
  if (hibernation_badge_) {
    hibernation_badge_->SetVisible(is_hibernated_);
    if (is_hibernated_ && GetColorProvider()) {
      hibernation_badge_->SetEnabledColor(
          GetColorProvider()->GetColor(ui::kColorLabelForegroundSecondary));
    }
  }
}

void AstraWorkspaceCardView::UpdateLastUsedTimeLabel() {
  if (!last_used_time_label_ || last_used_time_.is_null()) {
    return;
  }
  last_used_time_label_->SetText(
      u" · used " + FormatRelativeTime(last_used_time_));
}

int AstraWorkspaceCardView::GetCardWidth() const {
  switch (size_variant_) {
    case AstraWorkspaceOverviewCardSize::kSmall:
      return kCardWidthSmall;
    case AstraWorkspaceOverviewCardSize::kMedium:
      return kCardWidthMedium;
    case AstraWorkspaceOverviewCardSize::kLarge:
      return kCardWidthLarge;
  }
  return kCardWidthMedium;
}

int AstraWorkspaceCardView::GetCardHeight() const {
  switch (size_variant_) {
    case AstraWorkspaceOverviewCardSize::kSmall:
      return kCardHeightSmall;
    case AstraWorkspaceOverviewCardSize::kMedium:
      return kCardHeightMedium;
    case AstraWorkspaceOverviewCardSize::kLarge:
      return kCardHeightLarge;
  }
  return kCardHeightMedium;
}

int AstraWorkspaceCardView::GetListRowHeight() const {
  return kListRowHeight;
}

}  // namespace astra
