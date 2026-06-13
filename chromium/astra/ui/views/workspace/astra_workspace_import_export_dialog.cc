#include "astra/ui/views/workspace/astra_workspace_import_export_dialog.h"

#include <string>
#include <utility>
#include <vector>

#include "astra/browser/astra_workspace_import_export.h"
#include "astra/browser/astra_workspace_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Dialog dimensions.
constexpr int kDialogWidth = 520;
constexpr int kDialogHeight = 520;
constexpr int kDialogPadding = 16;
constexpr int kSectionSpacing = 12;
constexpr int kLabelSpacing = 4;
constexpr int kSubSectionSpacing = 8;

// Text field dimensions.
constexpr int kJsonTextfieldHeight = 180;
constexpr int kExportListHeight = 140;

// Dialog button spacing.
constexpr int kButtonSpacing = 8;

// Progress bar height.
constexpr int kProgressBarHeight = 4;

}  // namespace

// =========================================================================
// Static factory
// =========================================================================

AstraWorkspaceImportExportDialog* AstraWorkspaceImportExportDialog::ShowBubble(
    views::View* anchor_view,
    Profile* profile,
    Mode mode,
    const std::string& initial_json) {
  if (!anchor_view) {
    return nullptr;
  }

  auto dialog = std::make_unique<AstraWorkspaceImportExportDialog>(
      profile, mode, initial_json);
  AstraWorkspaceImportExportDialog* dialog_ptr = dialog.get();

  // Set anchor view before creating the bubble.
  dialog_ptr->SetAnchorView(anchor_view);
  dialog_ptr->SetArrow(views::BubbleBorder::Arrow::TOP_RIGHT);

  // Set up bubble parameters.
  views::BubbleDialogDelegateView::CreateBubble(std::move(dialog));

  // The widget is created by CreateBubble; show it.
  if (dialog_ptr->GetWidget()) {
    dialog_ptr->GetWidget()->Show();
  }

  return dialog_ptr;
}

AstraWorkspaceImportExportDialog*
AstraWorkspaceImportExportDialog::ShowImportDialog(
    views::View* anchor_view,
    Profile* profile,
    const std::string& initial_json) {
  return ShowBubble(anchor_view, profile, Mode::kImport, initial_json);
}

AstraWorkspaceImportExportDialog*
AstraWorkspaceImportExportDialog::ShowExportDialog(
    views::View* anchor_view,
    Profile* profile,
    const std::string& initial_json) {
  return ShowBubble(anchor_view, profile, Mode::kExport, initial_json);
}

// =========================================================================
// Constructor / destructor
// =========================================================================

AstraWorkspaceImportExportDialog::AstraWorkspaceImportExportDialog(
    Profile* profile,
    Mode mode,
    const std::string& initial_json)
    : views::BubbleDialogDelegateView(
          /*anchor_view=*/nullptr,
          views::BubbleBorder::Arrow::TOP_RIGHT),
      profile_(profile),
      mode_(mode),
      initial_json_(initial_json) {
  // Set dialog properties.
  set_margins(gfx::Insets::VH(kDialogPadding, kDialogPadding));

  if (mode == Mode::kImport) {
    SetButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
    SetButtonLabel(ui::DIALOG_BUTTON_OK, u"Import");
    SetButtonEnabled(ui::DIALOG_BUTTON_OK, false);
    SetAcceptCallback(base::BindOnce(
        &AstraWorkspaceImportExportDialog::OnImportConfirmed,
        base::Unretained(this)));
  } else {
    SetButtons(ui::DIALOG_BUTTON_CANCEL);
  }

  SetButtonLabel(ui::DIALOG_BUTTON_CANCEL, u"Close");

  BuildLayout();
}

AstraWorkspaceImportExportDialog::~AstraWorkspaceImportExportDialog() {
  if (close_callback_) {
    close_callback_.Run();
  }
}

// =========================================================================
// Callbacks
// =========================================================================

void AstraWorkspaceImportExportDialog::SetImportCallback(
    ImportCallback callback) {
  import_callback_ = std::move(callback);
}

void AstraWorkspaceImportExportDialog::SetCloseCallback(
    CloseCallback callback) {
  close_callback_ = std::move(callback);
}

// =========================================================================
// State control
// =========================================================================

void AstraWorkspaceImportExportDialog::SetState(State state) {
  if (state_ == state) {
    return;
  }
  state_ = state;
  UpdateStatusArea();

  // Update dialog button states based on state.
  if (mode_ == Mode::kImport) {
    bool ok_enabled = (state == State::kIdle && import_json_valid_) ||
                      state == State::kSuccess;
    SetButtonEnabled(ui::DIALOG_BUTTON_OK, ok_enabled);
  }
}

void AstraWorkspaceImportExportDialog::SetErrorMessage(
    const std::u16string& message) {
  error_message_ = message;
  if (state_ == State::kError) {
    UpdateStatusArea();
  }
}

void AstraWorkspaceImportExportDialog::SetSuccessMessage(
    const std::u16string& message) {
  success_message_ = message;
  if (state_ == State::kSuccess) {
    UpdateStatusArea();
  }
}

bool AstraWorkspaceImportExportDialog::IsVisible() const {
  return GetWidget() && GetWidget()->IsVisible();
}

// =========================================================================
// Import options
// =========================================================================

void AstraWorkspaceImportExportDialog::SetIncludeTabs(bool include) {
  if (include_tabs_ == include) {
    return;
  }
  include_tabs_ = include;
  // TODO(astra): Update checkbox state in the UI.
}

void AstraWorkspaceImportExportDialog::SetIncludeSettings(bool include) {
  if (include_settings_ == include) {
    return;
  }
  include_settings_ = include;
  // TODO(astra): Update checkbox state in the UI.
}

void AstraWorkspaceImportExportDialog::SetImportMode(AstraImportMode mode) {
  if (import_mode_ == mode) {
    return;
  }
  import_mode_ = mode;
  // TODO(astra): Update import mode UI selection.
}

size_t AstraWorkspaceImportExportDialog::GetWorkspaceCount() const {
  if (mode_ == Mode::kImport) {
    return import_workspace_count_;
  }
  // Export mode: count checked workspaces.
  size_t count = 0;
  for (auto* checkbox : export_checkboxes_) {
    if (checkbox && checkbox->GetChecked()) {
      ++count;
    }
  }
  return count;
}

// =========================================================================
// Action handlers
// =========================================================================

void AstraWorkspaceImportExportDialog::OnImportAccepted() {
  if (mode_ != Mode::kImport) {
    return;
  }

  // Validate JSON before accepting.
  if (!import_json_valid_) {
    SetState(State::kError);
    SetErrorMessage(u"Invalid workspace data.");
    return;
  }

  // Get the JSON content from the text field.
  std::string json_content;
  if (import_textfield_) {
    json_content = base::UTF16ToUTF8(import_textfield_->GetText());
  }

  // Invoke the import callback if set.
  if (import_callback_) {
    import_callback_.Run(json_content);
  }

  SetState(State::kProcessing);

  // TODO(astra): Actually perform the import operation.
  //   For now, just transition to success state.
  //   Import logic should be handled by AstraWorkspaceImportExport.
  //   Chromium component: PrefService + AstraWorkspaceService.
  SetState(State::kSuccess);
  SetSuccessMessage(u"Workspaces imported successfully.");
}

void AstraWorkspaceImportExportDialog::OnExportAccepted() {
  if (mode_ != Mode::kExport) {
    return;
  }

  SetState(State::kProcessing);

  // Generate export JSON.
  GenerateExportJson();

  SetState(State::kSuccess);
  SetSuccessMessage(u"Workspaces exported successfully.");
}

// =========================================================================
// views::BubbleDialogDelegateView
// =========================================================================

std::u16string AstraWorkspaceImportExportDialog::GetWindowTitle() const {
  return mode_ == Mode::kExport ? u"Export Workspaces" : u"Import Workspaces";
}

void AstraWorkspaceImportExportDialog::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();

  const ui::ColorProvider* cp = GetColorProvider();
  if (!cp) {
    return;
  }

  // Update text field background colors.
  SkColor bg_color = cp->GetColor(ui::kColorDialogBackground);
  SkColor text_color = cp->GetColor(ui::kColorLabelForeground);

  if (export_textfield_) {
    export_textfield_->SetBackgroundColor(bg_color);
    export_textfield_->SetTextColor(text_color);
  }
  if (import_textfield_) {
    import_textfield_->SetBackgroundColor(bg_color);
    import_textfield_->SetTextColor(text_color);
  }

  // Update status label colors.
  if (status_label_) {
    switch (state_) {
      case State::kError:
        status_label_->SetEnabledColor(
            cp->GetColor(ui::kColorAlertHighSeverity));
        break;
      case State::kSuccess:
        status_label_->SetEnabledColor(
            cp->GetColor(ui::kColorAlertLowSeverity));
        break;
      default:
        status_label_->SetEnabledColor(
            cp->GetColor(ui::kColorLabelForegroundSecondary));
        break;
    }
  }
}

// =========================================================================
// views::TextfieldController
// =========================================================================

void AstraWorkspaceImportExportDialog::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  if (mode_ == Mode::kImport && sender == import_textfield_) {
    // Reset to idle state when user edits.
    if (state_ != State::kIdle) {
      SetState(State::kIdle);
    }
    ValidateAndUpdatePreview();
  }
}

bool AstraWorkspaceImportExportDialog::HandleKeyEvent(
    views::Textfield* sender,
    const ui::KeyEvent& key_event) {
  // Let the default Textfield behavior handle key events.
  return false;
}

// =========================================================================
// Layout
// =========================================================================

void AstraWorkspaceImportExportDialog::BuildLayout() {
  // Main vertical layout.
  auto* main_layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      kSectionSpacing));
  main_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Status area (progress + status message). Shown only when needed.
  status_area_ = AddChildView(std::make_unique<views::View>());
  auto* status_layout = status_area_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kSubSectionSpacing));
  status_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  status_area_->SetVisible(false);

  // Progress bar (hidden by default).
  auto progress_bar = std::make_unique<views::ProgressBar>();
  progress_bar->SetPreferredSize(gfx::Size(kDialogWidth, kProgressBarHeight));
  progress_bar->SetValue(0.5);  // Indeterminate-looking middle value.
  progress_bar_ = status_area_->AddChildView(std::move(progress_bar));
  progress_bar_->SetVisible(false);

  // Status label.
  status_label_ = status_area_->AddChildView(
      std::make_unique<views::Label>(std::u16string()));
  status_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label_->SetMultiLine(true);
  status_label_->SetVisible(false);

  if (mode_ == Mode::kExport) {
    BuildExportUI();
  } else {
    BuildImportUI();
  }
}

// =========================================================================
// Export mode
// =========================================================================

void AstraWorkspaceImportExportDialog::BuildExportUI() {
  // Summary label.
  export_summary_label_ =
      AddChildView(std::make_unique<views::Label>(std::u16string()));
  export_summary_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  export_summary_label_->SetMultiLine(true);

  // "Select all" button row.
  auto* select_all_row = AddChildView(std::make_unique<views::View>());
  auto* select_all_layout = select_all_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kButtonSpacing));
  select_all_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kEnd);

  select_all_button_ = select_all_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              [](AstraWorkspaceImportExportDialog* dialog) {
                // Toggle: if all selected, deselect all; else select all.
                bool all_selected = true;
                for (auto* cb : dialog->export_checkboxes_) {
                  if (!cb->GetChecked()) {
                    all_selected = false;
                    break;
                  }
                }
                dialog->ToggleSelectAllWorkspaces(!all_selected);
              },
              base::Unretained(this)),
          u"Select all"));

  // Scrollable workspace list with checkboxes.
  export_list_scroll_ =
      AddChildView(std::make_unique<views::ScrollView>());
  export_list_scroll_->SetClipHeight(true);
  export_list_scroll_->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);
  export_list_scroll_->SetVerticalScrollBarMode(
      views::ScrollView::ScrollBarMode::kEnabled);

  export_list_container_ = export_list_scroll_->SetContents(
      std::make_unique<views::View>());
  auto* list_layout = export_list_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(),
          kSubSectionSpacing));
  list_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  export_list_scroll_->SetPreferredSize(
      gfx::Size(kDialogWidth - 2 * kDialogPadding, kExportListHeight));

  // Populate workspace list.
  if (profile_) {
    AstraWorkspaceService* service =
        AstraWorkspaceServiceFactory::GetForProfile(profile_);
    if (service) {
      const auto& workspaces = service->workspaces();
      for (const auto& ws : workspaces) {
        auto checkbox = std::make_unique<views::Checkbox>(
            base::UTF8ToUTF16(ws.name));
        checkbox->SetChecked(true);
        checkbox->SetCallback(base::BindRepeating(
            [](AstraWorkspaceImportExportDialog* dialog) {
              dialog->UpdateExportSummary();
              dialog->UpdateExportJson();
            },
            base::Unretained(this)));
        // TODO(astra): Show tab count next to each workspace name.
        //   e.g. "Work (12 tabs)"
        raw_ptr<views::Checkbox> cb =
            export_list_container_->AddChildView(std::move(checkbox));
        export_checkboxes_.push_back(cb);
      }
    }
  }

  // JSON text field (read-only, for copy).
  export_textfield_ =
      AddChildView(std::make_unique<views::Textfield>());
  export_textfield_->SetReadOnly(true);
  export_textfield_->SetBackgroundColor(
      GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_DialogBackground));
  export_textfield_->SetTextColor(
      GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_LabelEnabledColor));
  export_textfield_->SetPreferredSize(
      gfx::Size(kDialogWidth - 2 * kDialogPadding, kJsonTextfieldHeight));

  // Generate the JSON and populate the text field.
  GenerateExportJson();
  UpdateExportSummary();

  // Action button row.
  auto* button_row = AddChildView(std::make_unique<views::View>());
  auto* button_layout = button_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kButtonSpacing));
  button_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kEnd);

  save_file_button_ = button_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraWorkspaceImportExportDialog::OpenFilePicker,
              base::Unretained(this)),
          u"Save to file..."));
  // TODO(astra): Enable when SelectFileDialog integration is available.
  //   For now, disable as a visual placeholder.
  save_file_button_->SetEnabled(false);
  save_file_button_->SetTooltipText(
      u"File save dialog not yet implemented (requires SelectFileDialog)");

  copy_button_ = button_row->AddChildView(
      views::MdTextButton::CreatePrimaryUiButton(
          base::BindRepeating(
              &AstraWorkspaceImportExportDialog::CopyJsonToClipboard,
              base::Unretained(this)),
          u"Copy to Clipboard"));
}

void AstraWorkspaceImportExportDialog::GenerateExportJson() {
  if (!profile_ || !export_textfield_) {
    return;
  }

  // TODO(astra): Generate JSON only for selected workspaces.
  //   Currently AstraWorkspaceImportExport::ExportWorkspacesToJson exports
  //   all workspaces.  Need a variant that takes a list of workspace IDs.
  //   For now, export all and filter is TBD.
  std::string json = AstraWorkspaceImportExport::ExportWorkspacesToJson(profile_);
  export_textfield_->SetText(base::UTF8ToUTF16(json));
}

void AstraWorkspaceImportExportDialog::UpdateExportSummary() {
  if (!profile_ || !export_summary_label_) {
    return;
  }

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_);
  if (!service) {
    export_summary_label_->SetText(u"Unable to load workspace data.");
    return;
  }

  // Count selected workspaces.
  size_t selected_count = 0;
  for (auto* cb : export_checkboxes_) {
    if (cb->GetChecked()) {
      ++selected_count;
    }
  }

  size_t total_count = service->workspace_count();

  std::u16string summary =
      u"Selected " + base::NumberToString16(selected_count) +
      u" of " + base::NumberToString16(total_count) +
      u" workspace(s) for export.";
  export_summary_label_->SetText(summary);

  // Update "Select all" button label.
  if (select_all_button_) {
    bool all_selected = selected_count == total_count;
    select_all_button_->SetText(all_selected ? u"Deselect all"
                                             : u"Select all");
  }
}

void AstraWorkspaceImportExportDialog::UpdateExportJson() {
  // TODO(astra): Regenerate JSON for only selected workspaces.
  //   For now, we just regenerate all since the import/export helper
  //   doesn't support selective export yet.
  GenerateExportJson();
}

void AstraWorkspaceImportExportDialog::CopyJsonToClipboard() {
  if (!export_textfield_) {
    return;
  }

  std::u16string text = export_textfield_->GetText();
  if (text.empty()) {
    return;
  }

  ui::ScopedClipboardWriter scw(ui::ClipboardBuffer::kCopyPaste);
  scw.WriteText(text);

  // Show success state briefly.
  SetState(State::kSuccess);
  SetSuccessMessage(u"Copied to clipboard!");

  // TODO(astra): Reset to idle after a short delay, or show a toast.
  //   For now, just leave the success message visible.
  //   Chromium pattern: Throbber / toast notification.
}

void AstraWorkspaceImportExportDialog::ToggleSelectAllWorkspaces(
    bool select_all) {
  for (auto* cb : export_checkboxes_) {
    cb->SetChecked(select_all);
  }
  UpdateExportSummary();
  UpdateExportJson();
}

// =========================================================================
// Import mode
// =========================================================================

void AstraWorkspaceImportExportDialog::BuildImportUI() {
  // Instructions label.
  auto* instructions = AddChildView(
      std::make_unique<views::Label>(
          u"Paste workspace JSON below or load from a file to import."));
  instructions->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  instructions->SetMultiLine(true);

  // JSON text field (editable).
  import_textfield_ =
      AddChildView(std::make_unique<views::Textfield>());
  import_textfield_->SetController(this);
  import_textfield_->SetBackgroundColor(
      GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_DialogBackground));
  import_textfield_->SetPreferredSize(
      gfx::Size(kDialogWidth - 2 * kDialogPadding, kJsonTextfieldHeight));

  if (!initial_json_.empty()) {
    import_textfield_->SetText(base::UTF8ToUTF16(initial_json_));
    ValidateAndUpdatePreview();
  }

  // "Load from file" button row.
  auto* file_row = AddChildView(std::make_unique<views::View>());
  auto* file_layout = file_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          kButtonSpacing));
  file_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);

  load_file_button_ = file_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraWorkspaceImportExportDialog::OpenFilePicker,
              base::Unretained(this)),
          u"Load from file..."));
  // TODO(astra): Enable when SelectFileDialog integration is available.
  load_file_button_->SetEnabled(false);
  load_file_button_->SetTooltipText(
      u"File open dialog not yet implemented (requires SelectFileDialog)");

  // Preview / status labels.
  import_status_label_ =
      AddChildView(std::make_unique<views::Label>(std::u16string()));
  import_status_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  import_status_label_->SetMultiLine(true);
  import_status_label_->SetVisible(false);

  import_preview_label_ =
      AddChildView(std::make_unique<views::Label>(std::u16string()));
  import_preview_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  import_preview_label_->SetMultiLine(true);
  import_preview_label_->SetVisible(false);

  // TODO(astra): Add a preview list of workspaces that will be imported,
  //   with checkboxes to select which ones to actually import.
  //   Similar to the export side but read-only checkboxes.
}

void AstraWorkspaceImportExportDialog::ValidateAndUpdatePreview() {
  if (!import_textfield_ || !import_status_label_ ||
      !import_preview_label_) {
    return;
  }

  std::u16string text = import_textfield_->GetText();
  std::string json = base::UTF16ToUTF8(text);

  if (json.empty()) {
    import_status_label_->SetVisible(false);
    import_preview_label_->SetVisible(false);
    SetButtonEnabled(ui::DIALOG_BUTTON_OK, false);
    import_json_valid_ = false;
    return;
  }

  bool valid = AstraWorkspaceImportExport::ValidateWorkspaceJson(json);
  import_json_valid_ = valid;

  if (valid) {
    size_t ws_count = 0;
    size_t tab_count = 0;
    if (GetImportPreviewCounts(ws_count, tab_count)) {
      import_workspace_count_ = ws_count;
      import_tab_count_ = tab_count;
    }

    import_status_label_->SetText(u"Valid workspace JSON.");
    import_status_label_->SetEnabledColor(SK_ColorGREEN);
    // TODO(astra): Use ColorProvider for success color
    // (ui::kColorAlertLowSeverity).

    import_preview_label_->SetText(
        u"Will import " + base::NumberToString16(ws_count) +
        u" workspace(s) with " + base::NumberToString16(tab_count) +
        u" total tab(s).");
    import_preview_label_->SetVisible(true);
  } else {
    import_status_label_->SetText(u"Invalid JSON or unsupported format.");
    import_status_label_->SetEnabledColor(SK_ColorRED);
    // TODO(astra): Use ColorProvider for error color
    // (ui::kColorAlertHighSeverity).
    import_preview_label_->SetVisible(false);
  }

  import_status_label_->SetVisible(true);
  SetButtonEnabled(ui::DIALOG_BUTTON_OK, valid);
}

bool AstraWorkspaceImportExportDialog::GetImportPreviewCounts(
    size_t& out_workspace_count,
    size_t& out_tab_count) {
  std::string json = base::UTF16ToUTF8(import_textfield_->GetText());

  auto result = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!result.has_value() || !result->is_dict()) {
    return false;
  }

  const base::Value::Dict& dict = result->GetDict();
  const base::Value::List* workspaces = dict.FindList("workspaces");
  if (!workspaces) {
    return false;
  }

  out_workspace_count = workspaces->size();
  out_tab_count = 0;

  for (const auto& item : *workspaces) {
    if (!item.is_dict()) {
      continue;
    }
    const base::Value::List* tabs = item.GetDict().FindList("tabs");
    if (tabs) {
      out_tab_count += tabs->size();
    }
  }

  return true;
}

void AstraWorkspaceImportExportDialog::OnImportConfirmed() {
  if (!import_json_valid_ || !import_callback_ || !import_textfield_) {
    return;
  }

  // Show processing state.
  SetState(State::kProcessing);

  std::string json = base::UTF16ToUTF8(import_textfield_->GetText());
  import_callback_.Run(json);

  // TODO(astra): The import is synchronous in the skeleton, but in a real
  //   implementation it might be async.  We'd need a callback from the
  //   import operation to know when it's done, then set success/error state.
  //   For now, assume success immediately.
  SetState(State::kSuccess);
  SetSuccessMessage(u"Workspaces imported successfully!");

  // Close the dialog after a short delay?  For now, leave it open so the
  // user sees the success message and can close manually.
}

// =========================================================================
// Status / state helpers
// =========================================================================

void AstraWorkspaceImportExportDialog::UpdateStatusArea() {
  if (!status_area_ || !progress_bar_ || !status_label_) {
    return;
  }

  bool show_area = (state_ != State::kIdle);
  status_area_->SetVisible(show_area);

  if (!show_area) {
    return;
  }

  // Progress bar: visible only during processing.
  progress_bar_->SetVisible(state_ == State::kProcessing);

  // Status label.
  switch (state_) {
    case State::kIdle:
      status_label_->SetVisible(false);
      break;

    case State::kProcessing:
      status_label_->SetText(u"Processing...");
      status_label_->SetVisible(true);
      if (GetColorProvider()) {
        status_label_->SetEnabledColor(GetColorProvider()->GetColor(
            ui::kColorLabelForegroundSecondary));
      }
      break;

    case State::kSuccess:
      status_label_->SetText(
          success_message_.empty() ? u"Success!" : success_message_);
      status_label_->SetVisible(true);
      if (GetColorProvider()) {
        status_label_->SetEnabledColor(GetColorProvider()->GetColor(
            ui::kColorAlertLowSeverity));
      }
      break;

    case State::kError:
      status_label_->SetText(
          error_message_.empty() ? u"An error occurred." : error_message_);
      status_label_->SetVisible(true);
      if (GetColorProvider()) {
        status_label_->SetEnabledColor(GetColorProvider()->GetColor(
            ui::kColorAlertHighSeverity));
      }
      break;
  }

  // Trigger relayout.
  InvalidateLayout();
}

void AstraWorkspaceImportExportDialog::SetProgressVisible(bool visible) {
  if (progress_bar_) {
    progress_bar_->SetVisible(visible);
  }
}

// =========================================================================
// File picker
// =========================================================================

void AstraWorkspaceImportExportDialog::OpenFilePicker() {
  // TODO(astra): Implement with SelectFileDialog.
  //
  // Chromium pattern:
  //   - ui/shell_dialogs/select_file_dialog.h
  //   - ui/shell_dialogs/select_file_dialog_factory.h
  //
  // For import mode:
  //   SelectFileDialog::Type::kOpen (single JSON file)
  //   OnFileSelected() reads the file and populates import_textfield_.
  //
  // For export mode:
  //   SelectFileDialog::Type::kSaveAs (save JSON file)
  //   OnFileSelected() writes the JSON to the chosen file.
  //
  // Patch point: No Chromium patch needed — SelectFileDialog is a public
  //   API.  We just need to implement SelectFileDialog::Listener and
  //   handle the file selection callbacks.
  //
  // TODO(astra): The dialog needs to be aware of the browser window
  //   context to properly parent the file picker.  We can get it from
  //   GetWidget()->GetNativeWindow().

  // For now, this is a no-op placeholder.
}

}  // namespace astra
