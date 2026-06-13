#include "astra/ui/views/settings/astra_search_settings_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_search_engine_helper.h"
#include "astra/ui/views/settings/astra_settings_model.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kSectionSpacing = 16;
constexpr int kSectionHeaderBottomPadding = 8;
constexpr int kRowSpacing = 6;
constexpr int kEngineRowHeight = 48;
constexpr int kEngineRowPadding = 8;
constexpr int kButtonSpacing = 8;
constexpr int kSectionHeaderFontSizeDelta = 1;
constexpr int kOtherEnginesMaxHeight = 200;

// TODO(astra): Move color tokens to astra/ui/color/astra_color_ids.h
// with a proper ColorProvider mixin.
constexpr ui::ColorId kSectionHeaderTextColor =
    ui::kColorLabelForegroundSecondary;
constexpr ui::ColorId kDefaultEngineHighlightColor =
    ui::kColorSysPrimary;

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSearchSettingsView::AstraSearchSettingsView(Browser* browser)
    : browser_(browser) {
  DCHECK(browser_);
  BuildContents();
}

AstraSearchSettingsView::~AstraSearchSettingsView() = default;

// =========================================================================
// Model integration
// =========================================================================

void AstraSearchSettingsView::SetModel(AstraSettingsModel* model) {
  model_ = model;
  // Refresh from the model if available.
  if (model_) {
    // Sync search suggestions state with model.
    const AstraSettingItem* suggestions_setting =
        model_->GetSetting("search_suggestions");
    if (suggestions_setting && suggestions_toggle_) {
      suggestions_toggle_->SetIsOn(
          suggestions_setting->current_value.GetBool());
    }
  }
}

// =========================================================================
// Search engine management
// =========================================================================

std::string AstraSearchSettingsView::GetDefaultSearchEngine() const {
  if (!browser_ || !browser_->profile()) {
    return std::string();
  }
  auto info = AstraSearchEngineHelper::GetDefaultSearchEngineInfo(
      browser_->profile());
  return info.id;
}

bool AstraSearchSettingsView::SetDefaultSearchEngine(
    const std::string& engine_id) {
  if (!browser_ || !browser_->profile()) {
    return false;
  }
  if (engine_id.empty()) {
    return false;
  }
  bool success = AstraSearchEngineHelper::SetDefaultSearchEngineById(
      browser_->profile(), engine_id);
  if (success) {
    RefreshFromService();
  }
  return success;
}

std::vector<AstraSearchEngine>
AstraSearchSettingsView::GetSearchEngines() const {
  if (!browser_ || !browser_->profile()) {
    return {};
  }
  auto infos = AstraSearchEngineHelper::GetSearchEngines(browser_->profile());
  std::vector<AstraSearchEngine> result;
  result.reserve(infos.size());
  for (const auto& info : infos) {
    AstraSearchEngine engine;
    engine.id = info.id;
    engine.name = info.name;
    engine.keyword = info.keyword;
    engine.url = GURL(info.url);
    engine.is_default = info.is_default;
    engine.is_editable = info.is_editable;
    result.push_back(std::move(engine));
  }
  return result;
}

std::string AstraSearchSettingsView::AddSearchEngine(
    const std::string& name,
    const GURL& url) {
  if (!browser_ || !browser_->profile()) {
    return std::string();
  }
  if (name.empty() || !url.is_valid()) {
    return std::string();
  }

  // Use a default keyword based on the name.
  std::u16string keyword = base::UTF8ToUTF16(name);

  const TemplateURL* turl = AstraSearchEngineHelper::AddSearchEngine(
      browser_->profile(), base::UTF8ToUTF16(name), keyword, url.spec());

  if (turl) {
    RefreshFromService();
    // TODO(astra): Return the actual ID from the TemplateURL.
    // For now, try to find it by name.
    auto info = AstraSearchEngineHelper::GetSearchEngineByName(
        browser_->profile(), base::UTF8ToUTF16(name));
    return info.id;
  }

  return std::string();
}

bool AstraSearchSettingsView::RemoveSearchEngine(
    const std::string& engine_id) {
  if (!browser_ || !browser_->profile()) {
    return false;
  }
  if (engine_id.empty()) {
    return false;
  }
  // Can't remove the default engine.
  if (AstraSearchEngineHelper::IsDefaultSearchEngine(browser_->profile(),
                                                     engine_id)) {
    return false;
  }
  bool success = AstraSearchEngineHelper::RemoveSearchEngineById(
      browser_->profile(), engine_id);
  if (success) {
    RefreshFromService();
  }
  return success;
}

bool AstraSearchSettingsView::EditSearchEngine(const std::string& engine_id,
                                                const std::string& name,
                                                const GURL& url) {
  // TODO(astra): Implement edit search engine via TemplateURLService.
  // Currently not supported in the overlay stub implementation.
  // Chromium owner: TemplateURLService (components/search_engines/)
  // Patch point: TemplateURLService::Remove() + Add() to replace, or a
  //   dedicated edit method.
  //
  // For now, this is a no-op that returns false.
  if (engine_id.empty() || name.empty() || !url.is_valid()) {
    return false;
  }
  return false;
}

// =========================================================================
// Search suggestions
// =========================================================================

void AstraSearchSettingsView::SetSearchSuggestionsEnabled(bool enabled) {
  if (!browser_ || !browser_->profile()) {
    return;
  }
  AstraSearchEngineHelper::SetSuggestionsEnabled(browser_->profile(),
                                                  enabled);
  if (suggestions_toggle_) {
    suggestions_toggle_->SetIsOn(enabled);
  }

  // Also update the model if available.
  if (model_) {
    model_->SetSettingValue("search_suggestions", base::Value(enabled));
  }
}

bool AstraSearchSettingsView::IsSearchSuggestionsEnabled() const {
  if (!browser_ || !browser_->profile()) {
    return true;  // Default value.
  }
  return AstraSearchEngineHelper::GetSuggestionsEnabled(browser_->profile());
}

// =========================================================================
// Contents building
// =========================================================================

void AstraSearchSettingsView::BuildContents() {
  // Set search metadata.
  section_title_ = u"Search engine";
  search_keywords_ = {u"search", u"engine", u"google", u"bing", u"duckduckgo",
                      u"default search", u"url", u"keyword", u"suggestions"};

  // Vertical box layout for the whole section.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      kSectionSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  BuildDefaultEngineSection();
  BuildOtherEnginesSection();
  BuildSuggestionsSection();
  BuildAddEngineSection();

  RefreshFromService();
}

// =========================================================================
// Default engine section
// =========================================================================

void AstraSearchSettingsView::BuildDefaultEngineSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  auto* section_layout = section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kRowSpacing));
  section_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  AddSectionHeader(section, u"Default search engine");

  // Container for the default engine display.
  auto container = std::make_unique<views::View>();
  container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(kEngineRowPadding),  // padding inside highlight
      kRowSpacing));
  // TODO(astra): Add a subtle background highlight for the default engine
  // using SetBackground() with a rounded rect background.
  default_engine_container_ = container.get();
  section->AddChildView(std::move(container));
}

// =========================================================================
// Other engines section
// =========================================================================

void AstraSearchSettingsView::BuildOtherEnginesSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  auto* section_layout = section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kRowSpacing));
  section_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  AddSectionHeader(section, u"Other search engines");

  // Scroll view for the other engines list.
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->SetClipHeight(true);
  scroll_view->SetPreferredSize(
      gfx::Size(0, kOtherEnginesMaxHeight));
  scroll_view->SetDrawOverflowIndicator(true);
  other_engines_scroll_ = scroll_view.get();

  // Content container inside the scroll view.
  auto content = std::make_unique<views::View>();
  content->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      kRowSpacing));
  other_engines_container_ = content.get();
  scroll_view->SetContents(std::move(content));

  section->AddChildView(std::move(scroll_view));
}

// =========================================================================
// Suggestions section
// =========================================================================

void AstraSearchSettingsView::BuildSuggestionsSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  auto* section_layout = section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kRowSpacing));
  section_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  AddSectionHeader(section, u"Search suggestions");

  // Suggestions toggle row.
  auto row = std::make_unique<views::View>();
  auto* row_layout = row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kButtonSpacing));
  row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);
  row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  row->SetPreferredSize(gfx::Size(0, 40));

  // Label.
  auto label = std::make_unique<views::Label>(u"Show search suggestions");
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColorId(ui::kColorLabelForegroundPrimary);
  row->AddChildView(std::move(label));

  // Toggle button.
  auto toggle = std::make_unique<views::ToggleButton>(
      base::BindRepeating(
          &AstraSearchSettingsView::OnSearchSuggestionsToggled,
          base::Unretained(this)));
  toggle->SetIsOn(IsSearchSuggestionsEnabled());
  suggestions_toggle_ = toggle.get();
  row->AddChildView(std::move(toggle));

  section->AddChildView(std::move(row));

  // Description.
  auto desc = std::make_unique<views::Label>(
      u"Show search suggestions as you type in the search box.");
  desc->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  desc->SetMultiLine(true);
  desc->SetAutoColorReadabilityEnabled(false);
  desc->SetEnabledColorId(ui::kColorLabelForegroundSecondary);
  desc->SetFontList(
      desc->font_list().DeriveWithSizeDelta(-1));
  section->AddChildView(std::move(desc));
}

// =========================================================================
// Add engine section
// =========================================================================

void AstraSearchSettingsView::BuildAddEngineSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  auto* section_layout = section->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kRowSpacing));
  section_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  AddSectionHeader(section, u"Add search engine");

  auto button = views::MdTextButton::CreateSecondaryUiMedium(
      base::BindRepeating(&AstraSearchSettingsView::OnAddSearchEngine,
                          base::Unretained(this)),
      u"Add search engine");
  add_engine_button_ = button.get();
  section->AddChildView(std::move(button));

  // Explanatory text.
  auto hint_label = std::make_unique<views::Label>(
      u"Add custom search engines for quick access.");
  hint_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  hint_label->SetMultiLine(true);
  hint_label->SetAutoColorReadabilityEnabled(false);
  hint_label->SetEnabledColorId(
      ui::kColorLabelForegroundSecondary);
  hint_label->SetFontList(
      hint_label->font_list().DeriveWithSizeDelta(-1));
  section->AddChildView(std::move(hint_label));
}

// =========================================================================
// Event handlers
// =========================================================================

void AstraSearchSettingsView::OnMakeDefault(const std::string& engine_id) {
  if (!browser_ || !browser_->profile()) {
    return;
  }

  bool success = AstraSearchEngineHelper::SetDefaultSearchEngineById(
      browser_->profile(), engine_id);

  if (success) {
    RefreshFromService();

    // Also update the model if available.
    if (model_) {
      auto info = AstraSearchEngineHelper::GetDefaultSearchEngineInfo(
          browser_->profile());
      model_->SetSettingValue("default_search_engine",
                               base::Value(info.id));
    }
  }
}

void AstraSearchSettingsView::OnAddSearchEngine() {
  // TODO(astra): Show an "Add search engine" dialog inline.
  // For now, delegate to Chrome settings for full management.
  //
  // Chromium owner: SearchEngineDialog
  //   (chrome/browser/ui/search_engines/search_engine_dialog.h)
  // Patch point: The dialog can be shown directly from Astra UI.
  if (!browser_ || !browser_->profile()) {
    return;
  }
  AstraSearchEngineHelper::OpenChromeSearchSettings(browser_->profile());
}

void AstraSearchSettingsView::OnEditSearchEngine(
    const std::string& engine_id) {
  // TODO(astra): Show edit dialog inline.
  // For now, delegate to Chrome settings.
  if (!browser_ || !browser_->profile()) {
    return;
  }
  AstraSearchEngineHelper::OpenChromeSearchSettings(browser_->profile());
}

void AstraSearchSettingsView::OnDeleteSearchEngine(
    const std::string& engine_id) {
  if (!browser_ || !browser_->profile()) {
    return;
  }

  // Don't allow deleting the default engine.
  if (AstraSearchEngineHelper::IsDefaultSearchEngine(browser_->profile(),
                                                     engine_id)) {
    return;
  }

  bool success = AstraSearchEngineHelper::RemoveSearchEngineById(
      browser_->profile(), engine_id);

  if (success) {
    RefreshFromService();
  }
}

void AstraSearchSettingsView::OnSearchSuggestionsToggled() {
  if (!suggestions_toggle_ || !browser_ || !browser_->profile()) {
    return;
  }

  bool enabled = suggestions_toggle_->GetIsOn();
  AstraSearchEngineHelper::SetSuggestionsEnabled(browser_->profile(),
                                                  enabled);

  // Also update the model if available.
  if (model_) {
    model_->SetSettingValue("search_suggestions", base::Value(enabled));
  }
}

// =========================================================================
// UI helpers
// =========================================================================

std::unique_ptr<views::View> AstraSearchSettingsView::CreateEngineRow(
    const std::string& engine_id,
    const std::u16string& name,
    const std::u16string& keyword,
    const std::string& url,
    bool is_default,
    bool is_editable) {
  auto row = std::make_unique<views::View>();
  auto* layout = row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, kEngineRowPadding),
      kButtonSpacing));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  row->SetPreferredSize(gfx::Size(0, kEngineRowHeight));

  // Left side: name + keyword/URL (stacked vertically).
  auto left_container = std::make_unique<views::View>();
  auto* left_layout = left_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(), 2));
  left_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);

  // Engine name.
  auto name_label = std::make_unique<views::Label>(name);
  name_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label->SetAutoColorReadabilityEnabled(false);
  if (is_default) {
    // Highlight default engine name with primary color.
    name_label->SetEnabledColorId(kDefaultEngineHighlightColor);
    name_label->SetFontList(
        name_label->font_list().DeriveWithWeight(gfx::Font::Weight::BOLD));
  }
  left_container->AddChildView(std::move(name_label));

  // Keyword + URL subtitle.
  std::u16string subtitle =
      keyword + u"  \u00B7  " + base::UTF8ToUTF16(url);
  auto subtitle_label = std::make_unique<views::Label>(subtitle);
  subtitle_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  subtitle_label->SetAutoColorReadabilityEnabled(false);
  subtitle_label->SetEnabledColorId(ui::kColorLabelForegroundSecondary);
  subtitle_label->SetFontList(
      subtitle_label->font_list().DeriveWithSizeDelta(-1));
  left_container->AddChildView(std::move(subtitle_label));

  row->AddChildView(std::move(left_container));

  // Right side: action buttons.
  auto button_row = std::make_unique<views::View>();
  auto* button_layout = button_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 4));
  button_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kEnd);
  button_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  if (is_default) {
    // Show "Default" label instead of "Make default" button.
    auto default_label = std::make_unique<views::Label>(u"Default");
    default_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
    default_label->SetAutoColorReadabilityEnabled(false);
    default_label->SetEnabledColorId(kDefaultEngineHighlightColor);
    default_label->SetFontList(
        default_label->font_list().DeriveWithWeight(gfx::Font::Weight::MEDIUM));
    button_row->AddChildView(std::move(default_label));
  } else {
    // "Make default" button.
    auto make_default_button = views::MdTextButton::Create(
        base::BindRepeating(&AstraSearchSettingsView::OnMakeDefault,
                            base::Unretained(this), engine_id),
        u"Make default");
    button_row->AddChildView(std::move(make_default_button));

    // Edit button.
    auto edit_button = views::MdTextButton::Create(
        base::BindRepeating(&AstraSearchSettingsView::OnEditSearchEngine,
                            base::Unretained(this), engine_id),
        u"Edit");
    button_row->AddChildView(std::move(edit_button));

    // Delete button (only for editable engines).
    if (is_editable) {
      auto delete_button = views::MdTextButton::Create(
          base::BindRepeating(
              &AstraSearchSettingsView::OnDeleteSearchEngine,
              base::Unretained(this), engine_id),
          u"Delete");
      button_row->AddChildView(std::move(delete_button));
    }
  }

  row->AddChildView(std::move(button_row));

  return row;
}

views::Label* AstraSearchSettingsView::AddSectionHeader(
    views::View* parent,
    const std::u16string& title) {
  DCHECK(parent);

  auto header = std::make_unique<views::Label>(title);
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      header->font_list().DeriveWithSizeDelta(kSectionHeaderFontSizeDelta));
  header->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::TLBR(0, 0, kSectionHeaderBottomPadding, 0)));
  // TODO(astra): Use proper color ID for section headers.

  views::Label* header_ptr = header.get();
  parent->AddChildView(std::move(header));
  return header_ptr;
}

// =========================================================================
// Search matching
// =========================================================================

bool AstraSearchSettingsView::MatchesSearch(
    const std::u16string& query) const {
  if (query.empty()) {
    return true;
  }

  std::u16string lower_query = base::ToLowerASCII(query);

  // Match against section title.
  if (base::ToLowerASCII(section_title_).find(lower_query) !=
      std::u16string::npos) {
    return true;
  }

  // Match against keywords.
  for (const auto& kw : search_keywords_) {
    if (base::ToLowerASCII(kw).find(lower_query) != std::u16string::npos) {
      return true;
    }
  }

  // Match against engine names.
  // Since engine names are dynamic, we check against the cached list.
  for (const auto& engine : cached_engines_) {
    if (base::ToLowerASCII(engine.name).find(lower_query) !=
        std::u16string::npos) {
      return true;
    }
  }

  return false;
}

// =========================================================================
// Refresh from service
// =========================================================================

void AstraSearchSettingsView::RefreshFromService() {
  if (!browser_ || !browser_->profile()) {
    return;
  }

  // Get search engines from the helper (projection of TemplateURLService).
  auto infos =
      AstraSearchEngineHelper::GetSearchEngines(browser_->profile());

  // Cache the engine list for search matching.
  cached_engines_.clear();
  cached_engines_.reserve(infos.size());
  for (const auto& info : infos) {
    AstraSearchEngine engine;
    engine.id = info.id;
    engine.name = info.name;
    engine.keyword = info.keyword;
    engine.url = GURL(info.url);
    engine.is_default = info.is_default;
    engine.is_editable = info.is_editable;
    cached_engines_.push_back(std::move(engine));
  }

  // Rebuild default engine container.
  if (default_engine_container_) {
    default_engine_container_->RemoveAllChildViews();

    // Find the default engine.
    bool found_default = false;
    for (const auto& info : infos) {
      if (info.is_default) {
        auto row = CreateEngineRow(
            info.id, info.name, info.keyword, info.url,
            /*is_default=*/true, info.is_editable);
        default_engine_container_->AddChildView(std::move(row));
        found_default = true;
        break;
      }
    }

    if (!found_default) {
      // No default engine set — show placeholder.
      auto placeholder = std::make_unique<views::Label>(
          u"No default search engine set");
      placeholder->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      placeholder->SetAutoColorReadabilityEnabled(false);
      placeholder->SetEnabledColorId(ui::kColorLabelForegroundSecondary);
      default_engine_container_->AddChildView(std::move(placeholder));
    }

    default_engine_container_->InvalidateLayout();
  }

  // Rebuild other engines container.
  if (other_engines_container_) {
    other_engines_container_->RemoveAllChildViews();

    size_t other_count = 0;
    for (const auto& info : infos) {
      if (!info.is_default) {
        auto row = CreateEngineRow(
            info.id, info.name, info.keyword, info.url,
            /*is_default=*/false, info.is_editable);
        other_engines_container_->AddChildView(std::move(row));
        ++other_count;
      }
    }

    if (other_count == 0) {
      auto placeholder = std::make_unique<views::Label>(
          u"No other search engines");
      placeholder->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      placeholder->SetAutoColorReadabilityEnabled(false);
      placeholder->SetEnabledColorId(ui::kColorLabelForegroundSecondary);
      other_engines_container_->AddChildView(std::move(placeholder));
    }

    other_engines_container_->InvalidateLayout();
  }

  // Sync suggestions toggle with pref.
  if (suggestions_toggle_) {
    bool enabled =
        AstraSearchEngineHelper::GetSuggestionsEnabled(browser_->profile());
    suggestions_toggle_->SetIsOn(enabled);
  }
}

// =========================================================================
// Theme handling
// =========================================================================

void AstraSearchSettingsView::OnThemeChanged() {
  views::View::OnThemeChanged();
  // TODO(astra): Update colors based on ColorProvider.
}

// =========================================================================
// Helpers
// =========================================================================

PrefService* AstraSearchSettingsView::GetPrefs() {
  if (!browser_ || !browser_->profile()) {
    return nullptr;
  }
  return browser_->profile()->GetPrefs();
}

}  // namespace astra
