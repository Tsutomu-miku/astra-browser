#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_NOTE_EDITOR_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_NOTE_EDITOR_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"

namespace views {
class Label;
class LabelButton;
class Textfield;
}  // namespace views

namespace astra {

// Delegate interface for AstraNoteEditorView actions.
// Implemented by the parent view (e.g. AstraSidebarNotesView) which has
// access to the note service. The editor view itself does not know about
// services — it only handles presentation and text editing.
class AstraNoteEditorDelegate {
 public:
  virtual ~AstraNoteEditorDelegate() = default;

  // Called when the user saves or auto-saves the note.
  // |note_id| is the note being edited (empty for a new note).
  // |title| and |content| are the current editor values.
  // |color| is the selected accent color.
  // The delegate should call into AstraNoteService to persist.
  // Returns the id of the saved note (useful for new notes).
  virtual std::string OnNoteEditorSave(const std::string& note_id,
                                       const std::string& title,
                                       const std::string& content,
                                       const std::string& color) = 0;

  // Called when the user cancels / closes the editor.
  // The delegate should hide the editor and return to the list view.
  virtual void OnNoteEditorCancel() = 0;

  // Called when the user deletes the note from the editor.
  virtual void OnNoteEditorDelete(const std::string& note_id) = 0;
};

// Note editor view for creating and editing notes.
// Contains a title text field, a multi-line content text area,
// color picker options, and save/cancel buttons.
//
// This is a pure presentation view — it does not own note state.
// All data flows in via LoadNote() and out via the delegate.
//
// TODO(astra): Replace placeholder color picker with a proper color
// selection widget. For now, we use a row of colored circle buttons.
// Chromium owner: ColorPicker / ColorChooser (ui/views/color_chooser/)
// or a custom implementation using SkColor.
//
// TODO(astra): Add auto-save with debouncing. Currently we save on every
// keystroke via OnContentsChanged, but in a real implementation we'd
// debounce with a timer to avoid excessive pref writes.
// Chromium pattern: base::OneShotTimer for debounced auto-save.
class AstraNoteEditorView : public views::View,
                            public views::TextfieldController {
 public:
  // Mode: creating a new note or editing an existing one.
  enum class Mode {
    kNew,     // Creating a new note (shows "New Note" title, no delete)
    kEdit,    // Editing an existing note (shows title, has delete)
  };

  explicit AstraNoteEditorView(Mode mode);
  AstraNoteEditorView(const AstraNoteEditorView&) = delete;
  AstraNoteEditorView& operator=(const AstraNoteEditorView&) = delete;
  ~AstraNoteEditorView() override;

  // Load an existing note into the editor.
  // Sets the title, content, and color fields.
  void LoadNote(const std::string& note_id,
                const std::string& title,
                const std::string& content,
                const std::string& color);

  // Clear the editor and reset to empty state.
  void ClearEditor();

  // Set the delegate for action callbacks. Not owned by this view.
  void set_delegate(AstraNoteEditorDelegate* delegate) { delegate_ = delegate; }

  // Get the current note ID being edited (empty for new notes).
  const std::string& note_id() const { return note_id_; }

  // Get the current editor mode.
  Mode mode() const { return mode_; }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

  // -- TextfieldController -----------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Build the color picker row with colored circle buttons.
  void BuildColorPicker();

  // Handle the save button press.
  void OnSaveButtonPressed();

  // Handle the cancel button press.
  void OnCancelButtonPressed();

  // Handle the delete button press.
  void OnDeleteButtonPressed();

  // Handle a color button press.
  void OnColorButtonPressed(const std::string& color_hex);

  // Update the selected color button visual state.
  void UpdateSelectedColorButton();

  // Get the current title from the text field (as UTF-8).
  std::string GetTitleText() const;

  // Get the current content from the text area (as UTF-8).
  std::string GetContentText() const;

  // Mode: new note or edit existing note.
  Mode mode_;

  // ID of the note being edited. Empty for new notes.
  std::string note_id_;

  // Currently selected accent color (hex string).
  std::string selected_color_;

  // Action delegate. Not owned.
  raw_ptr<AstraNoteEditorDelegate> delegate_ = nullptr;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::Textfield> title_field_ = nullptr;
  raw_ptr<views::Textfield> content_field_ = nullptr;
  raw_ptr<views::View> color_picker_row_ = nullptr;
  raw_ptr<views::LabelButton> save_button_ = nullptr;
  raw_ptr<views::LabelButton> cancel_button_ = nullptr;
  raw_ptr<views::LabelButton> delete_button_ = nullptr;

  // Color buttons in the picker (for updating selection state).
  // Each pair is (color_hex, button_view).
  std::vector<std::pair<std::string, raw_ptr<views::View>>> color_buttons_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_NOTE_EDITOR_VIEW_H_
