#ifndef ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_IMPORT_EXPORT_DIALOG_H_
#define ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_IMPORT_EXPORT_DIALOG_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/textfield/textfield_controller.h"

class Profile;

namespace views {
class Checkbox;
class Label;
class MdTextButton;
class ProgressBar;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

struct AstraWorkspace;

// =========================================================================
// AstraWorkspaceImportExportDialog
// =========================================================================
//
// Dialog for importing and exporting workspace data as JSON.
//
// Two modes:
//   - Export mode: shows a list of workspaces with checkboxes to select
//     which ones to export, a summary, and a text area with the exported
//     JSON that the user can copy-paste. Also has a "Save to file" button.
//
//   - Import mode: shows a text area for pasting JSON (or loading from
//     file), validates it, shows a preview of workspaces to import, and
//     has confirm/cancel buttons. Shows progress during import and a
//     success / error state.
//
// This dialog is a presentation-only view.  It does not own any state.
// All actual import/export logic is in AstraWorkspaceImportExport
// (browser layer).  This dialog just calls into that helper and displays
// the results.
//
// Architecture:
//   - Truth source: AstraWorkspaceService + TabStripModel (Chromium).
//   - Import/export logic: AstraWorkspaceImportExport (browser layer).
//   - This view: pure presentation, no state of its own.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView (dialog framework).
//   - views::Textfield (JSON text input/output).
//   - views::Checkbox (workspace selection for export).
//   - views::ScrollView (scrollable workspace list).
//   - SelectFileDialog (for file picker, future).
//
// TODO(astra): Integrate with SelectFileDialog for proper file picker
// support.  Currently the dialog uses a textarea for copy-paste JSON.
// The file picker would be better UX but requires integrating with
// ui/shell_dialogs/select_file_dialog.h which has platform-specific
// implementations.
// Chromium owner: SelectFileDialog (ui/shell_dialogs/select_file_dialog.h)
// Patch point: File picker integration for save/load.
// =========================================================================

class AstraWorkspaceImportExportDialog
    : public views::BubbleDialogDelegateView,
      public views::TextfieldController {
 public:
  // Dialog mode.
  enum class Mode {
    kExport,
    kImport,
  };

  // State of the import/export operation.
  enum class State {
    kIdle,       // Dialog shown, waiting for user input.
    kProcessing, // Operation in progress (validating / importing).
    kSuccess,    // Operation completed successfully.
    kError,      // Operation failed.
  };

  // Import mode for handling workspace conflicts during import.
  enum class AstraImportMode {
    kMerge,        // Merge imported workspaces with existing ones
    kReplace,      // Replace existing workspaces with imported ones
    kNewWorkspaces,// Create new workspaces for all imported entries
  };

  // Callback for when the user confirms import.
  // The string parameter is the JSON content to import.
  using ImportCallback = base::RepeatingCallback<void(const std::string& json)>;

  // Callback for when the dialog is closed.
  using CloseCallback = base::RepeatingClosure;

  // Creates and shows the dialog as a bubble anchored to |anchor_view|.
  // Returns a raw pointer to the dialog (owned by the widget).
  //
  // In export mode, |json| is the pre-exported JSON string to display.
  // In import mode, |json| is optional initial content (e.g. from file).
  static AstraWorkspaceImportExportDialog* ShowBubble(
      views::View* anchor_view,
      Profile* profile,
      Mode mode,
      const std::string& initial_json = std::string());

  AstraWorkspaceImportExportDialog(const AstraWorkspaceImportExportDialog&) = delete;
  AstraWorkspaceImportExportDialog& operator=(
      const AstraWorkspaceImportExportDialog&) = delete;
  ~AstraWorkspaceImportExportDialog() override;

  // -- Callbacks (set by the controller / browser view) -------------------

  void SetImportCallback(ImportCallback callback);
  void SetCloseCallback(CloseCallback callback);

  // -- State control (called by controller) -------------------------------

  // Sets the dialog state and updates the UI accordingly.
  void SetState(State state);

  // Sets the error message shown in kError state.
  void SetErrorMessage(const std::u16string& message);

  // Sets the success message shown in kSuccess state.
  void SetSuccessMessage(const std::u16string& message);

  // Returns whether the dialog is currently visible.
  bool IsVisible() const;

  // -- Convenience show methods --------------------------------------------

  // Convenience: shows the dialog in import mode.
  static AstraWorkspaceImportExportDialog* ShowImportDialog(
      views::View* anchor_view,
      Profile* profile,
      const std::string& initial_json = std::string());

  // Convenience: shows the dialog in export mode.
  static AstraWorkspaceImportExportDialog* ShowExportDialog(
      views::View* anchor_view,
      Profile* profile,
      const std::string& initial_json = std::string());

  // -- File path -----------------------------------------------------------

  // Returns the import file path (if loaded from file), or empty string.
  const std::string& GetImportFilePath() const { return import_file_path_; }

  // Returns the export file path (if saved to file), or empty string.
  const std::string& GetExportFilePath() const { return export_file_path_; }

  // -- Import options ------------------------------------------------------

  // Sets whether to include tab data in the import/export.
  void SetIncludeTabs(bool include);

  // Returns whether tabs are included in the import/export.
  bool GetIncludeTabs() const { return include_tabs_; }

  // Sets whether to include settings in the import/export.
  void SetIncludeSettings(bool include);

  // Returns whether settings are included in the import/export.
  bool GetIncludeSettings() const { return include_settings_; }

  // Sets the import mode (merge/replace/new workspaces).
  void SetImportMode(AstraImportMode mode);

  // Returns the current import mode.
  AstraImportMode import_mode() const { return import_mode_; }

  // -- Workspace count -----------------------------------------------------

  // Returns the number of workspaces being imported/exported.
  size_t GetWorkspaceCount() const;

  // -- Action handlers -----------------------------------------------------

  // Called when the user accepts the import (clicks "Import" button).
  void OnImportAccepted();

  // Called when the user accepts the export (clicks "Export" button).
  void OnExportAccepted();

  // -- views::BubbleDialogDelegateView ------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

  // -- views::TextfieldController -----------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

 private:
  AstraWorkspaceImportExportDialog(Profile* profile,
                                   Mode mode,
                                   const std::string& initial_json);

  // Builds the dialog layout.
  void BuildLayout();

  // -- Export mode helpers ------------------------------------------------

  // Builds the export mode UI: workspace list + JSON preview + copy button.
  void BuildExportUI();

  // Generates the export JSON from selected workspaces and updates the
  // text area.
  void GenerateExportJson();

  // Updates the export summary label (N workspaces selected, M tabs).
  void UpdateExportSummary();

  // Updates the JSON text area with current export data.
  void UpdateExportJson();

  // Copies the JSON text to the clipboard.
  void CopyJsonToClipboard();

  // Handles "Select all" / "Deselect all" for export.
  void ToggleSelectAllWorkspaces(bool select_all);

  // -- Import mode helpers ------------------------------------------------

  // Builds the import mode UI: JSON text area + preview + confirm button.
  void BuildImportUI();

  // Validates the current JSON in the text area and updates the preview
  // and status.
  void ValidateAndUpdatePreview();

  // Updates the import preview labels and workspace list.
  void UpdateImportPreview();

  // Called when the user confirms import.
  void OnImportConfirmed();

  // Tries to parse the JSON and returns workspace count and total tab count.
  // Returns false if validation fails.
  bool GetImportPreviewCounts(size_t& out_workspace_count,
                              size_t& out_tab_count);

  // -- Status / state helpers ---------------------------------------------

  // Updates the status area (progress, error, success message).
  void UpdateStatusArea();

  // Shows or hides the progress indicator.
  void SetProgressVisible(bool visible);

  // -- File picker ----------------------------------------------------------

  // Opens a file picker for loading/saving JSON.
  // TODO(astra): Implement with SelectFileDialog.
  void OpenFilePicker();

  // -- Member variables ---------------------------------------------------

  raw_ptr<Profile> profile_;
  Mode mode_;
  State state_ = State::kIdle;
  std::string initial_json_;

  // File paths (for file-based import/export).
  std::string import_file_path_;
  std::string export_file_path_;

  // Import/export options.
  bool include_tabs_ = true;
  bool include_settings_ = false;

  // Import mode for handling workspace conflicts.
  AstraImportMode import_mode_ = AstraImportMode::kMerge;

  // Callbacks.
  ImportCallback import_callback_;
  CloseCallback close_callback_;

  // Status messages.
  std::u16string error_message_;
  std::u16string success_message_;

  // Export mode widgets.
  raw_ptr<views::Label> export_summary_label_ = nullptr;
  raw_ptr<views::ScrollView> export_list_scroll_ = nullptr;
  raw_ptr<views::View> export_list_container_ = nullptr;
  raw_ptr<views::MdTextButton> select_all_button_ = nullptr;
  raw_ptr<views::Textfield> export_textfield_ = nullptr;
  raw_ptr<views::MdTextButton> copy_button_ = nullptr;
  raw_ptr<views::MdTextButton> save_file_button_ = nullptr;

  // Import mode widgets.
  raw_ptr<views::Textfield> import_textfield_ = nullptr;
  raw_ptr<views::Label> import_status_label_ = nullptr;
  raw_ptr<views::Label> import_preview_label_ = nullptr;
  raw_ptr<views::MdTextButton> load_file_button_ = nullptr;

  // Shared status / progress widgets.
  raw_ptr<views::View> status_area_ = nullptr;
  raw_ptr<views::ProgressBar> progress_bar_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;

  // Cached validation state for import mode.
  bool import_json_valid_ = false;
  size_t import_workspace_count_ = 0;
  size_t import_tab_count_ = 0;

  // Checkbox states for export mode (one per workspace).
  std::vector<raw_ptr<views::Checkbox>> export_checkboxes_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_IMPORT_EXPORT_DIALOG_H_
