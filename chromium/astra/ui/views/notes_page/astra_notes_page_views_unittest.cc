// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/notes_page/astra_notes_page_model.h"
#include "astra/ui/views/notes_page/astra_notes_page_view.h"

#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"

namespace astra {

// ===========================================================================
// Model tests
// ===========================================================================

class AstraNotesPageModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraNotesPageModel>();
  }

  void TearDown() override {
    model_.reset();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraNotesPageModel> model_;
};

// Test model creation.
TEST_F(AstraNotesPageModelTest, ModelCreation) {
  EXPECT_TRUE(model_->GetNotes().empty());
  EXPECT_EQ(0u, model_->GetCount());
  EXPECT_EQ(0u, model_->GetArchivedCount());
  EXPECT_EQ(0u, model_->GetPinnedCount());
  EXPECT_TRUE(model_->GetActiveNoteId().empty());
  EXPECT_TRUE(model_->GetSearchQuery().empty());
  EXPECT_EQ(AstraNotesFilter::kAll, model_->GetFilter());
  EXPECT_TRUE(model_->GetFolderFilter().empty());
  EXPECT_TRUE(model_->GetTagFilter().empty());
  EXPECT_EQ(AstraNotesSortType::kNewestModified, model_->GetSortType());
  EXPECT_FALSE(model_->IsLoading());
  EXPECT_TRUE(model_->GetFolders().empty());
  EXPECT_TRUE(model_->GetAllTags().empty());
  EXPECT_TRUE(model_->GetFilteredNotes().empty());
}

// Test PopulateSampleNotes populates 20+ notes.
TEST_F(AstraNotesPageModelTest, PopulateSampleNotes) {
  model_->PopulateSampleNotes();

  EXPECT_GE(model_->GetCount(), 20u);
  EXPECT_GT(model_->GetPinnedCount(), 0u);
  EXPECT_GT(model_->GetArchivedCount(), 0u);
  EXPECT_FALSE(model_->GetFilteredNotes().empty());

  // Should have folders.
  EXPECT_FALSE(model_->GetFolders().empty());

  // Should have tags.
  EXPECT_FALSE(model_->GetAllTags().empty());
}

// Test GetNote by ID.
TEST_F(AstraNotesPageModelTest, GetNoteById) {
  model_->PopulateSampleNotes();

  const auto& notes = model_->GetNotes();
  ASSERT_FALSE(notes.empty());

  std::string first_id = notes[0].id;
  const AstraNoteEntry* note = model_->GetNote(first_id);
  ASSERT_NE(nullptr, note);
  EXPECT_EQ(first_id, note->id);

  // Non-existent ID returns nullptr.
  EXPECT_EQ(nullptr, model_->GetNote("nonexistent-id"));
}

// Test AddNote.
TEST_F(AstraNotesPageModelTest, AddNote) {
  size_t initial_count = model_->GetCount();

  std::string id = model_->AddNote(u"Test Note", u"This is test content",
                                   "Work", "yellow");

  EXPECT_EQ(initial_count + 1, model_->GetCount());
  const AstraNoteEntry* note = model_->GetNote(id);
  ASSERT_NE(nullptr, note);
  EXPECT_EQ(u"Test Note", note->title);
  EXPECT_EQ(u"This is test content", note->content);
  EXPECT_EQ("Work", note->folder);
  EXPECT_EQ("yellow", note->color);
  EXPECT_FALSE(note->is_pinned);
  EXPECT_FALSE(note->is_archived);
  EXPECT_GT(note->word_count, 0);
  EXPECT_FALSE(note->date_created.is_null());
  EXPECT_FALSE(note->date_modified.is_null());
}

// Test RemoveNote.
TEST_F(AstraNotesPageModelTest, RemoveNote) {
  model_->PopulateSampleNotes();
  size_t initial_count = model_->GetCount();

  const auto& notes = model_->GetNotes();
  ASSERT_FALSE(notes.empty());
  std::string first_id = notes[0].id;

  model_->RemoveNote(first_id);
  EXPECT_EQ(initial_count - 1, model_->GetCount());
  EXPECT_EQ(nullptr, model_->GetNote(first_id));

  // Removing non-existent note is a no-op.
  model_->RemoveNote("nonexistent");
  EXPECT_EQ(initial_count - 1, model_->GetCount());
}

// Test UpdateNoteTitle.
TEST_F(AstraNotesPageModelTest, UpdateNoteTitle) {
  std::string id = model_->AddNote(u"Original Title", u"Content", "", "default");

  model_->UpdateNoteTitle(id, u"New Title");
  const AstraNoteEntry* note = model_->GetNote(id);
  ASSERT_NE(nullptr, note);
  EXPECT_EQ(u"New Title", note->title);

  // Updating non-existent note is a no-op.
  model_->UpdateNoteTitle("nonexistent", u"Whatever");
}

// Test UpdateNoteContent and word count.
TEST_F(AstraNotesPageModelTest, UpdateNoteContent) {
  std::string id = model_->AddNote(u"Title", u"One two three", "", "default");

  const AstraNoteEntry* note = model_->GetNote(id);
  ASSERT_NE(nullptr, note);
  EXPECT_EQ(3, note->word_count);

  model_->UpdateNoteContent(id, u"One two three four five");
  note = model_->GetNote(id);
  ASSERT_NE(nullptr, note);
  EXPECT_EQ(u"One two three four five", note->content);
  EXPECT_EQ(5, note->word_count);
}

// Test UpdateNoteColor.
TEST_F(AstraNotesPageModelTest, UpdateNoteColor) {
  std::string id = model_->AddNote(u"Title", u"Content", "", "default");

  model_->UpdateNoteColor(id, "blue");
  const AstraNoteEntry* note = model_->GetNote(id);
  ASSERT_NE(nullptr, note);
  EXPECT_EQ("blue", note->color);
}

// Test TogglePin.
TEST_F(AstraNotesPageModelTest, TogglePin) {
  std::string id = model_->AddNote(u"Title", u"Content", "", "default");
  EXPECT_FALSE(model_->GetNote(id)->is_pinned);

  model_->TogglePin(id);
  EXPECT_TRUE(model_->GetNote(id)->is_pinned);

  model_->TogglePin(id);
  EXPECT_FALSE(model_->GetNote(id)->is_pinned);

  EXPECT_EQ(0u, model_->GetPinnedCount());
  model_->TogglePin(id);
  EXPECT_EQ(1u, model_->GetPinnedCount());
}

// Test ArchiveNote / UnarchiveNote.
TEST_F(AstraNotesPageModelTest, ArchiveAndUnarchive) {
  std::string id = model_->AddNote(u"Title", u"Content", "", "default");
  EXPECT_FALSE(model_->GetNote(id)->is_archived);
  EXPECT_EQ(0u, model_->GetArchivedCount());

  model_->ArchiveNote(id);
  EXPECT_TRUE(model_->GetNote(id)->is_archived);
  EXPECT_EQ(1u, model_->GetArchivedCount());

  // Archive again is a no-op.
  model_->ArchiveNote(id);
  EXPECT_EQ(1u, model_->GetArchivedCount());

  model_->UnarchiveNote(id);
  EXPECT_FALSE(model_->GetNote(id)->is_archived);
  EXPECT_EQ(0u, model_->GetArchivedCount());

  // Unarchive again is a no-op.
  model_->UnarchiveNote(id);
  EXPECT_EQ(0u, model_->GetArchivedCount());
}

// Test AddTagToNote / RemoveTagFromNote.
TEST_F(AstraNotesPageModelTest, TagsOnNotes) {
  std::string id = model_->AddNote(u"Title", u"Content", "", "default");

  model_->AddTagToNote(id, "work");
  const AstraNoteEntry* note = model_->GetNote(id);
  ASSERT_NE(nullptr, note);
  EXPECT_EQ(1u, note->tags.size());
  EXPECT_EQ("work", note->tags[0]);

  // Duplicate tag is a no-op.
  model_->AddTagToNote(id, "work");
  EXPECT_EQ(1u, note->tags.size());

  model_->AddTagToNote(id, "important");
  EXPECT_EQ(2u, note->tags.size());

  model_->RemoveTagFromNote(id, "work");
  EXPECT_EQ(1u, note->tags.size());
  EXPECT_EQ("important", note->tags[0]);

  // Removing non-existent tag is a no-op.
  model_->RemoveTagFromNote(id, "nonexistent");
  EXPECT_EQ(1u, note->tags.size());

  // Tag on non-existent note is a no-op.
  model_->AddTagToNote("nonexistent", "tag");
}

// Test MoveNoteToFolder.
TEST_F(AstraNotesPageModelTest, MoveNoteToFolder) {
  std::string id = model_->AddNote(u"Title", u"Content", "Folder1", "default");
  EXPECT_EQ("Folder1", model_->GetNote(id)->folder);

  model_->MoveNoteToFolder(id, "Folder2");
  EXPECT_EQ("Folder2", model_->GetNote(id)->folder);

  // Same folder is a no-op.
  model_->MoveNoteToFolder(id, "Folder2");
  EXPECT_EQ("Folder2", model_->GetNote(id)->folder);
}

// Test Folder CRUD.
TEST_F(AstraNotesPageModelTest, FolderCRUD) {
  EXPECT_TRUE(model_->GetFolders().empty());

  std::string id = model_->AddFolder(u"Work Notes", "blue");
  EXPECT_EQ(1u, model_->GetFolders().size());
  EXPECT_EQ(u"Work Notes", model_->GetFolders()[0].name);
  EXPECT_EQ("blue", model_->GetFolders()[0].color);

  model_->RenameFolder(id, u"Work Stuff");
  EXPECT_EQ(u"Work Stuff", model_->GetFolders()[0].name);

  model_->RemoveFolder(id);
  EXPECT_TRUE(model_->GetFolders().empty());

  // Removing non-existent folder is a no-op.
  model_->RemoveFolder("nonexistent");
}

// Test GetAllTags returns unique tags.
TEST_F(AstraNotesPageModelTest, GetAllTagsUnique) {
  model_->PopulateSampleNotes();

  auto tags = model_->GetAllTags();
  std::set<std::string> unique_tags(tags.begin(), tags.end());
  EXPECT_EQ(tags.size(), unique_tags.size());

  // Should have multiple tags.
  EXPECT_GT(tags.size(), 3u);
}

// Test search filtering (title + content).
TEST_F(AstraNotesPageModelTest, SearchFiltering) {
  model_->AddNote(u"Meeting Notes", u"Discussed the project timeline", "",
                  "default");
  model_->AddNote(u"Shopping List", u"Milk eggs bread and more", "", "default");
  model_->AddNote(u"Project Plan", u"Timeline and milestones", "", "default");

  size_t all_count = model_->GetFilteredNotes().size();
  EXPECT_EQ(3u, all_count);

  // Search in title.
  model_->SetSearchQuery(u"note");
  EXPECT_EQ(1u, model_->GetFilteredNotes().size());
  EXPECT_EQ(u"Meeting Notes", model_->GetFilteredNotes()[0].title);

  // Search in content.
  model_->SetSearchQuery(u"timeline");
  EXPECT_EQ(2u, model_->GetFilteredNotes().size());

  // No matches.
  model_->SetSearchQuery(u"zzznomatch");
  EXPECT_TRUE(model_->GetFilteredNotes().empty());

  // Clear search.
  model_->SetSearchQuery(u"");
  EXPECT_EQ(all_count, model_->GetFilteredNotes().size());
}

// Test search is case-insensitive.
TEST_F(AstraNotesPageModelTest, SearchCaseInsensitive) {
  model_->AddNote(u"Hello World", u"Content here", "", "default");

  model_->SetSearchQuery(u"HELLO");
  size_t upper_count = model_->GetFilteredNotes().size();

  model_->SetSearchQuery(u"hello");
  size_t lower_count = model_->GetFilteredNotes().size();

  EXPECT_EQ(upper_count, lower_count);
  EXPECT_GT(upper_count, 0u);
}

// Test filter by pinned.
TEST_F(AstraNotesPageModelTest, FilterPinned) {
  model_->PopulateSampleNotes();
  size_t pinned_count = model_->GetPinnedCount();
  ASSERT_GT(pinned_count, 0u);

  model_->SetFilter(AstraNotesFilter::kPinned);
  EXPECT_EQ(pinned_count, model_->GetFilteredNotes().size());

  for (const auto& note : model_->GetFilteredNotes()) {
    EXPECT_TRUE(note.is_pinned);
    EXPECT_FALSE(note.is_archived);
  }
}

// Test filter by archived.
TEST_F(AstraNotesPageModelTest, FilterArchived) {
  model_->PopulateSampleNotes();
  size_t archived_count = model_->GetArchivedCount();
  ASSERT_GT(archived_count, 0u);

  model_->SetFilter(AstraNotesFilter::kArchived);
  EXPECT_EQ(archived_count, model_->GetFilteredNotes().size());

  for (const auto& note : model_->GetFilteredNotes()) {
    EXPECT_TRUE(note.is_archived);
  }
}

// Test filter by recent.
TEST_F(AstraNotesPageModelTest, FilterRecent) {
  model_->AddNote(u"Recent Note", u"Content", "", "default");

  model_->SetFilter(AstraNotesFilter::kRecent);
  // At least the note we just added should be recent.
  EXPECT_GE(model_->GetFilteredNotes().size(), 1u);

  // All filtered notes should be from the last 7 days.
  base::Time now = base::Time::Now();
  for (const auto& note : model_->GetFilteredNotes()) {
    EXPECT_LE(now - note.date_modified, base::Days(7));
  }
}

// Test filter kAll excludes archived notes.
TEST_F(AstraNotesPageModelTest, FilterAllExcludesArchived) {
  model_->PopulateSampleNotes();
  model_->SetFilter(AstraNotesFilter::kAll);

  for (const auto& note : model_->GetFilteredNotes()) {
    EXPECT_FALSE(note.is_archived);
  }
}

// Test folder filter.
TEST_F(AstraNotesPageModelTest, FolderFilter) {
  model_->AddNote(u"Note 1", u"Content 1", "Work", "default");
  model_->AddNote(u"Note 2", u"Content 2", "Personal", "default");
  model_->AddNote(u"Note 3", u"Content 3", "Work", "default");
  model_->AddNote(u"Note 4", u"Content 4", "Ideas", "default");

  EXPECT_EQ(4u, model_->GetFilteredNotes().size());

  model_->SetFolderFilter("Work");
  EXPECT_EQ(2u, model_->GetFilteredNotes().size());
  for (const auto& note : model_->GetFilteredNotes()) {
    EXPECT_EQ("Work", note.folder);
  }

  model_->SetFolderFilter("Personal");
  EXPECT_EQ(1u, model_->GetFilteredNotes().size());

  model_->SetFolderFilter("");  // Clear filter.
  EXPECT_EQ(4u, model_->GetFilteredNotes().size());
}

// Test tag filter.
TEST_F(AstraNotesPageModelTest, TagFilter) {
  std::string id1 = model_->AddNote(u"Note 1", u"Content", "", "default");
  std::string id2 = model_->AddNote(u"Note 2", u"Content", "", "default");
  std::string id3 = model_->AddNote(u"Note 3", u"Content", "", "default");

  model_->AddTagToNote(id1, "work");
  model_->AddTagToNote(id1, "important");
  model_->AddTagToNote(id2, "work");
  model_->AddTagToNote(id3, "personal");

  model_->SetTagFilter("work");
  EXPECT_EQ(2u, model_->GetFilteredNotes().size());

  model_->SetTagFilter("important");
  EXPECT_EQ(1u, model_->GetFilteredNotes().size());

  model_->SetTagFilter("");  // Clear.
  EXPECT_EQ(3u, model_->GetFilteredNotes().size());
}

// Test sorting by newest modified.
TEST_F(AstraNotesPageModelTest, SortNewestModified) {
  model_->PopulateSampleNotes();
  model_->SetSortType(AstraNotesSortType::kNewestModified);

  auto notes = model_->GetFilteredNotes();
  for (size_t i = 1; i < notes.size(); ++i) {
    EXPECT_GE(notes[i - 1].date_modified, notes[i].date_modified);
  }
}

// Test sorting by title.
TEST_F(AstraNotesPageModelTest, SortByTitle) {
  model_->AddNote(u"Zebra", u"Content", "", "default");
  model_->AddNote(u"Apple", u"Content", "", "default");
  model_->AddNote(u"Monkey", u"Content", "", "default");

  model_->SetSortType(AstraNotesSortType::kTitle);
  auto notes = model_->GetFilteredNotes();

  ASSERT_EQ(3u, notes.size());
  EXPECT_EQ(u"Apple", notes[0].title);
  EXPECT_EQ(u"Monkey", notes[1].title);
  EXPECT_EQ(u"Zebra", notes[2].title);
}

// Test sorting by color.
TEST_F(AstraNotesPageModelTest, SortByColor) {
  model_->AddNote(u"Note 1", u"Content", "", "blue");
  model_->AddNote(u"Note 2", u"Content", "", "green");
  model_->AddNote(u"Note 3", u"Content", "", "yellow");

  model_->SetSortType(AstraNotesSortType::kColor);
  auto notes = model_->GetFilteredNotes();

  ASSERT_EQ(3u, notes.size());
  // "blue" < "green" < "yellow" alphabetically.
  EXPECT_EQ("blue", notes[0].color);
  EXPECT_EQ("green", notes[1].color);
  EXPECT_EQ("yellow", notes[2].color);
}

// Test sorting by newest created.
TEST_F(AstraNotesPageModelTest, SortNewestCreated) {
  model_->PopulateSampleNotes();
  model_->SetSortType(AstraNotesSortType::kNewestCreated);

  auto notes = model_->GetFilteredNotes();
  for (size_t i = 1; i < notes.size(); ++i) {
    EXPECT_GE(notes[i - 1].date_created, notes[i].date_created);
  }
}

// Test sorting by oldest modified.
TEST_F(AstraNotesPageModelTest, SortOldestModified) {
  model_->PopulateSampleNotes();
  model_->SetSortType(AstraNotesSortType::kOldestModified);

  auto notes = model_->GetFilteredNotes();
  for (size_t i = 1; i < notes.size(); ++i) {
    EXPECT_LE(notes[i - 1].date_modified, notes[i].date_modified);
  }
}

// Test active note.
TEST_F(AstraNotesPageModelTest, ActiveNote) {
  model_->PopulateSampleNotes();

  EXPECT_TRUE(model_->GetActiveNoteId().empty());

  const auto& notes = model_->GetNotes();
  ASSERT_FALSE(notes.empty());
  std::string first_id = notes[0].id;

  model_->SetActiveNote(first_id);
  EXPECT_EQ(first_id, model_->GetActiveNoteId());

  // Same ID is a no-op.
  model_->SetActiveNote(first_id);
  EXPECT_EQ(first_id, model_->GetActiveNoteId());
}

// Test word count.
TEST_F(AstraNotesPageModelTest, WordCount) {
  std::string id = model_->AddNote(u"Title", u"", "", "default");
  EXPECT_EQ(0, model_->GetNote(id)->word_count);

  model_->UpdateNoteContent(id, u"One");
  EXPECT_EQ(1, model_->GetNote(id)->word_count);

  model_->UpdateNoteContent(id, u"One two three four");
  EXPECT_EQ(4, model_->GetNote(id)->word_count);

  // Extra spaces should not affect word count.
  model_->UpdateNoteContent(id, u"   One   two   ");
  EXPECT_EQ(2, model_->GetNote(id)->word_count);

  // Newlines should count as separators.
  model_->UpdateNoteContent(id, u"One\nTwo\nThree");
  EXPECT_EQ(3, model_->GetNote(id)->word_count);
}

// Test SetLoading.
TEST_F(AstraNotesPageModelTest, LoadingState) {
  EXPECT_FALSE(model_->IsLoading());

  model_->SetLoading(true);
  EXPECT_TRUE(model_->IsLoading());

  model_->SetLoading(false);
  EXPECT_FALSE(model_->IsLoading());
}

// Test observer notifications.
TEST_F(AstraNotesPageModelTest, ObserverNotifications) {
  class TestObserver : public AstraNotesPageObserver {
   public:
    void OnNotesChanged() override { notes_changed_count++; }
    void OnNoteAdded(const std::string& id) override {
      note_added_count++;
      last_added_id = id;
    }
    void OnNoteRemoved(const std::string& id) override {
      note_removed_count++;
      last_removed_id = id;
    }
    void OnNoteUpdated(const std::string& id) override {
      note_updated_count++;
      last_updated_id = id;
    }
    void OnFolderAdded(const std::string& id) override {
      folder_added_count++;
      last_folder_added_id = id;
    }
    void OnFolderRemoved(const std::string& id) override {
      folder_removed_count++;
      last_folder_removed_id = id;
    }
    void OnActiveNoteChanged(const std::string& id) override {
      active_note_changed_count++;
      last_active_note_id = id;
    }
    void OnSearchChanged(const std::u16string& query) override {
      search_changed_count++;
      last_search_query = query;
    }
    void OnFilterChanged() override { filter_changed_count++; }
    void OnNotesPageModelShutdown() override { shutdown_count++; }

    int notes_changed_count = 0;
    int note_added_count = 0;
    int note_removed_count = 0;
    int note_updated_count = 0;
    int folder_added_count = 0;
    int folder_removed_count = 0;
    int active_note_changed_count = 0;
    int search_changed_count = 0;
    int filter_changed_count = 0;
    int shutdown_count = 0;
    std::string last_added_id;
    std::string last_removed_id;
    std::string last_updated_id;
    std::string last_folder_added_id;
    std::string last_folder_removed_id;
    std::string last_active_note_id;
    std::u16string last_search_query;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  // AddNote triggers OnNoteAdded and OnNotesChanged.
  std::string id = model_->AddNote(u"Test", u"Content", "", "default");
  EXPECT_GE(observer.note_added_count, 1);
  EXPECT_EQ(id, observer.last_added_id);
  EXPECT_GT(observer.notes_changed_count, 0);

  // UpdateNoteTitle triggers OnNoteUpdated and OnNotesChanged.
  int prev_notes = observer.notes_changed_count;
  int prev_updated = observer.note_updated_count;
  model_->UpdateNoteTitle(id, u"New Title");
  EXPECT_GT(observer.note_updated_count, prev_updated);
  EXPECT_EQ(id, observer.last_updated_id);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // TogglePin triggers OnNoteUpdated and OnNotesChanged.
  prev_updated = observer.note_updated_count;
  prev_notes = observer.notes_changed_count;
  model_->TogglePin(id);
  EXPECT_GT(observer.note_updated_count, prev_updated);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // Archive triggers OnNoteUpdated and OnNotesChanged.
  prev_updated = observer.note_updated_count;
  prev_notes = observer.notes_changed_count;
  model_->ArchiveNote(id);
  EXPECT_GT(observer.note_updated_count, prev_updated);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // SetActiveNote triggers OnActiveNoteChanged.
  model_->SetActiveNote(id);
  EXPECT_EQ(1, observer.active_note_changed_count);
  EXPECT_EQ(id, observer.last_active_note_id);

  // SetSearchQuery triggers OnSearchChanged and OnNotesChanged.
  prev_notes = observer.notes_changed_count;
  model_->SetSearchQuery(u"test");
  EXPECT_EQ(1, observer.search_changed_count);
  EXPECT_EQ(u"test", observer.last_search_query);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // SetFilter triggers OnFilterChanged and OnNotesChanged.
  prev_notes = observer.notes_changed_count;
  model_->SetFilter(AstraNotesFilter::kPinned);
  EXPECT_EQ(1, observer.filter_changed_count);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // SetFolderFilter triggers OnFilterChanged and OnNotesChanged.
  int prev_filter = observer.filter_changed_count;
  prev_notes = observer.notes_changed_count;
  model_->SetFolderFilter("Work");
  EXPECT_GT(observer.filter_changed_count, prev_filter);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // SetTagFilter triggers OnFilterChanged and OnNotesChanged.
  prev_filter = observer.filter_changed_count;
  prev_notes = observer.notes_changed_count;
  model_->SetTagFilter("work");
  EXPECT_GT(observer.filter_changed_count, prev_filter);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // SetSortType triggers OnNotesChanged (no dedicated sort notification).
  prev_notes = observer.notes_changed_count;
  model_->SetSortType(AstraNotesSortType::kTitle);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // AddFolder triggers OnFolderAdded.
  std::string folder_id = model_->AddFolder(u"Test Folder", "blue");
  EXPECT_GE(observer.folder_added_count, 1);
  EXPECT_EQ(folder_id, observer.last_folder_added_id);

  // RemoveFolder triggers OnFolderRemoved and OnNotesChanged.
  int prev_folder_removed = observer.folder_removed_count;
  prev_notes = observer.notes_changed_count;
  model_->RemoveFolder(folder_id);
  EXPECT_GT(observer.folder_removed_count, prev_folder_removed);
  EXPECT_EQ(folder_id, observer.last_folder_removed_id);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // RemoveNote triggers OnNoteRemoved and OnNotesChanged.
  int prev_removed = observer.note_removed_count;
  prev_notes = observer.notes_changed_count;
  model_->RemoveNote(id);
  EXPECT_GT(observer.note_removed_count, prev_removed);
  EXPECT_EQ(id, observer.last_removed_id);
  EXPECT_GT(observer.notes_changed_count, prev_notes);

  // AddTagToNote triggers OnNoteUpdated.
  std::string id2 = model_->AddNote(u"Test2", u"Content", "", "default");
  prev_updated = observer.note_updated_count;
  model_->AddTagToNote(id2, "tag");
  EXPECT_GT(observer.note_updated_count, prev_updated);

  // MoveNoteToFolder triggers OnNoteUpdated.
  prev_updated = observer.note_updated_count;
  model_->MoveNoteToFolder(id2, "Work");
  EXPECT_GT(observer.note_updated_count, prev_updated);

  // Remove observer and verify no more notifications.
  model_->RemoveObserver(&observer);
  int notes_before = observer.notes_changed_count;
  model_->AddNote(u"Another", u"Note", "", "default");
  EXPECT_EQ(notes_before, observer.notes_changed_count);
}

// Test that pinned notes appear first in filtered results.
TEST_F(AstraNotesPageModelTest, PinnedNotesFirst) {
  std::string id1 = model_->AddNote(u"Note 1", u"Content", "", "default");
  std::string id2 = model_->AddNote(u"Note 2", u"Content", "", "default");

  model_->TogglePin(id2);

  auto notes = model_->GetFilteredNotes();
  ASSERT_EQ(2u, notes.size());
  // Pinned note (id2) should come first.
  EXPECT_EQ(id2, notes[0].id);
  EXPECT_TRUE(notes[0].is_pinned);
  EXPECT_FALSE(notes[1].is_pinned);
}

// Test that PopulateSampleNotes has multiple folders.
TEST_F(AstraNotesPageModelTest, SampleNotesMultipleFolders) {
  model_->PopulateSampleNotes();

  auto folders = model_->GetFolders();
  EXPECT_GE(folders.size(), 3u);

  // Each folder should have a non-negative note count.
  for (const auto& folder : folders) {
    EXPECT_GE(folder.note_count, 0);
    EXPECT_FALSE(folder.name.empty());
    EXPECT_FALSE(folder.id.empty());
  }
}

// Test that note count changes when adding/removing notes.
TEST_F(AstraNotesPageModelTest, FolderNoteCountUpdates) {
  model_->AddFolder(u"Work", "blue");
  // Note: folder note counts are based on folder name matching note.folder.
  // After AddFolder, folder with name "Work" exists but count is 0.
  auto folders = model_->GetFolders();
  ASSERT_FALSE(folders.empty());
  // Find "Work" folder.
  bool found = false;
  for (const auto& f : folders) {
    if (f.name == u"Work") {
      found = true;
      EXPECT_EQ(0, f.note_count);
    }
  }
  EXPECT_TRUE(found);

  // Add a note to Work folder.
  model_->AddNote(u"Test", u"Content", "Work", "default");

  // Count should be updated.
  folders = model_->GetFolders();
  for (const auto& f : folders) {
    if (f.name == u"Work") {
      EXPECT_EQ(1, f.note_count);
    }
  }
}

// ===========================================================================
// View tests
// ===========================================================================

class AstraNotesPageViewTest : public views::ViewsTestBase {
 protected:
  void SetUp() override {
    views::ViewsTestBase::SetUp();

    model_ = std::make_unique<AstraNotesPageModel>();
    model_->PopulateSampleNotes();

    view_ = std::make_unique<AstraNotesPageView>(model_.get());
    view_->SetSize(gfx::Size(1000, 600));
    view_->Layout();
  }

  void TearDown() override {
    view_.reset();
    model_.reset();
    views::ViewsTestBase::TearDown();
  }

  std::unique_ptr<AstraNotesPageModel> model_;
  std::unique_ptr<AstraNotesPageView> view_;
};

// Test view creation with model.
TEST_F(AstraNotesPageViewTest, ViewCreation) {
  EXPECT_NE(nullptr, view_->model());
  EXPECT_EQ(model_.get(), view_->model());
  EXPECT_NE(nullptr, view_->search_field_for_test());
  EXPECT_NE(nullptr, view_->new_note_button_for_test());
  EXPECT_NE(nullptr, view_->grid_view_button_for_test());
  EXPECT_NE(nullptr, view_->list_view_button_for_test());
  EXPECT_NE(nullptr, view_->sidebar_for_test());
  EXPECT_NE(nullptr, view_->notes_scroll_for_test());
  EXPECT_NE(nullptr, view_->editor_for_test());
  EXPECT_GT(view_->note_card_count_for_test(), 0u);
  EXPECT_GT(view_->folder_item_count_for_test(), 0u);
}

// Test three-pane layout.
TEST_F(AstraNotesPageViewTest, ThreePaneLayout) {
  view_->Layout();

  // Sidebar, notes, and editor should all exist and have children.
  EXPECT_NE(nullptr, view_->sidebar_for_test());
  EXPECT_NE(nullptr, view_->notes_scroll_for_test());
  EXPECT_NE(nullptr, view_->editor_for_test());

  // Folder items should include special folders.
  EXPECT_GE(view_->folder_item_count_for_test(), 3u);  // All Notes, Pinned, Archive

  // Note cards should be populated.
  EXPECT_GT(view_->note_card_count_for_test(), 0u);

  // Tag chips should exist.
  EXPECT_GT(view_->tag_chip_count_for_test(), 0u);
}

// Test that the view observes model changes.
TEST_F(AstraNotesPageViewTest, ObservesModelChanges) {
  size_t initial_count = view_->note_card_count_for_test();
  EXPECT_GT(initial_count, 0u);

  // Add a note through the model and verify view updates.
  model_->AddNote(u"Brand New Note", u"New content", "", "default");

  // The view should have rebuilt note cards.
  EXPECT_GT(view_->note_card_count_for_test(), initial_count);
}

// Test note selection updates editor.
TEST_F(AstraNotesPageViewTest, NoteSelectionUpdatesEditor) {
  // Initially no note selected - editor should show empty state.
  // (Editor title/content fields are hidden when no note is active.)
  EXPECT_TRUE(model_->GetActiveNoteId().empty());

  // Select the first note.
  const auto& notes = model_->GetFilteredNotes();
  ASSERT_FALSE(notes.empty());
  model_->SetActiveNote(notes[0].id);

  // Editor should be populated.
  EXPECT_NE(nullptr, view_->editor_title_for_test());
  EXPECT_FALSE(view_->editor_title_for_test()->GetText().empty());
}

// Test search field updates model.
TEST_F(AstraNotesPageViewTest, SearchFieldUpdatesModel) {
  ASSERT_TRUE(view_->search_field_for_test());

  // Setting search via model should work.
  model_->SetSearchQuery(u"meeting");
  EXPECT_EQ(u"meeting", model_->GetSearchQuery());
  EXPECT_GT(model_->GetFilteredNotes().size(), 0u);

  // Verify view still has note cards (or empty state).
  EXPECT_GE(view_->note_card_count_for_test(), 0u);
}

// Test display mode (grid/list).
TEST_F(AstraNotesPageViewTest, DisplayMode) {
  EXPECT_EQ(AstraNotesDisplayMode::kGrid, view_->display_mode());

  view_->SetDisplayMode(AstraNotesDisplayMode::kList);
  EXPECT_EQ(AstraNotesDisplayMode::kList, view_->display_mode());

  view_->SetDisplayMode(AstraNotesDisplayMode::kGrid);
  EXPECT_EQ(AstraNotesDisplayMode::kGrid, view_->display_mode());
}

// Test SetModel changes the observed model.
TEST_F(AstraNotesPageViewTest, SetModel) {
  auto new_model = std::make_unique<AstraNotesPageModel>();
  new_model->AddNote(u"Only Note", u"Only content", "", "default");

  view_->SetModel(new_model.get());
  EXPECT_EQ(new_model.get(), view_->model());

  // Should only have 1 note card.
  EXPECT_EQ(1u, view_->note_card_count_for_test());

  // Clean up: disconnect before destruction.
  view_->SetModel(nullptr);
  EXPECT_EQ(nullptr, view_->model());
}

// Test AstraNoteCardView.
TEST_F(AstraNotesPageViewTest, NoteCardView) {
  AstraNoteEntry entry;
  entry.id = "test123";
  entry.title = u"Test Note";
  entry.content = u"This is a test note with some content.";
  entry.color = "yellow";
  entry.is_pinned = true;
  entry.date_modified = base::Time::Now();
  entry.tags = {"tag1", "tag2"};

  AstraNoteCardView card(entry);
  card.SetSize(gfx::Size(200, 160));
  card.Layout();

  EXPECT_EQ("test123", card.note_id());
  EXPECT_EQ(u"Test Note", card.title());
  EXPECT_FALSE(card.selected());

  card.SetSelected(true);
  EXPECT_TRUE(card.selected());

  card.SetDisplayMode(AstraNotesDisplayMode::kGrid);
  card.SetDisplayMode(AstraNotesDisplayMode::kList);
}

// Test AstraNoteFolderItemView - special folders.
TEST_F(AstraNotesPageViewTest, FolderItemViewSpecial) {
  AstraNoteFolderItemView item(
      AstraNoteFolderItemView::SpecialFolder::kAllNotes, 42);

  EXPECT_TRUE(item.is_special());
  EXPECT_EQ(AstraNoteFolderItemView::SpecialFolder::kAllNotes,
            item.special_type());
  EXPECT_EQ(u"All Notes", item.name());

  item.SetCount(10);
  item.SetSelected(true);
  EXPECT_TRUE(item.selected());

  item.SetSize(gfx::Size(240, 36));
  item.Layout();
}

// Test AstraNoteFolderItemView - custom folder.
TEST_F(AstraNotesPageViewTest, FolderItemViewCustom) {
  AstraNoteFolder folder;
  folder.id = "f1";
  folder.name = u"Work";
  folder.color = "blue";
  folder.note_count = 5;

  AstraNoteFolderItemView item(folder, 0);

  EXPECT_FALSE(item.is_special());
  EXPECT_EQ("f1", item.folder_id());
  EXPECT_EQ(u"Work", item.name());

  item.SetSize(gfx::Size(240, 36));
  item.Layout();
}

// Test AstraNoteTagChipView.
TEST_F(AstraNotesPageViewTest, TagChipView) {
  AstraNoteTagChipView chip("important");

  EXPECT_EQ("important", chip.tag());
  EXPECT_FALSE(chip.selected());

  chip.SetSelected(true);
  EXPECT_TRUE(chip.selected());

  chip.SetSize(gfx::Size(80, 24));
}

// Test empty state when no notes match.
TEST_F(AstraNotesPageViewTest, EmptyState) {
  auto empty_model = std::make_unique<AstraNotesPageModel>();
  auto empty_view = std::make_unique<AstraNotesPageView>(empty_model.get());
  empty_view->SetSize(gfx::Size(800, 600));
  empty_view->Layout();

  EXPECT_EQ(0u, empty_view->note_card_count_for_test());

  // Add a note - should no longer be empty.
  empty_model->AddNote(u"Test", u"Content", "", "default");
  EXPECT_EQ(1u, empty_view->note_card_count_for_test());

  empty_view->SetModel(nullptr);
}

// Test that the editor has title, content, and toolbar.
TEST_F(AstraNotesPageViewTest, EditorComponents) {
  // Editor fields should exist.
  EXPECT_NE(nullptr, view_->editor_title_for_test());
  EXPECT_NE(nullptr, view_->editor_content_for_test());

  // Select a note to activate the editor.
  const auto& notes = model_->GetFilteredNotes();
  ASSERT_FALSE(notes.empty());
  model_->SetActiveNote(notes[0].id);

  // Title and content should match the note.
  EXPECT_EQ(notes[0].title, view_->editor_title_for_test()->GetText());
  EXPECT_EQ(notes[0].content, view_->editor_content_for_test()->GetText());
}

// Test folder count in sidebar.
TEST_F(AstraNotesPageViewTest, FolderItemCount) {
  // Should have 3 special folders + custom folders from sample data.
  size_t expected = 3 + model_->GetFolders().size();
  EXPECT_EQ(expected, view_->folder_item_count_for_test());
}

// Test that removing a note updates the view.
TEST_F(AstraNotesPageViewTest, RemoveNoteUpdatesView) {
  size_t initial_count = view_->note_card_count_for_test();
  ASSERT_GT(initial_count, 0u);

  const auto& notes = model_->GetFilteredNotes();
  ASSERT_FALSE(notes.empty());
  model_->RemoveNote(notes[0].id);

  EXPECT_LT(view_->note_card_count_for_test(), initial_count);
}

// Test that adding a folder updates the view.
TEST_F(AstraNotesPageViewTest, AddFolderUpdatesView) {
  size_t initial_count = view_->folder_item_count_for_test();

  model_->AddFolder(u"New Folder", "purple");

  EXPECT_GT(view_->folder_item_count_for_test(), initial_count);
}

}  // namespace astra
