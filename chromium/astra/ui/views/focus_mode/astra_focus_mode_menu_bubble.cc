#include "astra/ui/views/focus_mode/astra_focus_mode_menu_bubble.h"

#include <string>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

#include "astra/ui/views/focus_mode/astra_focus_mode_indicator.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_model.h"

namespace astra {

namespace {

// Menu bubble dimensions.
constexpr int kBubbleWidth = 280;
constexpr int kBubbleCornerRadius = 8;

// Spacing and padding.
constexpr int kContentPadding = 12;
constexpr int kRowSpacing = 8;
constexpr int kSectionSpacing = 12;

// Button sizing.
constexpr int kButtonHeight = 32;
constexpr int kPresetButtonHeight = 28;
constexpr int kBlockLevelButtonHeight = 24;

// Colors.
constexpr SkColor kBubbleBackgroundColor =
    SkColorSetARGB(0xFF, 0x2D, 0x2E, 0x33);
constexpr SkColor kBubbleTextColor = SK_ColorWHITE;
constexpr SkColor kBubbleSecondaryTextColor =
    SkColorSetARGB(0xCC, 0xFF, 0xFF, 0xFF);
constexpr SkColor kButtonBackgroundColor =
    SkColorSetARGB(0x1F, 0xFF, 0xFF, 0xFF);
constexpr SkColor kButtonHoverColor =
    SkColorSetARGB(0x33, 0xFF, 0xFF, 0xFF);
constexpr SkColor kEndButtonBackgroundColor =
    SkColorSetARGB(0xFF, 0x5C, 0x3A, 0x3A);
constexpr SkColor kEndButtonHoverColor =
    SkColorSetARGB(0xFF, 0x7A, 0x4A, 0x4A);
constexpr SkColor kBreakButtonBackgroundColor =
    SkColorSetARGB(0xFF, 0x3A, 0x5C, 0x4A);
constexpr SkColor kBreakButtonHoverColor =
    SkColorSetARGB(0xFF, 0x4A, 0x7A, 0x5A);
constexpr SkColor kStartButtonBackgroundColor =
    SkColorSetARGB(0xFF, 0x3A, 0x5C, 0x7A);
constexpr SkColor kStartButtonHoverColor =
    SkColorSetARGB(0xFF, 0x4A, 0x7A, 0x9A);
constexpr SkColor kSelectedBlockLevelBg =
    SkColorSetARGB(0x33, 0x42, 0x85, 0xF4);

// Offset of the bubble relative to the anchor.
constexpr int kBubbleOffsetY = 8;

// Section title font size delta.
constexpr int kSectionTitleSizeDelta = -1;

// Number of preset durations.
constexpr size_t kPresetCount = 4;

// Block level display names.
const char* kBlockLevelNames[] = {
    "None",
    "Social",
    "Entertainment",
    "News",
    "Strict",
    "Custom",
};

}  // namespace

// ---------------------------------------------------------------------------
// AstraFocusModeMenuBubble
// ---------------------------------------------------------------------------

// static
AstraFocusModeMenuBubble* AstraFocusModeMenuBubble::Show(
    views::View* anchor_view,
    AstraFocusModeIndicatorDelegate* delegate,
    AstraFocusModeModel* model,
    base::TimeDelta remaining_time,
    size_t distraction_count,
    bool is_paused,
    AstraFocusPhase phase) {
  if (!anchor_view) {
    return nullptr;
  }

  auto* bubble = new AstraFocusModeMenuBubble(
      delegate, model, remaining_time, distraction_count, is_paused, phase);
  bubble->CreateWidget(anchor_view);
  return bubble;
}

AstraFocusModeMenuBubble::AstraFocusModeMenuBubble(
    AstraFocusModeIndicatorDelegate* delegate,
    AstraFocusModeModel* model,
    base::TimeDelta remaining_time,
    size_t distraction_count,
    bool is_paused,
    AstraFocusPhase phase)
    : delegate_(delegate),
      model_(model),
      remaining_time_(remaining_time),
      distraction_count_(distraction_count),
      is_paused_(is_paused),
      current_phase_(phase),
      is_active_(is_paused || remaining_time > base::TimeDelta()) {
  DCHECK(delegate_);

  if (model_) {
    block_level_ = model_->GetBlockLevel();
    total_duration_ = model_->GetDuration();
  }

  SetBackground(views::CreateRoundedRectBackground(
      kBubbleBackgroundColor, kBubbleCornerRadius));

  BuildLayout();
}

AstraFocusModeMenuBubble::~AstraFocusModeMenuBubble() {
  if (widget_ && observing_widget_) {
    widget_->RemoveObserver(this);
  }
}

// -- Visibility ------------------------------------------------------------

void AstraFocusModeMenuBubble::Show(const gfx::Rect& anchor_rect) {
  if (!widget_) {
    return;
  }
  // Reposition relative to anchor.
  int x = anchor_rect.right() - kBubbleWidth;
  int y = anchor_rect.bottom() + kBubbleOffsetY;
  widget_->SetBounds(gfx::Rect(x, y, kBubbleWidth, 0));
  widget_->Show();
}

void AstraFocusModeMenuBubble::Hide() {
  if (widget_) {
    widget_->Hide();
  }
}

bool AstraFocusModeMenuBubble::IsVisible() const {
  return widget_ && widget_->IsVisible();
}

// -- Duration --------------------------------------------------------------

void AstraFocusModeMenuBubble::SetDuration(base::TimeDelta duration) {
  total_duration_ = duration;
  if (time_label_) {
    time_label_->SetText(FormatTime(duration));
  }
}

base::TimeDelta AstraFocusModeMenuBubble::GetDuration() const {
  return total_duration_;
}

// -- Block level -----------------------------------------------------------

void AstraFocusModeMenuBubble::SetBlockLevel(AstraFocusBlockLevel level) {
  block_level_ = level;
  UpdateBlockLevelHighlight();
}

AstraFocusBlockLevel AstraFocusModeMenuBubble::GetBlockLevel() const {
  return block_level_;
}

// -- Active state ----------------------------------------------------------

void AstraFocusModeMenuBubble::SetIsActive(bool active) {
  is_active_ = active;
  UpdateStartButton();
  UpdatePauseResumeButton();
}

bool AstraFocusModeMenuBubble::IsActive() const {
  return is_active_;
}

// -- Time remaining --------------------------------------------------------

void AstraFocusModeMenuBubble::SetTimeRemaining(base::TimeDelta remaining) {
  remaining_time_ = remaining;
  if (time_label_) {
    time_label_->SetText(FormatTime(remaining));
  }
}

base::TimeDelta AstraFocusModeMenuBubble::GetTimeRemaining() const {
  return remaining_time_;
}

// -- Presets ---------------------------------------------------------------

void AstraFocusModeMenuBubble::SelectPreset(int preset_index) {
  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  if (preset_index < 0 ||
      preset_index >= static_cast<int>(presets.size())) {
    return;
  }
  base::TimeDelta duration = presets[preset_index];
  SetDuration(duration);
  SetTimeRemaining(duration);
  // Also update total duration.
  total_duration_ = duration;

  // Update all preset button highlights.
  for (size_t i = 0; i < preset_buttons_.size(); ++i) {
    if (static_cast<int>(i) == preset_index) {
      preset_buttons_[i]->SetBackground(views::CreateRoundedRectBackground(
          kSelectedBlockLevelBg, 4));
    } else {
      preset_buttons_[i]->SetBackground(views::CreateRoundedRectBackground(
          kButtonBackgroundColor, 4));
    }
  }
}

int AstraFocusModeMenuBubble::GetPresetCount() const {
  return static_cast<int>(preset_buttons_.size());
}

// -- Action buttons --------------------------------------------------------

void AstraFocusModeMenuBubble::StartFocusAction() {
  if (!delegate_) {
    return;
  }
  // Start using the current total duration.
  // TODO(astra): Add proper StartFocusMode to delegate interface.
  // For now, use extend as a way to start.
  is_active_ = true;
  is_paused_ = false;
  UpdateStartButton();
  UpdatePauseResumeButton();
}

void AstraFocusModeMenuBubble::PauseFocusAction() {
  if (!delegate_) {
    return;
  }
  delegate_->OnPauseFocusMode();
  is_paused_ = true;
  UpdatePauseResumeButton();
}

void AstraFocusModeMenuBubble::ResumeFocusAction() {
  if (!delegate_) {
    return;
  }
  delegate_->OnResumeFocusMode();
  is_paused_ = false;
  UpdatePauseResumeButton();
}

void AstraFocusModeMenuBubble::EndFocusAction() {
  if (!delegate_) {
    return;
  }
  delegate_->OnEndFocusMode();
  is_active_ = false;
  is_paused_ = false;
  UpdateStartButton();
  UpdatePauseResumeButton();
}

void AstraFocusModeMenuBubble::ExtendFocusAction(base::TimeDelta extension) {
  if (!delegate_) {
    return;
  }
  delegate_->OnExtendFocusMode(extension);
}

// -- Legacy API ------------------------------------------------------------

void AstraFocusModeMenuBubble::UpdateRemainingTime(base::TimeDelta remaining) {
  SetTimeRemaining(remaining);
}

void AstraFocusModeMenuBubble::UpdateDistractionCount(size_t count) {
  distraction_count_ = count;
  if (distraction_label_) {
    std::u16string text;
    if (count == 0) {
      text = u"No distractions blocked";
    } else if (count == 1) {
      text = u"1 distraction blocked";
    } else {
      text = base::ASCIIToUTF16(
          base::StringPrintf("%zu distractions blocked", count));
    }
    distraction_label_->SetText(text);
  }
}

void AstraFocusModeMenuBubble::UpdatePausedState(bool is_paused) {
  is_paused_ = is_paused;
  UpdatePauseResumeButton();
}

void AstraFocusModeMenuBubble::UpdatePhase(AstraFocusPhase phase) {
  current_phase_ = phase;
  // Update title label to reflect current phase.
  if (title_label_) {
    std::u16string title;
    switch (phase) {
      case AstraFocusPhase::kWork:
        title = u"Focus Mode";
        break;
      case AstraFocusPhase::kShortBreak:
        title = u"Short Break";
        break;
      case AstraFocusPhase::kLongBreak:
        title = u"Long Break";
        break;
    }
    title_label_->SetText(title);
  }
}

void AstraFocusModeMenuBubble::UpdateStats() {
  UpdateStatsLabels();
}

void AstraFocusModeMenuBubble::Close() {
  if (widget_) {
    widget_->Close();
  }
}

// -- Widget creation -------------------------------------------------------

void AstraFocusModeMenuBubble::CreateWidget(views::View* anchor_view) {
  DCHECK(anchor_view);

  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.ownership = views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET;

  // Anchor to the indicator widget's native window.
  views::Widget* anchor_widget = anchor_view->GetWidget();
  if (anchor_widget) {
    params.parent = anchor_widget->GetNativeView();
  }

  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.delegate = new views::WidgetDelegateView();

  // Position the bubble below the anchor, right-aligned.
  gfx::Rect anchor_bounds = anchor_view->GetBoundsInScreen();
  int x = anchor_bounds.right() - kBubbleWidth;
  int y = anchor_bounds.bottom() + kBubbleOffsetY;
  params.bounds = gfx::Rect(x, y, kBubbleWidth, 0);

  widget_ = new views::Widget();
  widget_->Init(std::move(params));

  // Set this view as the widget's content.
  views::View* contents = widget_->GetContentsView();
  contents->SetLayoutManager(std::make_unique<views::FillLayout>());
  contents->AddChildView(this);

  // Observe widget destruction.
  widget_->AddObserver(this);
  observing_widget_ = true;

  // Set initial content.
  UpdateRemainingTime(remaining_time_);
  UpdateDistractionCount(distraction_count_);
  UpdatePauseResumeButton();
  UpdateStartButton();
  UpdateStatsLabels();
  UpdateBlockLevelHighlight();

  widget_->Show();
}

// -- Layout building -------------------------------------------------------

void AstraFocusModeMenuBubble::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kContentPadding, kContentPadding),
      kRowSpacing));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  BuildHeader();
  BuildActionButtons();

  // Section divider.
  auto* divider1 = AddChildView(std::make_unique<views::View>());
  divider1->SetPreferredSize(gfx::Size(0, kSectionSpacing - kRowSpacing));

  BuildPresets();

  // Section divider.
  auto* divider2 = AddChildView(std::make_unique<views::View>());
  divider2->SetPreferredSize(gfx::Size(0, kSectionSpacing - kRowSpacing));

  BuildBlockLevelSelector();

  // Section divider.
  auto* divider3 = AddChildView(std::make_unique<views::View>());
  divider3->SetPreferredSize(gfx::Size(0, kSectionSpacing - kRowSpacing));

  BuildStats();

  // Section divider.
  auto* divider4 = AddChildView(std::make_unique<views::View>());
  divider4->SetPreferredSize(gfx::Size(0, kSectionSpacing - kRowSpacing));

  BuildDistractionInfo();
  BuildSettingsLink();
}

void AstraFocusModeMenuBubble::BuildHeader() {
  auto* header_row = AddChildView(std::make_unique<views::View>());
  auto* header_layout = header_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          8));
  header_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  title_label_ = header_row->AddChildView(
      std::make_unique<views::Label>(u"Focus Mode"));
  title_label_->SetFontList(
      title_label_->font_list().DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  title_label_->SetEnabledColor(kBubbleTextColor);
  title_label_->SetAutoColorReadabilityEnabled(false);
  title_label_->SetFocusBehavior(views::View::FocusBehavior::NEVER);

  time_label_ = header_row->AddChildView(
      std::make_unique<views::Label>(u""));
  time_label_->SetFontList(
      time_label_->font_list().DeriveWithWeight(gfx::Font::Weight::MEDIUM));
  time_label_->SetEnabledColor(kBubbleSecondaryTextColor);
  time_label_->SetAutoColorReadabilityEnabled(false);
  time_label_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
}

void AstraFocusModeMenuBubble::BuildActionButtons() {
  auto* button_row = AddChildView(std::make_unique<views::View>());
  auto* button_layout = button_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          6));
  button_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);
  button_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  int button_width = (kBubbleWidth - 2 * kContentPadding - 18) / 4;

  // Start button.
  start_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraFocusModeMenuBubble::OnStartClicked,
              base::Unretained(this)),
          u"Start"));
  start_button_->SetFocusForPlatform();
  start_button_->SetAccessibleName(u"Start focus session");
  start_button_->SetTooltipText(u"Start a new focus session");
  start_button_->SetBorder(views::NullBorder());
  start_button_->SetTextColor(views::Button::STATE_NORMAL, kBubbleTextColor);
  start_button_->SetTextColor(views::Button::STATE_HOVERED, kBubbleTextColor);
  start_button_->SetTextColor(views::Button::STATE_PRESSED, kBubbleTextColor);
  start_button_->SetBackground(views::CreateRoundedRectBackground(
      kStartButtonBackgroundColor, 4));
  start_button_->SetPreferredSize(gfx::Size(button_width, kButtonHeight));

  // Pause/resume button.
  pause_resume_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraFocusModeMenuBubble::OnPauseResumeClicked,
              base::Unretained(this)),
          u"Pause"));
  pause_resume_button_->SetFocusForPlatform();
  pause_resume_button_->SetAccessibleName(u"Pause or resume focus session");
  pause_resume_button_->SetTooltipText(u"Pause or resume the focus session");
  pause_resume_button_->SetBorder(views::NullBorder());
  pause_resume_button_->SetTextColor(views::Button::STATE_NORMAL,
                                      kBubbleTextColor);
  pause_resume_button_->SetTextColor(views::Button::STATE_HOVERED,
                                      kBubbleTextColor);
  pause_resume_button_->SetTextColor(views::Button::STATE_PRESSED,
                                      kBubbleTextColor);
  pause_resume_button_->SetPreferredSize(
      gfx::Size(button_width, kButtonHeight));

  // Break button.
  break_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraFocusModeMenuBubble::OnBreakClicked,
              base::Unretained(this)),
          u"Break"));
  break_button_->SetFocusForPlatform();
  break_button_->SetAccessibleName(u"Take a break");
  break_button_->SetTooltipText(u"Start a break from focus mode");
  break_button_->SetBorder(views::NullBorder());
  break_button_->SetTextColor(views::Button::STATE_NORMAL, kBubbleTextColor);
  break_button_->SetTextColor(views::Button::STATE_HOVERED, kBubbleTextColor);
  break_button_->SetTextColor(views::Button::STATE_PRESSED, kBubbleTextColor);
  break_button_->SetBackground(views::CreateRoundedRectBackground(
      kBreakButtonBackgroundColor, 4));
  break_button_->SetPreferredSize(gfx::Size(button_width, kButtonHeight));

  // End button.
  end_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraFocusModeMenuBubble::OnEndClicked,
              base::Unretained(this)),
          u"End"));
  end_button_->SetFocusForPlatform();
  end_button_->SetAccessibleName(u"End focus mode");
  end_button_->SetTooltipText(u"End the focus session now");
  end_button_->SetBorder(views::NullBorder());
  end_button_->SetTextColor(views::Button::STATE_NORMAL, kBubbleTextColor);
  end_button_->SetTextColor(views::Button::STATE_HOVERED, kBubbleTextColor);
  end_button_->SetTextColor(views::Button::STATE_PRESSED, kBubbleTextColor);
  end_button_->SetBackground(views::CreateRoundedRectBackground(
      kEndButtonBackgroundColor, 4));
  end_button_->SetPreferredSize(gfx::Size(button_width, kButtonHeight));
}

void AstraFocusModeMenuBubble::BuildPresets() {
  auto* section_title = AddChildView(
      std::make_unique<views::Label>(u"Quick Start"));
  section_title->SetFontList(
      section_title->font_list().DeriveWithSizeDelta(kSectionTitleSizeDelta)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  section_title->SetEnabledColor(kBubbleSecondaryTextColor);
  section_title->SetAutoColorReadabilityEnabled(false);
  section_title->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  section_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  auto* preset_row = AddChildView(std::make_unique<views::View>());
  auto* preset_layout = preset_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          6));
  preset_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);
  preset_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  int button_width =
      (kBubbleWidth - 2 * kContentPadding - 6 * (presets.size() - 1)) /
      presets.size();

  for (size_t i = 0; i < presets.size(); ++i) {
    int minutes = static_cast<int>(presets[i].InMinutes());
    auto* button = preset_row->AddChildView(
        std::make_unique<views::LabelButton>(
            base::BindRepeating(
                &AstraFocusModeMenuBubble::OnPresetClicked,
                base::Unretained(this), static_cast<int>(i)),
            base::ASCIIToUTF16(base::StringPrintf("%dm", minutes))));
    button->SetFocusForPlatform();
    button->SetAccessibleName(base::ASCIIToUTF16(
        base::StringPrintf("Start %d minute focus session", minutes)));
    button->SetTooltipText(base::ASCIIToUTF16(
        base::StringPrintf("Start a %d minute focus session", minutes)));
    button->SetBorder(views::NullBorder());
    button->SetTextColor(views::Button::STATE_NORMAL, kBubbleTextColor);
    button->SetTextColor(views::Button::STATE_HOVERED, kBubbleTextColor);
    button->SetTextColor(views::Button::STATE_PRESSED, kBubbleTextColor);
    button->SetBackground(views::CreateRoundedRectBackground(
        kButtonBackgroundColor, 4));
    button->SetPreferredSize(gfx::Size(button_width, kPresetButtonHeight));
    preset_buttons_.push_back(button);
  }
}

void AstraFocusModeMenuBubble::BuildBlockLevelSelector() {
  auto* section_title = AddChildView(
      std::make_unique<views::Label>(u"Distraction Blocking"));
  section_title->SetFontList(
      section_title->font_list().DeriveWithSizeDelta(kSectionTitleSizeDelta)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  section_title->SetEnabledColor(kBubbleSecondaryTextColor);
  section_title->SetAutoColorReadabilityEnabled(false);
  section_title->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  section_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  auto* block_row = AddChildView(std::make_unique<views::View>());
  auto* block_layout = block_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(),
          4));
  block_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);
  block_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // 6 levels: None, Social, Entertainment, News, Strict, Custom.
  const int kNumLevels = 6;
  int button_width =
      (kBubbleWidth - 2 * kContentPadding - 4 * (kNumLevels - 1)) / kNumLevels;

  for (int i = 0; i < kNumLevels; ++i) {
    AstraFocusBlockLevel level = static_cast<AstraFocusBlockLevel>(i);
    auto* button = block_row->AddChildView(
        std::make_unique<views::LabelButton>(
            base::BindRepeating(
                &AstraFocusModeMenuBubble::OnBlockLevelClicked,
                base::Unretained(this), level),
            base::ASCIIToUTF16(kBlockLevelNames[i])));
    button->SetFocusForPlatform();
    button->SetAccessibleName(base::ASCIIToUTF16(
        base::StringPrintf("Set block level to %s", kBlockLevelNames[i])));
    button->SetTooltipText(base::ASCIIToUTF16(
        base::StringPrintf("Block level: %s", kBlockLevelNames[i])));
    button->SetBorder(views::NullBorder());
    button->SetTextColor(views::Button::STATE_NORMAL, kBubbleTextColor);
    button->SetTextColor(views::Button::STATE_HOVERED, kBubbleTextColor);
    button->SetTextColor(views::Button::STATE_PRESSED, kBubbleTextColor);
    button->SetBackground(views::CreateRoundedRectBackground(
        kButtonBackgroundColor, 4));
    button->SetPreferredSize(
        gfx::Size(button_width, kBlockLevelButtonHeight));
    // Use smaller font for block level buttons.
    button->SetFontList(button->font_list().DeriveWithSizeDelta(-1));
    block_level_buttons_.push_back(button);
  }
}

void AstraFocusModeMenuBubble::BuildStats() {
  auto* section_title = AddChildView(
      std::make_unique<views::Label>(u"Today's Stats"));
  section_title->SetFontList(
      section_title->font_list().DeriveWithSizeDelta(kSectionTitleSizeDelta)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  section_title->SetEnabledColor(kBubbleSecondaryTextColor);
  section_title->SetAutoColorReadabilityEnabled(false);
  section_title->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  section_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  stats_total_label_ = AddChildView(
      std::make_unique<views::Label>(u""));
  stats_total_label_->SetEnabledColor(kBubbleTextColor);
  stats_total_label_->SetAutoColorReadabilityEnabled(false);
  stats_total_label_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  stats_total_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  stats_total_label_->SetFontList(
      stats_total_label_->font_list().DeriveWithSizeDelta(-1));

  stats_sessions_label_ = AddChildView(
      std::make_unique<views::Label>(u""));
  stats_sessions_label_->SetEnabledColor(kBubbleTextColor);
  stats_sessions_label_->SetAutoColorReadabilityEnabled(false);
  stats_sessions_label_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  stats_sessions_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  stats_sessions_label_->SetFontList(
      stats_sessions_label_->font_list().DeriveWithSizeDelta(-1));

  stats_streak_label_ = AddChildView(
      std::make_unique<views::Label>(u""));
  stats_streak_label_->SetEnabledColor(kBubbleTextColor);
  stats_streak_label_->SetAutoColorReadabilityEnabled(false);
  stats_streak_label_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  stats_streak_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  stats_streak_label_->SetFontList(
      stats_streak_label_->font_list().DeriveWithSizeDelta(-1));

  // Initialize stats labels.
  UpdateStatsLabels();
}

void AstraFocusModeMenuBubble::BuildDistractionInfo() {
  distraction_label_ = AddChildView(
      std::make_unique<views::Label>(u""));
  distraction_label_->SetFontList(
      distraction_label_->font_list().DeriveWithSizeDelta(-1));
  distraction_label_->SetEnabledColor(kBubbleSecondaryTextColor);
  distraction_label_->SetAutoColorReadabilityEnabled(false);
  distraction_label_->SetFocusBehavior(views::View::FocusBehavior::NEVER);
  distraction_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
}

void AstraFocusModeMenuBubble::BuildSettingsLink() {
  settings_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraFocusModeMenuBubble::OnSettingsClicked,
          base::Unretained(this)),
      u"Focus mode settings"));
  settings_button_->SetFocusForPlatform();
  settings_button_->SetAccessibleName(u"Open focus mode settings");
  settings_button_->SetTooltipText(u"Configure focus mode preferences");
  settings_button_->SetBorder(views::NullBorder());
  settings_button_->SetTextColor(views::Button::STATE_NORMAL,
                                  kBubbleSecondaryTextColor);
  settings_button_->SetTextColor(views::Button::STATE_HOVERED,
                                  kBubbleTextColor);
  settings_button_->SetTextColor(views::Button::STATE_PRESSED,
                                  kBubbleTextColor);
  settings_button_->SetFontList(
      settings_button_->font_list().DeriveWithSizeDelta(-1));
  settings_button_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
}

// -- WidgetObserver --------------------------------------------------------

void AstraFocusModeMenuBubble::OnWidgetDestroying(views::Widget* widget) {
  DCHECK_EQ(widget, widget_);
  widget_->RemoveObserver(this);
  observing_widget_ = false;
  widget_ = nullptr;
}

// -- View ------------------------------------------------------------------

void AstraFocusModeMenuBubble::OnThemeChanged() {
  views::View::OnThemeChanged();
  // TODO(astra): Update colors from color provider.
}

bool AstraFocusModeMenuBubble::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  // Escape key closes the menu.
  if (accelerator.key_code() == ui::VKEY_ESCAPE) {
    Close();
    return true;
  }
  return false;
}

// -- Private helpers -------------------------------------------------------

std::u16string AstraFocusModeMenuBubble::FormatTime(
    base::TimeDelta delta) const {
  if (delta < base::Hours(1)) {
    int total_seconds = static_cast<int>(delta.InSeconds());
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    return base::ASCIIToUTF16(base::StringPrintf("%02d:%02d",
                                                 minutes, seconds));
  } else {
    int total_minutes = static_cast<int>(delta.InMinutes());
    int hours = total_minutes / 60;
    int minutes = total_minutes % 60;
    return base::ASCIIToUTF16(base::StringPrintf("%dh %02dm",
                                                 hours, minutes));
  }
}

std::u16string AstraFocusModeMenuBubble::FormatDurationLong(
    base::TimeDelta duration) const {
  int total_minutes = static_cast<int>(duration.InMinutes());
  if (total_minutes < 60) {
    return base::ASCIIToUTF16(
        base::StringPrintf("%d min", total_minutes));
  }
  int hours = total_minutes / 60;
  int minutes = total_minutes % 60;
  if (minutes == 0) {
    return base::ASCIIToUTF16(base::StringPrintf("%d h", hours));
  }
  return base::ASCIIToUTF16(
      base::StringPrintf("%dh %dm", hours, minutes));
}

std::u16string AstraFocusModeMenuBubble::FormatBlockLevel(
    AstraFocusBlockLevel level) const {
  size_t index = static_cast<size_t>(level);
  if (index < std::size(kBlockLevelNames)) {
    return base::ASCIIToUTF16(kBlockLevelNames[index]);
  }
  return u"Unknown";
}

void AstraFocusModeMenuBubble::OnPauseResumeClicked(
    const ui::Event& /*event*/) {
  if (!delegate_) {
    return;
  }
  if (is_paused_) {
    delegate_->OnResumeFocusMode();
    is_paused_ = false;
  } else {
    delegate_->OnPauseFocusMode();
    is_paused_ = true;
  }
  UpdatePauseResumeButton();
}

void AstraFocusModeMenuBubble::OnBreakClicked(const ui::Event& /*event*/) {
  if (delegate_) {
    delegate_->OnStartBreak();
  }
}

void AstraFocusModeMenuBubble::OnEndClicked(const ui::Event& /*event*/) {
  if (delegate_) {
    delegate_->OnEndFocusMode();
  }
}

void AstraFocusModeMenuBubble::OnStartClicked(const ui::Event& /*event*/) {
  StartFocusAction();
}

void AstraFocusModeMenuBubble::OnPresetClicked(int index) {
  SelectPreset(index);
  // Also extend or start a session with this preset duration.
  if (delegate_) {
    const auto& presets = AstraFocusModeModel::GetPresetDurations();
    if (index >= 0 && index < static_cast<int>(presets.size())) {
      delegate_->OnExtendFocusMode(presets[index]);
    }
  }
}

void AstraFocusModeMenuBubble::OnBlockLevelClicked(AstraFocusBlockLevel level) {
  SetBlockLevel(level);
  // Also update the model if available.
  if (model_) {
    model_->SetBlockLevel(level);
  }
}

void AstraFocusModeMenuBubble::OnSettingsClicked(const ui::Event& /*event*/) {
  if (delegate_) {
    delegate_->OnOpenSettings();
  }
}

void AstraFocusModeMenuBubble::UpdatePauseResumeButton() {
  if (!pause_resume_button_) {
    return;
  }
  if (is_paused_) {
    pause_resume_button_->SetText(u"Resume");
    pause_resume_button_->SetAccessibleName(u"Resume focus session");
    pause_resume_button_->SetTooltipText(u"Resume the paused focus session");
  } else {
    pause_resume_button_->SetText(u"Pause");
    pause_resume_button_->SetAccessibleName(u"Pause focus session");
    pause_resume_button_->SetTooltipText(u"Pause the focus session");
  }
}

void AstraFocusModeMenuBubble::UpdateStartButton() {
  if (!start_button_) {
    return;
  }
  // Start button is shown when session is not active.
  if (is_active_) {
    start_button_->SetEnabled(false);
  } else {
    start_button_->SetEnabled(true);
  }
}

void AstraFocusModeMenuBubble::UpdateStatsLabels() {
  if (!stats_total_label_ || !stats_sessions_label_ || !stats_streak_label_) {
    return;
  }

  int total_minutes = 0;
  int sessions = 0;
  int streak = 0;

  if (model_) {
    total_minutes = model_->total_focus_minutes_today();
    sessions = model_->sessions_today();
    streak = model_->current_streak_days();
  }

  // Total focus time.
  stats_total_label_->SetText(base::ASCIIToUTF16(
      base::StringPrintf("\uD83D\uDD52 %s total focus",
                         base::UTF16ToUTF8(
                             FormatDurationLong(base::Minutes(total_minutes)))
                             .c_str())));

  // Sessions completed.
  if (sessions == 1) {
    stats_sessions_label_->SetText(u"\uD83D\uDCCA 1 session completed");
  } else {
    stats_sessions_label_->SetText(base::ASCIIToUTF16(
        base::StringPrintf("\uD83D\uDCCA %d sessions completed", sessions)));
  }

  // Streak.
  if (streak == 1) {
    stats_streak_label_->SetText(u"\uD83D\uDD25 1 day streak");
  } else {
    stats_streak_label_->SetText(base::ASCIIToUTF16(
        base::StringPrintf("\uD83D\uDD25 %d day streak", streak)));
  }
}

void AstraFocusModeMenuBubble::UpdateBlockLevelHighlight() {
  for (size_t i = 0; i < block_level_buttons_.size(); ++i) {
    AstraFocusBlockLevel level = static_cast<AstraFocusBlockLevel>(i);
    if (level == block_level_) {
      block_level_buttons_[i]->SetBackground(
          views::CreateRoundedRectBackground(kSelectedBlockLevelBg, 4));
    } else {
      block_level_buttons_[i]->SetBackground(
          views::CreateRoundedRectBackground(kButtonBackgroundColor, 4));
    }
  }
}

}  // namespace astra
