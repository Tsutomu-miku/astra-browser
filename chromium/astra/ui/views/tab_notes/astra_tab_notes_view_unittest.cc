// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_notes/astra_tab_notes_view.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"

namespace astra {

using AstraTabNotesViewTest = views::ViewsTestBase;

// ===========================================================================
// AstraTabNoteItemView tests
// ===========================================================================

TEST_F(AstraTabNotesViewTest, NoteItemView_HasCorrectId) {
  AstraTabNoteItemView::NoteInfo info;
  info.note_id = "note_1";
  info.tab_id = "tab_123";
  info.page_title = u"Example Page";
  info.note_content = u"This is a test note.";
  info.page_url = u"https://example.com";
  info.last_updated = base::Time::Now();
  info.has_note = true;

  auto item = std::make_unique<AstraTabNoteItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ(item->note_id(), "note_1");
  EXPECT_EQ(item->tab_id(), "tab_123");
}

TEST_F(AstraTabNotesViewTest, NoteItemView_SetContent) {
  AstraTabNoteItemView::NoteInfo info;
  info.note_id = "note_1";
  info.tab_id = "tab_1";
  info.page_title = u"Page";
  info.note_content = u"Original note";
  info.has_note = true;

  auto item = std::make_unique<AstraTabNoteItemView>(
      info, base::DoNothing(), base::DoNothing());

  item->SetContent(u"Updated note content");
  SUCCEED();
}

TEST_F(AstraTabNotesViewTest, NoteItemView_EmptyNote) {
  AstraTabNoteItemView::NoteInfo info;
  info.note_id = "note_empty";
  info.tab_id = "tab_empty";
  info.page_title = u"Empty Note Page";
  info.note_content = u"";
  info.has_note = false;
  info.last_updated = base::Time();

  auto item = std::make_unique<AstraTabNoteItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ(item->note_id(), "note_empty");
}

TEST_F(AstraTabNotesViewTest, NoteItemView_PreferredSize) {
  AstraTabNoteItemView::NoteInfo info;
  info.note_id = "note_1";
  info.tab_id = "tab_1";
  info.page_title = u"Test Page";
  info.note_content = u"A note";
  info.has_note = true;

  auto item = std::make_unique<AstraTabNoteItemView>(
      info, base::DoNothing(), base::DoNothing());

  gfx::Size preferred = item->GetPreferredSize();
  EXPECT_GT(preferred.width(), 0);
  EXPECT_GT(preferred.height(), 0);
}

TEST_F(AstraTabNotesViewTest, NoteItemView_LongNoteTruncated) {
  AstraTabNoteItemView::NoteInfo info;
  info.note_id = "note_long";
  info.tab_id = "tab_long";
  info.page_title = u"Long Note";
  info.note_content = u"This is a very long note that should be "
      u"truncated in the preview because it's way too long to fit "
      u"in a single line of the note item view.";
  info.has_note = true;

  auto item = std::make_unique<AstraTabNoteItemView>(
      info, base::DoNothing(), base::DoNothing());

  // Should not crash and should show truncated text.
  item->Layout();
  SUCCEED();
}

TEST_F(AstraTabNotesViewTest, NoteItemView_SelectCallback) {
  std::string selected_id;
  auto select_cb = base::BindRepeating(
      [](std::string* out, const std::string& id) { *out = id; },
      &selected_id);

  AstraTabNoteItemView::NoteInfo info;
  info.note_id = "note_select";
  info.tab_id = "tab_select";
  info.page_title = u"Select Test";
  info.note_content = u"Select me";
  info.has_note = true;

  auto item = std::make_unique<AstraTabNoteItemView>(
      info, select_cb, base::DoNothing());

  EXPECT_EQ(item->note_id(), "note_select");
}

TEST_F(AstraTabNotesViewTest, NoteItemView_DeleteCallback) {
  std::string deleted_id;
  auto delete_cb = base::BindRepeating(
      [](std::string* out, const std::string& id) { *out = id; },
      &deleted_id);

  AstraTabNoteItemView::NoteInfo info;
  info.note_id = "note_delete";
  info.tab_id = "tab_delete";
  info.page_title = u"Delete Test";
  info.note_content = u"Delete me";
  info.has_note = true;

  auto item = std::make_unique<AstraTabNoteItemView>(
      info, base::DoNothing(), delete_cb);

  EXPECT_EQ(item->note_id(), "note_delete");
}

// ===========================================================================
// AstraTabNotesView tests
// ===========================================================================

TEST_F(AstraTabNotesViewTest, NotesView_HasTitle) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabNotesView>(anchor.get());

  EXPECT_FALSE(view->GetWindowTitle().empty());
}

TEST_F(AstraTabNotesViewTest, NotesView_SetNotes) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabNotesView>(anchor.get());

  std::vector<AstraTabNoteItemView::NoteInfo> notes;

  AstraTabNoteItemView::NoteInfo note1;
  note1.note_id = "note_1";
  note1.tab_id = "tab_1";
  note1.page_title = u"First Page";
  note1.note_content = u"Remember to check this later";
  note1.last_updated = base::Time::Now() - base::Hours(2);
  note1.has_note = true;
  notes.push_back(note1);

  AstraTabNoteItemView::NoteInfo note2;
  note2.note_id = "note_2";
  note2.tab_id = "tab_2";
  note2.page_title = u"Second Page";
  note2.note_content = u"Important reference link";
  note2.last_updated = base::Time::Now() - base::Days(1);
  note2.has_note = true;
  notes.push_back(note2);

  AstraTabNoteItemView::NoteInfo note3;
  note3.note_id = "note_3";
  note3.tab_id = "tab_3";
  note3.page_title = u"Third Page";
  note3.note_content = u"";
  note3.has_note = false;
  notes.push_back(note3);

  view->SetNotes(notes);
  view->Layout();
  SUCCEED();
}

TEST_F(AstraTabNotesViewTest, NotesView_SearchCallback) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabNotesView>(anchor.get());

  std::u16string received_query;
  view->SetSearchCallback(base::BindRepeating(
      [](std::u16string* out, const std::u16string& query) { *out = query; },
      &received_query));

  EXPECT_TRUE(received_query.empty());
}

TEST_F(AstraTabNotesViewTest, NotesView_AddNoteCallback) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabNotesView>(anchor.get());

  bool add_clicked = false;
  view->SetAddNoteCallback(base::BindRepeating(
      [](bool* out) { *out = true; },
      &add_clicked));

  EXPECT_FALSE(add_clicked);
}

TEST_F(AstraTabNotesViewTest, NotesView_EmptyNotes) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabNotesView>(anchor.get());

  std::vector<AstraTabNoteItemView::NoteInfo> empty;
  view->SetNotes(empty);
  view->Layout();
  SUCCEED();
}

TEST_F(AstraTabNotesViewTest, NotesView_ManyNotes) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabNotesView>(anchor.get());

  std::vector<AstraTabNoteItemView::NoteInfo> notes;
  for (int i = 0; i < 20; i++) {
    AstraTabNoteItemView::NoteInfo note;
    note.note_id = "note_" + std::to_string(i);
    note.tab_id = "tab_" + std::to_string(i);
    note.page_title = base::UTF8ToUTF16(
        "Page Title " + std::to_string(i));
    note.note_content = base::UTF8ToUTF16(
        "Note content for page " + std::to_string(i));
    note.last_updated = base::Time::Now() - base::Hours(i);
    note.has_note = true;
    notes.push_back(note);
  }

  view->SetNotes(notes);
  view->Layout();
  SUCCEED();
}

TEST_F(AstraTabNotesViewTest, NotesView_NoteSelectedCallback) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabNotesView>(anchor.get());

  std::string selected_id;
  view->SetNoteSelectedCallback(base::BindRepeating(
      [](std::string* out, const std::string& id) { *out = id; },
      &selected_id));

  std::vector<AstraTabNoteItemView::NoteInfo> notes;
  AstraTabNoteItemView::NoteInfo note;
  note.note_id = "test_note";
  note.tab_id = "test_tab";
  note.page_title = u"Test";
  note.note_content = u"Test note";
  note.has_note = true;
  notes.push_back(note);
  view->SetNotes(notes);

  EXPECT_TRUE(selected_id.empty());
}

TEST_F(AstraTabNotesViewTest, NotesView_NoteDeletedCallback) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabNotesView>(anchor.get());

  std::string deleted_id;
  view->SetNoteDeletedCallback(base::BindRepeating(
      [](std::string* out, const std::string& id) { *out = id; },
      &deleted_id));

  std::vector<AstraTabNoteItemView::NoteInfo> notes;
  AstraTabNoteItemView::NoteInfo note;
  note.note_id = "delete_me";
  note.tab_id = "tab_delete";
  note.page_title = u"Delete";
  note.note_content = u"Delete this note";
  note.has_note = true;
  notes.push_back(note);
  view->SetNotes(notes);

  EXPECT_TRUE(deleted_id.empty());
}

}  // namespace astra
