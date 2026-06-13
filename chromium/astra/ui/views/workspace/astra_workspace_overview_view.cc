#include "astra/ui/views/workspace/astra_workspace_overview_view.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/workspace/astra_workspace_card_view.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace astra {

namespace {

// Overview dimensions / spacing.
constexpr int kOverviewPadding = 48;
constexpr int kHeaderRowHeight = 44;
constexpr int kSectionHeaderHeight = 36;
constexpr int kSectionHeaderBottomPadding = 16;
constexpr int kCardWidth = 280;
constexpr int kCardHeight = 220;
constexpr int kCardHorizontalSpacing = 20;
constexpr int kCardVerticalSpacing = 20;

// List view dimensions.
constexpr int kListRowHeight = 56;
constexpr int kListHorizontalPadding = 16;

// Search field.
constexpr int kSearchFieldWidth = 320;
constexpr int kSearchFieldHeight = 36;

// Action button spacing.
constexpr int kActionButtonSpacing = 8;

// Title / section styling.
constexpr int kTitleFontSizeDelta = 8;
constexpr int kSectionTitleFontSizeDelta = 2;

// Background overlay color (semi-transparent dark).
// TODO(astra): Use ColorProvider with a proper scrim color ID.
constexpr SkColor kOverlayBackgroundColor = SkColorSetARGB(204, 0, 0, 0);

// New workspace card styling.
constexpr SkColor kNewCardBackgroundColor = SkColorSetARGB(51, 255, 255, 255);
constexpr SkColor kNewCardTextColor = SK_ColorWHITE;
constexpr SkColor kNewCardBorderColor = SkColorSetARGB(102, 255, 255, 255);

// Returns true if the workspace matches the given search query.
bool WorkspaceMatchesQuery(const AstraWorkspace& workspace,
                           const std::u16string& query) {
  if (query.empty()) {
    return true;
  }

  std::u16string name_u16 = base::UTF8ToUTF16(workspace.name);
  // Case-insensitive substring match.
  // TODO(astra): Use base::i18n::ToLower or StringUtil for proper
  //   case-insensitive search with Unicode support.
  //   Chromium pattern: base/strings/string_util.h with
  //   base::i18n::ToLower or string_util::LowerCaseEqualsASCII.
  std::transform(name_u16.begin(), name_u16.end(), name_u16.begin(), ::tolower);
  std::u16string query_lower = query;
  std::transform(query_lower.begin(), query_lower.end(),
                 query_lower.begin(), ::tolower);

  return name_u16.find(query_lower) != std::u16string::npos;
}

}  // namespace

AstraWorkspaceOverviewView::AstraWorkspaceOverviewView() {
  BuildLayout();
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  // Register accelerators.
  AddAccelerator(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_LEFT, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_RIGHT, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_UP, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_DOWN, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_DELETE, ui::EF_NONE));
  AddAccelerator(ui::Accelerator(ui::VKEY_BACK, ui::EF_NONE));
  AddAccelerator(
      ui::Accelerator(ui::VKEY_TAB, ui::EF_CONTROL_DOWN));
  AddAccelerator(
      ui::Accelerator(ui::VKEY_TAB, ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN));
  // TODO(astra): On Mac, use Command (EF_COMMAND_DOWN) instead of Control.
  // Chromium handles platform key differences via ui::Accelerator and the
  // platform's native widget accelerator processing.
}

AstraWorkspaceOverviewView::~AstraWorkspaceOverviewView() = default;

// =========================================================================
// Observer management
// =========================================================================

void AstraWorkspaceOverviewView::AddObserver(
    AstraWorkspaceOverviewViewObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraWorkspaceOverviewView::RemoveObserver(
    AstraWorkspaceOverviewViewObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Data update (called by controller)
// =========================================================================

void AstraWorkspaceOverviewView::UpdateWorkspaces(
    const std::vector<AstraWorkspace>& workspaces,
    const std::string& active_workspace_id,
    const std::vector<int>& tab_counts,
    const std::vector<int>& window_counts) {
  workspaces_ = workspaces;
  active_workspace_id_ = active_workspace_id;
  tab_counts_ = tab_counts;
  window_counts_ = window_counts;

  // Update section count.
  auto filtered = FilterWorkspaces();
  UpdateWorkspaceCount(static_cast<int>(filtered.size()));

  RebuildCards();
}

void AstraWorkspaceOverviewView::SetWorkspaces(
    const std::vector<AstraWorkspace>& workspaces,
    const std::string& active_workspace_id,
    const std::vector<int>& tab_counts,
    const std::vector<int>& window_counts) {
  UpdateWorkspaces(workspaces, active_workspace_id, tab_counts, window_counts);
}

void AstraWorkspaceOverviewView::UpdateWorkspaceCount(int count) {
  if (section_count_) {
    section_count_->SetText(base::NumberToString16(count));
  }
}

void AstraWorkspaceOverviewView::SetSearchQuery(const std::u16string& query) {
  if (search_field_) {
    search_field_->SetText(query);
  }
  search_query_ = query;
  RebuildCards();
}

// =========================================================================
// Presentation settings
// =========================================================================

void AstraWorkspaceOverviewView::SetViewMode(
    AstraWorkspaceOverviewViewMode mode) {
  if (view_mode_ == mode) {
    return;
  }
  view_mode_ = mode;

  // Update view_mode_button_ text.
  if (view_mode_button_) {
    view_mode_button_->SetText(
        mode == AstraWorkspaceOverviewViewMode::kGrid ? u"Grid" : u"List");
  }

  RebuildCards();
  NotifyViewModeChanged();
}

void AstraWorkspaceOverviewView::SetCardSize(
    AstraWorkspaceOverviewCardSize size) {
  if (card_size_ == size) {
    return;
  }
  card_size_ = size;
  RebuildCards();
  NotifyCardSizeChanged();
}

void AstraWorkspaceOverviewView::SetShowStatistics(bool show) {
  if (show_statistics_ == show) {
    return;
  }
  show_statistics_ = show;

  // Update all cards.
  for (auto* child : cards_container_->children()) {
    if (child == new_workspace_card_) {
      continue;
    }
    auto* card = static_cast<AstraWorkspaceCardView*>(child);
    card->SetShowStatistics(show);
  }

  NotifyShowStatisticsChanged();
}

// =========================================================================
// Selection / focus helpers
// =========================================================================

void AstraWorkspaceOverviewView::SelectWorkspaceAt(int index) {
  int card_count = GetWorkspaceCardCount();
  if (card_count == 0) {
    selected_index_ = -1;
    return;
  }

  // Clamp to valid range.
  int old_index = selected_index_;
  selected_index_ = std::max(0, std::min(index, card_count - 1));

  // Update card visual states.
  int card_idx = 0;
  for (auto* child : cards_container_->children()) {
    if (child == new_workspace_card_) {
      continue;
    }
    auto* card = static_cast<AstraWorkspaceCardView*>(child);
    card->SetIsSelected(card_idx == selected_index_);
    if (card_idx == selected_index_) {
      card->RequestFocus();
    }
    ++card_idx;
  }

  ScrollSelectedCardIntoView();

  // Notify observers if selection changed.
  if (old_index != selected_index_) {
    std::string selected_id = GetSelectedWorkspaceId();
    if (!selected_id.empty()) {
      NotifyWorkspaceSelected(selected_id);
    }
  }
}

std::string AstraWorkspaceOverviewView::GetSelectedWorkspaceId() const {
  auto filtered = FilterWorkspaces();
  if (selected_index_ < 0 ||
      selected_index_ >= static_cast<int>(filtered.size())) {
    return std::string();
  }
  return filtered[selected_index_].id;
}

int AstraWorkspaceOverviewView::GetWorkspaceCardCount() const {
  // Total children minus the new workspace card.
  int total = static_cast<int>(cards_container_->children().size());
  int new_card_count = new_workspace_card_ ? 1 : 0;
  return total - new_card_count;
}

void AstraWorkspaceOverviewView::SelectFirstWorkspace() {
  SelectWorkspaceAt(0);
}

void AstraWorkspaceOverviewView::SelectLastWorkspace() {
  int card_count = GetWorkspaceCardCount();
  if (card_count > 0) {
    SelectWorkspaceAt(card_count - 1);
  }
}

AstraWorkspaceCardView* AstraWorkspaceOverviewView::GetWorkspaceCardAt(
    int index) const {
  if (!cards_container_) {
    return nullptr;
  }

  int card_idx = 0;
  for (auto* child : cards_container_->children()) {
    if (child == new_workspace_card_) {
      continue;
    }
    if (card_idx == index) {
      return static_cast<AstraWorkspaceCardView*>(child);
    }
    ++card_idx;
  }

  return nullptr;
}

void AstraWorkspaceOverviewView::ClearSelection() {
  if (selected_index_ < 0) {
    return;
  }

  // Clear selected state on the current selected card.
  int card_idx = 0;
  for (auto* child : cards_container_->children()) {
    if (child == new_workspace_card_) {
      continue;
    }
    if (card_idx == selected_index_) {
      auto* card = static_cast<AstraWorkspaceCardView*>(child);
      card->SetIsSelected(false);
      break;
    }
    ++card_idx;
  }

  selected_index_ = -1;
}

// =========================================================================
// Search
// =========================================================================

std::u16string AstraWorkspaceOverviewView::GetSearchQuery() const {
  return search_query_;
}

void AstraWorkspaceOverviewView::ShowSearch(bool show) {
  if (!search_field_) {
    return;
  }

  if (show == search_field_->GetVisible()) {
    return;
  }

  search_field_->SetVisible(show);

  if (show) {
    search_field_->RequestFocus();
  } else {
    SetSearchQuery(std::u16string());
  }

  Layout();
}

bool AstraWorkspaceOverviewView::IsSearchVisible() const {
  return search_field_ && search_field_->GetVisible();
}

// =========================================================================
// New workspace button
// =========================================================================

void AstraWorkspaceOverviewView::ShowNewWorkspaceButton(bool show) {
  if (!new_workspace_card_) {
    return;
  }

  if (show == new_workspace_card_->GetVisible()) {
    return;
  }

  new_workspace_card_->SetVisible(show);
  Layout();
}

bool AstraWorkspaceOverviewView::IsNewWorkspaceButtonVisible() const {
  return new_workspace_card_ && new_workspace_card_->GetVisible();
}

// =========================================================================
// Layout
// =========================================================================

void AstraWorkspaceOverviewView::SetLayout(AstraOverviewLayout layout) {
  SetViewMode(static_cast<AstraWorkspaceOverviewViewMode>(layout));
}

// =========================================================================
// views::WidgetDelegateView
// =========================================================================

void AstraWorkspaceOverviewView::WindowClosing() {
  for (auto& observer : observers_) {
    observer.OnOverviewClosing();
  }
}

bool AstraWorkspaceOverviewView::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  // If focus is in the search field, don't handle navigation keys
  // (let the textfield handle them).
  if (search_field_ && search_field_->HasFocus()) {
    // Only handle Escape from search field (to clear / close).
    if (accelerator.key_code() == ui::VKEY_ESCAPE) {
      if (!search_query_.empty()) {
        // First press: clear search.
        SetSearchQuery(std::u16string());
        search_field_->RequestFocus();
        return true;
      }
      // Fall through to close.
    } else {
      return views::WidgetDelegateView::AcceleratorPressed(accelerator);
    }
  }

  switch (accelerator.key_code()) {
    case ui::VKEY_ESCAPE:
      // Close the overview.
      if (GetWidget()) {
        GetWidget()->Close();
      }
      return true;

    case ui::VKEY_RETURN:
      ActivateSelectedWorkspace();
      return true;

    case ui::VKEY_LEFT:
      SelectPreviousWorkspace();
      return true;

    case ui::VKEY_RIGHT:
      SelectNextWorkspace();
      return true;

    case ui::VKEY_UP:
      SelectWorkspaceAbove();
      return true;

    case ui::VKEY_DOWN:
      SelectWorkspaceBelow();
      return true;

    case ui::VKEY_DELETE:
    case ui::VKEY_BACK:
      DeleteSelectedWorkspace();
      return true;

    case ui::VKEY_TAB:
      if (accelerator.IsCtrlDown()) {
        if (accelerator.IsShiftDown()) {
          SelectPreviousWorkspace();
        } else {
          SelectNextWorkspace();
        }
        return true;
      }
      break;

    default:
      break;
  }

  return views::WidgetDelegateView::AcceleratorPressed(accelerator);
}

gfx::Size AstraWorkspaceOverviewView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Size to fill the available space (full-window overlay).
  int width = 0;
  int height = 0;
  if (available_size.width().is_bounded()) {
    width = available_size.width().value();
  }
  if (available_size.height().is_bounded()) {
    height = available_size.height().value();
  }
  return gfx::Size(width, height);
}

// =========================================================================
// views::View
// =========================================================================

void AstraWorkspaceOverviewView::OnThemeChanged() {
  views::WidgetDelegateView::OnThemeChanged();

  const ui::ColorProvider* cp = GetColorProvider();
  if (!cp) {
    return;
  }

  // Update text colors.
  if (section_title_) {
    section_title_->SetEnabledColor(SK_ColorWHITE);
  }
  if (section_count_) {
    section_count_->SetEnabledColor(
        cp->GetColor(ui::kColorLabelForegroundSecondary));
  }

  // Update action button text colors.
  if (import_button_) {
    import_button_->SetTextColor(views::Button::STATE_NORMAL, SK_ColorWHITE);
    import_button_->SetTextColor(views::Button::STATE_HOVERED, SK_ColorWHITE);
  }
  if (export_button_) {
    export_button_->SetTextColor(views::Button::STATE_NORMAL, SK_ColorWHITE);
    export_button_->SetTextColor(views::Button::STATE_HOVERED, SK_ColorWHITE);
  }

  // TODO(astra): Update all child colors from ColorProvider when Astra has
  // its own full color mixin, including the overlay background.
}

bool AstraWorkspaceOverviewView::OnMousePressed(const ui::MouseEvent& event) {
  // Clicking outside the cards (on the overlay background) should close the
  // overview.  We check if the click is on the background view (not on any
  // card child or header).
  if (event.IsOnlyLeftMouseButton()) {
    // Check if the click point is over any interactive child.
    // Since child views consume their own clicks, if we reach this handler,
    // the click is on the background area.
    if (GetWidget()) {
      GetWidget()->Close();
    }
    return true;
  }
  return views::WidgetDelegateView::OnMousePressed(event);
}

void AstraWorkspaceOverviewView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kDialog;
  node_data->SetName(u"Workspace overview");
  node_data->SetDescription(
      u"Overview of all workspaces. Use arrow keys to navigate, "
      u"Enter to activate, Escape to close.");
}

void AstraWorkspaceOverviewView::Layout() {
  views::WidgetDelegateView::Layout();

  // The main layout is handled by BoxLayout for the header and section
  // title.  The cards container uses a custom layout.
  //
  // TODO(astra): Consider using a FlexLayout once available, or create
  //   a custom LayoutManager for flow/grid layout.
  //   Chromium pattern: ui/views/layout/flex_layout.h for modern flexbox.
  //   For the skeleton, manual Layout() override is simpler.

  if (!cards_container_) {
    return;
  }

  if (view_mode_ == AstraWorkspaceOverviewViewMode::kList) {
    LayoutListMode();
  } else {
    LayoutGridMode();
  }
}

void AstraWorkspaceOverviewView::LayoutGridMode() {
  int available_width = cards_container_->width();
  int columns = CalculateColumnCount(available_width);
  if (columns <= 0) {
    columns = 1;
  }

  int card_width = GetCardWidth();
  int card_height = GetCardHeight();

  int card_count = GetWorkspaceCardCount();
  int total_items = card_count + (new_workspace_card_ ? 1 : 0);

  if (total_items == 0) {
    return;
  }

  int rows = (total_items + columns - 1) / columns;

  // Calculate total content width (so we can center the grid).
  int total_content_width =
      columns * card_width + (columns - 1) * kCardHorizontalSpacing;

  // Left offset to center the grid in the available space.
  int x_offset = (available_width - total_content_width) / 2;
  if (x_offset < 0) {
    x_offset = 0;
  }

  // Position each card.
  int item_idx = 0;
  for (auto* child : cards_container_->children()) {
    int row = item_idx / columns;
    int col = item_idx % columns;

    int x = x_offset + col * (card_width + kCardHorizontalSpacing);
    int y = row * (card_height + kCardVerticalSpacing);

    child->SetBounds(x, y, card_width, card_height);
    ++item_idx;
  }

  // Update the preferred size of the cards container so the scroll view
  // knows the content height.
  int total_height = rows * card_height + (rows - 1) * kCardVerticalSpacing;
  cards_container_->SetPreferredSize(gfx::Size(available_width, total_height));

  // Notify the scroll view that the contents size changed.
  if (scroll_view_) {
    scroll_view_->Layout();
  }
}

void AstraWorkspaceOverviewView::LayoutListMode() {
  int available_width = cards_container_->width();
  int y = 0;

  for (auto* child : cards_container_->children()) {
    child->SetBounds(kListHorizontalPadding, y,
                     available_width - 2 * kListHorizontalPadding,
                     kListRowHeight);
    y += kListRowHeight;
  }

  // Update the preferred size of the cards container.
  cards_container_->SetPreferredSize(gfx::Size(available_width, y));

  // Notify the scroll view that the contents size changed.
  if (scroll_view_) {
    scroll_view_->Layout();
  }
}

// =========================================================================
// views::TextfieldController
// =========================================================================

void AstraWorkspaceOverviewView::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  if (sender == search_field_) {
    search_query_ = new_contents;

    // Notify observers.
    for (auto& observer : observers_) {
      observer.OnSearchQueryChanged(new_contents);
    }

    // Rebuild cards with filtered results.
    RebuildCards();
  }
}

bool AstraWorkspaceOverviewView::HandleKeyEvent(
    views::Textfield* sender,
    const ui::KeyEvent& key_event) {
  if (sender != search_field_) {
    return false;
  }

  // Handle navigation keys even when search field has focus.
  switch (key_event.key_code()) {
    case ui::VKEY_DOWN:
      // Move focus to first card.
      if (GetWorkspaceCardCount() > 0) {
        SelectWorkspaceAt(0);
      }
      return true;

    case ui::VKEY_ESCAPE:
      if (!search_query_.empty()) {
        // Clear search on first Escape.
        SetSearchQuery(std::u16string());
        return true;
      }
      // Let Escape bubble up to close the overview.
      return false;

    default:
      break;
  }

  return false;
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraWorkspaceOverviewView::BuildLayout() {
  // Main vertical layout: header + section header + cards (scrollable).
  auto* main_layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kOverviewPadding, kOverviewPadding),
      /*between_child_spacing=*/0));
  main_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // 1. Header row: search bar (left) + action buttons (right).
  header_row_ = AddChildView(std::make_unique<views::View>());
  header_row_->SetPreferredSize(gfx::Size(0, kHeaderRowHeight));

  auto* header_layout = header_row_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          /*between_child_spacing=*/0));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  header_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);

  // Search field (left).
  auto search_field = std::make_unique<views::Textfield>();
  search_field->SetPlaceholderText(u"Search workspaces...");
  search_field->SetController(this);
  search_field->SetPreferredSize(
      gfx::Size(kSearchFieldWidth, kSearchFieldHeight));
  search_field->SetAccessibleName(u"Search workspaces");
  // TODO(astra): Style the search field to match Astra design (rounded,
  //   with search icon).  Use a Textfield with a leading icon.
  //   Chromium pattern: omnibox or search field styling.
  search_field_ = header_row_->AddChildView(std::move(search_field));

  // Action buttons (right).
  auto* actions_row = header_row_->AddChildView(std::make_unique<views::View>());
  auto* actions_layout = actions_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          kActionButtonSpacing));
  actions_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  import_button_ = actions_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              [](AstraWorkspaceOverviewView* overview) {
                for (auto& observer : overview->observers_) {
                  observer.OnImportRequested();
                }
              },
              base::Unretained(this)),
          u"Import"));

  export_button_ = actions_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              [](AstraWorkspaceOverviewView* overview) {
                for (auto& observer : overview->observers_) {
                  observer.OnExportRequested();
                }
              },
              base::Unretained(this)),
          u"Export"));

  // View mode toggle button (grid/list).
  view_mode_button_ = actions_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              [](AstraWorkspaceOverviewView* overview) {
                // Toggle between grid and list.
                auto new_mode =
                    overview->view_mode_ == AstraWorkspaceOverviewViewMode::kGrid
                        ? AstraWorkspaceOverviewViewMode::kList
                        : AstraWorkspaceOverviewViewMode::kGrid;
                overview->SetViewMode(new_mode);
              },
              base::Unretained(this)),
          u"Grid"));
  view_mode_button_->SetTooltipText(u"Toggle view mode");

  // Settings button.
  settings_button_ = actions_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              [](AstraWorkspaceOverviewView* overview) {
                for (auto& observer : overview->observers_) {
                  observer.OnOverviewSettingsRequested();
                }
              },
              base::Unretained(this)),
          u"Settings"));
  settings_button_->SetTooltipText(u"Overview settings");

  // 2. Section header: title + count.
  section_header_ = AddChildView(std::make_unique<views::View>());
  section_header_->SetPreferredSize(
      gfx::Size(0, kSectionHeaderHeight + kSectionHeaderBottomPadding));

  auto* section_layout = section_header_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, 0),
          /*between_child_spacing=*/8));
  section_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  section_title_ = section_header_->AddChildView(
      std::make_unique<views::Label>(u"All Workspaces"));
  section_title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  section_title_->SetAutoColorReadabilityEnabled(false);
  section_title_->SetEnabledColor(SK_ColorWHITE);
  section_title_->SetFontList(section_title_->font_list().Derive(
      kSectionTitleFontSizeDelta, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));

  section_count_ = section_header_->AddChildView(
      std::make_unique<views::Label>(u"0"));
  section_count_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  section_count_->SetAutoColorReadabilityEnabled(false);
  section_count_->SetEnabledColor(SK_ColorLTGRAY);

  // 3. Scrollable cards area.
  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetBackgroundColor(absl::nullopt);
  scroll_view_->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);
  scroll_view_->SetVerticalScrollBarMode(
      views::ScrollView::ScrollBarMode::kEnabled);
  // Allow the scroll view to take all remaining vertical space.
  main_layout->SetFlexForView(scroll_view_, 1);

  cards_container_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  cards_container_->SetPaintToLayer();
  cards_container_->layer()->SetFillsBoundsOpaquely(false);

  // New workspace card (added last, after all real workspaces).
  new_workspace_card_ =
      cards_container_->AddChildView(CreateNewWorkspaceCard());
}

void AstraWorkspaceOverviewView::RebuildCards() {
  // Remove all existing workspace cards (but keep the new workspace card).
  std::vector<raw_ptr<views::View>> children_to_remove;
  for (auto* child : cards_container_->children()) {
    if (child != new_workspace_card_) {
      children_to_remove.push_back(child);
    }
  }
  for (auto* child : children_to_remove) {
    cards_container_->RemoveChildViewT(child);
  }

  // Get filtered workspaces.
  auto filtered = FilterWorkspaces();

  // Update section count.
  UpdateWorkspaceCount(static_cast<int>(filtered.size()));

  // Build a map from workspace id to tab/window count index.
  std::unordered_map<std::string, int> id_to_tab_count;
  std::unordered_map<std::string, int> id_to_window_count;
  for (size_t i = 0; i < workspaces_.size(); ++i) {
    if (i < tab_counts_.size()) {
      id_to_tab_count[workspaces_[i].id] = tab_counts_[i];
    }
    if (i < window_counts_.size()) {
      id_to_window_count[workspaces_[i].id] = window_counts_[i];
    }
  }

  // Create new cards for each filtered workspace.
  for (size_t i = 0; i < filtered.size(); ++i) {
    const auto& ws = filtered[i];

    auto card = std::make_unique<AstraWorkspaceCardView>();
    card->SetWorkspaceName(base::UTF8ToUTF16(ws.name));
    card->SetAccentColor(ws.accent_color);
    card->SetCreatedTime(ws.created_time);

    int tab_count = 0;
    auto tab_it = id_to_tab_count.find(ws.id);
    if (tab_it != id_to_tab_count.end()) {
      tab_count = tab_it->second;
    }
    card->SetTabCount(tab_count);

    int window_count = 1;
    auto win_it = id_to_window_count.find(ws.id);
    if (win_it != id_to_window_count.end()) {
      window_count = win_it->second;
    }
    card->SetWindowCount(window_count);

    card->SetLastUsedTime(ws.last_used_time);
    card->SetIcon(ws.icon);
    card->SetIsHibernated(ws.is_hibernated);
    card->SetDisplayMode(
        view_mode_ == AstraWorkspaceOverviewViewMode::kGrid
            ? AstraWorkspaceCardView::DisplayMode::kCard
            : AstraWorkspaceCardView::DisplayMode::kList);
    card->SetShowStatistics(show_statistics_);
    card->SetSizeVariant(card_size_);

    card->SetIsActive(ws.id == active_workspace_id_);
    card->SetIsSelected(static_cast<int>(i) == selected_index_);

    std::string workspace_id = ws.id;

    // Click: activate workspace.
    card->SetClickCallback(base::BindRepeating(
        [](AstraWorkspaceOverviewView* overview,
           const std::string& id) {
          for (auto& observer : overview->observers_) {
            observer.OnWorkspaceClicked(id);
          }
        },
        base::Unretained(this), workspace_id));

    // Double-click: rename (handled via context menu for now).
    // TODO(astra): Add double-click detection for rename.  Chromium
    //   pattern: OnMouseDoubleClicked override.
    card->SetRenameCallback(base::BindRepeating(
        [](AstraWorkspaceOverviewView* overview,
           const std::string& id) {
          for (auto& observer : overview->observers_) {
            observer.OnWorkspaceRenameRequested(id);
          }
        },
        base::Unretained(this), workspace_id));

    // Menu button: show actions menu.
    card->SetMenuActionCallback(base::BindRepeating(
        [](AstraWorkspaceOverviewView* overview,
           const std::string& id,
           const gfx::Point& screen_point) {
          for (auto& observer : overview->observers_) {
            observer.OnWorkspaceMenuRequested(id, screen_point);
          }
        },
        base::Unretained(this), workspace_id));

    // Delete callback (keyboard).
    card->SetDeleteCallback(base::BindRepeating(
        [](AstraWorkspaceOverviewView* overview,
           const std::string& id) {
          for (auto& observer : overview->observers_) {
            observer.OnWorkspaceDeleteRequested(id);
          }
        },
        base::Unretained(this), workspace_id));

    // Insert before the "new workspace" card.
    int insert_index =
        cards_container_->GetIndexOf(new_workspace_card_).value_or(
            cards_container_->children().size());
    cards_container_->AddChildViewAt(std::move(card), insert_index);
  }

  // Invalidate layout so the grid recalculates.
  cards_container_->InvalidateLayout();
  InvalidateLayout();
}

std::unique_ptr<views::View>
AstraWorkspaceOverviewView::CreateNewWorkspaceCard() {
  // A card-like button that says "New Workspace" with a + icon.
  auto card = std::make_unique<views::LabelButton>(
      base::BindRepeating(
          [](AstraWorkspaceOverviewView* overview) {
            for (auto& observer : overview->observers_) {
              observer.OnNewWorkspaceRequested();
            }
          },
          base::Unretained(this)),
      u"+ New Workspace");

  card->SetPaintToLayer();
  card->layer()->SetFillsBoundsOpaquely(false);
  card->SetPreferredSize(gfx::Size(kCardWidth, kCardHeight));
  card->SetTextColor(views::Button::STATE_NORMAL, kNewCardTextColor);
  card->SetTextColor(views::Button::STATE_HOVERED, kNewCardTextColor);
  card->SetTextColor(views::Button::STATE_PRESSED, kNewCardTextColor);
  card->SetBorder(views::CreateEmptyBorder(gfx::Insets()));
  card->SetAccessibleName(u"Create new workspace");

  // Style as a card with background and border.
  // TODO(astra): Style this as a proper card with background + border + icon.
  //   Use OnPaintBackground to draw rounded rect background.

  // TODO(astra): Add a + icon (vector icon) to the new workspace card.
  // Chromium pattern: use vector icons from ui/gfx/vector_icon_types.h and
  // ui/views/controls/vector_icon_button.h or views::ImageView.
  // For now, just the text label.

  return card;
}

AstraWorkspaceCardView* AstraWorkspaceOverviewView::FindCardForWorkspace(
    const std::string& workspace_id) {
  // Walk children of cards_container_, skipping the new workspace card.
  for (auto* child : cards_container_->children()) {
    if (child == new_workspace_card_) {
      continue;
    }
    // TODO(astra): Store workspace id as a property on the card or use a
    // map for O(1) lookup.  For the skeleton with few workspaces, this
    // linear search from the outside is acceptable but we'd need to
    // compare against data.
  }
  return nullptr;
}

std::vector<AstraWorkspace>
AstraWorkspaceOverviewView::FilterWorkspaces() const {
  if (search_query_.empty()) {
    return workspaces_;
  }

  std::vector<AstraWorkspace> filtered;
  for (const auto& ws : workspaces_) {
    if (WorkspaceMatchesQuery(ws, search_query_)) {
      filtered.push_back(ws);
    }
  }
  return filtered;
}

// =========================================================================
// Keyboard navigation
// =========================================================================

void AstraWorkspaceOverviewView::SelectNextWorkspace() {
  int card_count = GetWorkspaceCardCount();
  if (card_count == 0) {
    return;
  }
  if (selected_index_ < 0) {
    SelectWorkspaceAt(0);
  } else {
    SelectWorkspaceAt((selected_index_ + 1) % card_count);
  }
}

void AstraWorkspaceOverviewView::SelectPreviousWorkspace() {
  int card_count = GetWorkspaceCardCount();
  if (card_count == 0) {
    return;
  }
  if (selected_index_ < 0) {
    SelectWorkspaceAt(card_count - 1);
  } else {
    SelectWorkspaceAt((selected_index_ - 1 + card_count) % card_count);
  }
}

void AstraWorkspaceOverviewView::SelectWorkspaceAbove() {
  int card_count = GetWorkspaceCardCount();
  if (card_count == 0) {
    return;
  }

  int columns = CalculateColumnCount(cards_container_
                                         ? cards_container_->width()
                                         : kCardWidth * 3);
  if (columns <= 0) {
    columns = 1;
  }

  if (selected_index_ < 0) {
    SelectWorkspaceAt(0);
    return;
  }

  int new_index = selected_index_ - columns;
  if (new_index < 0) {
    // Wrap to last row (same column if possible).
    int rows = (card_count + columns - 1) / columns;
    int col = selected_index_ % columns;
    new_index = (rows - 1) * columns + col;
    if (new_index >= card_count) {
      new_index = card_count - 1;
    }
  }
  SelectWorkspaceAt(new_index);
}

void AstraWorkspaceOverviewView::SelectWorkspaceBelow() {
  int card_count = GetWorkspaceCardCount();
  if (card_count == 0) {
    return;
  }

  int columns = CalculateColumnCount(cards_container_
                                         ? cards_container_->width()
                                         : kCardWidth * 3);
  if (columns <= 0) {
    columns = 1;
  }

  if (selected_index_ < 0) {
    SelectWorkspaceAt(0);
    return;
  }

  int new_index = selected_index_ + columns;
  if (new_index >= card_count) {
    // Wrap to first row (same column).
    int col = selected_index_ % columns;
    new_index = col;
    if (new_index >= card_count) {
      new_index = 0;
    }
  }
  SelectWorkspaceAt(new_index);
}

void AstraWorkspaceOverviewView::ActivateSelectedWorkspace() {
  if (selected_index_ < 0) {
    return;
  }
  std::string id = GetSelectedWorkspaceId();
  if (id.empty()) {
    return;
  }
  for (auto& observer : observers_) {
    observer.OnWorkspaceClicked(id);
  }
}

void AstraWorkspaceOverviewView::DeleteSelectedWorkspace() {
  if (selected_index_ < 0) {
    return;
  }
  std::string id = GetSelectedWorkspaceId();
  if (id.empty()) {
    return;
  }
  // Don't allow deleting the active workspace via keyboard shortcut
  // without confirmation — the observer should handle confirmation.
  for (auto& observer : observers_) {
    observer.OnWorkspaceDeleteRequested(id);
  }
}

int AstraWorkspaceOverviewView::CalculateColumnCount(
    int available_width) const {
  if (available_width <= 0) {
    return 1;
  }
  int card_width = GetCardWidth();
  int columns =
      (available_width + kCardHorizontalSpacing) /
      (card_width + kCardHorizontalSpacing);
  return std::max(1, columns);
}

void AstraWorkspaceOverviewView::ScrollSelectedCardIntoView() {
  if (!scroll_view_ || !cards_container_ || selected_index_ < 0) {
    return;
  }

  int card_count = GetWorkspaceCardCount();
  if (selected_index_ >= card_count) {
    return;
  }

  // Find the selected card view and scroll it into view.
  int card_idx = 0;
  for (auto* child : cards_container_->children()) {
    if (child == new_workspace_card_) {
      continue;
    }
    if (card_idx == selected_index_) {
      scroll_view_->ScrollRectToVisible(child->bounds());
      break;
    }
    ++card_idx;
  }
}

void AstraWorkspaceOverviewView::NotifyWorkspaceSelected(
    const std::string& workspace_id) {
  for (auto& observer : observers_) {
    observer.OnWorkspaceSelected(workspace_id);
  }
}

void AstraWorkspaceOverviewView::NotifyViewModeChanged() {
  for (auto& observer : observers_) {
    observer.OnViewModeChanged(view_mode_);
  }
}

void AstraWorkspaceOverviewView::NotifyCardSizeChanged() {
  for (auto& observer : observers_) {
    observer.OnCardSizeChanged(card_size_);
  }
}

void AstraWorkspaceOverviewView::NotifyShowStatisticsChanged() {
  for (auto& observer : observers_) {
    observer.OnShowStatisticsChanged(show_statistics_);
  }
}

int AstraWorkspaceOverviewView::GetCardWidth() const {
  switch (card_size_) {
    case AstraWorkspaceOverviewCardSize::kSmall:
      return 200;
    case AstraWorkspaceOverviewCardSize::kMedium:
      return 280;
    case AstraWorkspaceOverviewCardSize::kLarge:
      return 360;
  }
  return 280;
}

int AstraWorkspaceOverviewView::GetCardHeight() const {
  switch (card_size_) {
    case AstraWorkspaceOverviewCardSize::kSmall:
      return 170;
    case AstraWorkspaceOverviewCardSize::kMedium:
      return 220;
    case AstraWorkspaceOverviewCardSize::kLarge:
      return 280;
  }
  return 220;
}

}  // namespace astra
