#include "astra/ui/views/command_palette/astra_command_palette_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

#include "astra/browser/astra_command_delegate.h"
#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/command_palette/astra_command_palette_item_view.h"
#include "astra/ui/views/command_palette/astra_command_palette_section_header_view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kPaletteWidth = 520;
constexpr int kSearchFieldHeight = 48;
constexpr int kSearchFieldHorizontalPadding = 16;
constexpr int kSearchFieldVerticalPadding = 8;
constexpr int kCategoryChipsHeight = 36;
constexpr int kCategoryChipSpacing = 8;
constexpr int kCategoryChipPaddingH = 12;
constexpr int kCategoryChipPaddingV = 4;
constexpr int kQuickActionsHeight = 32;
constexpr int kStatusBarHeight = 28;
constexpr int kStatusBarHorizontalPadding = 16;
constexpr int kResultsMaxHeight = 400;
constexpr int kDividerThickness = 1;

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraCommandPaletteView::AstraCommandPaletteView(Browser* browser)
    : browser_(browser) {
  DCHECK(browser_);

  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(true);

  // Create the default-owned model.
  model_ = new AstraCommandPaletteModel();
  model_owned_ = true;

  // Observe the model for changes.
  model_->AddObserver(
      static_cast<AstraCommandPaletteObserver*>(this));
  model_->AddObserver(
      static_cast<AstraCommandPaletteModelObserver*>(this));

  BuildLayout();
}

AstraCommandPaletteView::~AstraCommandPaletteView() {
  // Stop observing the model before it's destroyed.
  if (model_) {
    model_->RemoveObserver(
        static_cast<AstraCommandPaletteObserver*>(this));
    model_->RemoveObserver(
        static_cast<AstraCommandPaletteModelObserver*>(this));
  }

  // Delete the model only if we own it.
  if (model_owned_ && model_) {
    delete model_;
    model_ = nullptr;
  }
}

// =========================================================================
// Delegate
// =========================================================================

void AstraCommandPaletteView::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

// =========================================================================
// Model
// =========================================================================

void AstraCommandPaletteView::SetModel(AstraCommandPaletteModel* model) {
  if (model_ == model) {
    return;
  }

  // Remove observers from old model.
  if (model_) {
    model_->RemoveObserver(
        static_cast<AstraCommandPaletteObserver*>(this));
    model_->RemoveObserver(
        static_cast<AstraCommandPaletteModelObserver*>(this));
  }

  // Delete owned model if we had one.
  if (model_owned_ && model_) {
    delete model_;
    model_owned_ = false;
  }

  model_ = model;
  model_owned_ = false;

  // Add observers to new model.
  if (model_) {
    model_->AddObserver(
        static_cast<AstraCommandPaletteObserver*>(this));
    model_->AddObserver(
        static_cast<AstraCommandPaletteModelObserver*>(this));
  }

  // Rebuild the results list with the new model.
  RebuildResultsList();
  UpdateSelectionVisual();
  UpdateStatusBar();
}

// =========================================================================
// Search
// =========================================================================

void AstraCommandPaletteView::RequestSearchFocus() {
  if (search_field_) {
    search_field_->RequestFocus();
    // Select all text so the user can immediately start typing.
    search_field_->SelectAll(false);
  }
}

void AstraCommandPaletteView::SetQuery(const std::u16string& query) {
  if (search_field_) {
    search_field_->SetText(query);
  }
  // ContentsChanged callback will update the model.
  if (model_) {
    model_->SetQuery(query);
  }
}

std::u16string AstraCommandPaletteView::GetQuery() const {
  if (search_field_) {
    return search_field_->GetText();
  }
  if (model_) {
    return model_->query();
  }
  return std::u16string();
}

void AstraCommandPaletteView::ClearSearch() {
  if (search_field_) {
    search_field_->SetText(std::u16string());
  }
  // The model will update via ContentsChanged callback.
  if (model_) {
    model_->SetQuery(std::u16string());
  }
}

// =========================================================================
// Selection
// =========================================================================

int AstraCommandPaletteView::GetSelectedIndex() const {
  if (!model_) {
    return -1;
  }
  return model_->GetSelectedIndex();
}

void AstraCommandPaletteView::SelectNext() {
  if (model_) {
    model_->MoveSelection(1);
  }
  ScrollSelectedIntoView();
}

void AstraCommandPaletteView::SelectPrevious() {
  if (model_) {
    model_->MoveSelection(-1);
  }
  ScrollSelectedIntoView();
}

void AstraCommandPaletteView::SelectFirst() {
  if (model_ && model_->GetResultCount() > 0) {
    model_->SetSelectedIndex(0);
    ScrollSelectedIntoView();
  }
}

void AstraCommandPaletteView::SelectLast() {
  if (model_ && model_->GetResultCount() > 0) {
    model_->SetSelectedIndex(
        static_cast<int>(model_->GetResultCount()) - 1);
    ScrollSelectedIntoView();
  }
}

// =========================================================================
// Execution
// =========================================================================

void AstraCommandPaletteView::ExecuteSelected() {
  if (!model_) {
    return;
  }
  int selected = model_->GetSelectedIndex();
  if (selected >= 0) {
    model_->ExecuteCommand(selected);
  }
}

// =========================================================================
// Results
// =========================================================================

size_t AstraCommandPaletteView::GetResultCount() const {
  return item_views_.size();
}

AstraCommandPaletteItemView* AstraCommandPaletteView::GetResultViewAt(
    int index) const {
  if (index < 0 || index >= static_cast<int>(item_views_.size())) {
    return nullptr;
  }
  return item_views_[index];
}

void AstraCommandPaletteView::ScrollToIndex(int index) {
  if (index < 0 || index >= static_cast<int>(item_views_.size())) {
    return;
  }
  auto* item_view = item_views_[index];
  if (!item_view || !scroll_view_) {
    return;
  }
  scroll_view_->ScrollViewToVisible(item_view);
}

// =========================================================================
// Layout construction
// =========================================================================

void AstraCommandPaletteView::BuildLayout() {
  // Vertical box layout: search field, divider, results, divider, status bar.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // 1. Search textfield.
  search_field_ = AddChildView(std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"Type a command…");
  search_field_->SetBackgroundColor(kColorAstraCommandPaletteBackground);
  search_field_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  search_field_->set_controller(this);
  search_field_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kSearchFieldVerticalPadding,
                      kSearchFieldHorizontalPadding)));
  search_field_->SetPreferredSize(
      gfx::Size(kPaletteWidth, kSearchFieldHeight));
  // TODO(astra): Set a proper text style / font size for the search field.
  // Chromium component: ui/views/controls/textfield/textfield.h
  // Consider using views::StyledLabel or a custom text style.

  // Divider between search field and category chips.
  divider_top_ = AddChildView(std::make_unique<views::View>());
  divider_top_->SetPaintToLayer();
  divider_top_->layer()->SetFillsBoundsOpaquely(true);
  divider_top_->SetPreferredSize(gfx::Size(0, kDividerThickness));

  // Category filter chips (horizontal scroll).
  chips_scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  chips_scroll_view_->SetBackgroundColor(kColorAstraCommandPaletteBackground);
  chips_scroll_view_->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kHiddenButEnabled);
  chips_scroll_view_->ClipHeightTo(kCategoryChipsHeight, kCategoryChipsHeight);

  chips_container_ = chips_scroll_view_->SetContents(
      std::make_unique<views::View>());
  chips_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(4, kSearchFieldHorizontalPadding), kCategoryChipSpacing));
  chips_container_->SetPaintToLayer();
  chips_container_->layer()->SetFillsBoundsOpaquely(false);

  BuildCategoryChips();

  // 2. Scrollable results area.
  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetBackgroundColor(kColorAstraCommandPaletteBackground);
  scroll_view_->ClipHeightTo(0, kResultsMaxHeight);
  layout->SetFlexForView(scroll_view_, 1);

  // Results container inside the scroll view.
  results_container_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  results_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  results_container_->SetPaintToLayer();
  results_container_->layer()->SetFillsBoundsOpaquely(false);

  // Divider between results and quick actions / status bar.
  divider_bottom_ = AddChildView(std::make_unique<views::View>());
  divider_bottom_->SetPaintToLayer();
  divider_bottom_->layer()->SetFillsBoundsOpaquely(true);
  divider_bottom_->SetPreferredSize(gfx::Size(0, kDividerThickness));

  // Quick actions panel (bottom row of action shortcuts).
  quick_actions_panel_ = AddChildView(std::make_unique<views::View>());
  quick_actions_panel_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(4, kStatusBarHorizontalPadding), 16));
  quick_actions_panel_->SetPaintToLayer();
  quick_actions_panel_->layer()->SetFillsBoundsOpaquely(false);
  quick_actions_panel_->SetPreferredSize(
      gfx::Size(kPaletteWidth, kQuickActionsHeight));

  BuildQuickActionsPanel();

  // 3. Status bar at the bottom.
  status_bar_label_ = AddChildView(std::make_unique<views::Label>());
  status_bar_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_bar_label_->SetAutoColorReadabilityEnabled(false);
  status_bar_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kStatusBarHorizontalPadding)));
  status_bar_label_->SetElideBehavior(gfx::ELIDE_END);
  status_bar_label_->SetFontList(
      views::Label::GetDefaultFontList().Derive(
          -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
  status_bar_label_->SetPreferredSize(
      gfx::Size(kPaletteWidth, kStatusBarHeight));

  // Initial population — empty query shows recommended + recent commands.
  RebuildResultsList();
  UpdateSelectionVisual();
  UpdateStatusBar();
}

// =========================================================================
// Results list management
// =========================================================================

void AstraCommandPaletteView::RebuildResultsList() {
  if (!model_) {
    results_container_->RemoveAllChildViews();
    item_views_.clear();
    empty_state_view_ = nullptr;
    empty_state_label_ = nullptr;
    InvalidateLayout();
    return;
  }

  const auto& groups = model_->GetResultGroups();

  // Clear existing items.
  // TODO(astra): Use a more efficient update strategy — only add/remove
  // changed items instead of rebuilding everything each keystroke.
  results_container_->RemoveAllChildViews();
  item_views_.clear();
  empty_state_view_ = nullptr;
  empty_state_label_ = nullptr;

  if (groups.empty()) {
    // Show empty state.
    empty_state_view_ = results_container_->AddChildView(
        std::make_unique<views::View>());
    empty_state_view_->SetLayoutManager(
        std::make_unique<views::BoxLayout>(
            views::BoxLayout::Orientation::kVertical,
            gfx::Insets::VH(40, 16), 0));
    static_cast<views::BoxLayout*>(empty_state_view_->GetLayoutManager())
        ->set_main_axis_alignment(
            views::BoxLayout::MainAxisAlignment::kCenter);
    static_cast<views::BoxLayout*>(empty_state_view_->GetLayoutManager())
        ->set_cross_axis_alignment(
            views::BoxLayout::CrossAxisAlignment::kCenter);
    empty_state_view_->SetPaintToLayer();
    empty_state_view_->layer()->SetFillsBoundsOpaquely(false);

    // Empty state icon / large symbol.
    auto* icon_label = empty_state_view_->AddChildView(
        std::make_unique<views::Label>(u"🔍"));
    icon_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    icon_label->SetAutoColorReadabilityEnabled(false);
    icon_label->SetFontList(views::Label::GetDefaultFontList().Derive(
        8, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));

    // Empty state message.
    empty_state_label_ = empty_state_view_->AddChildView(
        std::make_unique<views::Label>(u"No commands found"));
    empty_state_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    empty_state_label_->SetAutoColorReadabilityEnabled(false);
    empty_state_label_->SetFontList(views::Label::GetDefaultFontList().Derive(
        0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::MEDIUM));

    // Empty state sub-message.
    auto* sub_label = empty_state_view_->AddChildView(
        std::make_unique<views::Label>(
            u"Try a different search term"));
    sub_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    sub_label->SetAutoColorReadabilityEnabled(false);
    sub_label->SetFontList(views::Label::GetDefaultFontList().Derive(
        -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));

    UpdateEmptyStateColors();

    InvalidateLayout();
    return;
  }

  // Build the results list with category section headers.
  for (const auto& group : groups) {
    // Add a section header for this category.
    results_container_->AddChildView(
        std::make_unique<AstraCommandPaletteSectionHeaderView>(
            GetCategoryLabel(group.category)));

    // Add all command items in this group.
    for (const auto& item : group.items) {
      auto* item_view = results_container_->AddChildView(
          std::make_unique<AstraCommandPaletteItemView>(item));

      // Apply presentation settings from the model.
      item_view->ShowDescription(model_->show_descriptions());
      item_view->ShowShortcut(model_->show_shortcuts());

      // Compute match ranges for highlighting.
      if (!model_->query().empty()) {
        auto ranges = AstraCommandPaletteModel::GetMatchRanges(
            model_->query(), item.title);
        item_view->SetMatchRanges(ranges);
      }

      // Click handler — execute command via model.
      // We capture the current size of item_views_ as the index; since
      // the list is rebuilt on every model change, the index stays valid
      // for the lifetime of this item view.
      int item_index = static_cast<int>(item_views_.size());
      item_view->SetActivatedCallback(base::BindRepeating(
          [](base::WeakPtr<AstraCommandPaletteView> weak_this, int index) {
            if (weak_this && weak_this->model_) {
              weak_this->model_->ExecuteCommand(index);
            }
          },
          weak_ptr_factory_.GetWeakPtr(), item_index));

      item_views_.push_back(item_view);
    }
  }

  InvalidateLayout();
}

void AstraCommandPaletteView::UpdateSelectionVisual() {
  if (!model_) {
    return;
  }
  int selected = model_->GetSelectedIndex();
  for (size_t i = 0; i < item_views_.size(); ++i) {
    item_views_[i]->SetSelected(static_cast<int>(i) == selected);
  }
}

void AstraCommandPaletteView::UpdateStatusBar() {
  if (!model_ || !status_bar_label_) {
    return;
  }

  size_t count = model_->GetResultCount();
  std::u16string status_text;

  if (model_->query().empty()) {
    if (count == 0) {
      status_text = u"No commands";
    } else if (count == 1) {
      status_text = u"1 recommended command";
    } else {
      status_text = base::NumberToString16(count) + u" recommended commands";
    }
  } else {
    if (count == 0) {
      status_text = u"No matching commands";
    } else if (count == 1) {
      status_text = u"1 result";
    } else {
      status_text = base::NumberToString16(count) + u" results";
    }
  }

  // Add navigation hint.
  if (count > 0) {
    status_text += u" · ↑↓ navigate";
  }

  status_bar_label_->SetText(status_text);
}

void AstraCommandPaletteView::ScrollSelectedIntoView() {
  if (!model_) {
    return;
  }
  int selected = model_->GetSelectedIndex();
  ScrollToIndex(selected);
}

// =========================================================================
// Category filter chips
// =========================================================================

void AstraCommandPaletteView::BuildCategoryChips() {
  // "All" chip (index 0).
  auto* all_chip = chips_container_->AddChildView(std::make_unique<views::Label>(u"All"));
  all_chip->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  all_chip->SetAutoColorReadabilityEnabled(false);
  all_chip->SetFontList(views::Label::GetDefaultFontList().Derive(
      -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::MEDIUM));
  all_chip->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kCategoryChipPaddingV, kCategoryChipPaddingH)));
  all_chip->SetPaintToLayer();
  all_chip->layer()->SetFillsBoundsOpaquely(false);

  // One chip per category.
  std::vector<AstraCommandCategory> categories = {
      AstraCommandCategory::kTabs,
      AstraCommandCategory::kBookmarks,
      AstraCommandCategory::kHistory,
      AstraCommandCategory::kActions,
      AstraCommandCategory::kWorkspaces,
      AstraCommandCategory::kView,
      AstraCommandCategory::kTools,
      AstraCommandCategory::kSettings,
      AstraCommandCategory::kHelp,
  };

  for (auto cat : categories) {
    auto* chip = chips_container_->AddChildView(
        std::make_unique<views::Label>(GetCategoryLabel(cat)));
    chip->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    chip->SetAutoColorReadabilityEnabled(false);
    chip->SetFontList(views::Label::GetDefaultFontList().Derive(
        -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::MEDIUM));
    chip->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(kCategoryChipPaddingV, kCategoryChipPaddingH)));
    chip->SetPaintToLayer();
    chip->layer()->SetFillsBoundsOpaquely(false);
  }
}

size_t AstraCommandPaletteView::GetCategoryChipCount() const {
  if (!chips_container_) {
    return 0;
  }
  return chips_container_->children().size();
}

void AstraCommandPaletteView::SelectCategoryChip(int chip_index) {
  if (chip_index < 0 ||
      chip_index >= static_cast<int>(GetCategoryChipCount())) {
    return;
  }

  selected_chip_index_ = chip_index;

  if (!model_) {
    UpdateCategoryChipsVisual();
    return;
  }

  if (chip_index == 0) {
    // "All" selected — clear the category filter.
    model_->ClearCategoryFilter();
  } else {
    // Map chip index to category (index 0 = "All", so category chips start at 1).
    std::vector<AstraCommandCategory> categories = {
        AstraCommandCategory::kTabs,
        AstraCommandCategory::kBookmarks,
        AstraCommandCategory::kHistory,
        AstraCommandCategory::kActions,
        AstraCommandCategory::kWorkspaces,
        AstraCommandCategory::kView,
        AstraCommandCategory::kTools,
        AstraCommandCategory::kSettings,
        AstraCommandCategory::kHelp,
    };
    int cat_index = chip_index - 1;
    if (cat_index >= 0 && cat_index < static_cast<int>(categories.size())) {
      std::set<AstraCommandCategory> filter = {categories[cat_index]};
      model_->SetCategoryFilter(filter);
    }
  }

  UpdateCategoryChipsVisual();
}

int AstraCommandPaletteView::GetSelectedCategoryChipIndex() const {
  return selected_chip_index_;
}

void AstraCommandPaletteView::UpdateCategoryChipsVisual() {
  if (!chips_container_) {
    return;
  }

  const auto& children = chips_container_->children();
  for (size_t i = 0; i < children.size(); ++i) {
    auto* chip_label = static_cast<views::Label*>(children[i]);
    bool selected = static_cast<int>(i) == selected_chip_index_;
    // TODO(astra): Use proper background styling for selected chips.
    // Chromium component: views::LabelButton or views::MdTextButton.
    // For now we use font weight to indicate selection.
    chip_label->SetFontList(views::Label::GetDefaultFontList().Derive(
        -1, gfx::Font::FontStyle::NORMAL,
        selected ? gfx::Font::Weight::BOLD : gfx::Font::Weight::MEDIUM));
  }
}

// =========================================================================
// Group access
// =========================================================================

size_t AstraCommandPaletteView::GetGroupCount() const {
  if (!model_) {
    return 0;
  }
  return model_->GetResultGroups().size();
}

AstraCommandCategory AstraCommandPaletteView::GetGroupCategoryAt(
    int group_index) const {
  if (!model_) {
    return AstraCommandCategory::kTools;
  }
  const auto& groups = model_->GetResultGroups();
  if (group_index < 0 ||
      group_index >= static_cast<int>(groups.size())) {
    return AstraCommandCategory::kTools;
  }
  return groups[group_index].category;
}

// =========================================================================
// Empty state
// =========================================================================

bool AstraCommandPaletteView::IsEmptyStateVisible() const {
  if (!empty_state_view_) {
    return false;
  }
  return empty_state_view_->GetVisible();
}

std::u16string AstraCommandPaletteView::GetEmptyStateMessage() const {
  if (!empty_state_label_) {
    return std::u16string();
  }
  return empty_state_label_->GetText();
}

void AstraCommandPaletteView::UpdateEmptyState() {
  if (!empty_state_view_ || !model_) {
    return;
  }

  bool has_results = model_->GetResultCount() > 0;
  bool is_searching = !model_->query().empty();

  empty_state_view_->SetVisible(!has_results);

  if (!has_results) {
    if (is_searching) {
      empty_state_label_->SetText(u"No commands found");
    } else {
      empty_state_label_->SetText(u"No commands available");
    }
  }
}

// =========================================================================
// Quick actions panel
// =========================================================================

bool AstraCommandPaletteView::IsQuickActionsPanelVisible() const {
  if (!quick_actions_panel_) {
    return false;
  }
  return quick_actions_panel_->GetVisible();
}

void AstraCommandPaletteView::SetQuickActionsVisible(bool visible) {
  if (quick_actions_panel_ && quick_actions_panel_->GetVisible() != visible) {
    quick_actions_panel_->SetVisible(visible);
    quick_actions_visible_ = visible;
    InvalidateLayout();
  }
}

void AstraCommandPaletteView::BuildQuickActionsPanel() {
  // Quick action: "Clear recent" (left-aligned).
  auto* clear_label = quick_actions_panel_->AddChildView(
      std::make_unique<views::Label>(u"Clear recent"));
  clear_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  clear_label->SetAutoColorReadabilityEnabled(false);
  clear_label->SetFontList(views::Label::GetDefaultFontList().Derive(
      -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));

  // Spacer (flex).
  auto* spacer = quick_actions_panel_->AddChildView(std::make_unique<views::View>());
  spacer->SetPaintToLayer();
  spacer->layer()->SetFillsBoundsOpaquely(false);
  static_cast<views::BoxLayout*>(quick_actions_panel_->GetLayoutManager())
      ->SetFlexForView(spacer, 1);

  // Quick action: "Settings" (right-aligned).
  auto* settings_label = quick_actions_panel_->AddChildView(
      std::make_unique<views::Label>(u"⌘, Settings"));
  settings_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  settings_label->SetAutoColorReadabilityEnabled(false);
  settings_label->SetFontList(views::Label::GetDefaultFontList().Derive(
      -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
}

// =========================================================================
// Theme & colors
// =========================================================================

void AstraCommandPaletteView::UpdateEmptyStateColors() {
  const auto* color_provider = GetColorProvider();
  if (!color_provider || !empty_state_label_) {
    return;
  }
  empty_state_label_->SetEnabledColor(
      color_provider->GetColor(kColorAstraCommandPaletteText));

  // Also update the sub-label if present (second child of empty_state_view_).
  if (empty_state_view_ && empty_state_view_->children().size() >= 3) {
    auto* sub_label = static_cast<views::Label*>(
        empty_state_view_->children()[2]);
    sub_label->SetEnabledColor(
        color_provider->GetColor(kColorAstraCommandPaletteDescriptionText));
  }
}

// =========================================================================
// views::View overrides
// =========================================================================

void AstraCommandPaletteView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (layer()) {
    layer()->SetColor(
        color_provider->GetColor(kColorAstraCommandPaletteBackground));
  }

  // Update search field colors.
  if (search_field_) {
    search_field_->SetBackgroundColor(
        color_provider->GetColor(kColorAstraCommandPaletteBackground));
    search_field_->SetColor(
        color_provider->GetColor(kColorAstraCommandPaletteSearchText));
    // Update placeholder text color — use a secondary / muted color.
    // TODO(astra): Add a placeholder text color to Astra color IDs.
  }

  // Update status bar label color.
  if (status_bar_label_) {
    status_bar_label_->SetEnabledColor(
        color_provider->GetColor(kColorAstraCommandPaletteDescriptionText));
  }

  // Update divider colors.
  if (divider_top_ && divider_top_->layer()) {
    divider_top_->layer()->SetColor(
        color_provider->GetColor(kColorAstraCommandPaletteBorder));
  }
  if (divider_bottom_ && divider_bottom_->layer()) {
    divider_bottom_->layer()->SetColor(
        color_provider->GetColor(kColorAstraCommandPaletteBorder));
  }

  // Update scroll view background.
  if (scroll_view_) {
    scroll_view_->SetBackgroundColor(
        color_provider->GetColor(kColorAstraCommandPaletteBackground));
  }

  // Update category chips colors.
  if (chips_container_) {
    for (auto* child : chips_container_->children()) {
      auto* label = static_cast<views::Label*>(child);
      label->SetEnabledColor(
          color_provider->GetColor(kColorAstraCommandPaletteText));
    }
  }

  // Update quick actions panel colors.
  if (quick_actions_panel_) {
    for (auto* child : quick_actions_panel_->children()) {
      if (child->GetClassName() == views::Label::kViewClassName) {
        auto* label = static_cast<views::Label*>(child);
        label->SetEnabledColor(
            color_provider->GetColor(kColorAstraCommandPaletteDescriptionText));
      }
    }
  }

  // Update empty state colors.
  UpdateEmptyStateColors();

  // Refresh all item views.
  for (auto* item_view : item_views_) {
    item_view->OnThemeChanged();
  }
}

bool AstraCommandPaletteView::OnKeyPressed(const ui::KeyEvent& event) {
  // Handle keyboard events when focus is on the view itself
  // (e.g. when results container has focus).
  if (event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }

  // Command/Ctrl+number shortcuts for category chips.
  if (event.IsControlDown() || event.IsCommandDown()) {
    if (event.key_code() >= ui::VKEY_1 && event.key_code() <= ui::VKEY_9) {
      int chip_index = event.key_code() - ui::VKEY_1;
      if (chip_index < static_cast<int>(GetCategoryChipCount())) {
        SelectCategoryChip(chip_index);
        return true;
      }
    }

    // Cmd/Ctrl+Down = next group.
    if (event.key_code() == ui::VKEY_DOWN && model_) {
      model_->SelectNextGroup();
      ScrollSelectedIntoView();
      return true;
    }

    // Cmd/Ctrl+Up = previous group.
    if (event.key_code() == ui::VKEY_UP && model_) {
      model_->SelectPrevGroup();
      ScrollSelectedIntoView();
      return true;
    }
  }

  switch (event.key_code()) {
    case ui::VKEY_UP:
      SelectPrevious();
      return true;

    case ui::VKEY_DOWN:
      SelectNext();
      return true;

    case ui::VKEY_RETURN:
      ExecuteSelected();
      return true;

    case ui::VKEY_ESCAPE:
      if (delegate_) {
        delegate_->OnCommandPaletteClose();
      }
      return true;

    case ui::VKEY_TAB:
      // Tab moves focus back to search field.
      if (!event.IsShiftDown()) {
        RequestSearchFocus();
        return true;
      }
      // Shift+Tab also goes to search field (cycle).
      RequestSearchFocus();
      return true;

    case ui::VKEY_HOME:
      SelectFirst();
      return true;

    case ui::VKEY_END:
      SelectLast();
      return true;

    default:
      return false;
  }
}

// =========================================================================
// AstraCommandPaletteObserver implementation
// =========================================================================

void AstraCommandPaletteView::OnCommandListChanged(
    AstraCommandPaletteModel* model) {
  DCHECK_EQ(model, model_);
  RebuildResultsList();
  UpdateSelectionVisual();
  UpdateStatusBar();
}

void AstraCommandPaletteView::OnSearchResultsChanged(
    AstraCommandPaletteModel* model) {
  DCHECK_EQ(model, model_);
  RebuildResultsList();
  UpdateSelectionVisual();
  UpdateStatusBar();
}

void AstraCommandPaletteView::OnCommandExecuted(
    AstraCommandPaletteModel* model,
    int command_id) {
  DCHECK_EQ(model, model_);
  // Forward execution request to delegate.
  if (delegate_) {
    // Determine if it's an Astra command.
    const auto* item = model_->GetCommandAt(model_->GetSelectedIndex());
    bool is_astra = item ? item->is_astra : false;
    delegate_->OnCommandPaletteExecute(command_id, is_astra);
  }
}

void AstraCommandPaletteView::OnCommandPaletteModelShutdown(
    AstraCommandPaletteModel* model) {
  DCHECK_EQ(model, model_);
  // Model is shutting down — drop our reference.
  model_ = nullptr;
  model_owned_ = false;
  RebuildResultsList();
  UpdateStatusBar();
}

// =========================================================================
// AstraCommandPaletteModelObserver implementation (legacy)
// =========================================================================

void AstraCommandPaletteView::OnModelChanged() {
  RebuildResultsList();
  UpdateSelectionVisual();
  UpdateStatusBar();
}

void AstraCommandPaletteView::OnSelectionChanged() {
  UpdateSelectionVisual();
  ScrollSelectedIntoView();

  // Forward selection change to delegate.
  if (delegate_ && model_) {
    delegate_->OnCommandPaletteSelectionChanged(model_->GetSelectedIndex());
  }
}

void AstraCommandPaletteView::OnSearchTextChanged(
    const std::u16string& new_text) {
  if (delegate_) {
    delegate_->OnCommandPaletteSearchTextChanged(new_text);
  }
}

void AstraCommandPaletteView::OnCommandExecutionRequested(int command_id,
                                                           bool is_astra) {
  if (delegate_) {
    delegate_->OnCommandPaletteExecute(command_id, is_astra);
  }
}

// =========================================================================
// TextfieldController — search input changes & keyboard navigation
// =========================================================================
//
// ContentsChanged fires after the textfield's text has changed.  We use
// it to refresh the results list in real time as the user types.
//
// HandleKeyEvent fires BEFORE the textfield processes a key.  We use it
// to intercept navigation keys (Up, Down, Enter, Escape) that should
// change the selection or execute a command rather than being typed.
// Returning true means "we handled this key, don't let the textfield
// process it."  Returning false means "let the textfield handle it."
// =========================================================================

void AstraCommandPaletteView::ContentsChanged(views::Textfield* sender,
                                              const std::u16string& new_contents) {
  DCHECK_EQ(sender, search_field_);
  if (model_) {
    model_->SetQuery(new_contents);
  }
  // OnModelChanged / OnSearchResultsChanged will fire, which rebuilds
  // the results list.
}

bool AstraCommandPaletteView::HandleKeyEvent(views::Textfield* sender,
                                             const ui::KeyEvent& key_event) {
  DCHECK_EQ(sender, search_field_);

  // Only handle key press events (not release or other types).
  if (key_event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }

  // Command/Ctrl+number shortcuts for category chips.
  if (key_event.IsControlDown() || key_event.IsCommandDown()) {
    if (key_event.key_code() >= ui::VKEY_1 &&
        key_event.key_code() <= ui::VKEY_9) {
      int chip_index = key_event.key_code() - ui::VKEY_1;
      if (chip_index < static_cast<int>(GetCategoryChipCount())) {
        SelectCategoryChip(chip_index);
        return true;
      }
    }

    // Cmd/Ctrl+Down = next group.
    if (key_event.key_code() == ui::VKEY_DOWN && model_) {
      model_->SelectNextGroup();
      ScrollSelectedIntoView();
      return true;
    }

    // Cmd/Ctrl+Up = previous group.
    if (key_event.key_code() == ui::VKEY_UP && model_) {
      model_->SelectPrevGroup();
      ScrollSelectedIntoView();
      return true;
    }
  }

  switch (key_event.key_code()) {
    case ui::VKEY_UP:
      SelectPrevious();
      return true;

    case ui::VKEY_DOWN:
      SelectNext();
      return true;

    case ui::VKEY_RETURN:
      ExecuteSelected();
      return true;

    case ui::VKEY_ESCAPE:
      if (delegate_) {
        delegate_->OnCommandPaletteClose();
      }
      return true;

    case ui::VKEY_TAB:
      // Tab moves focus to results; Shift+Tab stays in search field.
      if (!key_event.IsShiftDown() && model_ && model_->GetResultCount() > 0) {
        // Move focus to the first result item.
        if (item_views_.size() > 0) {
          item_views_[0]->RequestFocus();
        }
        return true;
      }
      return false;

    case ui::VKEY_HOME:
      if (key_event.IsControlDown() || key_event.IsCommandDown()) {
        SelectFirst();
        return true;
      }
      return false;

    case ui::VKEY_END:
      if (key_event.IsControlDown() || key_event.IsCommandDown()) {
        SelectLast();
        return true;
      }
      return false;

    default:
      // Let the textfield process the key normally.
      // ContentsChanged will fire after the text updates.
      return false;
  }
}

}  // namespace astra
