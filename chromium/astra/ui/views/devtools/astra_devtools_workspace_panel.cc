#include "astra/ui/views/devtools/astra_devtools_workspace_panel.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/views/devtools/astra_devtools_model.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Panel padding.
constexpr int kPanelPadding = 12;

// Spacing between sections.
constexpr int kSectionSpacing = 16;

// Spacing between rows in a section.
constexpr int kRowSpacing = 4;

// Spacing between action buttons.
constexpr int kButtonSpacing = 6;

// Font size delta for section headers.
constexpr int kSectionHeaderFontDelta = 2;

// Height of workspace list items (cards).
constexpr int kWorkspaceItemHeight = 56;

// Height of tab list items.
constexpr int kTabItemHeight = 24;

// Workspace card padding.
constexpr int kWorkspaceCardPadding = 8;

// Dark theme colors.
constexpr SkColor kDarkPanelBg = SkColorSetRGB(0x20, 0x20, 0x20);
constexpr SkColor kDarkSectionHeader = SK_ColorWHITE;
constexpr SkColor kDarkKeyText = SkColorSetRGB(0x9A, 0x9A, 0x9A);
constexpr SkColor kDarkValueText = SK_ColorWHITE;
constexpr SkColor kDarkMonoText = SkColorSetRGB(0xCE, 0xCE, 0xCE);
constexpr SkColor kDarkItemBg = SkColorSetRGB(0x2A, 0x2A, 0x2A);
constexpr SkColor kDarkItemHoverBg = SkColorSetRGB(0x3A, 0x3A, 0x3A);
constexpr SkColor kDarkItemSelectedBg = SkColorSetRGB(0x1A, 0x5A, 0x9A);
constexpr SkColor kDarkButtonBg = SkColorSetRGB(0x3A, 0x3A, 0x3A);
constexpr SkColor kDarkBorder = SkColorSetRGB(0x44, 0x44, 0x44);
constexpr SkColor kDarkStatsText = SkColorSetRGB(0x9A, 0x9A, 0x9A);

// Light theme colors.
constexpr SkColor kLightPanelBg = SkColorSetRGB(0xFA, 0xFA, 0xFA);
constexpr SkColor kLightSectionHeader = SkColorSetRGB(0x20, 0x20, 0x20);
constexpr SkColor kLightKeyText = SkColorSetRGB(0x66, 0x66, 0x66);
constexpr SkColor kLightValueText = SkColorSetRGB(0x20, 0x20, 0x20);
constexpr SkColor kLightMonoText = SkColorSetRGB(0x44, 0x44, 0x44);
constexpr SkColor kLightItemBg = SK_ColorWHITE;
constexpr SkColor kLightItemHoverBg = SkColorSetRGB(0xF0, 0xF0, 0xF0);
constexpr SkColor kLightItemSelectedBg = SkColorSetRGB(0xC8, 0xE0, 0xFC);
constexpr SkColor kLightButtonBg = SkColorSetRGB(0xE8, 0xE8, 0xE8);
constexpr SkColor kLightBorder = SkColorSetRGB(0xDD, 0xDD, 0xDD);
constexpr SkColor kLightStatsText = SkColorSetRGB(0x66, 0x66, 0x66);

// Helper: returns the color for a given theme.
SkColor ThemeColor(bool dark, SkColor dark_color, SkColor light_color) {
  return dark ? dark_color : light_color;
}

// Helper: get accent color for a workspace info.
SkColor GetWorkspaceAccentColor(const AstraWorkspaceInfo& info) {
  if (info.accent_color == AstraWorkspaceAccentColor::kCustom) {
    return info.custom_color;
  }
  switch (info.color) {
    case AstraWorkspaceColor::kBlue:
      return SkColorSetRGB(0x1A, 0x73, 0xE8);
    case AstraWorkspaceColor::kRed:
      return SkColorSetRGB(0xEA, 0x43, 0x35);
    case AstraWorkspaceColor::kGreen:
      return SkColorSetRGB(0x34, 0xA8, 0x53);
    case AstraWorkspaceColor::kYellow:
      return SkColorSetRGB(0xFB, 0xBC, 0x04);
    case AstraWorkspaceColor::kPurple:
      return SkColorSetRGB(0xA1, 0x42, 0xF4);
    case AstraWorkspaceColor::kPink:
      return SkColorSetRGB(0xEC, 0x40, 0x7A);
    case AstraWorkspaceColor::kCyan:
      return SkColorSetRGB(0x00, 0xBC, 0xD4);
    case AstraWorkspaceColor::kOrange:
      return SkColorSetRGB(0xFA, 0x90, 0x2F);
    default:
      return SkColorSetRGB(0x1A, 0x73, 0xE8);
  }
}

}  // namespace

// ---------------------------------------------------------------------------
// AstraDevToolsWorkspacePanel — construction
// ---------------------------------------------------------------------------

AstraDevToolsWorkspacePanel::AstraDevToolsWorkspacePanel() = default;

AstraDevToolsWorkspacePanel::~AstraDevToolsWorkspacePanel() = default;

// ---------------------------------------------------------------------------
// Build UI
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::BuildPanel() {
  // Background.
  SetBackground(views::CreateSolidBackground(
      ThemeColor(dark_theme_, kDarkPanelBg, kLightPanelBg)));

  // Vertical layout: sections stacked vertically.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(kPanelPadding),
      kSectionSpacing));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // --- Search box section.

  search_box_ = AddChildView(std::make_unique<views::Textfield>());
  search_box_->set_placeholder_text(u"Search workspaces and tabs...");
  search_box_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  search_box_->set_controller(
      base::BindRepeating(&AstraDevToolsWorkspacePanel::SetSearchQuery,
                          base::Unretained(this)));

  // --- Stats section.

  stats_label_ = AddChildView(std::make_unique<views::Label>());
  stats_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  stats_label_->SetEnabledColor(
      ThemeColor(dark_theme_, kDarkStatsText, kLightStatsText));
  stats_label_->SetFontList(stats_label_->font_list().Derive(
      -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));

  // --- Quick actions row.

  quick_actions_container_ = AddChildView(std::make_unique<views::View>());
  auto* actions_layout = quick_actions_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          kButtonSpacing));
  actions_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  actions_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* merge_button = quick_actions_container_->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraDevToolsWorkspacePanel::OnMergeWorkspacesButton,
              base::Unretained(this)),
          u"Merge"));
  merge_button->SetMinSize(gfx::Size(60, 24));
  merge_button->SetFocusForPlatform();

  auto* import_button = quick_actions_container_->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraDevToolsWorkspacePanel::OnImportWorkspacesButton,
              base::Unretained(this)),
          u"Import"));
  import_button->SetMinSize(gfx::Size(60, 24));
  import_button->SetFocusForPlatform();

  auto* export_button = quick_actions_container_->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraDevToolsWorkspacePanel::OnExportWorkspacesButton,
              base::Unretained(this)),
          u"Export"));
  export_button->SetMinSize(gfx::Size(60, 24));
  export_button->SetFocusForPlatform();

  // --- Workspace info section.

  auto* workspace_section = AddChildView(std::make_unique<views::View>());
  auto* workspace_layout =
      workspace_section->SetLayoutManager(
          std::make_unique<views::BoxLayout>(
              views::BoxLayout::Orientation::kVertical,
              gfx::Insets(),
              kRowSpacing));
  workspace_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  auto* header = AddSectionLabel(u"Current Workspace");
  workspace_section->AddChildView(std::unique_ptr<views::View>(header));

  workspace_name_label_ = workspace_section->AddChildView(
      std::make_unique<views::Label>());
  workspace_name_label_->SetEnabledColor(
      ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
  workspace_name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  workspace_id_label_ = workspace_section->AddChildView(
      std::make_unique<views::Label>());
  workspace_id_label_->SetEnabledColor(
      ThemeColor(dark_theme_, kDarkMonoText, kLightMonoText));
  workspace_id_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  workspace_color_label_ = workspace_section->AddChildView(
      std::make_unique<views::Label>());
  workspace_color_label_->SetEnabledColor(
      ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
  workspace_color_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // --- Workspace list section with action buttons.

  auto* list_section = AddChildView(std::make_unique<views::View>());
  auto* list_layout = list_section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(),
          kRowSpacing));
  list_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  auto* list_header_row = list_section->AddChildView(
      std::make_unique<views::View>());
  auto* header_row_layout = list_header_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          kButtonSpacing));
  header_row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  header_row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* list_header = AddSectionLabel(u"Workspaces");
  list_header_row->AddChildView(std::unique_ptr<views::View>(list_header));

  // Spacer to push buttons to the right.
  auto* spacer = list_header_row->AddChildView(
      std::make_unique<views::View>());
  header_row_layout->SetFlexForView(spacer, 1);

  new_workspace_button_ = list_header_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraDevToolsWorkspacePanel::OnNewWorkspaceButton,
              base::Unretained(this)),
          u"+ New"));
  new_workspace_button_->SetMinSize(gfx::Size(60, 24));
  new_workspace_button_->SetFocusForPlatform();

  rename_workspace_button_ = list_header_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraDevToolsWorkspacePanel::OnRenameWorkspaceButton,
              base::Unretained(this)),
          u"Rename"));
  rename_workspace_button_->SetMinSize(gfx::Size(70, 24));
  rename_workspace_button_->SetFocusForPlatform();

  delete_workspace_button_ = list_header_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraDevToolsWorkspacePanel::OnDeleteWorkspaceButton,
              base::Unretained(this)),
          u"Delete"));
  delete_workspace_button_->SetMinSize(gfx::Size(60, 24));
  delete_workspace_button_->SetFocusForPlatform();

  // Scrollable container for workspace list.
  workspace_scroll_view_ = list_section->AddChildView(
      std::make_unique<views::ScrollView>());
  workspace_scroll_view_->SetClipToBounds(true);
  workspace_scroll_view_->SetBackgroundColor(
      ThemeColor(dark_theme_, kDarkPanelBg, kLightPanelBg));
  workspace_scroll_view_->SetPreferredSize(gfx::Size(0, 200));
  workspace_scroll_view_->SetBorder(views::CreateSolidBorder(
      1, ThemeColor(dark_theme_, kDarkBorder, kLightBorder)));

  workspace_list_container_ = workspace_scroll_view_->SetContents(
      std::make_unique<views::View>());
  workspace_list_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(),
          kRowSpacing));

  // --- Tab list section.

  auto* tab_section = AddChildView(std::make_unique<views::View>());
  auto* tab_layout = tab_section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(),
          kRowSpacing));
  tab_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  auto* tab_header = AddSectionLabel(u"Tabs in Selected Workspace");
  tab_section->AddChildView(std::unique_ptr<views::View>(tab_header));

  tab_scroll_view_ = tab_section->AddChildView(
      std::make_unique<views::ScrollView>());
  tab_scroll_view_->SetClipToBounds(true);
  tab_scroll_view_->SetBackgroundColor(
      ThemeColor(dark_theme_, kDarkPanelBg, kLightPanelBg));
  tab_scroll_view_->SetPreferredSize(gfx::Size(0, 140));
  tab_scroll_view_->SetBorder(views::CreateSolidBorder(
      1, ThemeColor(dark_theme_, kDarkBorder, kLightBorder)));

  tab_list_container_ = tab_scroll_view_->SetContents(
      std::make_unique<views::View>());
  tab_list_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(),
          0));

  // --- Tab metadata section.

  auto* tab_meta_section = AddChildView(std::make_unique<views::View>());
  auto* tab_meta_layout = tab_meta_section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(),
          kRowSpacing));
  tab_meta_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  auto* tab_meta_header = AddSectionLabel(u"Tab Astra Metadata");
  tab_meta_section->AddChildView(std::unique_ptr<views::View>(tab_meta_header));

  tab_metadata_label_ = tab_meta_section->AddChildView(
      std::make_unique<views::Label>());
  tab_metadata_label_->SetEnabledColor(
      ThemeColor(dark_theme_, kDarkMonoText, kLightMonoText));
  tab_metadata_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tab_metadata_label_->SetMultiLine(true);
  tab_metadata_label_->SetAllowCharacterBreak(true);
}

views::Label* AstraDevToolsWorkspacePanel::AddSectionLabel(
    const std::u16string& text) {
  auto* label = new views::Label(text);
  label->SetEnabledColor(
      ThemeColor(dark_theme_, kDarkSectionHeader, kLightSectionHeader));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  gfx::FontList font_list = label->font_list();
  label->SetFontList(font_list.Derive(
      kSectionHeaderFontDelta, gfx::Font::FontStyle::NORMAL,
      gfx::Font::Weight::BOLD));
  return label;
}

void AstraDevToolsWorkspacePanel::AddKeyValueRow(
    views::View* container,
    const std::string& key,
    const std::string& value) {
  // TODO(astra): Implement proper two-column key-value rows.
}

void AstraDevToolsWorkspacePanel::AddWorkspaceCard(
    const AstraWorkspaceInfo& info, int index) {
  auto* card = workspace_list_container_->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraDevToolsWorkspacePanel::OnWorkspaceItemClicked,
              base::Unretained(this), info.id),
          info.name));
  card->SetMinSize(gfx::Size(0, kWorkspaceItemHeight));
  card->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  card->SetBorder(views::CreateEmptyBorder(
      gfx::Insets(kWorkspaceCardPadding)));
  card->SetFocusForPlatform();
  card->SetTooltipText(info.name);

  SkColor accent = GetWorkspaceAccentColor(info);
  SkColor text_color = ThemeColor(dark_theme_, kDarkValueText, kLightValueText);
  SkColor secondary_color =
      ThemeColor(dark_theme_, kDarkKeyText, kLightKeyText);

  card->SetTextColor(views::Button::STATE_NORMAL, text_color);
  card->SetTextColor(views::Button::STATE_HOVERED, text_color);
  card->SetTextColor(views::Button::STATE_PRESSED, text_color);

  // Set background based on selection state.
  bool is_selected = (index == selected_index_);
  if (is_selected) {
    card->SetBackground(views::CreateSolidBackground(
        ThemeColor(dark_theme_,
                   kDarkItemSelectedBg, kLightItemSelectedBg)));
  } else {
    card->SetBackground(views::CreateSolidBackground(
        ThemeColor(dark_theme_, kDarkItemBg, kLightItemBg)));
  }

  // Set border color based on accent color.
  card->SetBorder(views::CreateSolidSidedBorder(
      0, 4, 0, 0, accent));
}

// ---------------------------------------------------------------------------
// Model integration
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::SetModel(AstraDevToolsModel* model) {
  if (model_ == model) {
    return;
  }
  model_ = model;

  if (workspace_name_label_) {
    Refresh();
  }
}

// ---------------------------------------------------------------------------
// Workspace list management (model-driven)
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::SetWorkspaces(
    const std::vector<AstraWorkspaceInfo>& workspaces) {
  workspaces_ = workspaces;

  // If selected index is out of range, reset it.
  if (selected_index_ >= 0 &&
      static_cast<size_t>(selected_index_) >= workspaces_.size()) {
    selected_index_ = workspaces_.empty() ? -1 : 0;
  }

  // Also keep legacy selected_workspace_id_ in sync.
  if (selected_index_ >= 0 &&
      static_cast<size_t>(selected_index_) < workspaces_.size()) {
    selected_workspace_id_ = workspaces_[selected_index_].id;
  } else {
    selected_workspace_id_.clear();
  }

  if (workspace_list_container_) {
    RefreshWorkspaceList();
    RefreshStats();
    RefreshTabList();
  }
}

const AstraWorkspaceInfo* AstraDevToolsWorkspacePanel::GetWorkspaceAt(
    int index) const {
  if (index < 0 || static_cast<size_t>(index) >= workspaces_.size()) {
    return nullptr;
  }
  return &workspaces_[static_cast<size_t>(index)];
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::SelectWorkspace(int index) {
  if (index < -1 || static_cast<size_t>(index) >= workspaces_.size()) {
    return;
  }

  if (selected_index_ == index) {
    return;
  }

  selected_index_ = index;

  // Sync with legacy selected ID.
  if (index >= 0) {
    selected_workspace_id_ = workspaces_[index].id;
  } else {
    selected_workspace_id_.clear();
  }

  if (workspace_list_container_) {
    RefreshWorkspaceList();
    RefreshTabList();
  }

  if (delegate_ && index >= 0) {
    delegate_->OnWorkspaceSelected(workspaces_[index].id);
  }
}

// ---------------------------------------------------------------------------
// Workspace operations
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::NewWorkspace() {
  // Create a new workspace info with a default name.
  int new_index = static_cast<int>(workspaces_.size());
  std::string new_id = "workspace-" + std::to_string(new_index + 1);

  AstraWorkspaceInfo info;
  info.id = new_id;
  info.name = u"New Workspace " + base::NumberToString16(new_index + 1);
  info.order_index = new_index;
  info.color = static_cast<AstraWorkspaceColor>(
      new_index % static_cast<int>(AstraWorkspaceColor::kMaxValue) + 1);
  info.tab_count = 0;
  info.window_count = 1;

  workspaces_.push_back(info);

  // Select the new workspace.
  SelectWorkspace(new_index);

  if (delegate_) {
    delegate_->OnNewWorkspace();
  }

  if (workspace_list_container_) {
    RefreshWorkspaceList();
    RefreshStats();
  }
}

void AstraDevToolsWorkspacePanel::DeleteWorkspace(int index) {
  if (index < 0 || static_cast<size_t>(index) >= workspaces_.size()) {
    return;
  }

  std::string deleted_id = workspaces_[index].id;
  workspaces_.erase(workspaces_.begin() + index);

  // Update selected index.
  if (workspaces_.empty()) {
    selected_index_ = -1;
    selected_workspace_id_.clear();
  } else if (selected_index_ >= static_cast<int>(workspaces_.size())) {
    selected_index_ = static_cast<int>(workspaces_.size()) - 1;
    selected_workspace_id_ = workspaces_[selected_index_].id;
  } else if (selected_index_ > index) {
    selected_index_--;
  }

  // Renormalize order indices.
  for (size_t i = 0; i < workspaces_.size(); ++i) {
    workspaces_[i].order_index = i;
  }

  if (delegate_) {
    delegate_->OnDeleteWorkspace(deleted_id);
  }

  if (workspace_list_container_) {
    RefreshWorkspaceList();
    RefreshStats();
    RefreshTabList();
  }
}

void AstraDevToolsWorkspacePanel::RenameWorkspace(int index,
                                                  const std::u16string& new_name) {
  if (index < 0 || static_cast<size_t>(index) >= workspaces_.size()) {
    return;
  }

  if (workspaces_[index].name == new_name) {
    return;
  }

  workspaces_[index].name = new_name;

  if (delegate_) {
    delegate_->OnRenameWorkspace(
        workspaces_[index].id, base::UTF16ToUTF8(new_name));
  }

  if (workspace_list_container_) {
    RefreshWorkspaceList();
  }
}

void AstraDevToolsWorkspacePanel::SetWorkspaceColor(int index, SkColor color) {
  if (index < 0 || static_cast<size_t>(index) >= workspaces_.size()) {
    return;
  }

  workspaces_[index].accent_color = AstraWorkspaceAccentColor::kCustom;
  workspaces_[index].custom_color = color;

  if (delegate_) {
    delegate_->OnWorkspaceColorChanged(workspaces_[index].id, color);
  }

  if (workspace_list_container_) {
    RefreshWorkspaceList();
  }
}

// ---------------------------------------------------------------------------
// Workspace stats
// ---------------------------------------------------------------------------

int AstraDevToolsWorkspacePanel::GetTabCountForWorkspace(int index) const {
  const auto* ws = GetWorkspaceAt(index);
  if (!ws) {
    return 0;
  }
  return ws->tab_count;
}

int AstraDevToolsWorkspacePanel::GetWindowCountForWorkspace(int index) const {
  const auto* ws = GetWorkspaceAt(index);
  if (!ws) {
    return 0;
  }
  return ws->window_count;
}

// ---------------------------------------------------------------------------
// New workspace button visibility
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::ShowNewWorkspaceButton(bool show) {
  new_workspace_button_visible_ = show;
  if (new_workspace_button_) {
    new_workspace_button_->SetVisible(show);
  }
}

bool AstraDevToolsWorkspacePanel::IsNewWorkspaceButtonVisible() const {
  if (!new_workspace_button_) {
    return new_workspace_button_visible_;
  }
  return new_workspace_button_->GetVisible();
}

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::SetSearchQuery(
    const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;

  // Also update legacy search filter.
  search_filter_ = query;

  if (workspace_list_container_) {
    RefreshWorkspaceList();
    RefreshTabList();
  }
}

void AstraDevToolsWorkspacePanel::ShowSearch(bool show) {
  search_visible_ = show;
  if (search_box_) {
    search_box_->SetVisible(show);
  }
}

bool AstraDevToolsWorkspacePanel::IsSearchVisible() const {
  if (!search_box_) {
    return search_visible_;
  }
  return search_box_->GetVisible();
}

// ---------------------------------------------------------------------------
// Public API (legacy compatibility)
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::SetInspectedWebContents(
    content::WebContents* web_contents) {
  if (inspected_contents_ == web_contents) {
    return;
  }
  inspected_contents_ = web_contents;

  if (workspace_name_label_) {
    RefreshTabMetadata();
    RefreshTabList();
  }
}

void AstraDevToolsWorkspacePanel::SetWorkspaceService(
    AstraWorkspaceService* service) {
  if (workspace_service_ == service) {
    return;
  }
  workspace_service_ = service;

  if (workspace_name_label_) {
    RefreshWorkspaceInfo();
    RefreshWorkspaceList();
    RefreshTabList();
  }
}

void AstraDevToolsWorkspacePanel::Refresh() {
  if (!workspace_name_label_) {
    BuildPanel();
    ApplyTheme();
    // Apply initial visibility states.
    ShowSearch(search_visible_);
    ShowNewWorkspaceButton(new_workspace_button_visible_);
  }

  RefreshWorkspaceInfo();
  RefreshWorkspaceList();
  RefreshTabList();
  RefreshTabMetadata();
  RefreshStats();
}

void AstraDevToolsWorkspacePanel::SetSearchFilter(
    const std::u16string& filter) {
  // Update both new and legacy search.
  SetSearchQuery(filter);
}

void AstraDevToolsWorkspacePanel::SetTheme(bool dark_theme) {
  if (dark_theme_ == dark_theme) {
    return;
  }
  dark_theme_ = dark_theme;
  if (workspace_name_label_) {
    ApplyTheme();
  }
}

// ---------------------------------------------------------------------------
// Theme application
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::ApplyTheme() {
  SkColor bg = ThemeColor(dark_theme_, kDarkPanelBg, kLightPanelBg);
  SkColor value = ThemeColor(dark_theme_, kDarkValueText, kLightValueText);
  SkColor mono = ThemeColor(dark_theme_, kDarkMonoText, kLightMonoText);
  SkColor border = ThemeColor(dark_theme_, kDarkBorder, kLightBorder);
  SkColor button_bg = ThemeColor(dark_theme_, kDarkButtonBg, kLightButtonBg);
  SkColor stats_color = ThemeColor(dark_theme_, kDarkStatsText, kLightStatsText);

  SetBackground(views::CreateSolidBackground(bg));

  if (stats_label_) {
    stats_label_->SetEnabledColor(stats_color);
  }

  if (workspace_name_label_) {
    workspace_name_label_->SetEnabledColor(value);
  }
  if (workspace_id_label_) {
    workspace_id_label_->SetEnabledColor(mono);
  }
  if (workspace_color_label_) {
    workspace_color_label_->SetEnabledColor(value);
  }
  if (tab_metadata_label_) {
    tab_metadata_label_->SetEnabledColor(mono);
  }
  if (workspace_scroll_view_) {
    workspace_scroll_view_->SetBackgroundColor(bg);
    workspace_scroll_view_->SetBorder(
        views::CreateSolidBorder(1, border));
  }
  if (tab_scroll_view_) {
    tab_scroll_view_->SetBackgroundColor(bg);
    tab_scroll_view_->SetBorder(
        views::CreateSolidBorder(1, border));
  }

  // Update action buttons.
  auto style_button = [&](views::LabelButton* btn) {
    if (!btn) return;
    btn->SetTextColor(views::Button::STATE_NORMAL, value);
    btn->SetTextColor(views::Button::STATE_HOVERED, value);
    btn->SetTextColor(views::Button::STATE_PRESSED, value);
    btn->SetBackground(views::CreateSolidBackground(button_bg));
  };
  style_button(new_workspace_button_);
  style_button(rename_workspace_button_);
  style_button(delete_workspace_button_);

  // Update quick action buttons.
  if (quick_actions_container_) {
    for (auto* child : quick_actions_container_->children()) {
      auto* btn = static_cast<views::LabelButton*>(child);
      style_button(btn);
    }
  }
}

// ---------------------------------------------------------------------------
// Workspace info
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::RefreshWorkspaceInfo() {
  if (!workspace_name_label_) {
    return;
  }

  // If we have workspaces data, show the selected one.
  if (!workspaces_.empty() && selected_index_ >= 0) {
    const auto& ws = workspaces_[selected_index_];
    workspace_name_label_->SetText(u"Name: " + ws.name);
    workspace_id_label_->SetText(
        base::UTF8ToUTF16("ID: " + ws.id));
    workspace_color_label_->SetText(
        base::UTF8ToUTF16(
            base::StringPrintf(
                "Color: #%06X", GetWorkspaceAccentColor(ws) & 0xFFFFFF)));
    return;
  }

  // Fall back to workspace service.
  if (!workspace_service_) {
    workspace_name_label_->SetText(u"No workspace service");
    workspace_id_label_->SetText(u"");
    workspace_color_label_->SetText(u"");
    return;
  }

  const AstraWorkspace& active = workspace_service_->active_workspace();

  workspace_name_label_->SetText(
      base::UTF8ToUTF16("Name: " + active.name));
  workspace_id_label_->SetText(
      base::UTF8ToUTF16("ID: " + active.id));
  workspace_color_label_->SetText(
      base::UTF8ToUTF16("Color: " + active.accent_color));
}

// ---------------------------------------------------------------------------
// Workspace list
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::RefreshWorkspaceList() {
  if (!workspace_list_container_) {
    return;
  }

  workspace_list_container_->RemoveAllChildViews();

  // Use model-driven workspaces if available, otherwise fall back to service.
  if (!workspaces_.empty()) {
    auto filtered_indices = GetFilteredWorkspaceIndices();

    if (filtered_indices.empty()) {
      auto* label = workspace_list_container_->AddChildView(
          std::make_unique<views::Label>(u"No matching workspaces"));
      label->SetEnabledColor(
          ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
      label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      label->SetBorder(views::CreateEmptyBorder(
          gfx::Insets::VH(4, 8)));
    } else {
      for (int idx : filtered_indices) {
        AddWorkspaceCard(workspaces_[idx], idx);
      }
    }
  } else if (workspace_service_) {
    const auto& workspaces_svc = workspace_service_->workspaces();
    std::u16string filter_lower = base::ToLowerASCII(search_query_);

    for (size_t i = 0; i < workspaces_svc.size(); ++i) {
      const auto& ws = workspaces_svc[i];

      // Apply filter.
      if (!filter_lower.empty()) {
        std::string ws_text = ws.name + " " + ws.id;
        std::u16string ws_text16 = base::UTF8ToUTF16(ws_text);
        if (base::ToLowerASCII(ws_text16).find(filter_lower) ==
            std::u16string::npos) {
          continue;
        }
      }

      std::string line = base::StringPrintf(
          "%s (%zu tabs)",
          ws.name.c_str(),
          workspace_service_->GetTabCount(ws.id));

      auto* item = workspace_list_container_->AddChildView(
          std::make_unique<views::LabelButton>(
              base::BindRepeating(
                  &AstraDevToolsWorkspacePanel::OnWorkspaceItemClicked,
                  base::Unretained(this), ws.id),
              base::UTF8ToUTF16(line)));
      item->SetMinSize(gfx::Size(0, kWorkspaceItemHeight / 2));
      item->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      item->SetBorder(views::CreateEmptyBorder(
          gfx::Insets::VH(0, 8)));
      item->SetFocusForPlatform();

      item->SetTextColor(views::Button::STATE_NORMAL,
          ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
      item->SetTextColor(views::Button::STATE_HOVERED,
          ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
      item->SetTextColor(views::Button::STATE_PRESSED,
          ThemeColor(dark_theme_, kDarkValueText, kLightValueText));

      bool is_active = (ws.id == workspace_service_->active_workspace_id());
      bool is_selected = (ws.id == selected_workspace_id_);

      if (is_selected) {
        item->SetBackground(views::CreateSolidBackground(
            ThemeColor(dark_theme_,
                       kDarkItemSelectedBg, kLightItemSelectedBg)));
      } else if (is_active) {
        item->SetBackground(views::CreateSolidBackground(
            ThemeColor(dark_theme_, kDarkItemBg, kLightItemBg)));
        item->SetFontList(
            item->font_list().Derive(0, gfx::Font::FontStyle::NORMAL,
                                      gfx::Font::Weight::BOLD));
      }
    }
  } else {
    auto* label = workspace_list_container_->AddChildView(
        std::make_unique<views::Label>(u"No workspace data available"));
    label->SetEnabledColor(
        ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    label->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(4, 8)));
  }

  workspace_list_container_->InvalidateLayout();
}

void AstraDevToolsWorkspacePanel::OnWorkspaceItemClicked(
    const std::string& workspace_id) {
  // If using model-driven workspaces, find the index.
  int idx = FindWorkspaceIndexById(workspace_id);
  if (idx >= 0) {
    SelectWorkspace(idx);
    return;
  }

  // Legacy path: use selected_workspace_id_.
  if (selected_workspace_id_ == workspace_id) {
    return;
  }
  selected_workspace_id_ = workspace_id;

  // Also update selected_index_ if possible.
  for (size_t i = 0; i < workspaces_.size(); ++i) {
    if (workspaces_[i].id == workspace_id) {
      selected_index_ = static_cast<int>(i);
      break;
    }
  }

  RefreshWorkspaceList();
  RefreshTabList();

  if (delegate_) {
    delegate_->OnWorkspaceSelected(workspace_id);
  }
}

// ---------------------------------------------------------------------------
// Tab list
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::RefreshTabList() {
  if (!tab_list_container_) {
    return;
  }

  tab_list_container_->RemoveAllChildViews();

  // Use model-driven workspace data.
  if (!workspaces_.empty() && selected_index_ >= 0 &&
      static_cast<size_t>(selected_index_) < workspaces_.size()) {
    const auto& ws = workspaces_[selected_index_];
    int tab_count = ws.tab_count;

    if (tab_count == 0) {
      auto* label = tab_list_container_->AddChildView(
          std::make_unique<views::Label>(u"No tabs in this workspace"));
      label->SetEnabledColor(
          ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
      label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      label->SetBorder(views::CreateEmptyBorder(
          gfx::Insets::VH(4, 8)));
    } else {
      std::u16string filter_lower = base::ToLowerASCII(search_query_);

      for (int i = 0; i < tab_count; ++i) {
        std::string tab_label = base::StringPrintf("Tab %d", i + 1);

        // Apply filter.
        if (!filter_lower.empty()) {
          std::u16string label16 = base::UTF8ToUTF16(tab_label);
          if (base::ToLowerASCII(label16).find(filter_lower) ==
              std::u16string::npos) {
            continue;
          }
        }

        auto* item = tab_list_container_->AddChildView(
            std::make_unique<views::LabelButton>(
                base::BindRepeating(
                    &AstraDevToolsWorkspacePanel::OnTabSelected,
                    base::Unretained(this), i),
                base::UTF8ToUTF16(tab_label)));
        item->SetMinSize(gfx::Size(0, kTabItemHeight));
        item->SetHorizontalAlignment(gfx::ALIGN_LEFT);
        item->SetBorder(views::CreateEmptyBorder(
            gfx::Insets::VH(0, 12)));
        item->SetFocusForPlatform();
        item->SetTextColor(views::Button::STATE_NORMAL,
            ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
        item->SetTextColor(views::Button::STATE_HOVERED,
            ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
        item->SetTextColor(views::Button::STATE_PRESSED,
            ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
      }
    }
  } else if (workspace_service_ && !selected_workspace_id_.empty()) {
    // Legacy path: use workspace service.
    // TODO(astra): Get actual tab list for the workspace from TabStripModel.
    size_t tab_count = workspace_service_->GetTabCount(selected_workspace_id_);

    std::u16string filter_lower = base::ToLowerASCII(search_query_);

    for (size_t i = 0; i < tab_count; ++i) {
      std::string tab_label = base::StringPrintf("Tab %zu", i + 1);

      if (!filter_lower.empty()) {
        std::u16string label16 = base::UTF8ToUTF16(tab_label);
        if (base::ToLowerASCII(label16).find(filter_lower) ==
            std::u16string::npos) {
          continue;
        }
      }

      auto* item = tab_list_container_->AddChildView(
          std::make_unique<views::LabelButton>(
              base::BindRepeating(
                  &AstraDevToolsWorkspacePanel::OnTabSelected,
                  base::Unretained(this), static_cast<int>(i)),
              base::UTF8ToUTF16(tab_label)));
      item->SetMinSize(gfx::Size(0, kTabItemHeight));
      item->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      item->SetBorder(views::CreateEmptyBorder(
          gfx::Insets::VH(0, 12)));
      item->SetFocusForPlatform();
      item->SetTextColor(views::Button::STATE_NORMAL,
          ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
      item->SetTextColor(views::Button::STATE_HOVERED,
          ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
      item->SetTextColor(views::Button::STATE_PRESSED,
          ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
    }
  } else {
    auto* label = tab_list_container_->AddChildView(
        std::make_unique<views::Label>(u"Select a workspace to see tabs"));
    label->SetEnabledColor(
        ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    label->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(4, 8)));
  }

  tab_list_container_->InvalidateLayout();
}

// ---------------------------------------------------------------------------
// Tab metadata
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::RefreshTabMetadata() {
  if (!tab_metadata_label_) {
    return;
  }

  if (!inspected_contents_) {
    tab_metadata_label_->SetText(u"No inspected tab");
    return;
  }

  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(inspected_contents_);
  if (!features) {
    tab_metadata_label_->SetText(u"No Astra tab features");
    return;
  }

  std::string text;
  text += base::StringPrintf("workspace_id: %s\n",
                            features->workspace_id().c_str());
  text += base::StringPrintf("is_favorite: %s\n",
                            features->is_favorite() ? "true" : "false");
  text += base::StringPrintf("favorite_folder_id: %s\n",
                            features->favorite_folder_id().c_str());
  text += base::StringPrintf("favorite_order_index: %zu\n",
                            features->favorite_order_index());
  text += base::StringPrintf("is_in_split_view: %s\n",
                            features->is_in_split_view() ? "true" : "false");
  if (features->is_in_split_view()) {
    text += base::StringPrintf("split_view_partner_id: %s\n",
                              features->split_view_partner_id().c_str());
    text += base::StringPrintf("split_view_ratio: %.2f\n",
                              features->split_view_ratio());
    text += base::StringPrintf(
        "split_view_orientation: %s\n",
        features->split_view_orientation() == SplitViewOrientation::kHorizontal
            ? "horizontal"
            : "vertical");
  }
  text += base::StringPrintf("is_glance_tab: %s\n",
                            features->is_glance_tab() ? "true" : "false");
  text += base::StringPrintf("sidebar_pinned: %s\n",
                            features->sidebar_pinned() ? "true" : "false");
  text += base::StringPrintf("sidebar_hidden: %s\n",
                            features->sidebar_hidden() ? "true" : "false");
  text += base::StringPrintf("is_in_stack: %s\n",
                            features->is_in_stack() ? "true" : "false");
  if (features->is_in_stack()) {
    text += base::StringPrintf("stack_parent_id: %s\n",
                              features->stack_parent_id().c_str());
  }
  text += base::StringPrintf("is_stack_collapsed: %s\n",
                            features->is_stack_collapsed() ? "true" : "false");
  text += base::StringPrintf("is_pip_tab: %s\n",
                            features->is_pip_tab() ? "true" : "false");
  text += base::StringPrintf("is_suspended: %s\n",
                            features->is_suspended() ? "true" : "false");
  if (features->is_suspended()) {
    text += base::StringPrintf("suspended_url: %s\n",
                              features->suspended_url().spec().c_str());
  }

  tab_metadata_label_->SetText(base::UTF8ToUTF16(text));
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::RefreshStats() {
  if (!stats_label_) {
    return;
  }

  size_t workspace_count = 0;
  int total_tabs = 0;
  int total_windows = 0;

  if (!workspaces_.empty()) {
    workspace_count = workspaces_.size();
    for (const auto& ws : workspaces_) {
      total_tabs += ws.tab_count;
      total_windows += ws.window_count;
    }
  } else if (workspace_service_) {
    workspace_count = workspace_service_->workspaces().size();
    // TODO(astra): Calculate total tabs from all workspaces.
  }

  std::u16string stats_text = base::UTF8ToUTF16(
      base::StringPrintf(
          "%zu workspaces  \u00B7  %d tabs  \u00B7  %d windows",
          workspace_count, total_tabs, total_windows));
  stats_label_->SetText(stats_text);
}

// ---------------------------------------------------------------------------
// Button handlers
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::OnNewWorkspaceButton() {
  NewWorkspace();
}

void AstraDevToolsWorkspacePanel::OnDeleteWorkspaceButton() {
  if (selected_index_ >= 0) {
    DeleteWorkspace(selected_index_);
  } else if (!selected_workspace_id_.empty()) {
    if (delegate_) {
      delegate_->OnDeleteWorkspace(selected_workspace_id_);
    }
  }
}

void AstraDevToolsWorkspacePanel::OnRenameWorkspaceButton() {
  if (selected_index_ >= 0) {
    RenameWorkspace(selected_index_, u"Renamed Workspace");
  } else if (!selected_workspace_id_.empty()) {
    if (delegate_) {
      delegate_->OnRenameWorkspace(
          selected_workspace_id_, "Renamed Workspace");
    }
  }
}

void AstraDevToolsWorkspacePanel::OnMergeWorkspacesButton() {
  // TODO(astra): Implement workspace merge functionality.
  //   For now, just notify delegate that merge was requested.
  VLOG(1) << "Astra DevTools: Merge workspaces requested";
}

void AstraDevToolsWorkspacePanel::OnImportWorkspacesButton() {
  // TODO(astra): Implement workspace import functionality.
  VLOG(1) << "Astra DevTools: Import workspaces requested";
}

void AstraDevToolsWorkspacePanel::OnExportWorkspacesButton() {
  // TODO(astra): Implement workspace export functionality.
  VLOG(1) << "Astra DevTools: Export workspaces requested";
}

// ---------------------------------------------------------------------------
// Tab selection handler (for tab list items)
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::OnTabSelected(int tab_index) {
  if (delegate_) {
    delegate_->OnTabSelected(tab_index);
  }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int AstraDevToolsWorkspacePanel::FindWorkspaceIndexById(
    const std::string& id) const {
  for (size_t i = 0; i < workspaces_.size(); ++i) {
    if (workspaces_[i].id == id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

std::vector<int> AstraDevToolsWorkspacePanel::GetFilteredWorkspaceIndices()
    const {
  std::vector<int> result;

  if (search_query_.empty()) {
    for (size_t i = 0; i < workspaces_.size(); ++i) {
      result.push_back(static_cast<int>(i));
    }
    return result;
  }

  std::u16string filter_lower = base::ToLowerASCII(search_query_);
  for (size_t i = 0; i < workspaces_.size(); ++i) {
    const auto& ws = workspaces_[i];
    std::u16string ws_name_lower = base::ToLowerASCII(ws.name);
    std::u16string ws_id_lower = base::ToLowerASCII(base::UTF8ToUTF16(ws.id));

    if (ws_name_lower.find(filter_lower) != std::u16string::npos ||
        ws_id_lower.find(filter_lower) != std::u16string::npos) {
      result.push_back(static_cast<int>(i));
    }
  }

  return result;
}

// ---------------------------------------------------------------------------
// Testing accessors
// ---------------------------------------------------------------------------

size_t AstraDevToolsWorkspacePanel::workspace_item_count_for_testing() const {
  if (!workspace_list_container_) {
    return 0;
  }
  return workspace_list_container_->children().size();
}

size_t AstraDevToolsWorkspacePanel::tab_item_count_for_testing() const {
  if (!tab_list_container_) {
    return 0;
  }
  return tab_list_container_->children().size();
}

std::string AstraDevToolsWorkspacePanel::selected_workspace_id_for_testing()
    const {
  if (selected_index_ >= 0 &&
      static_cast<size_t>(selected_index_) < workspaces_.size()) {
    return workspaces_[selected_index_].id;
  }
  return selected_workspace_id_;
}

}  // namespace astra
