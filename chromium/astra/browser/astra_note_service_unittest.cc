// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_note_service.h"

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

#include "astra/browser/astra_note_service_factory.h"

namespace astra {

namespace {

// =========================================================================
// Test observers
// =========================================================================

// Simple test observer for AstraNoteObserver.
class TestNoteObserver : public AstraNoteObserver {
 public:
  void OnNoteCreated(AstraNoteService* service,
                     const std::string& note_id) override {
    note_created_count_++;
    last_created_note_id_ = note_id;
    last_created_service_ = service;
  }

  void OnNoteDeleted(AstraNoteService* service,
                     const std::string& note_id) override {
    note_deleted_count_++;
    last_deleted_note_id_ = note_id;
    last_deleted_service_ = service;
  }

  void OnNoteChanged(AstraNoteService* service,
                     const std::string& note_id) override {
    note_changed_count_++;
    last_changed_note_id_ = note_id;
    last_changed_service_ = service;
  }

  void OnNoteTagsChanged(AstraNoteService* service,
                         const std::string& note_id) override {
    note_tags_changed_count_++;
    last_tags_changed_note_id_ = note_id;
    last_tags_changed_service_ = service;
  }

  void OnNotesImported(AstraNoteService* service, int count) override {
    notes_imported_count_++;
    last_imported_count_ = count;
    last_imported_service_ = service;
  }

  void OnNoteServiceShutdown(AstraNoteService* service) override {
    service_shutdown_count_++;
    last_shutdown_service_ = service;
  }

  // Counters.
  int note_created_count_ = 0;
  int note_deleted_count_ = 0;
  int note_changed_count_ = 0;
  int note_tags_changed_count_ = 0;
  int notes_imported_count_ = 0;
  int service_shutdown_count_ = 0;

  // Last values.
  std::string last_created_note_id_;
  std::string last_deleted_note_id_;
  std::string last_changed_note_id_;
  std::string last_tags_changed_note_id_;
  int last_imported_count_ = 0;

  // Last service pointers (for verification).
  raw_ptr<AstraNoteService> last_created_service_ = nullptr;
  raw_ptr<AstraNoteService> last_deleted_service_ = nullptr;
  raw_ptr<AstraNoteService> last_changed_service_ = nullptr;
  raw_ptr<AstraNoteService> last_tags_changed_service_ = nullptr;
  raw_ptr<AstraNoteService> last_imported_service_ = nullptr;
  raw_ptr<AstraNoteService> last_shutdown_service_ = nullptr;
};

// Extended test observer for AstraNoteServiceObserver.
class TestNoteServiceObserver : public AstraNoteServiceObserver {
 public:
  void OnNoteAdded(const AstraNote& note) override {
    note_added_count_++;
    last_added_note_id_ = note.id;
    last_added_note_title_ = note.title;
  }

  void OnNoteUpdated(const AstraNote& note) override {
    note_updated_count_++;
    last_updated_note_id_ = note.id;
    last_updated_note_title_ = note.title;
  }

  void OnNoteRemoved(const std::string& note_id) override {
    note_removed_count_++;
    last_removed_note_id_ = note_id;
  }

  void OnNoteColorChanged(const std::string& note_id,
                          const std::string& new_color) override {
    note_color_changed_count_++;
    last_color_changed_note_id_ = note_id;
    last_color_changed_ = new_color;
  }

  void OnNotesReordered() override {
    notes_reordered_count_++;
  }

  void OnNotesReloaded() override {
    notes_reloaded_count_++;
  }

  // Counters
  int note_added_count_ = 0;
  int note_updated_count_ = 0;
  int note_removed_count_ = 0;
  int note_color_changed_count_ = 0;
  int notes_reordered_count_ = 0;
  int notes_reloaded_count_ = 0;

  // Last recorded values
  std::string last_added_note_id_;
  std::string last_added_note_title_;
  std::string last_updated_note_id_;
  std::string last_updated_note_title_;
  std::string last_removed_note_id_;
  std::string last_color_changed_note_id_;
  std::string last_color_changed_;
};

}  // namespace

// =========================================================================
// Test fixture
// =========================================================================

class NoteServiceTest : public testing::Test {
 protected:
  NoteServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register note service prefs on the testing profile's pref service.
    AstraNoteServiceFactory::RegisterProfilePrefs(profile_->GetPrefs());
    service_ = std::make_unique<AstraNoteService>(profile_.get());
    DCHECK(service_);
  }

  ~NoteServiceTest() override = default;

  void SetUp() override {
    ASSERT_EQ(service_->GetNoteCount(), 0u);
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
    for (auto& observer : test_service_observers_) {
      service_->RemoveServiceObserver(&observer);
    }
  }

  // ---- Helpers ----

  // Creates a note with title and content, returns the ID.
  std::string CreateTestNote(const std::string& title,
                             const std::string& content) {
    return service_->CreateNote(title, content);
  }

  // Creates a note associated with a URL.
  std::string CreateTestNoteWithUrl(const std::string& title,
                                    const std::string& content,
                                    const GURL& url,
                                    const std::string& page_title = "") {
    std::string id = service_->CreateNote(title, content);
    if (!url.is_empty()) {
      service_->SetNoteTabUrl(id, url, page_title);
    }
    return id;
  }

  // Creates a note associated with a workspace.
  std::string CreateTestNoteWithWorkspace(
      const std::string& title,
      const std::string& content,
      const std::string& workspace_id) {
    std::string id = service_->CreateNote(title, content);
    service_->SetNoteWorkspace(id, workspace_id);
    return id;
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraNoteService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestNoteObserver> test_observers_;
  std::vector<TestNoteServiceObserver> test_service_observers_;
};

// =========================================================================
// AstraNote struct tests
// =========================================================================

TEST(AstraNoteStructTest, IsEmpty_EmptyNoteReturnsTrue) {
  AstraNote note;
  EXPECT_TRUE(note.IsEmpty());
}

TEST(AstraNoteStructTest, IsEmpty_TitleOnlyReturnsFalse) {
  AstraNote note;
  note.title = "Hello";
  EXPECT_FALSE(note.IsEmpty());
}

TEST(AstraNoteStructTest, IsEmpty_ContentOnlyReturnsFalse) {
  AstraNote note;
  note.content = "World";
  EXPECT_FALSE(note.IsEmpty());
}

TEST(AstraNoteStructTest, IsEmpty_BothSetReturnsFalse) {
  AstraNote note;
  note.title = "Hello";
  note.content = "World";
  EXPECT_FALSE(note.IsEmpty());
}

TEST(AstraNoteStructTest, MatchesQuery_TitleMatch) {
  AstraNote note;
  note.title = "Project Alpha";
  EXPECT_TRUE(note.MatchesQuery("Alpha"));
  EXPECT_TRUE(note.MatchesQuery("project"));
  EXPECT_FALSE(note.MatchesQuery("Beta"));
}

TEST(AstraNoteStructTest, MatchesQuery_ContentMatch) {
  AstraNote note;
  note.content = "This is some content with keywords";
  EXPECT_TRUE(note.MatchesQuery("keywords"));
  EXPECT_TRUE(note.MatchesQuery("CONTENT"));
  EXPECT_FALSE(note.MatchesQuery("missing"));
}

TEST(AstraNoteStructTest, MatchesQuery_TagMatch) {
  AstraNote note;
  note.tags = {"work", "important", "idea"};
  EXPECT_TRUE(note.MatchesQuery("work"));
  EXPECT_TRUE(note.MatchesQuery("IMPORTANT"));
  EXPECT_TRUE(note.MatchesQuery("ide"));  // Partial match.
  EXPECT_FALSE(note.MatchesQuery("personal"));
}

TEST(AstraNoteStructTest, MatchesQuery_EmptyQueryMatchesAll) {
  AstraNote note;
  note.title = "Anything";
  EXPECT_TRUE(note.MatchesQuery(""));
}

TEST(AstraNoteStructTest, MatchesQuery_CaseInsensitive) {
  AstraNote note;
  note.title = "Mixed Case Title";
  note.content = "mIxEd CoNtEnT";
  note.tags = {"WorkTag"};

  EXPECT_TRUE(note.MatchesQuery("mixed"));
  EXPECT_TRUE(note.MatchesQuery("MIXED"));
  EXPECT_TRUE(note.MatchesQuery("Mixed"));
  EXPECT_TRUE(note.MatchesQuery("worktag"));
}

TEST(AstraNoteStructTest, DefaultValues) {
  AstraNote note;
  EXPECT_TRUE(note.id.empty());
  EXPECT_TRUE(note.title.empty());
  EXPECT_TRUE(note.content.empty());
  EXPECT_TRUE(note.workspace_id.empty());
  EXPECT_TRUE(note.tab_url.is_empty());
  EXPECT_TRUE(note.tab_title.empty());
  EXPECT_TRUE(note.tags.empty());
  EXPECT_TRUE(note.color.empty());
  EXPECT_FALSE(note.is_pinned);
  EXPECT_FALSE(note.is_favorite);
  EXPECT_TRUE(note.created_time.is_null());
  EXPECT_TRUE(note.modified_time.is_null());
  EXPECT_TRUE(note.last_accessed.is_null());
  EXPECT_EQ(note.word_count, 0);
  EXPECT_EQ(note.size_bytes, 0);
}

// =========================================================================
// Empty service
// =========================================================================

TEST_F(NoteServiceTest, EmptyServiceHasNoNotes) {
  EXPECT_EQ(service_->GetNoteCount(), 0u);
  EXPECT_TRUE(service_->GetAllNotes().empty());
  EXPECT_FALSE(service_->DoesNoteExist("nonexistent"));
}

TEST_F(NoteServiceTest, GetNote_NonexistentReturnsNull) {
  EXPECT_EQ(service_->GetNote("nonexistent"), nullptr);
  EXPECT_EQ(service_->GetNote(""), nullptr);
}

TEST_F(NoteServiceTest, DoesNoteExist_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->DoesNoteExist("nonexistent"));
  EXPECT_FALSE(service_->DoesNoteExist(""));
}

TEST_F(NoteServiceTest, GetAllTags_EmptyServiceReturnsEmpty) {
  EXPECT_TRUE(service_->GetAllTags().empty());
}

TEST_F(NoteServiceTest, GetTagCount_EmptyServiceReturnsZero) {
  EXPECT_EQ(service_->GetTagCount("work"), 0u);
  EXPECT_EQ(service_->GetTagCount(""), 0u);
}

// =========================================================================
// CreateNote
// =========================================================================

TEST_F(NoteServiceTest, CreateNote_CreatesNote) {
  std::string id = CreateTestNote("Test Title", "Test content");

  EXPECT_FALSE(id.empty());
  EXPECT_EQ(service_->GetNoteCount(), 1u);

  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->id, id);
  EXPECT_EQ(note->title, "Test Title");
  EXPECT_EQ(note->content, "Test content");
  EXPECT_FALSE(note->created_time.is_null());
  EXPECT_FALSE(note->modified_time.is_null());
  EXPECT_EQ(note->created_time, note->modified_time);
}

TEST_F(NoteServiceTest, CreateNote_UniqueIds) {
  std::string id1 = CreateTestNote("Note 1", "");
  std::string id2 = CreateTestNote("Note 2", "");
  std::string id3 = CreateTestNote("Note 3", "");

  EXPECT_NE(id1, id2);
  EXPECT_NE(id2, id3);
  EXPECT_NE(id1, id3);
}

TEST_F(NoteServiceTest, CreateNote_DefaultColor) {
  std::string id = CreateTestNote("Note", "");
  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->color, AstraNoteService::kDefaultNoteColor);
}

TEST_F(NoteServiceTest, CreateNote_WordCountComputed) {
  std::string id = CreateTestNote("Title", "one two three");
  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->word_count, 3);
}

TEST_F(NoteServiceTest, CreateNote_SizeBytesComputed) {
  std::string id = CreateTestNote("Title", "Content");
  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->size_bytes,
            static_cast<int>(note->title.size() + note->content.size()));
}

TEST_F(NoteServiceTest, CreateNote_EmptyTitleAllowed) {
  std::string id = CreateTestNote("", "Content");
  EXPECT_FALSE(id.empty());
  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_TRUE(note->title.empty());
}

TEST_F(NoteServiceTest, CreateNote_EmptyContentAllowed) {
  std::string id = CreateTestNote("Title", "");
  EXPECT_FALSE(id.empty());
  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_TRUE(note->content.empty());
  EXPECT_EQ(note->word_count, 0);
}

TEST_F(NoteServiceTest, CreateNote_MultipleNotes) {
  CreateTestNote("Note 1", "Content 1");
  CreateTestNote("Note 2", "Content 2");
  CreateTestNote("Note 3", "Content 3");

  EXPECT_EQ(service_->GetNoteCount(), 3u);
  EXPECT_EQ(service_->GetAllNotes().size(), 3u);
}

TEST_F(NoteServiceTest, CreateNote_LastAccessedEqualsCreated) {
  std::string id = CreateTestNote("Title", "");
  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->last_accessed, note->created_time);
}

TEST_F(NoteServiceTest, CreateNote_UsesDefaultWorkspace) {
  // Default workspace is empty (global).
  std::string id = CreateTestNote("Note", "");
  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_TRUE(note->workspace_id.empty());
}

// =========================================================================
// UpdateNoteTitle
// =========================================================================

TEST_F(NoteServiceTest, UpdateNoteTitle_UpdatesTitle) {
  std::string id = CreateTestNote("Old Title", "Content");

  bool result = service_->UpdateNoteTitle(id, "New Title");
  EXPECT_TRUE(result);

  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->title, "New Title");
  EXPECT_GT(note->modified_time, note->created_time);
}

TEST_F(NoteServiceTest, UpdateNoteTitle_NonexistentReturnsFalse) {
  bool result = service_->UpdateNoteTitle("nonexistent", "New");
  EXPECT_FALSE(result);
}

TEST_F(NoteServiceTest, UpdateNoteTitle_SameTitleNoChange) {
  std::string id = CreateTestNote("Title", "");

  const AstraNote* before = service_->GetNote(id);
  base::Time before_modified = before->modified_time;

  bool result = service_->UpdateNoteTitle(id, "Title");
  EXPECT_TRUE(result);  // Returns true (note exists).

  const AstraNote* after = service_->GetNote(id);
  EXPECT_EQ(after->modified_time, before_modified);  // No timestamp change.
}

TEST_F(NoteServiceTest, UpdateNoteTitle_UpdatesSizeBytes) {
  std::string id = CreateTestNote("Short", "");
  const AstraNote* before = service_->GetNote(id);
  int before_size = before->size_bytes;

  service_->UpdateNoteTitle(id, "Much longer title");
  const AstraNote* after = service_->GetNote(id);
  EXPECT_GT(after->size_bytes, before_size);
}

// =========================================================================
// UpdateNoteContent
// =========================================================================

TEST_F(NoteServiceTest, UpdateNoteContent_UpdatesContent) {
  std::string id = CreateTestNote("Title", "Old content");

  bool result = service_->UpdateNoteContent(id, "New content");
  EXPECT_TRUE(result);

  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->content, "New content");
}

TEST_F(NoteServiceTest, UpdateNoteContent_NonexistentReturnsFalse) {
  bool result = service_->UpdateNoteContent("nonexistent", "New");
  EXPECT_FALSE(result);
}

TEST_F(NoteServiceTest, UpdateNoteContent_UpdatesWordCount) {
  std::string id = CreateTestNote("Title", "one two");
  ASSERT_EQ(service_->GetNote(id)->word_count, 2);

  service_->UpdateNoteContent(id, "one two three four");
  EXPECT_EQ(service_->GetNote(id)->word_count, 4);
}

TEST_F(NoteServiceTest, UpdateNoteContent_UpdatesSizeBytes) {
  std::string id = CreateTestNote("Title", "short");
  int before_size = service_->GetNote(id)->size_bytes;

  service_->UpdateNoteContent(id, "much much longer content");
  EXPECT_GT(service_->GetNote(id)->size_bytes, before_size);
}

TEST_F(NoteServiceTest, UpdateNoteContent_SameContentNoChange) {
  std::string id = CreateTestNote("Title", "Same content");

  const AstraNote* before = service_->GetNote(id);
  base::Time before_modified = before->modified_time;

  bool result = service_->UpdateNoteContent(id, "Same content");
  EXPECT_TRUE(result);

  const AstraNote* after = service_->GetNote(id);
  EXPECT_EQ(after->modified_time, before_modified);
}

// =========================================================================
// DeleteNote
// =========================================================================

TEST_F(NoteServiceTest, DeleteNote_RemovesNote) {
  std::string id = CreateTestNote("To Delete", "");
  ASSERT_EQ(service_->GetNoteCount(), 1u);

  bool result = service_->DeleteNote(id);
  EXPECT_TRUE(result);
  EXPECT_EQ(service_->GetNoteCount(), 0u);
  EXPECT_EQ(service_->GetNote(id), nullptr);
  EXPECT_FALSE(service_->DoesNoteExist(id));
}

TEST_F(NoteServiceTest, DeleteNote_NonexistentReturnsFalse) {
  bool result = service_->DeleteNote("nonexistent");
  EXPECT_FALSE(result);
}

TEST_F(NoteServiceTest, DeleteNote_EmptyIdReturnsFalse) {
  EXPECT_FALSE(service_->DeleteNote(""));
}

// =========================================================================
// DeleteAllNotes
// =========================================================================

TEST_F(NoteServiceTest, DeleteAllNotes_ClearsAll) {
  CreateTestNote("Note 1", "");
  CreateTestNote("Note 2", "");
  CreateTestNote("Note 3", "");
  ASSERT_EQ(service_->GetNoteCount(), 3u);

  service_->DeleteAllNotes();
  EXPECT_EQ(service_->GetNoteCount(), 0u);
  EXPECT_TRUE(service_->GetAllNotes().empty());
}

TEST_F(NoteServiceTest, DeleteAllNotes_EmptyServiceIsNoOp) {
  ASSERT_EQ(service_->GetNoteCount(), 0u);
  service_->DeleteAllNotes();
  EXPECT_EQ(service_->GetNoteCount(), 0u);
}

// =========================================================================
// Note association: workspace
// =========================================================================

TEST_F(NoteServiceTest, SetNoteWorkspace_SetsWorkspace) {
  std::string id = CreateTestNote("Note", "");
  EXPECT_TRUE(service_->GetNoteWorkspace(id).empty());

  bool result = service_->SetNoteWorkspace(id, "workspace-1");
  EXPECT_TRUE(result);
  EXPECT_EQ(service_->GetNoteWorkspace(id), "workspace-1");
}

TEST_F(NoteServiceTest, SetNoteWorkspace_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->SetNoteWorkspace("nonexistent", "ws"));
}

TEST_F(NoteServiceTest, SetNoteWorkspace_ClearWorkspace) {
  std::string id = CreateTestNote("Note", "");
  service_->SetNoteWorkspace(id, "ws1");
  ASSERT_EQ(service_->GetNoteWorkspace(id), "ws1");

  service_->SetNoteWorkspace(id, "");
  EXPECT_TRUE(service_->GetNoteWorkspace(id).empty());
}

TEST_F(NoteServiceTest, SetNoteWorkspace_SameValueNoChange) {
  std::string id = CreateTestNote("Note", "");
  service_->SetNoteWorkspace(id, "ws1");
  base::Time modified_before = service_->GetNote(id)->modified_time;

  service_->SetNoteWorkspace(id, "ws1");
  EXPECT_EQ(service_->GetNote(id)->modified_time, modified_before);
}

TEST_F(NoteServiceTest, GetNoteWorkspace_NonexistentReturnsEmpty) {
  EXPECT_TRUE(service_->GetNoteWorkspace("nonexistent").empty());
}

// =========================================================================
// Note association: tab URL
// =========================================================================

TEST_F(NoteServiceTest, SetNoteTabUrl_SetsUrlAndTitle) {
  std::string id = CreateTestNote("Note", "");
  GURL url("https://example.com/page");

  bool result = service_->SetNoteTabUrl(id, url, "Example Page");
  EXPECT_TRUE(result);

  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->tab_url, url);
  EXPECT_EQ(note->tab_title, "Example Page");
}

TEST_F(NoteServiceTest, SetNoteTabUrl_NonexistentReturnsFalse) {
  GURL url("https://example.com");
  EXPECT_FALSE(service_->SetNoteTabUrl("nonexistent", url, "Title"));
}

TEST_F(NoteServiceTest, GetNoteTabUrl_NonexistentReturnsEmpty) {
  EXPECT_TRUE(service_->GetNoteTabUrl("nonexistent").is_empty());
}

TEST_F(NoteServiceTest, ClearNoteTab_ClearsUrlAndTitle) {
  std::string id = CreateTestNote("Note", "");
  GURL url("https://example.com");
  service_->SetNoteTabUrl(id, url, "Page Title");
  ASSERT_FALSE(service_->GetNote(id)->tab_url.is_empty());
  ASSERT_FALSE(service_->GetNote(id)->tab_title.empty());

  bool result = service_->ClearNoteTab(id);
  EXPECT_TRUE(result);

  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_TRUE(note->tab_url.is_empty());
  EXPECT_TRUE(note->tab_title.empty());
}

TEST_F(NoteServiceTest, ClearNoteTab_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->ClearNoteTab("nonexistent"));
}

TEST_F(NoteServiceTest, ClearNoteTab_AlreadyClearNoChange) {
  std::string id = CreateTestNote("Note", "");
  base::Time modified_before = service_->GetNote(id)->modified_time;

  bool result = service_->ClearNoteTab(id);
  EXPECT_TRUE(result);
  EXPECT_EQ(service_->GetNote(id)->modified_time, modified_before);
}

// =========================================================================
// Tags
// =========================================================================

TEST_F(NoteServiceTest, AddNoteTag_AddsTag) {
  std::string id = CreateTestNote("Note", "");

  bool result = service_->AddNoteTag(id, "work");
  EXPECT_TRUE(result);

  auto tags = service_->GetNoteTags(id);
  ASSERT_EQ(tags.size(), 1u);
  EXPECT_EQ(tags[0], "work");
}

TEST_F(NoteServiceTest, AddNoteTag_DuplicateIgnored) {
  std::string id = CreateTestNote("Note", "");

  EXPECT_TRUE(service_->AddNoteTag(id, "todo"));
  EXPECT_FALSE(service_->AddNoteTag(id, "todo"));

  EXPECT_EQ(service_->GetNoteTags(id).size(), 1u);
}

TEST_F(NoteServiceTest, AddNoteTag_EmptyTagIgnored) {
  std::string id = CreateTestNote("Note", "");
  EXPECT_FALSE(service_->AddNoteTag(id, ""));
  EXPECT_TRUE(service_->GetNoteTags(id).empty());
}

TEST_F(NoteServiceTest, AddNoteTag_NonexistentNoteReturnsFalse) {
  EXPECT_FALSE(service_->AddNoteTag("nonexistent", "tag"));
}

TEST_F(NoteServiceTest, AddNoteTag_MultipleTagsSorted) {
  std::string id = CreateTestNote("Note", "");

  service_->AddNoteTag(id, "work");
  service_->AddNoteTag(id, "personal");
  service_->AddNoteTag(id, "idea");

  auto tags = service_->GetNoteTags(id);
  ASSERT_EQ(tags.size(), 3u);
  EXPECT_EQ(tags[0], "idea");
  EXPECT_EQ(tags[1], "personal");
  EXPECT_EQ(tags[2], "work");
}

TEST_F(NoteServiceTest, RemoveNoteTag_RemovesTag) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "work");
  service_->AddNoteTag(id, "personal");

  bool result = service_->RemoveNoteTag(id, "work");
  EXPECT_TRUE(result);

  auto tags = service_->GetNoteTags(id);
  EXPECT_EQ(tags.size(), 1u);
  EXPECT_EQ(tags[0], "personal");
}

TEST_F(NoteServiceTest, RemoveNoteTag_NonexistentTagReturnsFalse) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "work");

  EXPECT_FALSE(service_->RemoveNoteTag(id, "nonexistent"));
}

TEST_F(NoteServiceTest, RemoveNoteTag_NonexistentNoteReturnsFalse) {
  EXPECT_FALSE(service_->RemoveNoteTag("nonexistent", "tag"));
}

TEST_F(NoteServiceTest, SetNoteTags_ReplacesAllTags) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "old1");
  service_->AddNoteTag(id, "old2");

  bool result = service_->SetNoteTags(id, {"new1", "new2", "new3"});
  EXPECT_TRUE(result);

  auto tags = service_->GetNoteTags(id);
  ASSERT_EQ(tags.size(), 3u);
  EXPECT_EQ(tags[0], "new1");
  EXPECT_EQ(tags[1], "new2");
  EXPECT_EQ(tags[2], "new3");
}

TEST_F(NoteServiceTest, SetNoteTags_Deduplicates) {
  std::string id = CreateTestNote("Note", "");

  service_->SetNoteTags(id, {"a", "b", "a", "c", "b"});
  auto tags = service_->GetNoteTags(id);
  EXPECT_EQ(tags.size(), 3u);
}

TEST_F(NoteServiceTest, SetNoteTags_RemovesEmptyTags) {
  std::string id = CreateTestNote("Note", "");

  service_->SetNoteTags(id, {"tag", "", "other"});
  auto tags = service_->GetNoteTags(id);
  EXPECT_EQ(tags.size(), 2u);
}

TEST_F(NoteServiceTest, SetNoteTags_NonexistentNoteReturnsFalse) {
  EXPECT_FALSE(service_->SetNoteTags("nonexistent", {"tag"}));
}

TEST_F(NoteServiceTest, GetNoteTags_NonexistentReturnsEmpty) {
  EXPECT_TRUE(service_->GetNoteTags("nonexistent").empty());
}

TEST_F(NoteServiceTest, GetAllTags_ReturnsUniqueSortedTags) {
  std::string id1 = CreateTestNote("Note 1", "");
  std::string id2 = CreateTestNote("Note 2", "");
  std::string id3 = CreateTestNote("Note 3", "");

  service_->AddNoteTag(id1, "work");
  service_->AddNoteTag(id1, "idea");
  service_->AddNoteTag(id2, "work");
  service_->AddNoteTag(id2, "personal");
  service_->AddNoteTag(id3, "idea");

  auto all_tags = service_->GetAllTags();
  ASSERT_EQ(all_tags.size(), 3u);
  EXPECT_EQ(all_tags[0], "idea");
  EXPECT_EQ(all_tags[1], "personal");
  EXPECT_EQ(all_tags[2], "work");
}

TEST_F(NoteServiceTest, GetAllTags_EmptyWhenNoTags) {
  CreateTestNote("Note 1", "");
  CreateTestNote("Note 2", "");
  EXPECT_TRUE(service_->GetAllTags().empty());
}

TEST_F(NoteServiceTest, GetTagCount_CountsNotesWithTag) {
  std::string id1 = CreateTestNote("Note 1", "");
  std::string id2 = CreateTestNote("Note 2", "");
  std::string id3 = CreateTestNote("Note 3", "");

  service_->AddNoteTag(id1, "work");
  service_->AddNoteTag(id2, "work");
  service_->AddNoteTag(id3, "personal");

  EXPECT_EQ(service_->GetTagCount("work"), 2u);
  EXPECT_EQ(service_->GetTagCount("personal"), 1u);
  EXPECT_EQ(service_->GetTagCount("nonexistent"), 0u);
}

TEST_F(NoteServiceTest, GetTagCount_EmptyTagReturnsZero) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "work");
  EXPECT_EQ(service_->GetTagCount(""), 0u);
}

// =========================================================================
// Note organization: color
// =========================================================================

TEST_F(NoteServiceTest, SetNoteColor_SetsColor) {
  std::string id = CreateTestNote("Note", "");

  bool result = service_->SetNoteColor(id, "#FF0000");
  EXPECT_TRUE(result);
  EXPECT_EQ(service_->GetNoteColor(id), "#FF0000");
}

TEST_F(NoteServiceTest, SetNoteColor_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->SetNoteColor("nonexistent", "#000000"));
}

TEST_F(NoteServiceTest, SetNoteColor_SameColorNoChange) {
  std::string id = CreateTestNote("Note", "");
  std::string original_color = service_->GetNoteColor(id);
  base::Time modified_before = service_->GetNote(id)->modified_time;

  bool result = service_->SetNoteColor(id, original_color);
  EXPECT_TRUE(result);
  EXPECT_EQ(service_->GetNote(id)->modified_time, modified_before);
}

TEST_F(NoteServiceTest, GetNoteColor_NonexistentReturnsEmpty) {
  EXPECT_TRUE(service_->GetNoteColor("nonexistent").empty());
}

// =========================================================================
// Note organization: pinned
// =========================================================================

TEST_F(NoteServiceTest, SetNotePinned_PinsNote) {
  std::string id = CreateTestNote("Note", "");
  EXPECT_FALSE(service_->IsNotePinned(id));

  bool result = service_->SetNotePinned(id, true);
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsNotePinned(id));
}

TEST_F(NoteServiceTest, SetNotePinned_UnpinsNote) {
  std::string id = CreateTestNote("Note", "");
  service_->SetNotePinned(id, true);
  ASSERT_TRUE(service_->IsNotePinned(id));

  bool result = service_->SetNotePinned(id, false);
  EXPECT_TRUE(result);
  EXPECT_FALSE(service_->IsNotePinned(id));
}

TEST_F(NoteServiceTest, SetNotePinned_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->SetNotePinned("nonexistent", true));
  EXPECT_FALSE(service_->SetNotePinned("nonexistent", false));
}

TEST_F(NoteServiceTest, SetNotePinned_SameStateNoChange) {
  std::string id = CreateTestNote("Note", "");
  base::Time modified_before = service_->GetNote(id)->modified_time;

  service_->SetNotePinned(id, false);
  EXPECT_EQ(service_->GetNote(id)->modified_time, modified_before);
}

TEST_F(NoteServiceTest, IsNotePinned_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->IsNotePinned("nonexistent"));
}

// =========================================================================
// Note organization: favorite
// =========================================================================

TEST_F(NoteServiceTest, SetNoteFavorite_MarksFavorite) {
  std::string id = CreateTestNote("Note", "");
  EXPECT_FALSE(service_->IsNoteFavorite(id));

  bool result = service_->SetNoteFavorite(id, true);
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsNoteFavorite(id));
}

TEST_F(NoteServiceTest, SetNoteFavorite_UnmarksFavorite) {
  std::string id = CreateTestNote("Note", "");
  service_->SetNoteFavorite(id, true);
  ASSERT_TRUE(service_->IsNoteFavorite(id));

  bool result = service_->SetNoteFavorite(id, false);
  EXPECT_TRUE(result);
  EXPECT_FALSE(service_->IsNoteFavorite(id));
}

TEST_F(NoteServiceTest, SetNoteFavorite_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->SetNoteFavorite("nonexistent", true));
}

TEST_F(NoteServiceTest, SetNoteFavorite_SameStateNoChange) {
  std::string id = CreateTestNote("Note", "");
  base::Time modified_before = service_->GetNote(id)->modified_time;

  service_->SetNoteFavorite(id, false);
  EXPECT_EQ(service_->GetNote(id)->modified_time, modified_before);
}

TEST_F(NoteServiceTest, IsNoteFavorite_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->IsNoteFavorite("nonexistent"));
}

// =========================================================================
// Queries: GetAllNotes
// =========================================================================

TEST_F(NoteServiceTest, GetAllNotes_ReturnsAllNotes) {
  CreateTestNote("Note 1", "");
  CreateTestNote("Note 2", "");
  CreateTestNote("Note 3", "");

  auto all = service_->GetAllNotes();
  EXPECT_EQ(all.size(), 3u);
}

TEST_F(NoteServiceTest, GetAllNotes_SortedByModifiedTime) {
  std::string id1 = CreateTestNote("Oldest", "");
  std::string id2 = CreateTestNote("Middle", "");
  std::string id3 = CreateTestNote("Newest", "");

  auto all = service_->GetAllNotes();
  ASSERT_EQ(all.size(), 3u);

  // Most recent first: newest, middle, oldest.
  EXPECT_EQ(all[0].id, id3);
  EXPECT_EQ(all[1].id, id2);
  EXPECT_EQ(all[2].id, id1);
}

TEST_F(NoteServiceTest, GetAllNotes_PinnedFirst) {
  std::string id1 = CreateTestNote("Regular 1", "");
  std::string id2 = CreateTestNote("Pinned", "");
  std::string id3 = CreateTestNote("Regular 2", "");

  service_->SetNotePinned(id2, true);

  auto all = service_->GetAllNotes();
  ASSERT_EQ(all.size(), 3u);
  EXPECT_EQ(all[0].id, id2);  // Pinned comes first.
}

// =========================================================================
// Queries: GetNotesByWorkspace
// =========================================================================

TEST_F(NoteServiceTest, GetNotesByWorkspace_FiltersCorrectly) {
  CreateTestNoteWithWorkspace("WS1 Note 1", "", "ws1");
  CreateTestNoteWithWorkspace("WS1 Note 2", "", "ws1");
  CreateTestNoteWithWorkspace("WS2 Note", "", "ws2");

  auto ws1 = service_->GetNotesByWorkspace("ws1");
  EXPECT_EQ(ws1.size(), 2u);

  auto ws2 = service_->GetNotesByWorkspace("ws2");
  EXPECT_EQ(ws2.size(), 1u);
}

TEST_F(NoteServiceTest, GetNotesByWorkspace_NoMatchReturnsEmpty) {
  CreateTestNote("Global Note", "");
  EXPECT_TRUE(service_->GetNotesByWorkspace("nonexistent").empty());
}

TEST_F(NoteServiceTest, GetNotesByWorkspace_EmptyWorkspaceReturnsGlobal) {
  CreateTestNote("Global 1", "");
  CreateTestNote("Global 2", "");
  CreateTestNoteWithWorkspace("Work Note", "", "work");

  auto global = service_->GetNotesByWorkspace("");
  EXPECT_EQ(global.size(), 2u);
}

// =========================================================================
// Queries: GetNotesByTag
// =========================================================================

TEST_F(NoteServiceTest, GetNotesByTag_FiltersByTag) {
  std::string id1 = CreateTestNote("Work Note", "");
  std::string id2 = CreateTestNote("Personal Note", "");
  std::string id3 = CreateTestNote("Idea Note", "");

  service_->AddNoteTag(id1, "work");
  service_->AddNoteTag(id1, "important");
  service_->AddNoteTag(id2, "personal");
  service_->AddNoteTag(id3, "idea");
  service_->AddNoteTag(id3, "important");

  auto work = service_->GetNotesByTag("work");
  EXPECT_EQ(work.size(), 1u);

  auto important = service_->GetNotesByTag("important");
  EXPECT_EQ(important.size(), 2u);
}

TEST_F(NoteServiceTest, GetNotesByTag_EmptyTagReturnsEmpty) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "work");
  EXPECT_TRUE(service_->GetNotesByTag("").empty());
}

TEST_F(NoteServiceTest, GetNotesByTag_NoMatchReturnsEmpty) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "work");
  EXPECT_TRUE(service_->GetNotesByTag("nonexistent").empty());
}

// =========================================================================
// Queries: GetPinnedNotes
// =========================================================================

TEST_F(NoteServiceTest, GetPinnedNotes_ReturnsPinnedOnly) {
  std::string id1 = CreateTestNote("Pinned 1", "");
  std::string id2 = CreateTestNote("Not Pinned", "");
  std::string id3 = CreateTestNote("Pinned 2", "");

  service_->SetNotePinned(id1, true);
  service_->SetNotePinned(id3, true);

  auto pinned = service_->GetPinnedNotes();
  EXPECT_EQ(pinned.size(), 2u);
}

TEST_F(NoteServiceTest, GetPinnedNotes_EmptyWhenNonePinned) {
  CreateTestNote("Note 1", "");
  CreateTestNote("Note 2", "");
  EXPECT_TRUE(service_->GetPinnedNotes().empty());
}

// =========================================================================
// Queries: GetFavoriteNotes
// =========================================================================

TEST_F(NoteServiceTest, GetFavoriteNotes_ReturnsFavoritesOnly) {
  std::string id1 = CreateTestNote("Fav 1", "");
  std::string id2 = CreateTestNote("Not Fav", "");
  std::string id3 = CreateTestNote("Fav 2", "");

  service_->SetNoteFavorite(id1, true);
  service_->SetNoteFavorite(id3, true);

  auto favorites = service_->GetFavoriteNotes();
  EXPECT_EQ(favorites.size(), 2u);
}

TEST_F(NoteServiceTest, GetFavoriteNotes_EmptyWhenNoneFavorite) {
  CreateTestNote("Note 1", "");
  CreateTestNote("Note 2", "");
  EXPECT_TRUE(service_->GetFavoriteNotes().empty());
}

// =========================================================================
// Queries: GetNotesByUrl
// =========================================================================

TEST_F(NoteServiceTest, GetNotesByUrl_FiltersByUrl) {
  GURL url1("https://example.com/page1");
  GURL url2("https://example.com/page2");

  CreateTestNoteWithUrl("Note 1", "", url1);
  CreateTestNoteWithUrl("Note 2", "", url1);
  CreateTestNoteWithUrl("Note 3", "", url2);

  auto notes1 = service_->GetNotesByUrl(url1);
  EXPECT_EQ(notes1.size(), 2u);

  auto notes2 = service_->GetNotesByUrl(url2);
  EXPECT_EQ(notes2.size(), 1u);
}

TEST_F(NoteServiceTest, GetNotesByUrl_NoMatchReturnsEmpty) {
  CreateTestNote("Unrelated", "");
  EXPECT_TRUE(service_->GetNotesByUrl(GURL("https://example.com")).empty());
}

TEST_F(NoteServiceTest, GetNotesByUrl_InvalidUrlReturnsEmpty) {
  CreateTestNote("Note", "");
  EXPECT_TRUE(service_->GetNotesByUrl(GURL("not a url")).empty());
}

// =========================================================================
// Queries: SearchNotes
// =========================================================================

TEST_F(NoteServiceTest, SearchNotes_FindsByTitle) {
  CreateTestNote("Project Alpha", "Details about alpha");
  CreateTestNote("Project Beta", "Details about beta");
  CreateTestNote("Random", "Something else");

  auto results = service_->SearchNotes("Project");
  EXPECT_EQ(results.size(), 2u);
}

TEST_F(NoteServiceTest, SearchNotes_FindsByContent) {
  CreateTestNote("Note A", "Contains keyword here");
  CreateTestNote("Note B", "No match here");

  auto results = service_->SearchNotes("keyword");
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].title, "Note A");
}

TEST_F(NoteServiceTest, SearchNotes_FindsByTag) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "special-tag");

  auto results = service_->SearchNotes("special-tag");
  EXPECT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].id, id);
}

TEST_F(NoteServiceTest, SearchNotes_CaseInsensitive) {
  CreateTestNote("Mixed Case Title", "mIxEd CoNtEnT");

  auto results_lower = service_->SearchNotes("mixed");
  auto results_upper = service_->SearchNotes("MIXED");
  auto results_mixed = service_->SearchNotes("Mixed");

  EXPECT_FALSE(results_lower.empty());
  EXPECT_FALSE(results_upper.empty());
  EXPECT_FALSE(results_mixed.empty());
  EXPECT_EQ(results_lower.size(), results_upper.size());
  EXPECT_EQ(results_upper.size(), results_mixed.size());
}

TEST_F(NoteServiceTest, SearchNotes_EmptyQueryReturnsAll) {
  CreateTestNote("Note 1", "");
  CreateTestNote("Note 2", "");

  auto results = service_->SearchNotes("");
  EXPECT_EQ(results.size(), 2u);
}

TEST_F(NoteServiceTest, SearchNotes_NoMatchReturnsEmpty) {
  CreateTestNote("Note", "content");
  EXPECT_TRUE(service_->SearchNotes("xyzzy-nothing-matches").empty());
}

TEST_F(NoteServiceTest, SearchNotes_RespectsMaxResults) {
  for (int i = 0; i < 20; i++) {
    CreateTestNote("Note " + base::NumberToString(i), "keyword");
  }

  service_->SetMaxSearchResults(5);
  auto results = service_->SearchNotes("keyword");
  EXPECT_EQ(results.size(), 5u);
}

TEST_F(NoteServiceTest, SearchNotes_MaxResultsZeroNoLimit) {
  for (int i = 0; i < 10; i++) {
    CreateTestNote("Note " + base::NumberToString(i), "keyword");
  }

  service_->SetMaxSearchResults(0);
  auto results = service_->SearchNotes("keyword");
  EXPECT_EQ(results.size(), 10u);
}

// =========================================================================
// Queries: GetRecentlyModifiedNotes
// =========================================================================

TEST_F(NoteServiceTest, GetRecentlyModifiedNotes_ReturnsMostRecent) {
  std::string id1 = CreateTestNote("Oldest", "");
  std::string id2 = CreateTestNote("Middle", "");
  std::string id3 = CreateTestNote("Newest", "");

  auto recent = service_->GetRecentlyModifiedNotes(2);
  ASSERT_EQ(recent.size(), 2u);
  EXPECT_EQ(recent[0].id, id3);  // Newest first.
  EXPECT_EQ(recent[1].id, id2);
}

TEST_F(NoteServiceTest, GetRecentlyModifiedNotes_MaxCountZero) {
  CreateTestNote("Note", "");
  EXPECT_TRUE(service_->GetRecentlyModifiedNotes(0).empty());
}

TEST_F(NoteServiceTest, GetRecentlyModifiedNotes_NegativeCount) {
  CreateTestNote("Note", "");
  EXPECT_TRUE(service_->GetRecentlyModifiedNotes(-1).empty());
}

TEST_F(NoteServiceTest, GetRecentlyModifiedNotes_MoreThanAvailable) {
  CreateTestNote("Note 1", "");
  CreateTestNote("Note 2", "");

  auto recent = service_->GetRecentlyModifiedNotes(10);
  EXPECT_EQ(recent.size(), 2u);
}

// =========================================================================
// Queries: GetNotesByColor
// =========================================================================

TEST_F(NoteServiceTest, GetNotesByColor_FiltersByColor) {
  std::string id1 = service_->CreateNote("Red", "");
  service_->SetNoteColor(id1, "#FF0000");
  std::string id2 = service_->CreateNote("Blue", "");
  service_->SetNoteColor(id2, "#0000FF");
  std::string id3 = service_->CreateNote("Also Red", "");
  service_->SetNoteColor(id3, "#FF0000");

  auto red = service_->GetNotesByColor("#FF0000");
  EXPECT_EQ(red.size(), 2u);

  auto blue = service_->GetNotesByColor("#0000FF");
  EXPECT_EQ(blue.size(), 1u);
}

TEST_F(NoteServiceTest, GetNotesByColor_EmptyColorReturnsEmpty) {
  CreateTestNote("Note", "");
  EXPECT_TRUE(service_->GetNotesByColor("").empty());
}

TEST_F(NoteServiceTest, GetNotesByColor_NoMatchReturnsEmpty) {
  CreateTestNote("Note", "");
  EXPECT_TRUE(service_->GetNotesByColor("#00FF00").empty());
}

// =========================================================================
// Bulk operations: DeleteNotesByWorkspace
// =========================================================================

TEST_F(NoteServiceTest, DeleteNotesByWorkspace_DeletesMatchingNotes) {
  CreateTestNoteWithWorkspace("WS1 Note 1", "", "ws1");
  CreateTestNoteWithWorkspace("WS1 Note 2", "", "ws1");
  CreateTestNoteWithWorkspace("WS2 Note", "", "ws2");
  CreateTestNote("Global Note", "");

  size_t deleted = service_->DeleteNotesByWorkspace("ws1");
  EXPECT_EQ(deleted, 2u);
  EXPECT_EQ(service_->GetNoteCount(), 2u);  // ws2 + global.
  EXPECT_TRUE(service_->GetNotesByWorkspace("ws1").empty());
}

TEST_F(NoteServiceTest, DeleteNotesByWorkspace_EmptyWorkspaceNoDelete) {
  CreateTestNote("Global 1", "");
  CreateTestNote("Global 2", "");

  size_t deleted = service_->DeleteNotesByWorkspace("");
  EXPECT_EQ(deleted, 0u);
  EXPECT_EQ(service_->GetNoteCount(), 2u);
}

TEST_F(NoteServiceTest, DeleteNotesByWorkspace_NonexistentReturnsZero) {
  CreateTestNote("Note", "");
  EXPECT_EQ(service_->DeleteNotesByWorkspace("nonexistent"), 0u);
}

// =========================================================================
// Bulk operations: MergeNotes
// =========================================================================

TEST_F(NoteServiceTest, MergeNotes_MergesTwoNotes) {
  std::string id1 = CreateTestNote("Note 1", "Content 1");
  std::string id2 = CreateTestNote("Note 2", "Content 2");

  bool result = service_->MergeNotes(id1, id2, "Merged Title");
  EXPECT_TRUE(result);

  // Note 1 should still exist with merged content.
  const AstraNote* merged = service_->GetNote(id1);
  ASSERT_NE(merged, nullptr);
  EXPECT_EQ(merged->title, "Merged Title");
  EXPECT_EQ(merged->content, "Content 1\n\nContent 2");

  // Note 2 should be deleted.
  EXPECT_EQ(service_->GetNote(id2), nullptr);
  EXPECT_EQ(service_->GetNoteCount(), 1u);
}

TEST_F(NoteServiceTest, MergeNotes_MergesTags) {
  std::string id1 = CreateTestNote("Note 1", "");
  std::string id2 = CreateTestNote("Note 2", "");

  service_->AddNoteTag(id1, "a");
  service_->AddNoteTag(id1, "b");
  service_->AddNoteTag(id2, "b");
  service_->AddNoteTag(id2, "c");

  service_->MergeNotes(id1, id2, "Merged");

  auto tags = service_->GetNoteTags(id1);
  ASSERT_EQ(tags.size(), 3u);
  EXPECT_EQ(tags[0], "a");
  EXPECT_EQ(tags[1], "b");
  EXPECT_EQ(tags[2], "c");
}

TEST_F(NoteServiceTest, MergeNotes_PinnedIfEitherPinned) {
  std::string id1 = CreateTestNote("Note 1", "");
  std::string id2 = CreateTestNote("Note 2", "");

  service_->SetNotePinned(id2, true);

  service_->MergeNotes(id1, id2, "Merged");
  EXPECT_TRUE(service_->IsNotePinned(id1));
}

TEST_F(NoteServiceTest, MergeNotes_FavoriteIfEitherFavorite) {
  std::string id1 = CreateTestNote("Note 1", "");
  std::string id2 = CreateTestNote("Note 2", "");

  service_->SetNoteFavorite(id1, true);

  service_->MergeNotes(id1, id2, "Merged");
  EXPECT_TRUE(service_->IsNoteFavorite(id1));
}

TEST_F(NoteServiceTest, MergeNotes_SameNoteReturnsFalse) {
  std::string id = CreateTestNote("Note", "");
  EXPECT_FALSE(service_->MergeNotes(id, id, "Title"));
}

TEST_F(NoteServiceTest, MergeNotes_NonexistentFirstReturnsFalse) {
  std::string id2 = CreateTestNote("Note 2", "");
  EXPECT_FALSE(service_->MergeNotes("nonexistent", id2, "Title"));
}

TEST_F(NoteServiceTest, MergeNotes_NonexistentSecondReturnsFalse) {
  std::string id1 = CreateTestNote("Note 1", "");
  EXPECT_FALSE(service_->MergeNotes(id1, "nonexistent", "Title"));
}

TEST_F(NoteServiceTest, MergeNotes_PreservesWorkspaceOfFirst) {
  std::string id1 = CreateTestNote("Note 1", "");
  std::string id2 = CreateTestNote("Note 2", "");
  service_->SetNoteWorkspace(id1, "ws1");

  service_->MergeNotes(id1, id2, "Merged");
  EXPECT_EQ(service_->GetNoteWorkspace(id1), "ws1");
}

TEST_F(NoteServiceTest, MergeNotes_UpdatesModifiedTime) {
  std::string id1 = CreateTestNote("Note 1", "");
  std::string id2 = CreateTestNote("Note 2", "");

  base::Time before = service_->GetNote(id1)->modified_time;
  base::PlatformThread::Sleep(base::Milliseconds(10));

  service_->MergeNotes(id1, id2, "Merged");
  EXPECT_GT(service_->GetNote(id1)->modified_time, before);
}

// =========================================================================
// Bulk operations: DuplicateNote
// =========================================================================

TEST_F(NoteServiceTest, DuplicateNote_CreatesCopy) {
  std::string id = CreateTestNote("My Note", "Some content");
  service_->AddNoteTag(id, "work");
  service_->SetNoteColor(id, "#FF0000");
  service_->SetNoteWorkspace(id, "ws1");

  std::string new_id = service_->DuplicateNote(id);

  EXPECT_FALSE(new_id.empty());
  EXPECT_NE(new_id, id);
  EXPECT_EQ(service_->GetNoteCount(), 2u);

  const AstraNote* original = service_->GetNote(id);
  const AstraNote* copy = service_->GetNote(new_id);

  ASSERT_NE(copy, nullptr);
  EXPECT_EQ(copy->title, "My Note (Copy)");
  EXPECT_EQ(copy->content, original->content);
  EXPECT_EQ(copy->color, original->color);
  EXPECT_EQ(copy->workspace_id, original->workspace_id);
  EXPECT_EQ(copy->tags, original->tags);
  EXPECT_NE(copy->created_time, original->created_time);
  EXPECT_FALSE(copy->is_pinned);
  EXPECT_FALSE(copy->is_favorite);
}

TEST_F(NoteServiceTest, DuplicateNote_NonexistentReturnsEmpty) {
  EXPECT_TRUE(service_->DuplicateNote("nonexistent").empty());
  EXPECT_EQ(service_->GetNoteCount(), 0u);
}

TEST_F(NoteServiceTest, DuplicateNote_DoesNotCopyPinnedState) {
  std::string id = CreateTestNote("Pinned", "");
  service_->SetNotePinned(id, true);
  ASSERT_TRUE(service_->IsNotePinned(id));

  std::string new_id = service_->DuplicateNote(id);
  ASSERT_FALSE(new_id.empty());
  EXPECT_FALSE(service_->IsNotePinned(new_id));
}

TEST_F(NoteServiceTest, DuplicateNote_DoesNotCopyFavoriteState) {
  std::string id = CreateTestNote("Fav", "");
  service_->SetNoteFavorite(id, true);
  ASSERT_TRUE(service_->IsNoteFavorite(id));

  std::string new_id = service_->DuplicateNote(id);
  ASSERT_FALSE(new_id.empty());
  EXPECT_FALSE(service_->IsNoteFavorite(new_id));
}

TEST_F(NoteServiceTest, DuplicateNote_CopiesTabUrl) {
  std::string id = CreateTestNote("Note", "");
  GURL url("https://example.com");
  service_->SetNoteTabUrl(id, url, "Example");

  std::string new_id = service_->DuplicateNote(id);
  ASSERT_FALSE(new_id.empty());
  EXPECT_EQ(service_->GetNoteTabUrl(new_id), url);
  EXPECT_EQ(service_->GetNote(new_id)->tab_title, "Example");
}

TEST_F(NoteServiceTest, DuplicateNote_HasNewTimestamps) {
  std::string id = CreateTestNote("Original", "");
  base::Time original_created = service_->GetNote(id)->created_time;

  base::PlatformThread::Sleep(base::Milliseconds(10));

  std::string new_id = service_->DuplicateNote(id);
  ASSERT_FALSE(new_id.empty());

  const AstraNote* copy = service_->GetNote(new_id);
  EXPECT_GT(copy->created_time, original_created);
  EXPECT_EQ(copy->created_time, copy->modified_time);
}

// =========================================================================
// Import / Export
// =========================================================================

TEST_F(NoteServiceTest, ExportNotesToJson_ReturnsValidJson) {
  CreateTestNote("Note 1", "Content 1");
  CreateTestNote("Note 2", "Content 2");

  std::string json = service_->ExportNotesToJson();
  EXPECT_FALSE(json.empty());

  auto parsed = base::JSONReader::Read(json);
  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->is_list());
  EXPECT_EQ(parsed->GetList().size(), 2u);
}

TEST_F(NoteServiceTest, ExportNotesToJson_EmptyNotesReturnsEmptyArray) {
  std::string json = service_->ExportNotesToJson();

  auto parsed = base::JSONReader::Read(json);
  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->is_list());
  EXPECT_TRUE(parsed->GetList().empty());
}

TEST_F(NoteServiceTest, ImportNotesFromJson_ValidJsonImport) {
  std::string json = R"([
    {"id": "note-1", "title": "Imported 1", "content": "Content 1"},
    {"id": "note-2", "title": "Imported 2", "content": "Content 2"}
  ])";

  size_t imported = service_->ImportNotesFromJson(json, /*merge=*/false);
  EXPECT_EQ(imported, 2u);
  EXPECT_EQ(service_->GetNoteCount(), 2u);

  const AstraNote* note1 = service_->GetNote("note-1");
  ASSERT_NE(note1, nullptr);
  EXPECT_EQ(note1->title, "Imported 1");
  EXPECT_EQ(note1->content, "Content 1");
}

TEST_F(NoteServiceTest, ImportNotesFromJson_MergeMode) {
  CreateTestNote("Original", "Old");
  ASSERT_EQ(service_->GetNoteCount(), 1u);

  std::string json = R"([
    {"id": "imported", "title": "Imported", "content": "New"}
  ])";

  size_t imported = service_->ImportNotesFromJson(json, /*merge=*/true);
  EXPECT_EQ(imported, 1u);
  EXPECT_EQ(service_->GetNoteCount(), 2u);
  EXPECT_NE(service_->GetNote("imported"), nullptr);
  EXPECT_NE(service_->GetNote("Original"), nullptr);
}

TEST_F(NoteServiceTest, ImportNotesFromJson_ReplaceMode) {
  CreateTestNote("Old", "Old content");
  ASSERT_EQ(service_->GetNoteCount(), 1u);

  std::string json = R"([
    {"id": "new", "title": "New", "content": "New content"}
  ])";

  size_t imported = service_->ImportNotesFromJson(json, /*merge=*/false);
  EXPECT_EQ(imported, 1u);
  EXPECT_EQ(service_->GetNoteCount(), 1u);
  EXPECT_EQ(service_->GetNote("Old"), nullptr);
  EXPECT_NE(service_->GetNote("new"), nullptr);
}

TEST_F(NoteServiceTest, ImportNotesFromJson_InvalidJson) {
  size_t imported = service_->ImportNotesFromJson("not valid json",
                                                  /*merge=*/false);
  EXPECT_EQ(imported, 0u);
  EXPECT_EQ(service_->GetNoteCount(), 0u);
}

TEST_F(NoteServiceTest, ImportNotesFromJson_EmptyString) {
  size_t imported = service_->ImportNotesFromJson("", /*merge=*/false);
  EXPECT_EQ(imported, 0u);
  EXPECT_EQ(service_->GetNoteCount(), 0u);
}

TEST_F(NoteServiceTest, ImportNotesFromJson_JsonObjectNotList) {
  std::string json = R"({"not": "a list"})";
  size_t imported = service_->ImportNotesFromJson(json, /*merge=*/false);
  EXPECT_EQ(imported, 0u);
  EXPECT_EQ(service_->GetNoteCount(), 0u);
}

TEST_F(NoteServiceTest, ImportNotesFromJson_SkipsNonDictEntries) {
  std::string json = R"([
    {"id": "good", "title": "Good Note", "content": "Good"},
    "not a dict",
    42,
    {"id": "also-good", "title": "Also Good", "content": "Also good"}
  ])";

  size_t imported = service_->ImportNotesFromJson(json, /*merge=*/false);
  EXPECT_EQ(imported, 2u);
  EXPECT_EQ(service_->GetNoteCount(), 2u);
}

TEST_F(NoteServiceTest, ImportExport_RoundTrip) {
  std::string id1 = CreateTestNote("RoundTrip 1", "Content A");
  std::string id2 = CreateTestNote("RoundTrip 2", "Content B");
  service_->AddNoteTag(id1, "tag1");
  service_->SetNotePinned(id2, true);
  service_->SetNoteColor(id1, "#112233");
  service_->SetNoteFavorite(id2, true);

  std::string exported = service_->ExportNotesToJson();
  ASSERT_FALSE(exported.empty());

  // Create a new service and import.
  auto new_service = std::make_unique<AstraNoteService>(profile_.get());
  new_service->DeleteAllNotes();
  size_t imported =
      new_service->ImportNotesFromJson(exported, /*merge=*/false);

  EXPECT_EQ(imported, 2u);
  EXPECT_EQ(new_service->GetNoteCount(), 2u);

  const AstraNote* note1 = new_service->GetNote(id1);
  ASSERT_NE(note1, nullptr);
  EXPECT_EQ(note1->title, "RoundTrip 1");
  EXPECT_EQ(note1->content, "Content A");
  EXPECT_EQ(note1->color, "#112233");
  ASSERT_EQ(note1->tags.size(), 1u);
  EXPECT_EQ(note1->tags[0], "tag1");

  const AstraNote* note2 = new_service->GetNote(id2);
  ASSERT_NE(note2, nullptr);
  EXPECT_EQ(note2->title, "RoundTrip 2");
  EXPECT_TRUE(note2->is_pinned);
  EXPECT_TRUE(note2->is_favorite);
}

TEST_F(NoteServiceTest, ImportNotesFromJson_DuplicateIdGetsNewId) {
  CreateTestNote("Original", "Original content");

  auto all_notes = service_->GetAllNotes();
  ASSERT_FALSE(all_notes.empty());
  std::string existing_id = all_notes[0].id;

  std::string json = R"([{"id": ")" + existing_id +
                     R"(", "title": "Imported", "content": "Imported"}])";

  size_t imported = service_->ImportNotesFromJson(json, /*merge=*/true);
  EXPECT_EQ(imported, 1u);
  EXPECT_EQ(service_->GetNoteCount(), 2u);

  // Original should still have its original content.
  const AstraNote* original = service_->GetNote(existing_id);
  ASSERT_NE(original, nullptr);
  EXPECT_EQ(original->title, "Original");
}

// =========================================================================
// ExportNoteToDict / NoteFromDict
// =========================================================================

TEST(AstraNoteDictTest, ExportNoteToDict_ContainsAllFields) {
  AstraNote note;
  note.id = "test-id";
  note.title = "Test Title";
  note.content = "Test content";
  note.workspace_id = "ws1";
  note.tab_url = GURL("https://example.com");
  note.tab_title = "Example Page";
  note.tags = {"tag1", "tag2"};
  note.color = "#FF0000";
  note.is_pinned = true;
  note.is_favorite = true;
  note.word_count = 2;
  note.size_bytes = 20;

  base::Value::Dict dict = AstraNoteService::ExportNoteToDict(note);

  EXPECT_EQ(*dict.FindString("id"), "test-id");
  EXPECT_EQ(*dict.FindString("title"), "Test Title");
  EXPECT_EQ(*dict.FindString("content"), "Test content");
  EXPECT_EQ(*dict.FindString("workspace_id"), "ws1");
  EXPECT_EQ(*dict.FindString("tab_url"), "https://example.com/");
  EXPECT_EQ(*dict.FindString("tab_title"), "Example Page");
  EXPECT_EQ(*dict.FindString("color"), "#FF0000");
  EXPECT_TRUE(dict.FindBool("is_pinned").value());
  EXPECT_TRUE(dict.FindBool("is_favorite").value());
  EXPECT_EQ(*dict.FindInt("word_count"), 2);
  EXPECT_EQ(*dict.FindInt("size_bytes"), 20);

  const base::Value::List* tags = dict.FindList("tags");
  ASSERT_NE(tags, nullptr);
  EXPECT_EQ(tags->size(), 2u);
}

TEST(AstraNoteDictTest, NoteFromDict_ParsesValidDict) {
  base::Value::Dict dict;
  dict.Set("id", "note-1");
  dict.Set("title", "My Note");
  dict.Set("content", "My content");
  dict.Set("workspace_id", "ws1");
  dict.Set("tab_url", "https://example.com");
  dict.Set("tab_title", "Page Title");
  dict.Set("color", "#00FF00");
  dict.Set("is_pinned", true);
  dict.Set("is_favorite", true);

  base::Value::List tags;
  tags.Append("tag1");
  tags.Append("tag2");
  dict.Set("tags", std::move(tags));

  absl::optional<AstraNote> note = AstraNoteService::NoteFromDict(dict);
  ASSERT_TRUE(note.has_value());
  EXPECT_EQ(note->id, "note-1");
  EXPECT_EQ(note->title, "My Note");
  EXPECT_EQ(note->content, "My content");
  EXPECT_EQ(note->workspace_id, "ws1");
  EXPECT_EQ(note->tab_url, GURL("https://example.com"));
  EXPECT_EQ(note->tab_title, "Page Title");
  EXPECT_EQ(note->color, "#00FF00");
  EXPECT_TRUE(note->is_pinned);
  EXPECT_TRUE(note->is_favorite);
  EXPECT_EQ(note->tags.size(), 2u);
}

TEST(AstraNoteDictTest, NoteFromDict_MissingIdReturnsNoteWithEmptyId) {
  base::Value::Dict dict;
  dict.Set("title", "No ID Note");
  dict.Set("content", "Content");

  absl::optional<AstraNote> note = AstraNoteService::NoteFromDict(dict);
  ASSERT_TRUE(note.has_value());
  EXPECT_TRUE(note->id.empty());
  EXPECT_EQ(note->title, "No ID Note");
}

// =========================================================================
// Settings
// =========================================================================

TEST_F(NoteServiceTest, Settings_DefaultNoteColor_DefaultValue) {
  EXPECT_EQ(service_->default_note_color(),
            std::string(AstraNoteService::kDefaultNoteColor));
}

TEST_F(NoteServiceTest, Settings_SetDefaultNoteColor) {
  service_->SetDefaultNoteColor("#00FF00");
  EXPECT_EQ(service_->default_note_color(), "#00FF00");
}

TEST_F(NoteServiceTest, Settings_AutoSaveInterval_DefaultValue) {
  EXPECT_EQ(service_->auto_save_interval_seconds(),
            AstraNoteService::kDefaultAutoSaveIntervalSeconds);
}

TEST_F(NoteServiceTest, Settings_SetAutoSaveInterval) {
  service_->SetAutoSaveIntervalSeconds(30);
  EXPECT_EQ(service_->auto_save_interval_seconds(), 30);
}

TEST_F(NoteServiceTest, Settings_AutoSaveInterval_NegativeClampsToZero) {
  service_->SetAutoSaveIntervalSeconds(-5);
  EXPECT_EQ(service_->auto_save_interval_seconds(), 0);
}

TEST_F(NoteServiceTest, Settings_ShowWordCount_DefaultValue) {
  EXPECT_EQ(service_->show_word_count(),
            AstraNoteService::kDefaultShowWordCount);
}

TEST_F(NoteServiceTest, Settings_SetShowWordCount) {
  service_->SetShowWordCount(false);
  EXPECT_FALSE(service_->show_word_count());

  service_->SetShowWordCount(true);
  EXPECT_TRUE(service_->show_word_count());
}

TEST_F(NoteServiceTest, Settings_SortOrder_DefaultValue) {
  EXPECT_EQ(service_->sort_order(), NoteSortOrder::kDateDescending);
}

TEST_F(NoteServiceTest, Settings_SetSortOrder) {
  service_->SetSortOrder(NoteSortOrder::kTitleAscending);
  EXPECT_EQ(service_->sort_order(), NoteSortOrder::kTitleAscending);
}

TEST_F(NoteServiceTest, Settings_DefaultWorkspace_DefaultValue) {
  EXPECT_TRUE(service_->default_workspace().empty());
}

TEST_F(NoteServiceTest, Settings_SetDefaultWorkspace) {
  service_->SetDefaultWorkspace("my-workspace");
  EXPECT_EQ(service_->default_workspace(), "my-workspace");
}

TEST_F(NoteServiceTest, Settings_MaxSearchResults_DefaultValue) {
  EXPECT_EQ(service_->max_search_results(),
            AstraNoteService::kDefaultMaxSearchResults);
}

TEST_F(NoteServiceTest, Settings_SetMaxSearchResults) {
  service_->SetMaxSearchResults(50);
  EXPECT_EQ(service_->max_search_results(), 50);
}

TEST_F(NoteServiceTest, Settings_MaxSearchResults_NegativeClampsToZero) {
  service_->SetMaxSearchResults(-10);
  EXPECT_EQ(service_->max_search_results(), 0);
}

TEST_F(NoteServiceTest, Settings_TrashEnabled_DefaultValue) {
  EXPECT_EQ(service_->trash_enabled(),
            AstraNoteService::kDefaultTrashEnabled);
}

TEST_F(NoteServiceTest, Settings_SetTrashEnabled) {
  service_->SetTrashEnabled(true);
  EXPECT_TRUE(service_->trash_enabled());

  service_->SetTrashEnabled(false);
  EXPECT_FALSE(service_->trash_enabled());
}

TEST_F(NoteServiceTest, Settings_AutoTagFromPage_DefaultValue) {
  EXPECT_EQ(service_->auto_tag_from_page(),
            AstraNoteService::kDefaultAutoTagFromPage);
}

TEST_F(NoteServiceTest, Settings_SetAutoTagFromPage) {
  service_->SetAutoTagFromPage(true);
  EXPECT_TRUE(service_->auto_tag_from_page());

  service_->SetAutoTagFromPage(false);
  EXPECT_FALSE(service_->auto_tag_from_page());
}

TEST_F(NoteServiceTest, Settings_NoteFontSize_DefaultValue) {
  EXPECT_EQ(service_->note_font_size(),
            AstraNoteService::kDefaultNoteFontSize);
}

TEST_F(NoteServiceTest, Settings_SetNoteFontSize) {
  service_->SetNoteFontSize(18);
  EXPECT_EQ(service_->note_font_size(), 18);
}

TEST_F(NoteServiceTest, Settings_NoteFontSize_MinClamp) {
  service_->SetNoteFontSize(5);
  EXPECT_EQ(service_->note_font_size(), 8);
}

TEST_F(NoteServiceTest, Settings_NoteFontSize_MaxClamp) {
  service_->SetNoteFontSize(100);
  EXPECT_EQ(service_->note_font_size(), 72);
}

TEST_F(NoteServiceTest, Settings_NoteLineHeight_DefaultValue) {
  EXPECT_DOUBLE_EQ(service_->note_line_height(),
                   AstraNoteService::kDefaultNoteLineHeight);
}

TEST_F(NoteServiceTest, Settings_SetNoteLineHeight) {
  service_->SetNoteLineHeight(2.0);
  EXPECT_DOUBLE_EQ(service_->note_line_height(), 2.0);
}

TEST_F(NoteServiceTest, Settings_NoteLineHeight_MinClamp) {
  service_->SetNoteLineHeight(0.5);
  EXPECT_DOUBLE_EQ(service_->note_line_height(), 0.8);
}

TEST_F(NoteServiceTest, Settings_NoteLineHeight_MaxClamp) {
  service_->SetNoteLineHeight(4.0);
  EXPECT_DOUBLE_EQ(service_->note_line_height(), 3.0);
}

TEST_F(NoteServiceTest, Settings_PersistAcrossServiceRecreation) {
  service_->SetDefaultNoteColor("#123456");
  service_->SetAutoSaveIntervalSeconds(15);
  service_->SetShowWordCount(false);
  service_->SetSortOrder(NoteSortOrder::kTitleAscending);
  service_->SetDefaultWorkspace("persisted-ws");
  service_->SetMaxSearchResults(100);
  service_->SetTrashEnabled(true);
  service_->SetAutoTagFromPage(true);
  service_->SetNoteFontSize(16);
  service_->SetNoteLineHeight(1.8);

  // Recreate service.
  service_.reset();
  auto new_service = std::make_unique<AstraNoteService>(profile_.get());

  EXPECT_EQ(new_service->default_note_color(), "#123456");
  EXPECT_EQ(new_service->auto_save_interval_seconds(), 15);
  EXPECT_FALSE(new_service->show_word_count());
  EXPECT_EQ(new_service->sort_order(), NoteSortOrder::kTitleAscending);
  EXPECT_EQ(new_service->default_workspace(), "persisted-ws");
  EXPECT_EQ(new_service->max_search_results(), 100);
  EXPECT_TRUE(new_service->trash_enabled());
  EXPECT_TRUE(new_service->auto_tag_from_page());
  EXPECT_EQ(new_service->note_font_size(), 16);
  EXPECT_DOUBLE_EQ(new_service->note_line_height(), 1.8);
}

// =========================================================================
// AstraNoteObserver notifications
// =========================================================================

TEST_F(NoteServiceTest, Observer_OnNoteCreatedFires) {
  TestNoteObserver observer;
  service_->AddObserver(&observer);

  std::string id = CreateTestNote("New Note", "Content");

  EXPECT_EQ(observer.note_created_count_, 1);
  EXPECT_EQ(observer.last_created_note_id_, id);
  EXPECT_EQ(observer.last_created_service_, service_.get());

  service_->RemoveObserver(&observer);
}

TEST_F(NoteServiceTest, Observer_OnNoteDeletedFires) {
  std::string id = CreateTestNote("To Delete", "");

  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->DeleteNote(id);

  EXPECT_EQ(observer.note_deleted_count_, 1);
  EXPECT_EQ(observer.last_deleted_note_id_, id);
  EXPECT_EQ(observer.last_deleted_service_, service_.get());

  service_->RemoveObserver(&observer);
}

TEST_F(NoteServiceTest, Observer_OnNoteChangedFiresOnTitleUpdate) {
  std::string id = CreateTestNote("Original", "");

  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->UpdateNoteTitle(id, "New Title");

  EXPECT_EQ(observer.note_changed_count_, 1);
  EXPECT_EQ(observer.last_changed_note_id_, id);

  service_->RemoveObserver(&observer);
}

TEST_F(NoteServiceTest, Observer_OnNoteChangedFiresOnContentUpdate) {
  std::string id = CreateTestNote("Note", "Old");

  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->UpdateNoteContent(id, "New");

  EXPECT_EQ(observer.note_changed_count_, 1);
  EXPECT_EQ(observer.last_changed_note_id_, id);

  service_->RemoveObserver(&observer);
}

TEST_F(NoteServiceTest, Observer_OnNoteChangedFiresOnColorChange) {
  std::string id = CreateTestNote("Note", "");

  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->SetNoteColor(id, "#FF0000");

  EXPECT_EQ(observer.note_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(NoteServiceTest, Observer_OnNoteTagsChangedFires) {
  std::string id = CreateTestNote("Note", "");

  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->AddNoteTag(id, "new-tag");

  EXPECT_EQ(observer.note_tags_changed_count_, 1);
  EXPECT_EQ(observer.last_tags_changed_note_id_, id);

  service_->RemoveObserver(&observer);
}

TEST_F(NoteServiceTest, Observer_OnNoteTagsChangedFiresOnRemove) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "tag");

  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->RemoveNoteTag(id, "tag");

  EXPECT_EQ(observer.note_tags_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(NoteServiceTest, Observer_OnNoteTagsChangedFiresOnSet) {
  std::string id = CreateTestNote("Note", "");

  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->SetNoteTags(id, {"tag1", "tag2"});

  EXPECT_EQ(observer.note_tags_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(NoteServiceTest, Observer_OnNotesImportedFires) {
  std::string json = R"([
    {"id": "n1", "title": "A", "content": "1"},
    {"id": "n2", "title": "B", "content": "2"}
  ])";

  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->ImportNotesFromJson(json, /*merge=*/false);

  EXPECT_EQ(observer.notes_imported_count_, 1);
  EXPECT_EQ(observer.last_imported_count_, 2);
  EXPECT_EQ(observer.last_imported_service_, service_.get());

  service_->RemoveObserver(&observer);
}

TEST_F(NoteServiceTest, Observer_OnNoteServiceShutdownFires) {
  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  EXPECT_EQ(observer.service_shutdown_count_, 1);
  EXPECT_EQ(observer.last_shutdown_service_, service_.get());

  // Don't remove observer — Shutdown clears them.
}

TEST_F(NoteServiceTest, Observer_ShutdownPreventsFurtherNotifications) {
  TestNoteObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();
  CreateTestNote("After Shutdown", "");

  EXPECT_EQ(observer.note_created_count_, 0);
}

TEST_F(NoteServiceTest, Observer_MultipleObserversAllNotified) {
  TestNoteObserver observer1;
  TestNoteObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  CreateTestNote("Test", "");

  EXPECT_EQ(observer1.note_created_count_, 1);
  EXPECT_EQ(observer2.note_created_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

TEST_F(NoteServiceTest, Observer_RemoveObserverStopsNotifications) {
  TestNoteObserver observer;
  service_->AddObserver(&observer);

  CreateTestNote("First", "");
  EXPECT_EQ(observer.note_created_count_, 1);

  service_->RemoveObserver(&observer);

  CreateTestNote("Second", "");
  EXPECT_EQ(observer.note_created_count_, 1);
}

// =========================================================================
// AstraNoteServiceObserver notifications (extended)
// =========================================================================

TEST_F(NoteServiceTest, ServiceObserver_OnNoteAddedFires) {
  TestNoteServiceObserver observer;
  service_->AddServiceObserver(&observer);

  std::string id = CreateTestNote("New Note", "Content");

  EXPECT_EQ(observer.note_added_count_, 1);
  EXPECT_EQ(observer.last_added_note_id_, id);
  EXPECT_EQ(observer.last_added_note_title_, "New Note");

  service_->RemoveServiceObserver(&observer);
}

TEST_F(NoteServiceTest, ServiceObserver_OnNoteRemovedFires) {
  std::string id = CreateTestNote("To Delete", "");

  TestNoteServiceObserver observer;
  service_->AddServiceObserver(&observer);

  service_->DeleteNote(id);

  EXPECT_EQ(observer.note_removed_count_, 1);
  EXPECT_EQ(observer.last_removed_note_id_, id);

  service_->RemoveServiceObserver(&observer);
}

TEST_F(NoteServiceTest, ServiceObserver_OnNotesReloadedFiresOnDeleteAll) {
  CreateTestNote("Note 1", "");
  CreateTestNote("Note 2", "");

  TestNoteServiceObserver observer;
  service_->AddServiceObserver(&observer);

  service_->DeleteAllNotes();

  EXPECT_GE(observer.notes_reloaded_count_, 1);

  service_->RemoveServiceObserver(&observer);
}

TEST_F(NoteServiceTest, ServiceObserver_OnNoteColorChangedFires) {
  std::string id = CreateTestNote("Note", "");

  TestNoteServiceObserver observer;
  service_->AddServiceObserver(&observer);

  service_->SetNoteColor(id, "#FF0000");

  EXPECT_EQ(observer.note_color_changed_count_, 1);
  EXPECT_EQ(observer.last_color_changed_note_id_, id);
  EXPECT_EQ(observer.last_color_changed_, "#FF0000");

  service_->RemoveServiceObserver(&observer);
}

// =========================================================================
// Edge cases
// =========================================================================

TEST_F(NoteServiceTest, EdgeCase_SpecialCharactersInContent) {
  std::string id = CreateTestNote(
      "Title", "Content with <special> chars & symbols \"quotes\"");

  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->content,
            "Content with <special> chars & symbols \"quotes\"");
}

TEST_F(NoteServiceTest, EdgeCase_VeryLongContent) {
  std::string long_content(10000, 'x');
  std::string id = CreateTestNote("Long Note", long_content);

  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->content.size(), 10000u);
  EXPECT_GT(note->size_bytes, 10000);
}

TEST_F(NoteServiceTest, EdgeCase_UnicodeContent) {
  std::string id = CreateTestNote("Note", "Hello 世界 🌍");

  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->content, "Hello 世界 🌍");
}

TEST_F(NoteServiceTest, EdgeCase_EmptyNoteIsEmpty) {
  std::string id = CreateTestNote("", "");
  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_TRUE(note->IsEmpty());
}

TEST_F(NoteServiceTest, EdgeCase_SearchSpecialCharacters) {
  CreateTestNote("Note", "Content with <special> chars");

  auto results = service_->SearchNotes("<special>");
  EXPECT_EQ(results.size(), 1u);
}

TEST_F(NoteServiceTest, EdgeCase_DuplicateTagAddNoOp) {
  std::string id = CreateTestNote("Note", "");

  EXPECT_TRUE(service_->AddNoteTag(id, "tag"));
  EXPECT_FALSE(service_->AddNoteTag(id, "tag"));
  EXPECT_FALSE(service_->AddNoteTag(id, "tag"));

  EXPECT_EQ(service_->GetNoteTags(id).size(), 1u);
}

TEST_F(NoteServiceTest, EdgeCase_RemoveNonExistentTagNoOp) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "existing");

  EXPECT_FALSE(service_->RemoveNoteTag(id, "nonexistent"));
  EXPECT_EQ(service_->GetNoteTags(id).size(), 1u);
}

TEST_F(NoteServiceTest, EdgeCase_ImportWithInvalidJsonNoChange) {
  CreateTestNote("Existing", "Content");
  ASSERT_EQ(service_->GetNoteCount(), 1u);

  size_t imported = service_->ImportNotesFromJson("{invalid json",
                                                  /*merge=*/true);
  EXPECT_EQ(imported, 0u);
  EXPECT_EQ(service_->GetNoteCount(), 1u);
}

TEST_F(NoteServiceTest, EdgeCase_ImportEmptyJsonArray) {
  CreateTestNote("Existing", "Content");
  ASSERT_EQ(service_->GetNoteCount(), 1u);

  size_t imported = service_->ImportNotesFromJson("[]", /*merge=*/true);
  EXPECT_EQ(imported, 0u);
  EXPECT_EQ(service_->GetNoteCount(), 1u);
}

TEST_F(NoteServiceTest, EdgeCase_MergeNonExistentNotes) {
  std::string id = CreateTestNote("Note", "");

  EXPECT_FALSE(service_->MergeNotes("nonexistent", id, "Title"));
  EXPECT_FALSE(service_->MergeNotes(id, "nonexistent", "Title"));
  EXPECT_EQ(service_->GetNoteCount(), 1u);
}

TEST_F(NoteServiceTest, EdgeCase_DuplicateNotePreservesStats) {
  std::string id = CreateTestNote("Original", "one two three four");
  ASSERT_EQ(service_->GetNote(id)->word_count, 4);

  std::string new_id = service_->DuplicateNote(id);
  ASSERT_FALSE(new_id.empty());

  const AstraNote* copy = service_->GetNote(new_id);
  EXPECT_EQ(copy->word_count, 4);
  EXPECT_GT(copy->size_bytes, 0);
}

TEST_F(NoteServiceTest, EdgeCase_SetNoteTagsEmptyClearsTags) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "tag1");
  service_->AddNoteTag(id, "tag2");
  ASSERT_EQ(service_->GetNoteTags(id).size(), 2u);

  service_->SetNoteTags(id, {});
  EXPECT_TRUE(service_->GetNoteTags(id).empty());
}

TEST_F(NoteServiceTest, EdgeCase_SearchTagPartialMatch) {
  std::string id = CreateTestNote("Note", "");
  service_->AddNoteTag(id, "important");

  auto results = service_->SearchNotes("imp");
  EXPECT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].id, id);
}

TEST_F(NoteServiceTest, EdgeCase_WordCountWithWhitespace) {
  std::string id = CreateTestNote("Title", "  one   two   three  ");
  const AstraNote* note = service_->GetNote(id);
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(note->word_count, 3);
}

TEST_F(NoteServiceTest, EdgeCase_WordCountEmptyContent) {
  std::string id = CreateTestNote("Title", "");
  EXPECT_EQ(service_->GetNote(id)->word_count, 0);
}

TEST_F(NoteServiceTest, EdgeCase_WordCountSingleWord) {
  std::string id = CreateTestNote("Title", "hello");
  EXPECT_EQ(service_->GetNote(id)->word_count, 1);
}

// =========================================================================
// Persistence round-trip
// =========================================================================

TEST_F(NoteServiceTest, Persistence_NotesSurviveServiceRecreation) {
  std::string id1 = CreateTestNote("Persisted Note 1", "Content 1");
  std::string id2 = CreateTestNote("Persisted Note 2", "Content 2");
  ASSERT_EQ(service_->GetNoteCount(), 2u);

  service_.reset();
  auto new_service = std::make_unique<AstraNoteService>(profile_.get());

  EXPECT_EQ(new_service->GetNoteCount(), 2u);
  EXPECT_NE(new_service->GetNote(id1), nullptr);
  EXPECT_NE(new_service->GetNote(id2), nullptr);

  const AstraNote* loaded = new_service->GetNote(id1);
  ASSERT_NE(loaded, nullptr);
  EXPECT_EQ(loaded->title, "Persisted Note 1");
  EXPECT_EQ(loaded->content, "Content 1");
}

TEST_F(NoteServiceTest, Persistence_AllFieldsPersist) {
  std::string id = CreateTestNote("Full Note", "Full content");
  service_->SetNoteWorkspace(id, "ws1");
  service_->SetNoteTabUrl(id, GURL("https://example.com"), "Page Title");
  service_->SetNoteColor(id, "#AABBCC");
  service_->SetNotePinned(id, true);
  service_->SetNoteFavorite(id, true);
  service_->AddNoteTag(id, "tag1");
  service_->AddNoteTag(id, "tag2");

  service_.reset();
  auto new_service = std::make_unique<AstraNoteService>(profile_.get());

  const AstraNote* loaded = new_service->GetNote(id);
  ASSERT_NE(loaded, nullptr);
  EXPECT_EQ(loaded->title, "Full Note");
  EXPECT_EQ(loaded->content, "Full content");
  EXPECT_EQ(loaded->workspace_id, "ws1");
  EXPECT_EQ(loaded->tab_url, GURL("https://example.com"));
  EXPECT_EQ(loaded->tab_title, "Page Title");
  EXPECT_EQ(loaded->color, "#AABBCC");
  EXPECT_TRUE(loaded->is_pinned);
  EXPECT_TRUE(loaded->is_favorite);
  EXPECT_EQ(loaded->tags.size(), 2u);
  EXPECT_GT(loaded->word_count, 0);
  EXPECT_GT(loaded->size_bytes, 0);
}

TEST_F(NoteServiceTest, Persistence_DeletePersists) {
  std::string id = CreateTestNote("ToDelete", "");
  ASSERT_EQ(service_->GetNoteCount(), 1u);

  service_->DeleteNote(id);
  ASSERT_EQ(service_->GetNoteCount(), 0u);

  service_.reset();
  auto new_service = std::make_unique<AstraNoteService>(profile_.get());

  EXPECT_EQ(new_service->GetNoteCount(), 0u);
  EXPECT_EQ(new_service->GetNote(id), nullptr);
}

TEST_F(NoteServiceTest, Persistence_DeleteAllPersists) {
  CreateTestNote("Note 1", "");
  CreateTestNote("Note 2", "");
  CreateTestNote("Note 3", "");

  service_->DeleteAllNotes();

  service_.reset();
  auto new_service = std::make_unique<AstraNoteService>(profile_.get());

  EXPECT_EQ(new_service->GetNoteCount(), 0u);
}

TEST_F(NoteServiceTest, Persistence_EmptyServiceIsEmpty) {
  ASSERT_EQ(service_->GetNoteCount(), 0u);

  service_.reset();
  auto new_service = std::make_unique<AstraNoteService>(profile_.get());

  EXPECT_EQ(new_service->GetNoteCount(), 0u);
}

// =========================================================================
// Color palette
// =========================================================================

TEST(AstraNoteColorPaletteTest, HasMultipleColors) {
  auto palette = AstraNoteService::GetNoteColorPalette();
  EXPECT_GT(palette.size(), 5u);
}

TEST(AstraNoteColorPaletteTest, AllColorsAreValidHex) {
  auto palette = AstraNoteService::GetNoteColorPalette();
  for (const auto& color : palette) {
    EXPECT_EQ(color[0], '#');
    EXPECT_EQ(color.size(), 7u);  // #RRGGBB
  }
}

TEST(AstraNoteColorPaletteTest, DefaultColorIsInPalette) {
  auto palette = AstraNoteService::GetNoteColorPalette();
  ASSERT_FALSE(palette.empty());
  EXPECT_EQ(palette[0], AstraNoteService::kDefaultNoteColor);
}

// =========================================================================
// Sorting
// =========================================================================

TEST_F(NoteServiceTest, DefaultSortOrderIsDateDescending) {
  EXPECT_EQ(service_->sort_order(), NoteSortOrder::kDateDescending);
}

TEST_F(NoteServiceTest, GetNotesSortedBy_DateDescending) {
  std::string id1 = CreateTestNote("Note A", "");
  base::PlatformThread::Sleep(base::Milliseconds(5));
  std::string id2 = CreateTestNote("Note B", "");
  base::PlatformThread::Sleep(base::Milliseconds(5));
  std::string id3 = CreateTestNote("Note C", "");

  auto sorted = service_->GetNotesSortedBy(NoteSortOrder::kDateDescending);
  ASSERT_EQ(sorted.size(), 3u);
  EXPECT_EQ(sorted[0].id, id3);
  EXPECT_EQ(sorted[1].id, id2);
  EXPECT_EQ(sorted[2].id, id1);
}

TEST_F(NoteServiceTest, GetNotesSortedBy_TitleAscending) {
  CreateTestNote("Charlie", "");
  CreateTestNote("Alpha", "");
  CreateTestNote("Bravo", "");

  auto sorted = service_->GetNotesSortedBy(NoteSortOrder::kTitleAscending);
  ASSERT_EQ(sorted.size(), 3u);
  EXPECT_EQ(sorted[0].title, "Alpha");
  EXPECT_EQ(sorted[1].title, "Bravo");
  EXPECT_EQ(sorted[2].title, "Charlie");
}

TEST_F(NoteServiceTest, GetNotesSortedBy_PinnedNotesComeFirst) {
  std::string id1 = CreateTestNote("Unpinned 1", "");
  std::string id2 = CreateTestNote("Pinned", "");
  std::string id3 = CreateTestNote("Unpinned 2", "");

  service_->SetNotePinned(id2, true);

  auto sorted = service_->GetNotesSortedBy(NoteSortOrder::kDateDescending);
  ASSERT_EQ(sorted.size(), 3u);
  EXPECT_EQ(sorted[0].id, id2);
}

TEST_F(NoteServiceTest, SetSortOrder_NotifiesReorder) {
  CreateTestNote("B", "");
  CreateTestNote("A", "");

  TestNoteServiceObserver observer;
  service_->AddServiceObserver(&observer);

  service_->SetSortOrder(NoteSortOrder::kTitleAscending);

  EXPECT_EQ(observer.notes_reordered_count_, 1);

  service_->RemoveServiceObserver(&observer);
}

TEST_F(NoteServiceTest, SetSortOrder_SameOrderNoOp) {
  TestNoteServiceObserver observer;
  service_->AddServiceObserver(&observer);

  service_->SetSortOrder(NoteSortOrder::kDateDescending);
  EXPECT_EQ(observer.notes_reordered_count_, 0);

  service_->RemoveServiceObserver(&observer);
}

// =========================================================================
// Observer default implementations
// =========================================================================

TEST(AstraNoteObserverTest, DefaultImplementationsAreNoOps) {
  class TestObserver : public AstraNoteObserver {};

  TestObserver observer;
  observer.OnNoteCreated(nullptr, "id");
  observer.OnNoteDeleted(nullptr, "id");
  observer.OnNoteChanged(nullptr, "id");
  observer.OnNoteTagsChanged(nullptr, "id");
  observer.OnNotesImported(nullptr, 5);
  observer.OnNoteServiceShutdown(nullptr);
  // No crash = success.
}

TEST(AstraNoteServiceObserverTest, DefaultImplementationsAreNoOps) {
  class TestObserver : public AstraNoteServiceObserver {};

  TestObserver observer;
  AstraNote note;
  observer.OnNoteAdded(note);
  observer.OnNoteUpdated(note);
  observer.OnNoteRemoved("id");
  observer.OnNoteColorChanged("id", "#FF0000");
  observer.OnNotesReordered();
  observer.OnNotesReloaded();
  // No crash = success.
}

// =========================================================================
// Factory
// =========================================================================

TEST(AstraNoteServiceFactoryTest, RegisterProfilePrefsIsCallable) {
  TestingProfile profile;
  AstraNoteServiceFactory::RegisterProfilePrefs(profile.GetPrefs());
  // No crash = success.
}

TEST(AstraNoteServiceFactoryTest, GetInstanceIsCallable) {
  AstraNoteServiceFactory* factory =
      AstraNoteServiceFactory::GetInstance();
  EXPECT_NE(factory, nullptr);
}

TEST(AstraNoteServiceFactoryTest, RegistersAllPrefs) {
  TestingProfile profile;
  PrefService* prefs = profile.GetPrefs();
  AstraNoteServiceFactory::RegisterProfilePrefs(prefs);

  // Verify all prefs are registered by checking defaults.
  EXPECT_EQ(prefs->GetString(AstraNoteService::kPrefNotes), "[]");

  // List prefs return empty list by default.
  // String prefs default to empty string or specified default.
  EXPECT_EQ(prefs->GetString(AstraNoteService::kPrefDefaultNoteColor),
            std::string(AstraNoteService::kDefaultNoteColor));
  EXPECT_EQ(prefs->GetInteger(AstraNoteService::kPrefNoteSortOrder),
            AstraNoteService::kDefaultNoteSortOrder);
  EXPECT_EQ(prefs->GetInteger(AstraNoteService::kPrefAutoSaveIntervalSeconds),
            AstraNoteService::kDefaultAutoSaveIntervalSeconds);
  EXPECT_EQ(prefs->GetBoolean(AstraNoteService::kPrefShowWordCount),
            AstraNoteService::kDefaultShowWordCount);
  EXPECT_EQ(prefs->GetString(AstraNoteService::kPrefDefaultWorkspace),
            std::string(AstraNoteService::kDefaultWorkspace));
  EXPECT_EQ(prefs->GetInteger(AstraNoteService::kPrefMaxSearchResults),
            AstraNoteService::kDefaultMaxSearchResults);
  EXPECT_EQ(prefs->GetBoolean(AstraNoteService::kPrefTrashEnabled),
            AstraNoteService::kDefaultTrashEnabled);
  EXPECT_EQ(prefs->GetBoolean(AstraNoteService::kPrefAutoTagFromPage),
            AstraNoteService::kDefaultAutoTagFromPage);
  EXPECT_EQ(prefs->GetInteger(AstraNoteService::kPrefNoteFontSize),
            AstraNoteService::kDefaultNoteFontSize);
  EXPECT_DOUBLE_EQ(prefs->GetDouble(AstraNoteService::kPrefNoteLineHeight),
                   AstraNoteService::kDefaultNoteLineHeight);
}

// =========================================================================
// Count of tests
// =========================================================================
//
// This file contains 200+ tests covering:
//   - AstraNote struct (11 tests)
//   - Empty service (5 tests)
//   - CreateNote (10 tests)
//   - UpdateNoteTitle (5 tests)
//   - UpdateNoteContent (5 tests)
//   - DeleteNote (3 tests)
//   - DeleteAllNotes (2 tests)
//   - Workspace association (5 tests)
//   - Tab URL association (6 tests)
//   - Tags (13 tests)
//   - Color organization (4 tests)
//   - Pinned organization (5 tests)
//   - Favorite organization (5 tests)
//   - GetAllNotes queries (3 tests)
//   - GetNotesByWorkspace queries (3 tests)
//   - GetNotesByTag queries (3 tests)
//   - GetPinnedNotes queries (2 tests)
//   - GetFavoriteNotes queries (2 tests)
//   - GetNotesByUrl queries (3 tests)
//   - SearchNotes queries (8 tests)
//   - GetRecentlyModifiedNotes queries (4 tests)
//   - GetNotesByColor queries (3 tests)
//   - DeleteNotesByWorkspace bulk (3 tests)
//   - MergeNotes bulk (9 tests)
//   - DuplicateNote bulk (7 tests)
//   - Import/Export (8 tests)
//   - ExportNoteToDict / NoteFromDict (3 tests)
//   - Settings (27 tests)
//   - AstraNoteObserver notifications (14 tests)
//   - Service observer notifications (4 tests)
//   - Edge cases (11 tests)
//   - Persistence (5 tests)
//   - Color palette (3 tests)
//   - Sorting (6 tests)
//   - Observer defaults (2 tests)
//   - Factory (3 tests)
//
// TODO(astra): Add browser_tests for note service integration with real
// profile and pref persistence.
// Chromium component: InProcessBrowserTest + PrefService.

}  // namespace astra
