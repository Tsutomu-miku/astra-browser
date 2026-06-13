// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_NOTES_ASTRA_TAB_NOTES_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_NOTES_ASTRA_TAB_NOTES_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
class Textfield;
class Textarea;
}  // namespace views

namespace astra {

// =========================================================================
// AstraTabNoteItemView — single tab note item
// =========================================================================
//
// A row showing a note attached to a tab: page title, note preview, and
// last updated time.
//
// Layout:
//   +-------------------------------------------+
//   |  Page Title                               |
//   |  Note preview text...           2h ago   |
//   +-------------------------------------------+
// =========================================================================

class AstraTabNoteItemView : public views::View {
 public:
  using SelectCallback =
      base::RepeatingCallback<void(const std::string& note_id)>;
  using DeleteCallback =
      base::RepeatingCallback<void(const std::string& note_id)>;

  struct NoteInfo {
    std::string note_id;
    std::string tab_id;
    std::u16string page_title;
    std::u16string note_content;
    std::u16string page_url;
    base::Time last_updated;
    bool has_note = false;
  };

  AstraTabNoteItemView(const NoteInfo& info,
                       SelectCallback select_callback,
                       DeleteCallback delete_callback);
  ~AstraTabNoteItemView() override;

  AstraTabNoteItemView(const AstraTabNoteItemView&) = delete;
  AstraTabNoteItemView& operator=(const AstraTabNoteItemView&) = delete;

  const std::string& note_id() const { return note_id_; }
  const std::string& tab_id() const { return tab_id_; }

  void SetContent(const std::u16string& content);

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnSelect();
  void OnDelete();
  std::u16string FormatTime();

  std::string note_id_;
  std::string tab_id_;
  std::u16string page_title_;
  std::u16string note_content_;
  std::u16string page_url_;
  base::Time last_updated_;
  bool has_note_ = false;

  SelectCallback select_callback_;
  DeleteCallback delete_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> preview_label_ = nullptr;
  raw_ptr<views::Label> time_label_ = nullptr;
};

// =========================================================================
// AstraTabNotesView — tab notes management panel
// =========================================================================
//
// A bubble showing all notes attached to tabs. Users can browse, search,
// edit, and delete tab notes. Each note is Astra metadata attached to a
// Chromium WebContents.
//
// Layout:
//   +-------------------------------------------+
//   |  Tab Notes                    [Close]    |
//   +-------------------------------------------+
//   |  🔍 Search notes...                       |
//   +-------------------------------------------+
//   |  Page Title One                           |
//   |  This is a note about the...   2h ago   |
//   +-------------------------------------------+
//   |  Page Title Two                           |
//   |  Reminder: check this later     1d ago   |
//   +-------------------------------------------+
//   |  ...                                      |
//   +-------------------------------------------+
//   |  [+ Add note to current tab]             |
//   +-------------------------------------------+
//
// This is a presentation-only view. Note data is persisted by Astra's
// tab notes service as WebContentsUserData-style metadata.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - content::WebContentsUserData (note attachment point)
// =========================================================================

class AstraTabNotesView : public views::BubbleDialogDelegateView {
 public:
  using NoteSelectedCallback =
      base::RepeatingCallback<void(const std::string& note_id)>;
  using NoteDeletedCallback =
      base::RepeatingCallback<void(const std::string& note_id)>;
  using AddNoteCallback = base::RepeatingClosure;
  using SearchCallback =
      base::RepeatingCallback<void(const std::u16string& query)>;

  explicit AstraTabNotesView(views::View* anchor_view);
  ~AstraTabNotesView() override;

  AstraTabNotesView(const AstraTabNotesView&) = delete;
  AstraTabNotesView& operator=(const AstraTabNotesView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetNotes(const std::vector<AstraTabNoteItemView::NoteInfo>& notes);

  // -- Callbacks -----------------------------------------------------------

  void SetNoteSelectedCallback(NoteSelectedCallback callback);
  void SetNoteDeletedCallback(NoteDeletedCallback callback);
  void SetAddNoteCallback(AddNoteCallback callback);
  void SetSearchCallback(SearchCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildSearchBar();
  void BuildNotesList();
  void BuildFooter();

  void RefreshNotes();
  void OnSearchTextChanged();
  void OnAddNote();

  std::vector<AstraTabNoteItemView::NoteInfo> notes_;

  NoteSelectedCallback select_callback_;
  NoteDeletedCallback delete_callback_;
  AddNoteCallback add_note_callback_;
  SearchCallback search_callback_;

  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::View> notes_list_ = nullptr;
  raw_ptr<views::MdTextButton> add_button_ = nullptr;

  std::vector<raw_ptr<AstraTabNoteItemView>> note_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_NOTES_ASTRA_TAB_NOTES_VIEW_H_
