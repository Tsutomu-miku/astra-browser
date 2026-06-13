#include "astra/ui/views/devtools/astra_devtools_toolbar.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "astra/ui/views/devtools/astra_devtools_model.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"

namespace astra {

namespace {

// Spacing between toolbar elements.
constexpr int kElementSpacing = 4;

// Horizontal padding inside the toolbar.
constexpr int kToolbarHorizontalPadding = 8;

// Vertical padding inside the toolbar.
constexpr int kToolbarVerticalPadding = 4;

// Spacing between panel tabs.
constexpr int kPanelTabSpacing = 2;

// Search box width.
constexpr int kSearchBoxWidth = 200;

// Dark theme colors (DevTools-like).
constexpr SkColor kDarkToolbarBg = SkColorSetRGB(0x2D, 0x2D, 0x2D);
constexpr SkColor kDarkTextColor = SK_ColorWHITE;
constexpr SkColor kDarkTextSecondary = SkColorSetRGB(0x9A, 0x9A, 0x9A);
constexpr SkColor kDarkActiveTabBg = SkColorSetRGB(0x1E, 0x1E, 0x1E);
constexpr SkColor kDarkHoverTabBg = SkColorSetRGB(0x3A, 0x3A, 0x3A);
constexpr SkColor kDarkSearchBoxBg = SkColorSetRGB(0x1E, 0x1E, 0x1E);
constexpr SkColor kDarkSearchBoxBorder = SkColorSetRGB(0x55, 0x55, 0x55);

// Light theme colors.
constexpr SkColor kLightToolbarBg = SkColorSetRGB(0xF5, 0xF5, 0xF5);
constexpr SkColor kLightTextColor = SkColorSetRGB(0x20, 0x20, 0x20);
constexpr SkColor kLightTextSecondary = SkColorSetRGB(0x66, 0x66, 0x66);
constexpr SkColor kLightActiveTabBg = SK_ColorWHITE;
constexpr SkColor kLightHoverTabBg = SkColorSetRGB(0xE8, 0xE8, 0xE8);
constexpr SkColor kLightSearchBoxBg = SK_ColorWHITE;
constexpr SkColor kLightSearchBoxBorder = SkColorSetRGB(0xCC, 0xCC, 0xCC);

// Tab padding.
constexpr int kTabHorizontalPadding = 12;
constexpr int kTabVerticalPadding = 6;

// Toolbar button height.
constexpr int kToolbarButtonHeight = 24;

}  // namespace

// ---------------------------------------------------------------------------
// AstraDevToolsToolbar — construction / destruction
// ---------------------------------------------------------------------------

AstraDevToolsToolbar::AstraDevToolsToolbar(Delegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate_);
  BuildToolbar();
}

AstraDevToolsToolbar::~AstraDevToolsToolbar() = default;

// ---------------------------------------------------------------------------
// Build UI
// ---------------------------------------------------------------------------

void AstraDevToolsToolbar::BuildToolbar() {
  // Horizontal layout: all elements in a row.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(kToolbarVerticalPadding, kToolbarHorizontalPadding),
      kElementSpacing));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Apply dark theme background by default.
  SetBackground(views::CreateSolidBackground(kDarkToolbarBg));

  // --- Back/forward navigation buttons.

  back_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraDevToolsToolbar::OnBackButtonPressed,
          base::Unretained(this)),
      u"\u2190"));  // Left arrow
  back_button_->SetMinSize(gfx::Size(kToolbarButtonHeight, kToolbarButtonHeight));
  back_button_->SetFocusForPlatform();

  forward_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraDevToolsToolbar::OnForwardButtonPressed,
          base::Unretained(this)),
      u"\u2192"));  // Right arrow
  forward_button_->SetMinSize(gfx::Size(kToolbarButtonHeight, kToolbarButtonHeight));
  forward_button_->SetFocusForPlatform();

  // --- Panel tabs container.

  panel_tabs_container_ = AddChildView(std::make_unique<views::View>());
  auto* tabs_layout = panel_tabs_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          kPanelTabSpacing));
  tabs_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  tabs_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // --- Search box.

  search_box_ = AddChildView(std::make_unique<views::Textfield>());
  search_box_->set_placeholder_text(u"Search panels...");
  search_box_->SetPreferredSize(gfx::Size(kSearchBoxWidth, kToolbarButtonHeight));
  search_box_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  search_box_->set_controller(
      base::BindRepeating(&AstraDevToolsToolbar::OnSearchTextChanged,
                          base::Unretained(this)));

  // --- Focus mode quick toggle.

  focus_mode_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraDevToolsToolbar::OnFocusModeButtonPressed,
          base::Unretained(this)),
      u"Focus"));
  focus_mode_button_->SetMinSize(gfx::Size(60, kToolbarButtonHeight));
  focus_mode_button_->SetFocusForPlatform();

  // --- Settings button.

  settings_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraDevToolsToolbar::OnSettingsButtonPressed,
          base::Unretained(this)),
      u"\u2699"));  // Gear icon
  settings_button_->SetMinSize(gfx::Size(kToolbarButtonHeight, kToolbarButtonHeight));
  settings_button_->SetFocusForPlatform();
  settings_button_->SetTooltipText(u"Settings");

  // --- Detach button.

  detach_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraDevToolsToolbar::OnDetachButtonPressed,
          base::Unretained(this)),
      u"\u2197"));  // Detach icon (diagonal arrow)
  detach_button_->SetMinSize(gfx::Size(kToolbarButtonHeight, kToolbarButtonHeight));
  detach_button_->SetFocusForPlatform();
  detach_button_->SetTooltipText(u"Undock DevTools");

  // --- Menu button.

  menu_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraDevToolsToolbar::OnMenuButtonPressed,
          base::Unretained(this)),
      u"\u22EE"));  // Vertical ellipsis (more)
  menu_button_->SetMinSize(gfx::Size(kToolbarButtonHeight, kToolbarButtonHeight));
  menu_button_->SetFocusForPlatform();
  menu_button_->SetTooltipText(u"More");

  // Apply initial theme.
  ApplyTheme();
}

void AstraDevToolsToolbar::RebuildPanelTabs() {
  if (!panel_tabs_container_) {
    return;
  }
  panel_tabs_.clear();
  panel_tabs_container_->RemoveAllChildViews();

  if (!model_) {
    return;
  }

  auto panels = model_->GetVisiblePanels();
  for (const auto& panel : panels) {
    std::u16string label;
    if (model_->show_panel_icons() && !panel.icon.empty()) {
      // TODO(astra): Use actual icon resources.  For now, we use the
      //   first letter of the title as a placeholder icon + text.
      //   Chromium owner: ui/gfx/vector_icon_*.h
      if (model_->show_panel_labels()) {
        label = base::UTF8ToUTF16(panel.title);
      } else {
        // Just show first letter as icon placeholder.
        if (!panel.title.empty()) {
          label = base::UTF8ToUTF16(panel.title.substr(0, 1));
        }
      }
    } else if (model_->show_panel_labels()) {
      label = base::UTF8ToUTF16(panel.title);
    } else {
      // Neither icons nor labels — skip?
      // For accessibility, we should show something.
      label = base::UTF8ToUTF16(panel.title);
    }

    auto* tab = panel_tabs_container_->AddChildView(
        std::make_unique<views::LabelButton>(
            base::BindRepeating(
                &AstraDevToolsToolbar::OnPanelTabPressed,
                base::Unretained(this), panel.id),
            label));
    tab->SetMinSize(gfx::Size(0, kToolbarButtonHeight));
    tab->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(0, kTabHorizontalPadding)));
    tab->SetFocusForPlatform();
    tab->SetTooltipText(base::UTF8ToUTF16(panel.title));
    panel_tabs_.push_back(tab);
  }

  UpdateActivePanelTab();
  ApplyTheme();
  panel_tabs_container_->InvalidateLayout();
}

void AstraDevToolsToolbar::UpdateActivePanelTab() {
  if (!model_) {
    return;
  }

  const std::string& active_id = model_->active_panel_id();
  auto panels = model_->GetVisiblePanels();

  for (size_t i = 0; i < panel_tabs_.size() && i < panels.size(); ++i) {
    bool is_active = (panels[i].id == active_id);
    panel_tabs_[i]->SetIsDefault(is_active);

    // Set background based on active state.
    if (is_active) {
      panel_tabs_[i]->SetBackground(
          views::CreateSolidBackground(
              dark_theme_ ? kDarkActiveTabBg : kLightActiveTabBg));
    } else {
      panel_tabs_[i]->SetBackground(nullptr);
    }
  }
}

// ---------------------------------------------------------------------------
// Model integration
// ---------------------------------------------------------------------------

void AstraDevToolsToolbar::SetModel(AstraDevToolsModel* model) {
  if (model_ == model) {
    return;
  }
  model_ = model;
  UpdateFromModel();
}

void AstraDevToolsToolbar::UpdateFromModel() {
  if (!model_) {
    return;
  }

  RebuildPanelTabs();
  UpdateActivePanelTab();

  // Update search box visibility based on toolbar setting.
  if (search_box_) {
    search_box_->SetVisible(model_->show_panel_toolbar());
  }

  // Apply theme from model.
  AstraDevToolsTheme effective_theme = model_->GetEffectiveTheme();
  SetTheme(effective_theme == AstraDevToolsTheme::kDark);
}

// ---------------------------------------------------------------------------
// WebContents
// ---------------------------------------------------------------------------

void AstraDevToolsToolbar::SetInspectedWebContents(
    content::WebContents* web_contents) {
  if (inspected_contents_ == web_contents) {
    return;
  }
  inspected_contents_ = web_contents;

  // TODO(astra): Update focus mode button state based on the inspected tab.
  //   Chromium owner: AstraFocusModeService (astra/browser)
}

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------

void AstraDevToolsToolbar::SetTheme(bool dark_theme) {
  if (dark_theme_ == dark_theme) {
    return;
  }
  dark_theme_ = dark_theme;
  ApplyTheme();
}

void AstraDevToolsToolbar::ApplyTheme() {
  SkColor bg_color = dark_theme_ ? kDarkToolbarBg : kLightToolbarBg;
  SkColor text_color = dark_theme_ ? kDarkTextColor : kLightTextColor;
  SkColor text_secondary = dark_theme_ ? kDarkTextSecondary : kLightTextSecondary;
  SkColor search_bg = dark_theme_ ? kDarkSearchBoxBg : kLightSearchBoxBg;
  SkColor search_border = dark_theme_ ? kDarkSearchBoxBorder : kLightSearchBoxBorder;

  SetBackground(views::CreateSolidBackground(bg_color));

  // Update navigation buttons.
  if (back_button_) {
    back_button_->SetTextColor(views::Button::STATE_NORMAL, text_color);
    back_button_->SetTextColor(views::Button::STATE_HOVERED, text_color);
    back_button_->SetTextColor(views::Button::STATE_PRESSED, text_color);
  }
  if (forward_button_) {
    forward_button_->SetTextColor(views::Button::STATE_NORMAL, text_color);
    forward_button_->SetTextColor(views::Button::STATE_HOVERED, text_color);
    forward_button_->SetTextColor(views::Button::STATE_PRESSED, text_color);
  }

  // Update focus mode button.
  if (focus_mode_button_) {
    focus_mode_button_->SetTextColor(views::Button::STATE_NORMAL, text_color);
    focus_mode_button_->SetTextColor(views::Button::STATE_HOVERED, text_color);
    focus_mode_button_->SetTextColor(views::Button::STATE_PRESSED, text_color);
  }

  // Update settings/detach/menu buttons.
  if (settings_button_) {
    settings_button_->SetTextColor(views::Button::STATE_NORMAL, text_color);
    settings_button_->SetTextColor(views::Button::STATE_HOVERED, text_color);
    settings_button_->SetTextColor(views::Button::STATE_PRESSED, text_color);
  }
  if (detach_button_) {
    detach_button_->SetTextColor(views::Button::STATE_NORMAL, text_color);
    detach_button_->SetTextColor(views::Button::STATE_HOVERED, text_color);
    detach_button_->SetTextColor(views::Button::STATE_PRESSED, text_color);
  }
  if (menu_button_) {
    menu_button_->SetTextColor(views::Button::STATE_NORMAL, text_color);
    menu_button_->SetTextColor(views::Button::STATE_HOVERED, text_color);
    menu_button_->SetTextColor(views::Button::STATE_PRESSED, text_color);
  }

  // Update search box.
  if (search_box_) {
    search_box_->SetBackgroundColor(search_bg);
    search_box_->SetColor(text_color);
    // TODO(astra): Set textfield border color properly.
    //   LabelButton text color is straightforward but Textfield needs
    //   a border update for proper theming.
  }

  // Update panel tabs.
  SkColor active_bg = dark_theme_ ? kDarkActiveTabBg : kLightActiveTabBg;
  SkColor hover_bg = dark_theme_ ? kDarkHoverTabBg : kLightHoverTabBg;

  const std::string& active_id = model_ ? model_->active_panel_id() : "";
  auto panels = model_ ? model_->GetVisiblePanels() :
                         std::vector<AstraDevToolsPanel>();

  for (size_t i = 0; i < panel_tabs_.size(); ++i) {
    auto* tab = panel_tabs_[i];
    bool is_active = (i < panels.size() && panels[i].id == active_id);

    tab->SetTextColor(views::Button::STATE_NORMAL, text_color);
    tab->SetTextColor(views::Button::STATE_HOVERED, text_color);
    tab->SetTextColor(views::Button::STATE_PRESSED, text_color);

    if (is_active) {
      tab->SetBackground(views::CreateSolidBackground(active_bg));
    }
    // TODO(astra): Add hover background for non-active tabs.
    //   LabelButton doesn't have built-in hover background support;
    //   we'd need to subclass or use a different button type.
  }
}

// ---------------------------------------------------------------------------
// Button handlers
// ---------------------------------------------------------------------------

void AstraDevToolsToolbar::OnBackButtonPressed() {
  if (delegate_) {
    delegate_->OnBackClicked();
  }
}

void AstraDevToolsToolbar::OnForwardButtonPressed() {
  if (delegate_) {
    delegate_->OnForwardClicked();
  }
}

void AstraDevToolsToolbar::OnSettingsButtonPressed() {
  if (delegate_) {
    delegate_->OnSettingsClicked();
  }
}

void AstraDevToolsToolbar::OnDetachButtonPressed() {
  if (delegate_) {
    delegate_->OnDetachClicked();
  }
}

void AstraDevToolsToolbar::OnMenuButtonPressed() {
  if (delegate_) {
    delegate_->OnMenuClicked();
  }
}

void AstraDevToolsToolbar::OnFocusModeButtonPressed() {
  if (delegate_) {
    delegate_->OnFocusModeToggled();
  }
}

void AstraDevToolsToolbar::OnPanelTabPressed(const std::string& panel_id) {
  if (delegate_) {
    delegate_->OnPanelTabClicked(panel_id);
  }
}

void AstraDevToolsToolbar::OnSearchTextChanged() {
  if (delegate_ && search_box_) {
    delegate_->OnSearchTextChanged(search_box_->GetText());
  }
}

// ---------------------------------------------------------------------------
// Testing accessors
// ---------------------------------------------------------------------------

size_t AstraDevToolsToolbar::panel_tab_count_for_testing() const {
  return panel_tabs_.size();
}

}  // namespace astra
