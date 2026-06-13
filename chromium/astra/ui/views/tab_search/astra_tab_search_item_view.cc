#include "astra/ui/views/tab_search/astra_tab_search_item_view.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/include/core/SkColor.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"

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

// Astra color IDs for tab search items.
// TODO(astra): Define dedicated tab-search color IDs in astra_color_ids.h
//   instead of reusing command palette / sidebar colors.
constexpr ui::ColorId kItemSelectedBackground =
    kColorAstraCommandPaletteSelectedBackground;
constexpr ui::ColorId kItemHoverBackground =
    kColorAstraSidebarItemHoverBackground;
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

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraTabSearchItemView::AstraTabSearchItemView(
    const AstraTabSearchItem& tab)
    : tab_data_(tab),
      display_index_(0) {
  // Map group for legacy compatibility.
  // TODO(astra): Remove legacy_group_ once bubble is fully migrated.
  legacy_group_ = Group::kOpenTabs;
  legacy_identifier_ = base::NumberToString(tab.tab_id);

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
  tab_data_.tab_id = tab_info.tab_index;  // Use tab_index as ID for legacy.

  legacy_group_ = tab_info.group;
  legacy_identifier_ = tab_info.identifier;

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
  if (activated_callback_) {
    activated_callback_.Run();
  }
  return true;
}

void AstraTabSearchItemView::OnMouseEntered(const ui::MouseEvent& event) {
  SetHighlighted(true);
}

void AstraTabSearchItemView::OnMouseExited(const ui::MouseEvent& event) {
  SetHighlighted(false);
}

void AstraTabSearchItemView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kListItem;

  // Accessible name: "Tab title - example.com"
  std::u16string name = tab_data_.title;
  if (!tab_data_.hostname.empty()) {
    name += u" - " + tab_data_.hostname;
  }
  node_data->SetName(name);

  // Description: group category + workspace.
  std::u16string description = GetGroupName(legacy_group_);
  if (!tab_data_.workspace_name.empty()) {
    description += u" - " + tab_data_.workspace_name;
  }
  if (tab_data_.is_audible) {
    description += u" - Audio";
  }
  if (tab_data_.is_pinned) {
    description += u" - Pinned";
  }
  node_data->SetDescription(description);

  node_data->AddState(ax::mojom::State::kSelectable);
  if (selected_) {
    node_data->AddState(ax::mojom::State::kSelected);
  }
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

  // Use different colors for different groups as a visual indicator.
  // TODO(astra): Replace with real favicons.
  SkColor color;
  switch (legacy_group_) {
    case Group::kOpenTabs:
      color = color_provider->GetColor(kFaviconPlaceholderColor);
      break;
    case Group::kRecentlyClosed:
      color = color_provider->GetColor(kFaviconPlaceholderColor);
      break;
    case Group::kBookmarks:
      color = color_provider->GetColor(kColorAstraWorkspaceAccent);
      break;
  }
  favicon_placeholder_->layer()->SetColor(color);
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

}  // namespace astra
