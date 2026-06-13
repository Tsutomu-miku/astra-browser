#include "astra/ui/views/tab_search/astra_tab_search_item_view.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/include/core/SkColor.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/ui_base_types.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/accessibility/view_ax_platform_node_delegate.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/metadata/metadata_header_macros.h"

#include "astra/ui/color/astra_color_ids.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kItemVerticalPadding = 10;
constexpr int kItemHorizontalPadding = 12;
constexpr int kItemSpacing = 6;
constexpr int kFaviconSize = 16;
constexpr int kFaviconSpacing = 12;
constexpr int kCloseButtonSize = 16;
constexpr int kShortcutSpacing = 8;
constexpr int kTextRowSpacing = 2;
constexpr int kGroupColorStripWidth = 3;
constexpr int kWorkspaceLabelPaddingHorizontal = 6;
constexpr int kWorkspaceLabelPaddingVertical = 2;
constexpr int kAudioIndicatorSize = 14;
constexpr int kPinnedIndicatorSize = 12;

// Font sizes relative to default.
constexpr int kTitleFontSizeDelta = 0;
constexpr int kHostFontSizeDelta = -2;
constexpr int kShortcutFontSizeDelta = -1;
constexpr int kWorkspaceFontSizeDelta = -2;

// Group header constants.
constexpr int kGroupHeaderVerticalPadding = 8;
constexpr int kGroupHeaderHorizontalPadding = 12;

// Astra color IDs for tab search items.
// TODO(astra): Define dedicated tab-search color IDs in astra_color_ids.h
//   instead of reusing command palette / sidebar colors.
constexpr ui::ColorId kItemSelectedBackground =
    kColorAstraCommandPaletteSelectedBackground;
constexpr ui::ColorId kItemHoverBackground =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kItemFocusRing =
    kColorAstraWorkspaceAccent;
constexpr ui::ColorId kTitleTextColor = kColorAstraCommandPaletteText;
constexpr ui::ColorId kHostTextColor =
    kColorAstraCommandPaletteDescriptionText;
constexpr ui::ColorId kShortcutTextColor =
    kColorAstraCommandPaletteShortcutText;
constexpr ui::ColorId kWorkspaceTextColor =
    kColorAstraCommandPaletteDescriptionText;
constexpr ui::ColorId kWorkspaceBackground =
    kColorAstraSidebarItemHoverBackground;
constexpr ui::ColorId kFaviconPlaceholderColor = ui::kColorIcon;
constexpr ui::ColorId kAudioIndicatorColor = ui::kColorIcon;

constexpr ui::ColorId kGroupHeaderTextColor =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kGroupHeaderCountTextColor =
    kColorAstraCommandPaletteDescriptionText;

// Get a human-readable group name for accessibility.
std::u16string GetGroupName(AstraTabSearchItemView::Group group) {
  switch (group) {
    case AstraTabSearchItemView::Group::kOpenTabs:
      return u"Open tab";
    case AstraTabSearchItemView::Group::kRecentlyClosed:
      return u"Recently closed";
    case AstraTabSearchItemView::Group::kBookmarks:
      return u"Bookmark";
  }
  return u"Tab";
}

// Get a human-readable result type label.
std::u16string GetResultTypeName(AstraTabSearchResultType type) {
  switch (type) {
    case AstraTabSearchResultType::kOpenTab:
      return u"Open tab";
    case AstraTabSearchResultType::kRecentlyClosed:
      return u"Recently closed";
    case AstraTabSearchResultType::kBookmark:
      return u"Bookmark";
    case AstraTabSearchResultType::kHistory:
      return u"History";
    case AstraTabSearchResultType::kSearchHistory:
      return u"Recent search";
    case AstraTabSearchResultType::kAction:
      return u"Action";
  }
  return u"Result";
}

}  // namespace

// =========================================================================
// AstraTabSearchGroupHeaderView — group section header
// =========================================================================

AstraTabSearchGroupHeaderView::AstraTabSearchGroupHeaderView(
    const std::u16string& title,
    size_t count)
    : title_(title), count_(count) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  BuildLayout();
}

AstraTabSearchGroupHeaderView::~AstraTabSearchGroupHeaderView() = default;

void AstraTabSearchGroupHeaderView::SetTitle(const std::u16string& title) {
  if (title_ == title) {
    return;
  }
  title_ = title;
  if (title_label_) {
    title_label_->SetText(title);
  }
  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

void AstraTabSearchGroupHeaderView::SetCount(size_t count) {
  if (count_ == count) {
    return;
  }
  count_ = count;
  if (count_label_) {
    count_label_->SetText(base::NumberToString16(count));
  }
  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

void AstraTabSearchGroupHeaderView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

void AstraTabSearchGroupHeaderView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kGroup;
  node_data->SetName(title_);
  node_data->SetDescription(
      base::NumberToString16(count_) + u" items");
}

gfx::Size AstraTabSearchGroupHeaderView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraTabSearchGroupHeaderView::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(kGroupHeaderVerticalPadding,
                      kGroupHeaderHorizontalPadding),
      kItemSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Title label (bold, section header color).
  title_label_ = AddChildView(std::make_unique<views::Label>(title_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kHostFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::BOLD));
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  layout->SetFlexForView(title_label_, 1);

  // Count label (secondary color).
  count_label_ = AddChildView(
      std::make_unique<views::Label>(base::NumberToString16(count_)));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kShortcutFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::NORMAL));

  UpdateColors();
}

void AstraTabSearchGroupHeaderView::UpdateColors() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (title_label_) {
    title_label_->SetEnabledColor(
        color_provider->GetColor(kGroupHeaderTextColor));
  }
  if (count_label_) {
    count_label_->SetEnabledColor(
        color_provider->GetColor(kGroupHeaderCountTextColor));
  }
}

// =========================================================================
// AstraTabSearchItemView — single result item
// =========================================================================

// =========================================================================
// Construction
// =========================================================================

AstraTabSearchItemView::AstraTabSearchItemView(
    const AstraTabSearchItem& tab)
    : tab_data_(tab),
      display_index_(0) {
  // Map result type to legacy group.
  switch (tab.result_type) {
    case AstraTabSearchResultType::kOpenTab:
      legacy_group_ = Group::kOpenTabs;
      break;
    case AstraTabSearchResultType::kRecentlyClosed:
      legacy_group_ = Group::kRecentlyClosed;
      break;
    case AstraTabSearchResultType::kBookmark:
      legacy_group_ = Group::kBookmarks;
      break;
    default:
      legacy_group_ = Group::kOpenTabs;
      break;
  }
  legacy_identifier_ = base::NumberToString(tab.item_id);

  // Make the view focusable for keyboard navigation.
  SetFocusBehavior(FocusBehavior::ALWAYS);

  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  BuildLayout();
}

AstraTabSearchItemView::AstraTabSearchItemView(const TabInfo& tab_info,
                                               int display_index)
    : display_index_(display_index) {
  // Populate tab_data_ from legacy TabInfo.
  tab_data_.title = tab_info.title;
  tab_data_.hostname = tab_info.host;
  tab_data_.tab_index = tab_info.tab_index;
  tab_data_.tab_id = tab_info.tab_index;
  tab_data_.item_id = tab_info.tab_index;
  tab_data_.result_type = AstraTabSearchResultType::kOpenTab;

  legacy_group_ = tab_info.group;
  legacy_identifier_ = tab_info.identifier;

  // Make the view focusable.
  SetFocusBehavior(FocusBehavior::ALWAYS);

  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  BuildLayout();
}

AstraTabSearchItemView::~AstraTabSearchItemView() = default;

// =========================================================================
// Layout construction
// =========================================================================

void AstraTabSearchItemView::BuildLayout() {
  // Horizontal layout: group strip | favicon | text area (flex) | right side.
  auto* main_layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 0));
  main_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // 1. Group color strip (left edge).
  group_color_strip_ = AddChildView(std::make_unique<views::View>());
  group_color_strip_->SetPreferredSize(
      gfx::Size(kGroupColorStripWidth, 0));
  group_color_strip_->SetPaintToLayer();
  group_color_strip_->layer()->SetFillsBoundsOpaquely(true);
  group_color_strip_->layer()->SetColor(SK_ColorTRANSPARENT);

  // Inner content area (with padding).
  auto* content = AddChildView(std::make_unique<views::View>());
  auto* content_layout = content->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kItemVerticalPadding, kItemHorizontalPadding),
          kFaviconSpacing));
  content_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  main_layout->SetFlexForView(content, 1);

  // 2. Favicon placeholder (colored square).
  // TODO(astra): Replace with real favicon from FaviconService.
  favicon_placeholder_ = content->AddChildView(std::make_unique<views::View>());
  favicon_placeholder_->SetPreferredSize(
      gfx::Size(kFaviconSize, kFaviconSize));
  favicon_placeholder_->SetPaintToLayer();
  favicon_placeholder_->layer()->SetFillsBoundsOpaquely(true);

  // Pinned indicator overlay (small corner indicator).
  // TODO(astra): Use a proper pin vector icon.
  //   For now, we show/hide a small indicator view.
  pinned_indicator_ = content->AddChildView(std::make_unique<views::View>());
  pinned_indicator_->SetPreferredSize(
      gfx::Size(kPinnedIndicatorSize, kPinnedIndicatorSize));
  pinned_indicator_->SetPaintToLayer();
  pinned_indicator_->layer()->SetFillsBoundsOpaquely(true);
  pinned_indicator_->layer()->SetColor(SK_ColorTRANSPARENT);
  pinned_indicator_->SetVisible(false);

  // 3. Text container (vertical stack: title row + host row).
  auto* text_container = content->AddChildView(std::make_unique<views::View>());
  auto* text_layout = text_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kTextRowSpacing));
  text_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  content_layout->SetFlexForView(text_container, 1);

  // Title row: title (flex) + workspace + shortcut hint.
  auto* title_row = text_container->AddChildView(std::make_unique<views::View>());
  auto* title_row_layout = title_row->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  title_row_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  title_row_layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  title_row_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  // Title label (bold, primary text).
  title_label_ =
      title_row->AddChildView(std::make_unique<views::Label>(tab_data_.title));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  title_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kTitleFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::BOLD));
  title_row_layout->SetFlexForView(title_label_, 1);

  // Workspace name label (right of title).
  workspace_label_ =
      title_row->AddChildView(std::make_unique<views::Label>(
          tab_data_.workspace_name));
  workspace_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  workspace_label_->SetAutoColorReadabilityEnabled(false);
  workspace_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kWorkspaceFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::NORMAL));
  workspace_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kWorkspaceLabelPaddingVertical,
                       kWorkspaceLabelPaddingHorizontal)));
  workspace_label_->SetPaintToLayer();
  workspace_label_->layer()->SetFillsBoundsOpaquely(true);

  // Shortcut hint label (right-aligned, secondary text).
  // Shows "Ctrl+1" .. "Ctrl+9" for the first 9 results.
  shortcut_label_ =
      title_row->AddChildView(std::make_unique<views::Label>(std::u16string()));
  shortcut_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  shortcut_label_->SetAutoColorReadabilityEnabled(false);
  shortcut_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kShortcutFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::NORMAL));
  UpdateShortcutHint();

  // Host / URL label (secondary text, below title).
  host_label_ =
      text_container->AddChildView(std::make_unique<views::Label>(
          tab_data_.hostname));
  host_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  host_label_->SetAutoColorReadabilityEnabled(false);
  host_label_->SetElideBehavior(gfx::ELIDE_END);
  host_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
      kHostFontSizeDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::NORMAL));

  // 4. Right side: audio indicator + close button.
  auto* right_side = content->AddChildView(std::make_unique<views::View>());
  auto* right_layout = right_side->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kItemSpacing));
  right_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Audio indicator.
  // TODO(astra): Use a proper vector icon for audio/muted state.
  //   Chromium component: ui/views/vector_icons/
  //   For now, use a colored square as placeholder.
  audio_indicator_ = right_side->AddChildView(std::make_unique<views::View>());
  audio_indicator_->SetPreferredSize(
      gfx::Size(kAudioIndicatorSize, kAudioIndicatorSize));
  audio_indicator_->SetPaintToLayer();
  audio_indicator_->layer()->SetFillsBoundsOpaquely(true);
  audio_indicator_->SetTooltipText(u"Audio playing");
  audio_indicator_->SetAccessibleName(u"Audio playing");

  // Close button (visible on hover / selection).
  // TODO(astra): Use a proper close button icon from Chromium's vector icon
  //   set (e.g. kCloseIcon from ui/views/vector_icons/).
  close_button_ = right_side->AddChildView(std::make_unique<views::ImageButton>());
  close_button_->SetPreferredSize(
      gfx::Size(kCloseButtonSize, kCloseButtonSize));
  close_button_->SetVisible(false);
  close_button_->SetTooltipText(u"Close tab");
  close_button_->SetAccessibleName(u"Close tab");
  close_button_->SetCallback(base::BindRepeating(
      [](base::WeakPtr<AstraTabSearchItemView> weak_this) {
        if (weak_this && weak_this->close_callback_) {
          weak_this->close_callback_.Run();
        }
      },
      weak_ptr_factory_.GetWeakPtr()));

  // Update initial state.
  UpdateTextFromTab();
  UpdateGroupColorStrip();
  UpdateAudioIndicator();
  UpdateWorkspaceLabel();
  UpdatePinnedIndicator();
  UpdateFaviconAppearance();
  UpdateBackground();
  UpdateTextColors();
  UpdateCloseButtonVisibility();
  UpdateShortcutHint();
  UpdateSiteInfo();
}

// =========================================================================
// Public API — tab data
// =========================================================================

void AstraTabSearchItemView::SetTab(const AstraTabSearchItem& tab) {
  tab_data_ = tab;
  UpdateTextFromTab();
  UpdateGroupColorStrip();
  UpdateAudioIndicator();
  UpdateWorkspaceLabel();
  UpdatePinnedIndicator();
  UpdateFaviconAppearance();
  UpdateSiteInfo();
  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

// =========================================================================
// Public API — selection / highlighting
// =========================================================================

void AstraTabSearchItemView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  UpdateBackground();
  UpdateCloseButtonVisibility();
  if (selected) {
    NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
    NotifyAccessibilityEvent(ax::mojom::Event::kSelectedChildrenChanged, true);
  }
}

void AstraTabSearchItemView::SetHighlighted(bool highlighted) {
  if (highlighted_ == highlighted) {
    return;
  }
  highlighted_ = highlighted;
  UpdateBackground();
  UpdateCloseButtonVisibility();
}

// =========================================================================
// Public API — match highlighting
// =========================================================================

void AstraTabSearchItemView::SetTitleMatchRanges(
    const std::vector<gfx::Range>& ranges) {
  title_match_ranges_ = ranges;
  // TODO(astra): Apply match highlighting to title_label_ using StyledLabel.
  //   Currently we store the ranges but visual highlighting is not yet
  //   implemented — requires switching from views::Label to views::StyledLabel.
  //   Chromium component: views::StyledLabel (ui/views/controls/styled_label.h)
}

void AstraTabSearchItemView::SetUrlMatchRanges(
    const std::vector<gfx::Range>& ranges) {
  url_match_ranges_ = ranges;
  // TODO(astra): Apply match highlighting to host_label_ using StyledLabel.
}

void AstraTabSearchItemView::SetMatches(
    const std::vector<AstraTabSearchMatch>& matches) {
  // Separate matches by type.
  std::vector<gfx::Range> title_ranges;
  std::vector<gfx::Range> url_ranges;

  for (const auto& match : matches) {
    if (match.type == AstraTabSearchMatch::Type::kTitle) {
      title_ranges.push_back(match.range);
    } else if (match.type == AstraTabSearchMatch::Type::kHostname ||
               match.type == AstraTabSearchMatch::Type::kUrl) {
      url_ranges.push_back(match.range);
    }
  }

  SetTitleMatchRanges(title_ranges);
  SetUrlMatchRanges(url_ranges);
}

// =========================================================================
// Public API — visibility toggles
// =========================================================================

void AstraTabSearchItemView::ShowFavicon(bool show) {
  if (show_favicon_ == show) {
    return;
  }
  show_favicon_ = show;
  if (favicon_placeholder_) {
    favicon_placeholder_->SetVisible(show);
  }
}

void AstraTabSearchItemView::ShowWorkspace(bool show) {
  if (show_workspace_ == show) {
    return;
  }
  show_workspace_ = show;
  UpdateWorkspaceLabel();
}

void AstraTabSearchItemView::ShowAudioIndicator(bool show) {
  if (show_audio_indicator_ == show) {
    return;
  }
  show_audio_indicator_ = show;
  UpdateAudioIndicator();
}

void AstraTabSearchItemView::ShowCloseButton(bool show) {
  if (show_close_button_ == show) {
    return;
  }
  show_close_button_ = show;
  UpdateCloseButtonVisibility();
}

void AstraTabSearchItemView::ShowShortcutHint(bool show) {
  if (show_shortcut_hint_ == show) {
    return;
  }
  show_shortcut_hint_ = show;
  UpdateShortcutHint();
}

void AstraTabSearchItemView::ShowSiteInfo(bool show) {
  if (show_site_info_ == show) {
    return;
  }
  show_site_info_ = show;
  UpdateSiteInfo();
}

// =========================================================================
// Public API — group color
// =========================================================================

void AstraTabSearchItemView::SetGroupColor(SkColor color) {
  tab_data_.group_color = color;
  tab_data_.is_in_group = (color != SK_ColorTRANSPARENT);
  UpdateGroupColorStrip();
}

// =========================================================================
// Public API — display index
// =========================================================================

void AstraTabSearchItemView::SetDisplayIndex(int index) {
  if (display_index_ == index) {
    return;
  }
  display_index_ = index;
  UpdateShortcutHint();
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraTabSearchItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraTabSearchItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateTextColors();
  UpdateBackground();
  UpdateFaviconAppearance();
  UpdateGroupColorStrip();
  UpdateAudioIndicator();
  UpdateWorkspaceLabel();
}

bool AstraTabSearchItemView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyMiddleMouseButton()) {
    if (middle_click_callback_) {
      middle_click_callback_.Run();
    }
    return true;
  }

  if (event.IsOnlyLeftMouseButton()) {
    RequestFocus();
    if (activated_callback_) {
      activated_callback_.Run();
    }
    return true;
  }

  return views::View::OnMousePressed(event);
}

bool AstraTabSearchItemView::OnMouseDragged(const ui::MouseEvent& event) {
  // Start a drag operation could be implemented here.
  // For now, just consume the event.
  return true;
}

void AstraTabSearchItemView::OnMouseEntered(const ui::MouseEvent& event) {
  SetHighlighted(true);
}

void AstraTabSearchItemView::OnMouseExited(const ui::MouseEvent& event) {
  SetHighlighted(false);
}

bool AstraTabSearchItemView::OnKeyPressed(const ui::KeyEvent& event) {
  if (event.key_code() == ui::VKEY_RETURN ||
      event.key_code() == ui::VKEY_SPACE) {
    if (activated_callback_) {
      activated_callback_.Run();
    }
    return true;
  }

  if (event.key_code() == ui::VKEY_DELETE ||
      event.key_code() == ui::VKEY_BACK) {
    if (close_callback_) {
      close_callback_.Run();
    }
    return true;
  }

  return views::View::OnKeyPressed(event);
}

void AstraTabSearchItemView::OnFocus() {
  views::View::OnFocus();
  focused_ = true;
  SetSelected(true);
  NotifyAccessibilityEvent(ax::mojom::Event::kFocus, true);
}

void AstraTabSearchItemView::OnBlur() {
  views::View::OnBlur();
  focused_ = false;
  // Don't unselect on blur — selection is managed by the bubble.
  // The bubble controls selection state; focus is just visual.
}

void AstraTabSearchItemView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kListItem;

  // Accessible name: "Tab title - example.com"
  std::u16string name = tab_data_.title;
  if (!tab_data_.hostname.empty()) {
    name += u" - " + tab_data_.hostname;
  }
  node_data->SetName(name);

  // Description: result type + group category + workspace + audio + pinned.
  std::u16string description = GetResultTypeLabel();
  if (tab_data_.is_in_group && !tab_data_.group_name.empty()) {
    description += u" - " + tab_data_.group_name;
  }
  if (!tab_data_.workspace_name.empty()) {
    description += u" - " + tab_data_.workspace_name;
  }
  if (tab_data_.is_audible) {
    description += tab_data_.is_muted ? u" - Muted" : u" - Audio";
  }
  if (tab_data_.is_pinned) {
    description += u" - Pinned";
  }
  if (tab_data_.visit_count > 0) {
    description += u" - " + base::NumberToString16(tab_data_.visit_count) +
                    u" visits";
  }
  node_data->SetDescription(description);

  // Position info (1-based).
  if (display_index_ > 0) {
    node_data->AddIntAttribute(ax::mojom::IntAttribute::kPosInSet,
                                display_index_);
  }

  node_data->AddState(ax::mojom::State::kSelectable);
  if (selected_) {
    node_data->AddState(ax::mojom::State::kSelected);
  }
  if (tab_data_.is_audible && !tab_data_.is_muted) {
    node_data->AddState(ax::mojom::State::kBusy);
  }

  // Value: URL for accessibility tools.
  if (tab_data_.url.is_valid()) {
    node_data->SetValue(base::UTF8ToUTF16(tab_data_.url.spec()));
  }
}

bool AstraTabSearchItemView::OnGestureEvent(ui::GestureEvent* event) {
  switch (event->type()) {
    case ui::ET_GESTURE_TAP:
      if (activated_callback_) {
        activated_callback_.Run();
      }
      event->SetHandled();
      return true;

    case ui::ET_GESTURE_LONG_PRESS:
      // Could show a context menu here.
      event->SetHandled();
      return true;

    default:
      break;
  }

  return views::View::OnGestureEvent(event);
}

// =========================================================================
// Private helpers — text updates
// =========================================================================

void AstraTabSearchItemView::UpdateTextFromTab() {
  if (title_label_) {
    title_label_->SetText(tab_data_.title);
  }
  if (host_label_) {
    host_label_->SetText(tab_data_.hostname);
  }
}

// =========================================================================
// Private helpers — visual updates
// =========================================================================

void AstraTabSearchItemView::UpdateBackground() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider || !layer()) {
    return;
  }

  SkColor color = SK_ColorTRANSPARENT;
  if (selected_) {
    color = color_provider->GetColor(kItemSelectedBackground);
  } else if (highlighted_) {
    color = color_provider->GetColor(kItemHoverBackground);
  }
  layer()->SetColor(color);
}

void AstraTabSearchItemView::UpdateCloseButtonVisibility() {
  if (!close_button_) {
    return;
  }
  // Show close button when hovered, selected, or explicitly forced visible.
  bool should_show =
      show_close_button_ || highlighted_ || selected_;
  close_button_->SetVisible(should_show);
}

void AstraTabSearchItemView::UpdateShortcutHint() {
  if (!shortcut_label_) {
    return;
  }

  if (!show_shortcut_hint_) {
    shortcut_label_->SetVisible(false);
    return;
  }

  // Show "Ctrl+N" for the first 9 results.
  if (display_index_ > 0 && display_index_ <= 9) {
    // TODO(astra): Use platform-appropriate modifier key (Ctrl on Linux/Win,
    //   Cmd on Mac).  For now we hardcode "Ctrl+" as a placeholder.
    //   Chromium component: ui/base/accelerators/accelerator.h
    shortcut_label_->SetText(
        u"Ctrl+" + base::ASCIIToUTF16(base::NumberToString(display_index_)));
    shortcut_label_->SetVisible(true);
  } else {
    shortcut_label_->SetVisible(false);
  }
}

void AstraTabSearchItemView::UpdateTextColors() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (title_label_) {
    title_label_->SetEnabledColor(color_provider->GetColor(kTitleTextColor));
  }
  if (host_label_) {
    host_label_->SetEnabledColor(color_provider->GetColor(kHostTextColor));
  }
  if (shortcut_label_) {
    shortcut_label_->SetEnabledColor(
        color_provider->GetColor(kShortcutTextColor));
  }
  if (workspace_label_) {
    workspace_label_->SetEnabledColor(
        color_provider->GetColor(kWorkspaceTextColor));
  }
}

void AstraTabSearchItemView::UpdateFaviconAppearance() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider || !favicon_placeholder_) {
    return;
  }

  // Use different colors for different result types as a visual indicator.
  // TODO(astra): Replace with real favicons.
  SkColor color;
  switch (tab_data_.result_type) {
    case AstraTabSearchResultType::kOpenTab:
      color = color_provider->GetColor(kFaviconPlaceholderColor);
      break;
    case AstraTabSearchResultType::kRecentlyClosed:
      color = color_provider->GetColor(ui::kColorIconSecondary);
      break;
    case AstraTabSearchResultType::kBookmark:
      color = color_provider->GetColor(kColorAstraWorkspaceAccent);
      break;
    case AstraTabSearchResultType::kHistory:
      color = color_provider->GetColor(kFaviconPlaceholderColor);
      break;
    default:
      color = color_provider->GetColor(kFaviconPlaceholderColor);
      break;
  }
  favicon_placeholder_->layer()->SetColor(color);
  favicon_placeholder_->SetVisible(show_favicon_);
}

void AstraTabSearchItemView::UpdateGroupColorStrip() {
  if (!group_color_strip_) {
    return;
  }

  SkColor color = tab_data_.is_in_group
                      ? tab_data_.group_color
                      : SK_ColorTRANSPARENT;
  group_color_strip_->layer()->SetColor(color);
}

void AstraTabSearchItemView::UpdateAudioIndicator() {
  if (!audio_indicator_) {
    return;
  }

  bool visible = show_audio_indicator_ && tab_data_.is_audible;
  audio_indicator_->SetVisible(visible);

  if (visible) {
    const auto* color_provider = GetColorProvider();
    if (color_provider) {
      SkColor color = tab_data_.is_muted
                          ? color_provider->GetColor(ui::kColorIconSecondary)
                          : color_provider->GetColor(kAudioIndicatorColor);
      audio_indicator_->layer()->SetColor(color);
    }
    audio_indicator_->SetTooltipText(
        tab_data_.is_muted ? u"Muted" : u"Audio playing");
    audio_indicator_->SetAccessibleName(
        tab_data_.is_muted ? u"Muted tab" : u"Tab playing audio");
  }
}

void AstraTabSearchItemView::UpdateWorkspaceLabel() {
  if (!workspace_label_) {
    return;
  }

  bool show = show_workspace_ && !tab_data_.workspace_name.empty();
  workspace_label_->SetVisible(show);

  if (show) {
    workspace_label_->SetText(tab_data_.workspace_name);
    const auto* color_provider = GetColorProvider();
    if (color_provider) {
      workspace_label_->SetEnabledColor(
          color_provider->GetColor(kWorkspaceTextColor));
      workspace_label_->layer()->SetColor(
          color_provider->GetColor(kWorkspaceBackground));
    }
  }
}

void AstraTabSearchItemView::UpdatePinnedIndicator() {
  if (!pinned_indicator_) {
    return;
  }

  pinned_indicator_->SetVisible(tab_data_.is_pinned);
  if (tab_data_.is_pinned) {
    const auto* color_provider = GetColorProvider();
    if (color_provider) {
      pinned_indicator_->layer()->SetColor(
          color_provider->GetColor(kFaviconPlaceholderColor));
    }
  }
}

void AstraTabSearchItemView::UpdateSiteInfo() {
  if (!host_label_) {
    return;
  }

  host_label_->SetVisible(show_site_info_ && !tab_data_.hostname.empty());
}

std::u16string AstraTabSearchItemView::GetResultTypeLabel() const {
  return GetResultTypeName(tab_data_.result_type);
}

}  // namespace astra
