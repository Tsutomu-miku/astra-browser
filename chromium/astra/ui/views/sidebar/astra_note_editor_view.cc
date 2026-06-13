#include "astra/ui/views/sidebar/astra_note_editor_view.h"

#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kEditorTopPadding = 8;
constexpr int kEditorBottomPadding = 12;
constexpr int kEditorHorizontalPadding = 12;
constexpr int kEditorSectionSpacing = 12;
constexpr int kEditorItemSpacing = 6;
constexpr int kEditorTitleHeight = 28;
constexpr int kEditorContentHeight = 120;
constexpr int kColorButtonSize = 20;
constexpr int kColorButtonSpacing = 8;
constexpr int kColorButtonCornerRadius = 10;
constexpr int kButtonRowSpacing = 8;

// Default note color (amber).
constexpr char kDefaultNoteColor[] = "#FFD93D";

// Available note colors for the color picker.
// Each is a hex color string.
constexpr const char* kNoteColors[] = {
    "#FFD93D",  // Yellow / amber (default)
    "#5AD8A6",  // Green
    "#5B8FF9",  // Blue
    "#E86452",  // Red / coral
    "#9270CA",  // Purple
    "#F6BD16",  // Orange
    "#6DC8EC",  // Light blue
    "#945FB4",  // Violet
};

constexpr size_t kNoteColorCount = std::size(kNoteColors);

// Header labels.
const char16_t kNewNoteHeader[] = u"New Note";
const char16_t kEditNoteHeader[] = u"Edit Note";

// Placeholder text.
const char16_t kTitlePlaceholder[] = u"Title";
const char16_t kContentPlaceholder[] = u"Write your note here...";

// Button labels.
const char16_t kSaveButtonLabel[] = u"Save";
const char16_t kCancelButtonLabel[] = u"Cancel";
const char16_t kDeleteButtonLabel[] = u"Delete";

// Astra color ID for note editor header text.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra owner: UI Color System (astra/ui/color/astra_color_ids.h)
constexpr ui::ColorId kEditorHeaderColorId = kColorAstraSidebarItemText;

// TODO(astra): Add Astra-specific editor/border/button color IDs to
// astra_color_ids.h. For now, reuse Chromium's button and border colors.
// Chromium owner: views::Textfield, views::LabelButton
constexpr ui::ColorId kEditorBorderColor = ui::kColorButtonBorder;
constexpr ui::ColorId kEditorButtonBgColor =
    ui::kColorButtonBackgroundProminent;
constexpr ui::ColorId kEditorButtonTextColor =
    ui::kColorButtonForegroundProminent;

}  // namespace

AstraNoteEditorView::AstraNoteEditorView(Mode mode)
    : mode_(mode), selected_color_(kDefaultNoteColor) {
  BuildLayout();
}

AstraNoteEditorView::~AstraNoteEditorView() = default;

void AstraNoteEditorView::LoadNote(const std::string& note_id,
                                   const std::string& title,
                                   const std::string& content,
                                   const std::string& color) {
  note_id_ = note_id;
  mode_ = Mode::kEdit;

  if (header_label_) {
    header_label_->SetText(kEditNoteHeader);
  }

  if (title_field_) {
    title_field_->SetText(base::UTF8ToUTF16(title));
  }

  if (content_field_) {
    content_field_->SetText(base::UTF8ToUTF16(content));
  }

  if (!color.empty()) {
    selected_color_ = color;
    UpdateSelectedColorButton();
  }

  // Show delete button in edit mode.
  if (delete_button_) {
    delete_button_->SetVisible(true);
  }
}

void AstraNoteEditorView::ClearEditor() {
  note_id_.clear();
  mode_ = Mode::kNew;
  selected_color_ = kDefaultNoteColor;

  if (header_label_) {
    header_label_->SetText(kNewNoteHeader);
  }
  if (title_field_) {
    title_field_->SetText(std::u16string());
  }
  if (content_field_) {
    content_field_->SetText(std::u16string());
  }
  if (delete_button_) {
    delete_button_->SetVisible(false);
  }
  UpdateSelectedColorButton();
}

void AstraNoteEditorView::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kEditorTopPadding, kEditorHorizontalPadding),
      kEditorSectionSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Header label.
  header_label_ = AddChildView(std::make_unique<views::Label>(
      mode_ == Mode::kNew ? kNewNoteHeader : kEditNoteHeader));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetFontList(
      header_label_->font_list().DeriveWithSizeDelta(2).DeriveWithWeight(
          gfx::Font::Weight::MEDIUM));

  // Title text field.
  title_field_ = AddChildView(std::make_unique<views::Textfield>());
  title_field_->SetPlaceholderText(kTitlePlaceholder);
  title_field_->SetController(this);
  title_field_->SetAccessibleName(u"Note title");
  title_field_->SetBorder(views::CreateSolidBorder(1, SK_ColorGRAY));
  // TODO(astra): Use proper color provider for the border.
  // Chromium pattern: views::Textfield uses the native theme by default.

  // Content text area (multi-line).
  content_field_ = AddChildView(std::make_unique<views::Textfield>());
  content_field_->SetPlaceholderText(kContentPlaceholder);
  content_field_->SetController(this);
  content_field_->SetAccessibleName(u"Note content");
  content_field_->SetMultiLine(true);
  content_field_->SetBorder(views::CreateSolidBorder(1, SK_ColorGRAY));
  content_field_->SetPreferredSize(
      gfx::Size(0, kEditorContentHeight));
  // TODO(astra): Use a proper Textarea / multi-line text view.
  // Chromium owner: views::Textfield with multi-line or a dedicated
  // Textarea view (ui/views/controls/textfield/).

  // Color picker row.
  BuildColorPicker();

  // Button row: Save + Cancel (and Delete for edit mode).
  auto* button_row = AddChildView(std::make_unique<views::View>());
  auto* button_row_layout = button_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kButtonRowSpacing));
  button_row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  button_row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kEnd);

  // Delete button (only visible in edit mode).
  delete_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(&AstraNoteEditorView::OnDeleteButtonPressed,
                              base::Unretained(this)),
          kDeleteButtonLabel));
  delete_button_->SetVisible(mode_ == Mode::kEdit);

  // Spacer to push save/cancel to the right.
  auto* spacer = button_row->AddChildView(std::make_unique<views::View>());
  button_row_layout->SetFlexForView(spacer, 1);

  // Cancel button.
  cancel_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(&AstraNoteEditorView::OnCancelButtonPressed,
                              base::Unretained(this)),
          kCancelButtonLabel));

  // Save button.
  save_button_ = button_row->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(&AstraNoteEditorView::OnSaveButtonPressed,
                              base::Unretained(this)),
          kSaveButtonLabel));
}

void AstraNoteEditorView::BuildColorPicker() {
  color_picker_row_ = AddChildView(std::make_unique<views::View>());
  auto* color_layout = color_picker_row_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kColorButtonSpacing));
  color_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // "Color:" label.
  auto* color_label = color_picker_row_->AddChildView(
      std::make_unique<views::Label>(u"Color:"));
  color_label->SetAutoColorReadabilityEnabled(false);
  color_label->SetFontList(
      color_label->font_list().DeriveWithSizeDelta(-1));

  for (size_t i = 0; i < kNoteColorCount; ++i) {
    const char* color_hex = kNoteColors[i];

    auto color_button = std::make_unique<views::Button>(
        base::BindRepeating(&AstraNoteEditorView::OnColorButtonPressed,
                            base::Unretained(this),
                            std::string(color_hex)));
    color_button->SetPreferredSize(
        gfx::Size(kColorButtonSize, kColorButtonSize));
    color_button->SetPaintToLayer();
    color_button->layer()->SetFillsBoundsOpaquely(true);
    color_button->layer()->SetRoundedCornerRadius(
        gfx::RoundedCornersF(kColorButtonCornerRadius));
    color_button->SetTooltipText(base::UTF8ToUTF16(color_hex));

    // Parse and set the background color.
    unsigned int r, g, b;
    if (sscanf(color_hex, "#%02x%02x%02x", &r, &g, &b) == 3) {
      color_button->layer()->SetColor(
          SkColorSetRGB(static_cast<unsigned char>(r),
                        static_cast<unsigned char>(g),
                        static_cast<unsigned char>(b)));
    }

    raw_ptr<views::View> button_ptr =
        color_picker_row_->AddChildView(std::move(color_button));
    color_buttons_.emplace_back(color_hex, button_ptr);
  }

  UpdateSelectedColorButton();
}

void AstraNoteEditorView::OnSaveButtonPressed() {
  if (delegate_) {
    std::string saved_id = delegate_->OnNoteEditorSave(
        note_id_, GetTitleText(), GetContentText(), selected_color_);

    // If this was a new note, update our state to reflect the saved note.
    if (mode_ == Mode::kNew && !saved_id.empty()) {
      note_id_ = saved_id;
      mode_ = Mode::kEdit;
      if (header_label_) {
        header_label_->SetText(kEditNoteHeader);
      }
      if (delete_button_) {
        delete_button_->SetVisible(true);
      }
    }
  }
}

void AstraNoteEditorView::OnCancelButtonPressed() {
  if (delegate_) {
    delegate_->OnNoteEditorCancel();
  }
}

void AstraNoteEditorView::OnDeleteButtonPressed() {
  if (!note_id_.empty() && delegate_) {
    delegate_->OnNoteEditorDelete(note_id_);
  }
}

void AstraNoteEditorView::OnColorButtonPressed(const std::string& color_hex) {
  selected_color_ = color_hex;
  UpdateSelectedColorButton();

  // TODO(astra): Auto-save color change? For now, the color only takes
  // effect when the user clicks Save, which is consistent with how the
  // title and content work.
}

void AstraNoteEditorView::UpdateSelectedColorButton() {
  for (const auto& [color_hex, button] : color_buttons_) {
    if (!button || !button->layer()) {
      continue;
    }
    // Add a border/ring for the selected color, or adjust opacity.
    // For simplicity, we'll add a white border to unselected and a
    // dark border to selected, or use opacity.
    //
    // TODO(astra): Use a proper selection indicator (e.g. checkmark or
    // outer ring). For now, scale the selected button slightly to show
    // selection.
    if (color_hex == selected_color_) {
      button->layer()->SetOpacity(1.0f);
      // TODO(astra): Add a selection ring around the selected color.
    } else {
      button->layer()->SetOpacity(0.6f);
    }
  }
}

std::string AstraNoteEditorView::GetTitleText() const {
  if (!title_field_) {
    return std::string();
  }
  return base::UTF16ToUTF8(title_field_->GetText());
}

std::string AstraNoteEditorView::GetContentText() const {
  if (!content_field_) {
    return std::string();
  }
  return base::UTF16ToUTF8(content_field_->GetText());
}

// =========================================================================
// TextfieldController
// =========================================================================

void AstraNoteEditorView::ContentsChanged(views::Textfield* sender,
                                          const std::u16string& new_contents) {
  // TODO(astra): Auto-save with debouncing.
  // For now, we only save when the user explicitly clicks Save.
  // Auto-save would use a base::OneShotTimer to debounce keystrokes
  // and call delegate_->OnNoteEditorSave(...) after a short delay.
  //
  // Chromium pattern: base::OneShotTimer + Start(FROM_HERE, delay, ...)
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraNoteEditorView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraNoteEditorView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (header_label_) {
    header_label_->SetEnabledColor(
        color_provider->GetColor(kEditorHeaderColorId));
  }

  // TODO(astra): Style text fields with proper ColorProvider colors.
  // The default Textfield styling should work with the native theme.
}

}  // namespace astra
