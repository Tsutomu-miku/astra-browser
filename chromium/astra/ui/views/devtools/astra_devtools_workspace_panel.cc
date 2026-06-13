#include "astra/ui/views/devtools/astra_devtools_workspace_panel.h"

#include <string>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/browser/astra_workspace_service.h"
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

// Height of workspace list items.
constexpr int kWorkspaceItemHeight = 28;

// Height of tab list items.
constexpr int kTabItemHeight = 24;

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

// Helper: returns the color for a given theme.
SkColor ThemeColor(bool dark, SkColor dark_color, SkColor light_color) {
  return dark ? dark_color : light_color;
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
      base::BindRepeating(&AstraDevToolsWorkspacePanel::SetSearchFilter,
                          base::Unretained(this)));

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
  workspace_scroll_view_->SetPreferredSize(gfx::Size(0, 140));
  workspace_scroll_view_->SetBorder(views::CreateSolidBorder(
      1, ThemeColor(dark_theme_, kDarkBorder, kLightBorder)));

  workspace_list_container_ = workspace_scroll_view_->SetContents(
      std::make_unique<views::View>());
  workspace_list_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(),
          0));

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
  //   The current implementation uses a single monospaced label.
  //   This helper is reserved for future use with a two-column layout.
}

// ---------------------------------------------------------------------------
// Public API
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
  }

  RefreshWorkspaceInfo();
  RefreshWorkspaceList();
  RefreshTabList();
  RefreshTabMetadata();
}

void AstraDevToolsWorkspacePanel::SetSearchFilter(
    const std::u16string& filter) {
  if (search_filter_ == filter) {
    return;
  }
  search_filter_ = filter;

  if (workspace_list_container_) {
    RefreshWorkspaceList();
    RefreshTabList();
  }
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

  SetBackground(views::CreateSolidBackground(bg));

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
}

// ---------------------------------------------------------------------------
// Workspace info
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::RefreshWorkspaceInfo() {
  if (!workspace_name_label_) {
    return;
  }

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

  if (!workspace_service_) {
    auto* label = workspace_list_container_->AddChildView(
        std::make_unique<views::Label>(u"No workspace service available"));
    label->SetEnabledColor(
        ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    label->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(4, 8)));
    return;
  }

  const auto& workspaces = workspace_service_->workspaces();
  std::u16string filter_lower = base::ToLowerASCII(search_filter_);

  for (const auto& ws : workspaces) {
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
    item->SetMinSize(gfx::Size(0, kWorkspaceItemHeight));
    item->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    item->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(0, 8)));
    item->SetFocusForPlatform();

    // Set colors.
    item->SetTextColor(views::Button::STATE_NORMAL,
        ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
    item->SetTextColor(views::Button::STATE_HOVERED,
        ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
    item->SetTextColor(views::Button::STATE_PRESSED,
        ThemeColor(dark_theme_, kDarkValueText, kLightValueText));

    // Highlight active workspace and selected workspace.
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

  workspace_list_container_->InvalidateLayout();
}

void AstraDevToolsWorkspacePanel::OnWorkspaceItemClicked(
    const std::string& workspace_id) {
  if (selected_workspace_id_ == workspace_id) {
    return;
  }
  selected_workspace_id_ = workspace_id;
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

  if (!workspace_service_ || selected_workspace_id_.empty()) {
    auto* label = tab_list_container_->AddChildView(
        std::make_unique<views::Label>(u"Select a workspace to see tabs"));
    label->SetEnabledColor(
        ThemeColor(dark_theme_, kDarkValueText, kLightValueText));
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    label->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(4, 8)));
    return;
  }

  // TODO(astra): Get actual tab list for the workspace from TabStripModel
  //   or AstraWorkspaceService.  For now, show a placeholder count.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  size_t tab_count = workspace_service_->GetTabCount(selected_workspace_id_);

  std::u16string filter_lower = base::ToLowerASCII(search_filter_);

  for (size_t i = 0; i < tab_count; ++i) {
    std::string tab_label = base::StringPrintf("Tab %zu", i + 1);

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
// Button handlers
// ---------------------------------------------------------------------------

void AstraDevToolsWorkspacePanel::OnNewWorkspaceButton() {
  if (delegate_) {
    delegate_->OnNewWorkspace();
  }
}

void AstraDevToolsWorkspacePanel::OnDeleteWorkspaceButton() {
  if (delegate_ && !selected_workspace_id_.empty()) {
    delegate_->OnDeleteWorkspace(selected_workspace_id_);
  }
}

void AstraDevToolsWorkspacePanel::OnRenameWorkspaceButton() {
  if (delegate_ && !selected_workspace_id_.empty()) {
    // TODO(astra): Show a rename dialog or inline editor.
    //   For now, we just dispatch with a placeholder name.
    delegate_->OnRenameWorkspace(selected_workspace_id_, "Renamed Workspace");
  }
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

}  // namespace astra
