#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_NOTES_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_NOTES_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/browser/astra_note_service.h"
#include "astra/ui/views/sidebar/astra_note_editor_view.h"
#include "astra/ui/views/sidebar/astra_note_item_view.h"

namespace views {
class Label;
class LabelButton;
class Textfield;
class View;
}  // namespace views

namespace astra {

// =========================================================================
// AstraSidebarNotesView
// =========================================================================
//
// Sidebar section showing notes from AstraNoteService.
// Shows "Page notes" (notes for the current URL) and "All notes" sections,
// with a "+ New note" button and a search box.
//
// This is a pure projection view:
//   - It observes AstraNoteService for changes and rebuilds the note list
//     reactively.
//   - It never mutates note state directly — all user actions (create,
//     edit, delete) go through the service via the delegate pattern.
//   - The truth source is AstraNoteService.
//
// Sections (top to bottom):
//   1. Header: "Notes" title + New note button
//   2. Search box
//   3. Page notes section (notes linked to the current URL)
//   4. All notes section (all notes, most recent first)
//   5. Note editor (shown inline when creating/editing, replaces list)
//
// TODO(astra): Consider using AstraSidebarSectionView as the container
// for each sub-section (Page notes, All notes), similar to how favorites
// and pinned tabs use it. The notes view has a different structure with
// the editor inline, so for now we use a custom layout.
// Chromium UI reference: BookmarkEditor / Reading list UI.
// =========================================================================

class AstraSidebarNotesView : public views::View,
                              public AstraNoteServiceObserver,
                              public AstraNoteItemDelegate,
                              public AstraNoteEditorDelegate,
                              public views::TextfieldController {
 public:
  explicit AstraSidebarNotesView(AstraNoteService* note_service);
  AstraSidebarNotesView(const AstraSidebarNotesView&) = delete;
  AstraSidebarNotesView& operator=(const AstraSidebarNotesView&) = delete;
  ~AstraSidebarNotesView() override;

  // Refresh the entire notes list from the service.
  void UpdateFromService();

  // Set the current URL for "Page notes" section.
  // When set, shows notes associated with this URL in the page notes section.
  void SetCurrentUrl(const GURL& url);
  const GURL& current_url() const { return current_url_; }

  // Show the note editor for creating a new note.
  // If |with_url| is true, pre-links the new note to the current URL.
  void ShowNewNoteEditor(bool link_to_current_url = false);

  // Show the note editor for editing an existing note.
  void ShowNoteEditor(const std::string& note_id);

  // Hide the editor and return to the list view.
  void HideEditor();

  // Toggle whether the notes section is expanded.
  void SetExpanded(bool expanded);
  bool expanded() const { return expanded_; }

  // -- AstraNoteServiceObserver -------------------------------------------

  void OnNoteAdded(const AstraNote& note) override;
  void OnNoteUpdated(const AstraNote& note) override;
  void OnNoteRemoved(const std::string& note_id) override;
  void OnNotesReloaded() override;

  // -- AstraNoteItemDelegate ----------------------------------------------

  void OnNoteItemClicked(const std::string& note_id) override;
  void OnNoteDeleteRequested(const std::string& note_id) override;

  // -- AstraNoteEditorDelegate --------------------------------------------

  std::string OnNoteEditorSave(const std::string& note_id,
                               const std::string& title,
                               const std::string& content,
                               const std::string& color) override;
  void OnNoteEditorCancel() override;
  void OnNoteEditorDelete(const std::string& note_id) override;

  // -- TextfieldController (search box) -----------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Build the child views and layout. Called once from constructor.
  void BuildLayout();

  // Populate the page notes section from the service.
  void PopulatePageNotes();

  // Populate the all notes section from the service.
  void PopulateAllNotes();

  // Create a note item view from a note struct.
  std::unique_ptr<AstraNoteItemView> CreateNoteItemView(
      const AstraNote& note);

  // Format a base::Time into a user-friendly display string.
  // E.g. "2h ago", "Yesterday", "Jun 12".
  static std::u16 FormatTime(base::Time time);

  // Extract a short snippet from the note content (first line, truncated).
  static std::u16 GetContentSnippet(const std::string& content);

  // Handle the "New note" button press.
  void OnNewNoteButtonPressed();

  // Handle the header click (toggle expand/collapse).
  void OnHeaderClicked();

  raw_ptr<AstraNoteService> note_service_ = nullptr;

  // Observation of the note service for reactive UI updates.
  base::ScopedObservation<AstraNoteService, AstraNoteServiceObserver>
      service_observation_{this};

  // Current URL for page notes. Empty if no page context.
  GURL current_url_;

  // Whether the notes section is expanded.
  bool expanded_ = true;

  // Whether we're currently showing the editor (true) or the list (false).
  bool showing_editor_ = false;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> header_row_ = nullptr;
  raw_ptr<views::Label> header_label_ = nullptr;
  raw_ptr<views::LabelButton> new_note_button_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;

  // Container for list content (page notes + all notes sections).
  raw_ptr<views::View> list_container_ = nullptr;
  raw_ptr<views::View> page_notes_section_ = nullptr;
  raw_ptr<views::Label> page_notes_label_ = nullptr;
  raw_ptr<views::View> page_notes_container_ = nullptr;
  raw_ptr<views::Label> all_notes_label_ = nullptr;
  raw_ptr<views::View> all_notes_container_ = nullptr;

  // Note editor view (shown inline when editing).
  raw_ptr<AstraNoteEditorView> note_editor_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_NOTES_VIEW_H_
