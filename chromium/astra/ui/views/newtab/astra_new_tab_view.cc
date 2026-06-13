#include "astra/ui/views/newtab/astra_new_tab_view.h"

#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/scroll_view.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/ui/views/newtab/astra_new_tab_model.h"
#include "astra/ui/views/newtab/astra_ntp_shortcut_view.h"
#include "astra/ui/views/newtab/astra_ntp_workspace_card.h"

namespace astra {

// Quick action ID constants.
constexpr char AstraNewTabView::kActionNewWorkspace[];
constexpr char AstraNewTabView::kActionScreenshot[];
constexpr char AstraNewTabView::kActionFocusMode[];
constexpr char AstraNewTabView::kActionHistory[];
constexpr char AstraNewTabView::kActionDownloads[];
constexpr char AstraNewTabView::kActionBookmarks[];

namespace {

// Layout constants.
constexpr int kNtpMaxWidth = 900;
constexpr int kSectionSpacing = 32;
constexpr int kSectionHeaderSpacing = 16;
constexpr int kContentHorizontalPadding = 48;
constexpr int kContentTopPadding = 16;
constexpr int kContentBottomPadding = 32;

// Greeting section.
constexpr int kGreetingFontSizeDelta = 10;
constexpr int kGreetingSubtitleFontSizeDelta = 0;
constexpr SkColor kGreetingTextColor = SkColorSetRGB(0x20, 0x20, 0x20);
constexpr SkColor kGreetingSubtitleColor = SkColorSetRGB(0x66, 0x66, 0x66);

// Section header.
constexpr int kSectionHeaderFontSizeDelta = 3;
constexpr SkColor kSectionHeaderTextColor = SkColorSetRGB(0x33, 0x33, 0x33);

// Shortcuts grid.
constexpr int kDefaultShortcutColumns = 4;
constexpr int kShortcutRowSpacing = 16;
constexpr int kShortcutColumnSpacing = 16;
constexpr int kMinShortcutColumns = 2;
constexpr int kMaxShortcutColumns = 8;

// Workspace cards row.
constexpr size_t kMaxWorkspaceCards = 5;
constexpr int kWorkspaceCardSpacing = 12;

// Recently closed section.
constexpr int kRecentlyClosedItemCount = 8;
constexpr int kRecentlyClosedItemWidth = 140;
constexpr int kRecentlyClosedItemHeight = 80;
constexpr int kRecentlyClosedItemSpacing = 12;

// Quick actions.
constexpr int kQuickActionCount = 6;
constexpr int kQuickActionButtonWidth = 100;
constexpr int kQuickActionButtonHeight = 80;
constexpr int kQuickActionIconSize = 24;
constexpr int kQuickActionSpacing = 12;
constexpr SkColor kQuickActionBgColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kQuickActionHoverBgColor = SkColorSetRGB(0xF0, 0xF0, 0xF0);
constexpr SkColor kQuickActionTextColor = SkColorSetRGB(0x33, 0x33, 0x33);

// Colors.
constexpr SkColor kNtpBackgroundColor = SkColorSetRGB(0xF8, 0xF9, 0xFA);
constexpr SkColor kSectionCardBackgroundColor = SK_ColorWHITE;
constexpr SkColor kDividerColor = SkColorSetRGB(0xE8, 0xE8, 0xE8);

// Settings gear button.
constexpr int kSettingsGearSize = 24;
constexpr int kSettingsGearTopInset = 12;
constexpr int kSettingsGearRightInset = 12;
constexpr SkColor kSettingsGearColor = SkColorSetRGB(0x99, 0x99, 0x99);
constexpr SkColor kSettingsGearHoverColor = SkColorSetRGB(0x33, 0x33, 0x33);

// Quick action icon characters.
constexpr char16_t kQuickActionNewWorkspaceIcon = '+';
constexpr char16_t kQuickActionScreenshotIcon = 0x1F4F7;  // 📷
constexpr char16_t kQuickActionFocusModeIcon = 0x1F512;  // 🔒
constexpr char16_t kQuickActionHistoryIcon = 0x1F4DC;    // 📜
constexpr char16_t kQuickActionDownloadsIcon = 0x2B07;   // ⬇
constexpr char16_t kQuickActionBookmarksIcon = 0x2B50;   // ⭐

// =========================================================================
// QuickActionButton — small icon + label button for quick actions
// =========================================================================
class QuickActionButton : public views::View {
 public:
  using ClickCallback = base::RepeatingClosure;

  QuickActionButton(const std::u16string& label, char16_t icon_char) {
    SetPreferredSize(
        gfx::Size(kQuickActionButtonWidth, kQuickActionButtonHeight));
    SetBackground(views::Background::CreateRoundedRectBackground(
        kQuickActionBgColor, 12));
    SetBorder(views::CreateRoundedRectBorder(
        /*thickness=*/1, /*corner_radius=*/12, kDividerColor));
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
    SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    SetAccessibleName(label);
    SetTooltipText(label);

    auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical,
        gfx::Insets::VH(12, 8),
        /*between_child_spacing=*/6));
    layout->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);
    layout->set_main_axis_alignment(
        views::BoxLayout::MainAxisAlignment::kCenter);

    // Icon label.
    auto icon = std::make_unique<views::Label>(std::u16string(1, icon_char));
    icon->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    icon->SetAutoColorReadabilityEnabled(false);
    icon->SetEnabledColor(kQuickActionTextColor);
    icon->SetFontList(icon->font_list().DeriveWithSizeDelta(6));
    icon->SetPreferredSize(
        gfx::Size(kQuickActionIconSize, kQuickActionIconSize));
    AddChildView(std::move(icon));

    // Text label.
    auto text = std::make_unique<views::Label>(label);
    text->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    text->SetAutoColorReadabilityEnabled(false);
    text->SetEnabledColor(kQuickActionTextColor);
    text->SetFontList(text->font_list().Derive(
        -1, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
    text->SetElideBehavior(gfx::ELIDE_TAIL);
    AddChildView(std::move(text));
  }

  ~QuickActionButton() override = default;

  void SetClickCallback(ClickCallback callback) {
    click_callback_ = std::move(callback);
  }

  // views::View:
  bool OnMousePressed(const ui::MouseEvent& event) override {
    if (event.IsOnlyLeftMouseButton() && click_callback_) {
      click_callback_.Run();
      return true;
    }
    return views::View::OnMousePressed(event);
  }

  void OnMouseEntered(const ui::MouseEvent& event) override {
    is_hovered_ = true;
    SchedulePaint();
    views::View::OnMouseEntered(event);
  }

  void OnMouseExited(const ui::MouseEvent& event) override {
    is_hovered_ = false;
    SchedulePaint();
    views::View::OnMouseExited(event);
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    SkColor bg_color =
        is_hovered_ ? kQuickActionHoverBgColor : kQuickActionBgColor;
    cc::PaintFlags flags;
    flags.setColor(bg_color);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);
    canvas->DrawRoundRect(GetLocalBounds(), 12, flags);
  }

  void OnFocus() override {
    views::View::OnFocus();
    SchedulePaint();
  }

  void OnBlur() override {
    views::View::OnBlur();
    SchedulePaint();
  }

  bool OnKeyPressed(const ui::KeyEvent& event) override {
    if (event.key_code() == ui::VKEY_SPACE ||
        event.key_code() == ui::VKEY_RETURN) {
      if (click_callback_) {
        click_callback_.Run();
      }
      return true;
    }
    return views::View::OnKeyPressed(event);
  }

  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    views::View::GetAccessibleNodeData(node_data);
    node_data->role = ax::mojom::Role::kButton;
  }

 private:
  bool is_hovered_ = false;
  ClickCallback click_callback_;
};

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraNewTabView::AstraNewTabView(Browser* browser)
    : browser_(browser),
      profile_(browser ? browser->profile() : nullptr) {
  DCHECK(browser_);
  BuildLayout();
  RefreshContent();
}

AstraNewTabView::~AstraNewTabView() = default;

void AstraNewTabView::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

void AstraNewTabView::SetModel(AstraNewTabModel* model) {
  model_ = model;
  if (model_) {
    UpdateFromSettings();
  }
}

void AstraNewTabView::RefreshContent() {
  UpdateGreeting();
  UpdateWorkspaceSection();
  UpdateShortcutsSection();
  UpdateRecentlyClosedSection();
  UpdateQuickActionsSection();
}

void AstraNewTabView::UpdateFromSettings() {
  if (!model_) {
    return;
  }

  SetGreetingVisible(model_->show_greeting());
  // Search bar is in the bubble, not in this view.
  SetWorkspaceCardsVisible(model_->show_workspace_cards());
  SetShortcutsVisible(model_->show_shortcuts());
  SetRecentlyClosedVisible(model_->show_recently_closed());
  SetQuickActionsVisible(model_->show_quick_actions());

  // Update shortcut columns if needed.
  int columns = model_->shortcut_columns();
  if (columns != current_shortcut_columns_) {
    RebuildShortcutGrid(columns);
  }

  UpdateResponsiveLayout();
}

// =========================================================================
// Section visibility controls
// =========================================================================

void AstraNewTabView::SetGreetingVisible(bool visible) {
  if (greeting_section_) {
    greeting_section_->SetVisible(visible);
    InvalidateLayout();
  }
}

void AstraNewTabView::SetSearchBarVisible(bool visible) {
  // Search bar is in the bubble, not in this view.
  // TODO(astra): Add search bar to the view if needed.
}

void AstraNewTabView::SetWorkspaceCardsVisible(bool visible) {
  if (workspace_section_) {
    workspace_section_->SetVisible(visible);
    InvalidateLayout();
  }
}

void AstraNewTabView::SetShortcutsVisible(bool visible) {
  if (shortcut_section_) {
    shortcut_section_->SetVisible(visible);
    InvalidateLayout();
  }
}

void AstraNewTabView::SetRecentlyClosedVisible(bool visible) {
  if (recent_section_) {
    recent_section_->SetVisible(visible);
    InvalidateLayout();
  }
}

void AstraNewTabView::SetQuickActionsVisible(bool visible) {
  if (quick_actions_section_) {
    quick_actions_section_->SetVisible(visible);
    InvalidateLayout();
  }
}

// =========================================================================
// Drag and drop
// =========================================================================

void AstraNewTabView::OnShortcutDragStarted(AstraNtpShortcutView* dragged_view) {
  dragged_shortcut_ = dragged_view;
  drag_drop_index_ = -1;
}

void AstraNewTabView::OnShortcutDragMoved(const gfx::Point& screen_point) {
  // TODO(astra): Implement proper drop target highlighting during drag.
  // This would show a visual indicator of where the shortcut would be dropped.
}

void AstraNewTabView::OnShortcutDragEnded(const gfx::Point& screen_point) {
  if (!dragged_shortcut_ || shortcut_views_.empty()) {
    dragged_shortcut_ = nullptr;
    return;
  }

  // Find the from index.
  int from_index = -1;
  for (size_t i = 0; i < shortcut_views_.size(); ++i) {
    if (shortcut_views_[i] == dragged_shortcut_) {
      from_index = static_cast<int>(i);
      break;
    }
  }

  if (from_index < 0) {
    dragged_shortcut_ = nullptr;
    return;
  }

  // Convert screen point to local point in the shortcut grid.
  gfx::Point local_point = screen_point;
  if (shortcut_grid_) {
    views::View::ConvertPointFromScreen(shortcut_grid_, &local_point);
  }

  // Find the drop target index.
  int to_index = -1;
  for (size_t i = 0; i < shortcut_views_.size(); ++i) {
    if (shortcut_views_[i] == dragged_shortcut_) {
      continue;
    }
    if (shortcut_views_[i]->HitTestPoint(
            local_point - shortcut_views_[i]->bounds().OffsetFromOrigin())) {
      to_index = static_cast<int>(i);
      break;
    }
  }

  if (to_index >= 0 && from_index != to_index && delegate_) {
    delegate_->OnShortcutReordered(
        static_cast<size_t>(from_index),
        static_cast<size_t>(to_index));
  }

  dragged_shortcut_ = nullptr;
  drag_drop_index_ = -1;
}

// =========================================================================
// View overrides
// =========================================================================

void AstraNewTabView::OnThemeChanged() {
  views::View::OnThemeChanged();
  // TODO(astra): Update colors from ColorProvider when Astra has its
  // own color mixin.
}

void AstraNewTabView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  views::View::OnBoundsChanged(previous_bounds);
  int new_width = width();
  if (new_width != current_content_width_) {
    current_content_width_ = new_width;
    UpdateResponsiveLayout();
  }
}

bool AstraNewTabView::OnKeyPressed(const ui::KeyEvent& event) {
  // Tab key is handled by the focus manager for normal navigation.
  // Add keyboard shortcuts for common NTP actions.
  if (event.IsControlDown() && event.key_code() == ui::VKEY_K) {
    // Ctrl+K focuses search (handled by the bubble).
    return true;
  }

  // Arrow keys for navigating between shortcut tiles.
  // TODO(astra): Implement arrow key navigation within the shortcut grid.

  return views::View::OnKeyPressed(event);
}

void AstraNewTabView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kPane;
  node_data->SetName("Astra new tab page");
  node_data->AddStringAttribute(
      ax::mojom::StringAttribute::kDescription,
      "New tab page with shortcuts, workspaces, and quick actions");
}

// =========================================================================
// Layout construction
// =========================================================================

void AstraNewTabView::BuildLayout() {
  // Root layout: centered vertical stack of sections.
  auto* root_layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kContentTopPadding, kContentHorizontalPadding,
                      kContentBottomPadding, kContentHorizontalPadding),
      /*between_child_spacing=*/kSectionSpacing));
  root_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  set_background(
      views::Background::CreateSolidBackground(kNtpBackgroundColor));

  // Settings gear button (top-right corner).
  auto gear_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraNewTabView::OnSettingsGearPressed,
                          base::Unretained(this)));
  gear_button->SetPreferredSize(
      gfx::Size(kSettingsGearSize, kSettingsGearSize));
  gear_button->SetTooltipText(u"Customize new tab page");
  gear_button->SetAccessibleName(u"Customize new tab page");
  gear_button->SetHasInkDrop(true);
  settings_gear_button_ = gear_button.get();
  AddChildView(std::move(gear_button));

  // ---- Greeting section ----
  greeting_section_ = AddChildView(std::make_unique<views::View>());
  greeting_section_->SetPreferredSize(gfx::Size(kNtpMaxWidth, 72));
  auto* greeting_layout = greeting_section_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  greeting_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);
  greeting_layout->set_between_child_spacing(4);

  greeting_label_ =
      greeting_section_->AddChildView(std::make_unique<views::Label>());
  greeting_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  greeting_label_->SetAutoColorReadabilityEnabled(false);
  greeting_label_->SetEnabledColor(kGreetingTextColor);
  greeting_label_->SetFontList(greeting_label_->font_list().Derive(
      kGreetingFontSizeDelta, gfx::Font::NORMAL, gfx::Font::Weight::BOLD));

  auto* subtitle_label =
      greeting_section_->AddChildView(std::make_unique<views::Label>());
  subtitle_label->SetText(u"Here's what's going on today");
  subtitle_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  subtitle_label->SetAutoColorReadabilityEnabled(false);
  subtitle_label->SetEnabledColor(kGreetingSubtitleColor);
  subtitle_label->SetFontList(subtitle_label->font_list().Derive(
      kGreetingSubtitleFontSizeDelta, gfx::Font::NORMAL,
      gfx::Font::Weight::NORMAL));

  // ---- Workspace section ----
  workspace_section_ = AddChildView(std::make_unique<views::View>());
  auto* ws_layout = workspace_section_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  ws_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  ws_layout->set_between_child_spacing(kSectionHeaderSpacing);

  workspace_section_->AddChildView(CreateSectionLabel(u"Workspaces"));

  // Workspace cards row (horizontal).
  auto* ws_cards_row =
      workspace_section_->AddChildView(std::make_unique<views::View>());
  auto* ws_cards_layout = ws_cards_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  ws_cards_layout->set_between_child_spacing(kWorkspaceCardSpacing);
  ws_cards_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  ws_cards_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // ---- Shortcuts section ----
  shortcut_section_ = AddChildView(std::make_unique<views::View>());
  auto* sc_layout = shortcut_section_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  sc_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  sc_layout->set_between_child_spacing(kSectionHeaderSpacing);

  shortcut_section_->AddChildView(CreateSectionLabel(u"Shortcuts"));

  // Shortcuts grid container.
  shortcut_grid_ =
      shortcut_section_->AddChildView(std::make_unique<views::View>());
  auto* grid_layout = shortcut_grid_->SetLayoutManager(
      std::make_unique<views::GridLayout>());

  RebuildShortcutGrid(kDefaultShortcutColumns);

  // ---- Recently closed section ----
  recent_section_ = AddChildView(std::make_unique<views::View>());
  auto* recent_layout = recent_section_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  recent_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  recent_layout->set_between_child_spacing(kSectionHeaderSpacing);

  recent_section_->AddChildView(CreateSectionLabel(u"Recently closed"));

  // Horizontally scrollable recently closed items row.
  recent_scroll_view_ =
      recent_section_->AddChildView(std::make_unique<views::ScrollView>());
  recent_scroll_view_->SetBackgroundColor(SK_ColorTRANSPARENT);
  recent_scroll_view_->SetPaintToLayer();
  recent_scroll_view_->layer()->SetFillsBoundsOpaquely(false);
  recent_scroll_view_->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);
  recent_scroll_view_->SetVerticalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);

  // Items row inside the scroll view.
  auto items_row = std::make_unique<views::View>();
  auto* items_layout = items_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  items_layout->set_between_child_spacing(kRecentlyClosedItemSpacing);
  items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  items_row->SetPaintToLayer();
  items_row->layer()->SetFillsBoundsOpaquely(false);

  // Create placeholder recently closed items.
  for (int i = 0; i < kRecentlyClosedItemCount; ++i) {
    auto item = std::make_unique<views::View>();
    item->SetPreferredSize(
        gfx::Size(kRecentlyClosedItemWidth, kRecentlyClosedItemHeight));
    item->SetBackground(views::Background::CreateRoundedRectBackground(
        kSectionCardBackgroundColor, 8));
    item->SetBorder(views::CreateRoundedRectBorder(
        /*thickness=*/1, /*corner_radius=*/8, kDividerColor));
    item->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    item->SetAccessibleName(u"Recently closed tab placeholder");
    items_row->AddChildView(std::move(item));
  }

  recent_items_row_ = recent_scroll_view_->SetContents(std::move(items_row));

  // ---- Quick actions section ----
  quick_actions_section_ = AddChildView(std::make_unique<views::View>());
  auto* qa_layout = quick_actions_section_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  qa_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  qa_layout->set_between_child_spacing(kSectionHeaderSpacing);

  quick_actions_section_->AddChildView(CreateSectionLabel(u"Quick actions"));

  // Quick actions row.
  auto* qa_row =
      quick_actions_section_->AddChildView(std::make_unique<views::View>());
  auto* qa_row_layout = qa_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  qa_row_layout->set_between_child_spacing(kQuickActionSpacing);
  qa_row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  qa_row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Create quick action buttons.
  qa_row->AddChildView(CreateQuickActionButton(
      kActionNewWorkspace, u"New workspace", kQuickActionNewWorkspaceIcon));
  qa_row->AddChildView(CreateQuickActionButton(
      kActionScreenshot, u"Screenshot", kQuickActionScreenshotIcon));
  qa_row->AddChildView(CreateQuickActionButton(
      kActionFocusMode, u"Focus mode", kQuickActionFocusModeIcon));
  qa_row->AddChildView(CreateQuickActionButton(
      kActionHistory, u"History", kQuickActionHistoryIcon));
  qa_row->AddChildView(CreateQuickActionButton(
      kActionDownloads, u"Downloads", kQuickActionDownloadsIcon));
  qa_row->AddChildView(CreateQuickActionButton(
      kActionBookmarks, u"Bookmarks", kQuickActionBookmarksIcon));
}

// =========================================================================
// Shortcut grid
// =========================================================================

void AstraNewTabView::RebuildShortcutGrid(int columns) {
  if (!shortcut_grid_) {
    return;
  }

  // Clamp columns to valid range.
  columns = std::max(kMinShortcutColumns,
                     std::min(kMaxShortcutColumns, columns));
  current_shortcut_columns_ = columns;

  // Clear existing shortcuts.
  shortcut_views_.clear();
  shortcut_grid_->RemoveAllChildViews();

  // Recreate the GridLayout with the new column count.
  auto grid_layout = std::make_unique<views::GridLayout>();
  views::GridLayout* grid = grid_layout.get();
  shortcut_grid_->SetLayoutManager(std::move(grid_layout));

  // Build grid columns.
  views::ColumnSet* col_set = grid->AddColumnSet(0);
  for (int col = 0; col < columns; ++col) {
    if (col > 0) {
      col_set->AddPaddingColumn(0, kShortcutColumnSpacing);
    }
    col_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                       1.0, views::GridLayout::ColumnSize::kUsePreferred,
                       0, 0);
  }

  // Create shortcut tiles (placeholder count based on columns * 2).
  int count = columns * 2;
  for (int i = 0; i < count; ++i) {
    int col = i % columns;
    int row = i / columns;
    if (col == 0) {
      if (row > 0) {
        grid->AddPaddingRow(0, kShortcutRowSpacing);
      }
      grid->StartRow(views::GridLayout::kFixedSize, 0);
    }

    auto shortcut = std::make_unique<AstraNtpShortcutView>();
    shortcut->SetClickCallback(base::BindRepeating(
        &AstraNewTabView::OnShortcutClicked, weak_factory_.GetWeakPtr()));
    shortcut->SetRemoveCallback(base::BindRepeating(
        &AstraNewTabView::OnShortcutRemove, weak_factory_.GetWeakPtr()));
    shortcut->SetContextMenuCallback(base::BindRepeating(
        &AstraNewTabView::OnShortcutContextMenu, weak_factory_.GetWeakPtr()));
    shortcut->SetDragStartCallback(base::BindRepeating(
        &AstraNewTabView::OnShortcutDragStart, weak_factory_.GetWeakPtr()));
    shortcut->SetShowDragHandle(true);
    shortcut->SetVisible(false);  // Hidden until data is populated

    raw_ptr<AstraNtpShortcutView> ptr =
        grid->AddView(std::move(shortcut));
    shortcut_views_.push_back(ptr);
  }

  shortcut_grid_->InvalidateLayout();
}

int AstraNewTabView::GetCurrentShortcutColumns() const {
  // If model is available, use the model's setting.
  if (model_) {
    return model_->shortcut_columns();
  }
  // Otherwise, calculate from width for responsive layout.
  int available_width = width() - kContentHorizontalPadding * 2;
  int tile_width = 96 + kShortcutColumnSpacing;  // tile + spacing
  int columns = available_width / tile_width;
  return std::max(kMinShortcutColumns,
                  std::min(kMaxShortcutColumns, columns));
}

// =========================================================================
// Section updates
// =========================================================================

void AstraNewTabView::UpdateGreeting() {
  if (!greeting_label_) {
    return;
  }

  // Determine greeting based on time of day.
  base::Time::Exploded now;
  base::Time::Now().LocalExplode(&now);

  std::u16string greeting;
  if (now.hour < 5) {
    greeting = u"Good night";
  } else if (now.hour < 12) {
    greeting = u"Good morning";
  } else if (now.hour < 18) {
    greeting = u"Good afternoon";
  } else {
    greeting = u"Good evening";
  }

  // Append profile name or just a general greeting.
  greeting += u" — welcome to Astra";

  greeting_label_->SetText(greeting);
}

void AstraNewTabView::UpdateWorkspaceSection() {
  if (!profile_) {
    return;
  }

  // Find the workspace cards row container.
  if (workspace_section_->children().size() < 2) {
    return;
  }
  views::View* cards_row = workspace_section_->children()[1];
  if (!cards_row) {
    return;
  }

  // Remove all existing card views.
  workspace_card_views_.clear();
  cards_row->RemoveAllChildViews();

  // If we have a model, use it; otherwise, create placeholder cards.
  size_t count = kMaxWorkspaceCards;
  if (model_) {
    count = std::min(static_cast<size_t>(model_->max_workspaces_shown()),
                     model_->GetWorkspaceCardCount());
  }

  // Add workspace cards.
  for (size_t i = 0; i < count; ++i) {
    auto card = std::make_unique<AstraNtpWorkspaceCard>();
    if (model_ && model_->GetWorkspaceCardAt(i)) {
      const auto* info = model_->GetWorkspaceCardAt(i);
      card->SetWorkspaceId(info->id);
      card->SetWorkspaceName(info->name);
      card->SetAccentColor(info->accent_color_hex);
      card->SetTabCount(info->tab_count);
      card->SetIsActive(info->is_active);
    } else {
      // Placeholder card.
      card->SetWorkspaceId(base::NumberToString(i));
      card->SetWorkspaceName(base::UTF8ToUTF16("Workspace " +
                                               base::NumberToString(i + 1)));
      card->SetTabCount(static_cast<int>(i) * 3);
    }
    card->SetClickCallback(base::BindRepeating(
        &AstraNewTabView::OnWorkspaceCardClicked,
        weak_factory_.GetWeakPtr()));
    card->SetMenuCallback(base::BindRepeating(
        &AstraNewTabView::OnWorkspaceCardMenu,
        weak_factory_.GetWeakPtr()));
    card->SetShowDragHandle(true);
    raw_ptr<AstraNtpWorkspaceCard> ptr =
        cards_row->AddChildView(std::move(card));
    workspace_card_views_.push_back(ptr);
  }

  // Add "New workspace" card at the end.
  auto new_card = std::make_unique<AstraNtpWorkspaceCard>();
  new_card->SetIsNewWorkspaceCard(true);
  new_card->SetWorkspaceId(std::string());
  new_card->SetClickCallback(base::BindRepeating(
      &AstraNewTabView::OnWorkspaceCardClicked,
      weak_factory_.GetWeakPtr()));
  raw_ptr<AstraNtpWorkspaceCard> ptr =
      cards_row->AddChildView(std::move(new_card));
  workspace_card_views_.push_back(ptr);
}

void AstraNewTabView::UpdateShortcutsSection() {
  if (!profile_) {
    return;
  }

  // Update each shortcut view with data from the model, or hide it.
  size_t shortcut_count = shortcut_views_.size();
  if (model_) {
    shortcut_count = std::min(
        shortcut_count, model_->GetShortcutCount());
  }

  for (size_t i = 0; i < shortcut_views_.size(); ++i) {
    if (i < shortcut_count) {
      shortcut_views_[i]->SetVisible(true);
      if (model_ && model_->GetShortcutAt(i)) {
        const auto* info = model_->GetShortcutAt(i);
        shortcut_views_[i]->SetTitle(info->title);
        shortcut_views_[i]->SetURL(info->url);
        shortcut_views_[i]->SetIconURL(info->icon_url);
      } else {
        // Placeholder data.
        shortcut_views_[i]->SetTitle(
            base::UTF8ToUTF16("Site " + base::NumberToString(i + 1)));
        shortcut_views_[i]->SetURL(
            GURL("https://example" + base::NumberToString(i + 1) + ".com"));
      }
    } else {
      shortcut_views_[i]->SetVisible(false);
    }
  }
}

void AstraNewTabView::UpdateRecentlyClosedSection() {
  // Recently closed items are placeholder views created in BuildLayout().
  // In production, this would populate with real data from TabRestoreService.
  if (!profile_) {
    return;
  }
}

void AstraNewTabView::UpdateQuickActionsSection() {
  // Quick action buttons are static — created once in BuildLayout().
}

void AstraNewTabView::UpdateResponsiveLayout() {
  // TODO(astra): Implement proper responsive layout that adapts to width.
  // For now, just invalidate layout to trigger relayout.
  InvalidateLayout();
}

// =========================================================================
// Helpers
// =========================================================================

std::unique_ptr<views::Label> AstraNewTabView::CreateSectionLabel(
    const std::u16string& text) {
  auto label = std::make_unique<views::Label>(text);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColor(kSectionHeaderTextColor);
  label->SetFontList(label->font_list().Derive(
      kSectionHeaderFontSizeDelta, gfx::Font::NORMAL,
      gfx::Font::Weight::SEMIBOLD));
  return label;
}

std::unique_ptr<views::View> AstraNewTabView::CreateQuickActionButton(
    const std::string& action_id,
    const std::u16string& label,
    char16_t icon_char) {
  auto button = std::make_unique<QuickActionButton>(label, icon_char);
  button->SetClickCallback(base::BindRepeating(
      &AstraNewTabView::OnQuickActionButton, weak_factory_.GetWeakPtr(),
      action_id));
  quick_action_buttons_.push_back(button.get());
  return button;
}

// =========================================================================
// Action callbacks
// =========================================================================

void AstraNewTabView::OnShortcutClicked(const GURL& url) {
  if (delegate_) {
    delegate_->OnNavigateToURL(url);
  }
}

void AstraNewTabView::OnShortcutRemove(const GURL& url) {
  if (delegate_) {
    delegate_->OnRemoveShortcut(url);
  }
}

void AstraNewTabView::OnShortcutContextMenu(const GURL& url,
                                            const gfx::Point& screen_point) {
  if (delegate_) {
    delegate_->OnShowShortcutContextMenu(url, screen_point);
  }
}

void AstraNewTabView::OnShortcutDragStart(AstraNtpShortcutView* view) {
  OnShortcutDragStarted(view);
}

void AstraNewTabView::OnWorkspaceCardClicked(
    const std::string& workspace_id) {
  if (!delegate_) {
    return;
  }

  if (workspace_id.empty()) {
    delegate_->OnNewWorkspace();
  } else {
    delegate_->OnOpenWorkspace(workspace_id);
  }
}

void AstraNewTabView::OnWorkspaceCardMenu(const std::string& workspace_id,
                                          const gfx::Point& screen_point) {
  if (delegate_ && !workspace_id.empty()) {
    delegate_->OnShowWorkspaceContextMenu(workspace_id, screen_point);
  }
}

void AstraNewTabView::OnQuickActionButton(const std::string& action_id) {
  if (delegate_) {
    delegate_->OnQuickAction(action_id);
  }
}

void AstraNewTabView::OnRecentlyClosedClicked(int session_id) {
  if (delegate_) {
    delegate_->OnRestoreRecentlyClosed(session_id);
  }
}

void AstraNewTabView::OnSettingsGearPressed() {
  if (delegate_) {
    delegate_->OnSettingsGearPressed();
  }
}

}  // namespace astra
