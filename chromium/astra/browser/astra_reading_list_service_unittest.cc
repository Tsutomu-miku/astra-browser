// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_reading_list_service.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#include "astra/browser/astra_reading_list_service_factory.h"

namespace astra {

namespace {

// =========================================================================
// Test observer that records all calls for verification.
// =========================================================================

class TestReadingListObserver : public AstraReadingListObserver {
 public:
  void OnReadingListEntryAdded(AstraReadingListService* service,
                               const GURL& url) override {
    added_count_++;
    last_added_service_ = service;
    last_added_url_ = url;
  }

  void OnReadingListEntryRemoved(AstraReadingListService* service,
                                 const GURL& url) override {
    removed_count_++;
    last_removed_service_ = service;
    last_removed_url_ = url;
  }

  void OnReadingListEntryChanged(AstraReadingListService* service,
                                 const GURL& url) override {
    changed_count_++;
    last_changed_service_ = service;
    last_changed_url_ = url;
  }

  void OnReadingListEntryStatusChanged(AstraReadingListService* service,
                                       const GURL& url,
                                       bool read) override {
    status_changed_count_++;
    last_status_changed_service_ = service;
    last_status_changed_url_ = url;
    last_status_read_ = read;
  }

  void OnReadingListFolderCreated(AstraReadingListService* service,
                                  const std::string& folder_id) override {
    folder_created_count_++;
    last_folder_created_service_ = service;
    last_folder_created_id_ = folder_id;
  }

  void OnReadingListFolderDeleted(AstraReadingListService* service,
                                  const std::string& folder_id) override {
    folder_deleted_count_++;
    last_folder_deleted_service_ = service;
    last_folder_deleted_id_ = folder_id;
  }

  void OnReadingListChanged(AstraReadingListService* service) override {
    list_changed_count_++;
    last_list_changed_service_ = service;
  }

  void OnReadingListServiceShutdown(AstraReadingListService* service) override {
    shutdown_count_++;
    last_shutdown_service_ = service;
  }

  // Counters
  int added_count_ = 0;
  int removed_count_ = 0;
  int changed_count_ = 0;
  int status_changed_count_ = 0;
  int folder_created_count_ = 0;
  int folder_deleted_count_ = 0;
  int list_changed_count_ = 0;
  int shutdown_count_ = 0;

  // Last recorded values
  raw_ptr<AstraReadingListService> last_added_service_ = nullptr;
  raw_ptr<AstraReadingListService> last_removed_service_ = nullptr;
  raw_ptr<AstraReadingListService> last_changed_service_ = nullptr;
  raw_ptr<AstraReadingListService> last_status_changed_service_ = nullptr;
  raw_ptr<AstraReadingListService> last_folder_created_service_ = nullptr;
  raw_ptr<AstraReadingListService> last_folder_deleted_service_ = nullptr;
  raw_ptr<AstraReadingListService> last_list_changed_service_ = nullptr;
  raw_ptr<AstraReadingListService> last_shutdown_service_ = nullptr;

  GURL last_added_url_;
  GURL last_removed_url_;
  GURL last_changed_url_;
  GURL last_status_changed_url_;
  bool last_status_read_ = false;
  std::string last_folder_created_id_;
  std::string last_folder_deleted_id_;
};

// Helper to create a test URL.
GURL TestUrl(int n) {
  return GURL("https://example.com/article/" + base::NumberToString(n));
}

}  // namespace

// =========================================================================
// Test fixture
// =========================================================================

class ReadingListServiceTest : public testing::Test {
 protected:
  ReadingListServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register reading list prefs before constructing the service.
    AstraReadingListServiceFactory::RegisterProfilePrefs(
        profile_->GetPrefs());
    service_ = std::make_unique<AstraReadingListService>(profile_.get());
    DCHECK(service_);
  }

  ~ReadingListServiceTest() override = default;

  void SetUp() override {
    // Verify initial empty state.
    ASSERT_EQ(service_->GetEntryCount(), 0u);
    ASSERT_TRUE(service_->IsModelLoaded());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  // -- Helpers -------------------------------------------------------------

  // Add a test entry with the given index.  Returns the URL.
  GURL AddTestEntry(int n) {
    GURL url = TestUrl(n);
    std::string title = "Article " + base::NumberToString(n);
    EXPECT_TRUE(service_->AddEntry(url, title));
    return url;
  }

  // Add multiple test entries.
  void AddTestEntries(int count) {
    for (int i = 1; i <= count; ++i) {
      AddTestEntry(i);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraReadingListService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<std::unique_ptr<TestReadingListObserver>> test_observers_;
};

// =========================================================================
// Initial state
// =========================================================================

TEST_F(ReadingListServiceTest, InitialState_EntryCountIsZero) {
  EXPECT_EQ(service_->GetEntryCount(), 0u);
}

TEST_F(ReadingListServiceTest, InitialState_UnreadCountIsZero) {
  EXPECT_EQ(service_->GetUnreadCount(), 0u);
}

TEST_F(ReadingListServiceTest, InitialState_ReadCountIsZero) {
  EXPECT_EQ(service_->GetReadCount(), 0u);
}

TEST_F(ReadingListServiceTest, InitialState_AllEntriesEmpty) {
  auto entries = service_->GetAllEntries();
  EXPECT_TRUE(entries.empty());
}

TEST_F(ReadingListServiceTest, InitialState_UnreadEntriesEmpty) {
  auto entries = service_->GetUnreadEntries();
  EXPECT_TRUE(entries.empty());
}

TEST_F(ReadingListServiceTest, InitialState_ReadEntriesEmpty) {
  auto entries = service_->GetReadEntries();
  EXPECT_TRUE(entries.empty());
}

TEST_F(ReadingListServiceTest, InitialState_FolderCountIsZero) {
  EXPECT_EQ(service_->GetFolderCount(), 0u);
}

TEST_F(ReadingListServiceTest, InitialState_AllFoldersEmpty) {
  auto folders = service_->GetAllFolders();
  EXPECT_TRUE(folders.empty());
}

TEST_F(ReadingListServiceTest, InitialState_ModelIsLoaded) {
  EXPECT_TRUE(service_->IsModelLoaded());
}

TEST_F(ReadingListServiceTest, InitialState_GetAllTagsEmpty) {
  auto tags = service_->GetAllTags();
  EXPECT_TRUE(tags.empty());
}

TEST_F(ReadingListServiceTest, InitialState_FavoriteEntriesEmpty) {
  auto entries = service_->GetFavoriteEntries();
  EXPECT_TRUE(entries.empty());
}

// =========================================================================
// AddEntry
// =========================================================================

TEST_F(ReadingListServiceTest, AddEntry_SingleEntrySucceeds) {
  GURL url = TestUrl(1);
  EXPECT_TRUE(service_->AddEntry(url, "Test Article"));
  EXPECT_EQ(service_->GetEntryCount(), 1u);
}

TEST_F(ReadingListServiceTest, AddEntry_MultipleEntriesSucceed) {
  AddTestEntries(5);
  EXPECT_EQ(service_->GetEntryCount(), 5u);
}

TEST_F(ReadingListServiceTest, AddEntry_DuplicateUrlReturnsFalse) {
  GURL url = TestUrl(1);
  EXPECT_TRUE(service_->AddEntry(url, "First"));
  EXPECT_FALSE(service_->AddEntry(url, "Second"));
  EXPECT_EQ(service_->GetEntryCount(), 1u);
}

TEST_F(ReadingListServiceTest, AddEntry_InvalidUrlReturnsFalse) {
  EXPECT_FALSE(service_->AddEntry(GURL(), "Title"));
  EXPECT_FALSE(service_->AddEntry(GURL("not a url"), "Title"));
}

TEST_F(ReadingListServiceTest, AddEntry_EmptyTitleUsesUrl) {
  GURL url = TestUrl(1);
  EXPECT_TRUE(service_->AddEntry(url, ""));
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->title, url.spec());
}

TEST_F(ReadingListServiceTest, AddEntry_SetsCorrectUrl) {
  GURL url = TestUrl(42);
  service_->AddEntry(url, "Test");
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->url, url);
}

TEST_F(ReadingListServiceTest, AddEntry_SetsCorrectTitle) {
  GURL url = TestUrl(1);
  std::string title = "My Test Article";
  service_->AddEntry(url, title);
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->title, title);
}

TEST_F(ReadingListServiceTest, AddEntry_StatusIsUnseen) {
  GURL url = AddTestEntry(1);
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->status, AstraReadingListStatus::kUnseen);
}

TEST_F(ReadingListServiceTest, AddEntry_SetsAddedTime) {
  base::Time before = base::Time::Now();
  GURL url = AddTestEntry(1);
  base::Time after = base::Time::Now();

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_GE(entry->added_time, before);
  EXPECT_LE(entry->added_time, after);
}

TEST_F(ReadingListServiceTest, AddEntry_SetsUpdateTimeEqualToAddedTime) {
  GURL url = AddTestEntry(1);
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->added_time, entry->update_time);
}

TEST_F(ReadingListServiceTest, AddEntry_DefaultScoreIsMinusOne) {
  GURL url = AddTestEntry(1);
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_DOUBLE_EQ(entry->score, -1.0);
}

TEST_F(ReadingListServiceTest, AddEntry_DefaultWordCountIsMinusOne) {
  GURL url = AddTestEntry(1);
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->word_count, -1);
}

TEST_F(ReadingListServiceTest, AddEntry_DefaultReadCountIsZero) {
  GURL url = AddTestEntry(1);
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->read_count, 0);
}

TEST_F(ReadingListServiceTest, AddEntry_DefaultDistillStateIsUnknown) {
  GURL url = AddTestEntry(1);
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->distill_state, AstraReadingListDistillState::kUnknown);
}

TEST_F(ReadingListServiceTest, AddEntry_UnreadCountIncrements) {
  AddTestEntries(3);
  EXPECT_EQ(service_->GetUnreadCount(), 3u);
}

TEST_F(ReadingListServiceTest, AddEntry_ReadCountStaysZero) {
  AddTestEntries(3);
  EXPECT_EQ(service_->GetReadCount(), 0u);
}

// =========================================================================
// HasEntry / GetEntryByUrl
// =========================================================================

TEST_F(ReadingListServiceTest, HasEntry_ExistingReturnsTrue) {
  GURL url = AddTestEntry(1);
  EXPECT_TRUE(service_->HasEntry(url));
}

TEST_F(ReadingListServiceTest, HasEntry_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->HasEntry(TestUrl(999)));
}

TEST_F(ReadingListServiceTest, HasEntry_InvalidUrlReturnsFalse) {
  EXPECT_FALSE(service_->HasEntry(GURL()));
  EXPECT_FALSE(service_->HasEntry(GURL("not a url")));
}

TEST_F(ReadingListServiceTest, GetEntryByUrl_ExistingReturnsEntry) {
  GURL url = AddTestEntry(1);
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->url, url);
}

TEST_F(ReadingListServiceTest, GetEntryByUrl_NonexistentReturnsNull) {
  EXPECT_EQ(service_->GetEntryByUrl(TestUrl(999)), nullptr);
}

TEST_F(ReadingListServiceTest, GetEntryByUrl_InvalidUrlReturnsNull) {
  EXPECT_EQ(service_->GetEntryByUrl(GURL()), nullptr);
}

TEST_F(ReadingListServiceTest, GetEntryByUrl_CaseSensitiveUrl) {
  GURL url("https://Example.COM/Article");
  service_->AddEntry(url, "Test");
  // GURL comparison is case-sensitive for path but not host.
  GURL same_host_diff_case("https://example.com/Article");
  // The host part should be normalized to lowercase.
  EXPECT_TRUE(service_->HasEntry(same_host_diff_case));
}

// =========================================================================
// RemoveEntry
// =========================================================================

TEST_F(ReadingListServiceTest, RemoveEntry_ExistingRemoves) {
  GURL url = AddTestEntry(1);
  EXPECT_TRUE(service_->RemoveEntry(url));
  EXPECT_EQ(service_->GetEntryCount(), 0u);
  EXPECT_FALSE(service_->HasEntry(url));
}

TEST_F(ReadingListServiceTest, RemoveEntry_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->RemoveEntry(TestUrl(999)));
}

TEST_F(ReadingListServiceTest, RemoveEntry_InvalidUrlReturnsFalse) {
  EXPECT_FALSE(service_->RemoveEntry(GURL()));
}

TEST_F(ReadingListServiceTest, RemoveEntry_MultipleRemoves) {
  GURL url1 = AddTestEntry(1);
  GURL url2 = AddTestEntry(2);
  GURL url3 = AddTestEntry(3);

  EXPECT_TRUE(service_->RemoveEntry(url2));
  EXPECT_EQ(service_->GetEntryCount(), 2u);
  EXPECT_TRUE(service_->HasEntry(url1));
  EXPECT_FALSE(service_->HasEntry(url2));
  EXPECT_TRUE(service_->HasEntry(url3));
}

TEST_F(ReadingListServiceTest, RemoveEntry_RemovesMetadata) {
  GURL url = AddTestEntry(1);
  service_->SetEntryFavorite(url, true);
  service_->AddEntryTag(url, "tag1");
  service_->SetEntryNote(url, "My note");

  EXPECT_TRUE(service_->RemoveEntry(url));

  // After removal, metadata queries should return default/false.
  EXPECT_FALSE(service_->IsEntryFavorite(url));
  EXPECT_TRUE(service_->GetEntryTags(url).empty());
  EXPECT_TRUE(service_->GetEntryNote(url).empty());
}

// =========================================================================
// MarkEntryRead / MarkEntryUnread
// =========================================================================

TEST_F(ReadingListServiceTest, MarkEntryRead_ChangesStatus) {
  GURL url = AddTestEntry(1);
  EXPECT_TRUE(service_->MarkEntryRead(url));

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->status, AstraReadingListStatus::kRead);
}

TEST_F(ReadingListServiceTest, MarkEntryRead_UpdatesReadCount) {
  AddTestEntries(3);
  service_->MarkEntryRead(TestUrl(1));
  service_->MarkEntryRead(TestUrl(2));

  EXPECT_EQ(service_->GetReadCount(), 2u);
  EXPECT_EQ(service_->GetUnreadCount(), 1u);
}

TEST_F(ReadingListServiceTest, MarkEntryRead_AlreadyReadReturnsFalse) {
  GURL url = AddTestEntry(1);
  EXPECT_TRUE(service_->MarkEntryRead(url));
  EXPECT_FALSE(service_->MarkEntryRead(url));
}

TEST_F(ReadingListServiceTest, MarkEntryRead_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->MarkEntryRead(TestUrl(999)));
}

TEST_F(ReadingListServiceTest, MarkEntryUnread_ChangesStatus) {
  GURL url = AddTestEntry(1);
  service_->MarkEntryRead(url);
  EXPECT_TRUE(service_->MarkEntryUnread(url));

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->status, AstraReadingListStatus::kUnread);
}

TEST_F(ReadingListServiceTest, MarkEntryUnread_AlreadyUnreadReturnsFalse) {
  GURL url = AddTestEntry(1);
  // New entries start as unseen, marking unread should succeed.
  EXPECT_TRUE(service_->MarkEntryUnread(url));
  // Already unread, should return false.
  EXPECT_FALSE(service_->MarkEntryUnread(url));
}

TEST_F(ReadingListServiceTest, MarkEntryUnread_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->MarkEntryUnread(TestUrl(999)));
}

TEST_F(ReadingListServiceTest, MarkEntryRead_SetsLastReadTime) {
  GURL url = AddTestEntry(1);
  base::Time before = base::Time::Now();
  service_->MarkEntryRead(url);
  base::Time after = base::Time::Now();

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_FALSE(entry->last_read_time.is_null());
  EXPECT_GE(entry->last_read_time, before);
  EXPECT_LE(entry->last_read_time, after);
}

TEST_F(ReadingListServiceTest, MarkEntryRead_SetsFirstReadTime) {
  GURL url = AddTestEntry(1);
  service_->MarkEntryRead(url);

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_FALSE(entry->first_read_time.is_null());
  EXPECT_EQ(entry->first_read_time, entry->last_read_time);
}

TEST_F(ReadingListServiceTest, MarkEntryRead_IncrementsReadCount) {
  GURL url = AddTestEntry(1);
  service_->MarkEntryRead(url);
  service_->MarkEntryUnread(url);
  service_->MarkEntryRead(url);

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_GE(entry->read_count, 2);
}

TEST_F(ReadingListServiceTest, MarkEntryRead_UpdatesUpdateTime) {
  GURL url = AddTestEntry(1);
  base::Time original_update;
  {
    const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
    ASSERT_NE(entry, nullptr);
    original_update = entry->update_time;
  }

  // Small sleep to ensure time changes.
  base::TimeDelta delay = base::Milliseconds(1);
  task_environment_.FastForwardBy(delay);

  service_->MarkEntryRead(url);

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_GT(entry->update_time, original_update);
}

// =========================================================================
// UpdateEntryTitle
// =========================================================================

TEST_F(ReadingListServiceTest, UpdateEntryTitle_ChangesTitle) {
  GURL url = AddTestEntry(1);
  std::string new_title = "New Title";
  EXPECT_TRUE(service_->UpdateEntryTitle(url, new_title));

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->title, new_title);
}

TEST_F(ReadingListServiceTest, UpdateEntryTitle_SameTitleReturnsFalse) {
  GURL url = AddTestEntry(1);
  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_FALSE(service_->UpdateEntryTitle(url, entry->title));
}

TEST_F(ReadingListServiceTest, UpdateEntryTitle_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->UpdateEntryTitle(TestUrl(999), "New Title"));
}

TEST_F(ReadingListServiceTest, UpdateEntryTitle_EmptyTitleAllowed) {
  GURL url = AddTestEntry(1);
  EXPECT_TRUE(service_->UpdateEntryTitle(url, ""));
}

// =========================================================================
// MarkAllRead / DeleteRead
// =========================================================================

TEST_F(ReadingListServiceTest, MarkAllRead_MarksAllAsRead) {
  AddTestEntries(5);
  size_t count = service_->MarkAllRead();
  EXPECT_EQ(count, 5u);
  EXPECT_EQ(service_->GetReadCount(), 5u);
  EXPECT_EQ(service_->GetUnreadCount(), 0u);
}

TEST_F(ReadingListServiceTest, MarkAllRead_EmptyListReturnsZero) {
  EXPECT_EQ(service_->MarkAllRead(), 0u);
}

TEST_F(ReadingListServiceTest, MarkAllRead_AlreadyAllReadReturnsZero) {
  AddTestEntries(3);
  service_->MarkAllRead();
  EXPECT_EQ(service_->MarkAllRead(), 0u);
}

TEST_F(ReadingListServiceTest, DeleteRead_DeletesAllReadEntries) {
  AddTestEntries(5);
  service_->MarkEntryRead(TestUrl(1));
  service_->MarkEntryRead(TestUrl(3));
  service_->MarkEntryRead(TestUrl(5));

  size_t deleted = service_->DeleteRead();
  EXPECT_EQ(deleted, 3u);
  EXPECT_EQ(service_->GetEntryCount(), 2u);
  EXPECT_EQ(service_->GetReadCount(), 0u);
}

TEST_F(ReadingListServiceTest, DeleteRead_NoReadEntriesReturnsZero) {
  AddTestEntries(3);
  EXPECT_EQ(service_->DeleteRead(), 0u);
}

TEST_F(ReadingListServiceTest, DeleteRead_EmptyListReturnsZero) {
  EXPECT_EQ(service_->DeleteRead(), 0u);
}

// =========================================================================
// GetUnreadEntries / GetReadEntries / GetAllEntries
// =========================================================================

TEST_F(ReadingListServiceTest, GetAllEntries_ReturnsAllEntries) {
  AddTestEntries(5);
  auto entries = service_->GetAllEntries();
  EXPECT_EQ(entries.size(), 5u);
}

TEST_F(ReadingListServiceTest, GetUnreadEntries_ReturnsOnlyUnread) {
  AddTestEntries(5);
  service_->MarkEntryRead(TestUrl(2));
  service_->MarkEntryRead(TestUrl(4));

  auto entries = service_->GetUnreadEntries();
  EXPECT_EQ(entries.size(), 3u);
  for (const auto& entry : entries) {
    EXPECT_NE(entry.status, AstraReadingListStatus::kRead);
  }
}

TEST_F(ReadingListServiceTest, GetReadEntries_ReturnsOnlyRead) {
  AddTestEntries(5);
  service_->MarkEntryRead(TestUrl(1));
  service_->MarkEntryRead(TestUrl(3));
  service_->MarkEntryRead(TestUrl(5));

  auto entries = service_->GetReadEntries();
  EXPECT_EQ(entries.size(), 3u);
  for (const auto& entry : entries) {
    EXPECT_EQ(entry.status, AstraReadingListStatus::kRead);
  }
}

TEST_F(ReadingListServiceTest, GetUnreadEntries_WithMaxCount) {
  AddTestEntries(10);
  auto entries = service_->GetUnreadEntries(3);
  EXPECT_EQ(entries.size(), 3u);
}

TEST_F(ReadingListServiceTest, GetReadEntries_WithMaxCount) {
  AddTestEntries(10);
  for (int i = 1; i <= 8; ++i) {
    service_->MarkEntryRead(TestUrl(i));
  }
  auto entries = service_->GetReadEntries(3);
  EXPECT_EQ(entries.size(), 3u);
}

TEST_F(ReadingListServiceTest, GetUnreadEntries_ZeroMaxMeansAll) {
  AddTestEntries(10);
  auto entries = service_->GetUnreadEntries(0);
  EXPECT_EQ(entries.size(), 10u);
}

TEST_F(ReadingListServiceTest, GetReadEntries_MaxCountLargerThanAvailable) {
  AddTestEntries(3);
  service_->MarkEntryRead(TestUrl(1));

  auto entries = service_->GetReadEntries(10);
  EXPECT_EQ(entries.size(), 1u);
}

// =========================================================================
// GetRecentlyAddedEntries / GetRecentlyReadEntries
// =========================================================================

TEST_F(ReadingListServiceTest, GetRecentlyAddedEntries_MostRecentFirst) {
  // Add entries with time gaps.
  for (int i = 1; i <= 5; ++i) {
    AddTestEntry(i);
    task_environment_.FastForwardBy(base::Milliseconds(10));
  }

  auto entries = service_->GetRecentlyAddedEntries(3);
  ASSERT_EQ(entries.size(), 3u);
  // Most recent first: article 5, 4, 3
  EXPECT_TRUE(entries[0].title.find("5") != std::string::npos);
  EXPECT_TRUE(entries[1].title.find("4") != std::string::npos);
  EXPECT_TRUE(entries[2].title.find("3") != std::string::npos);
}

TEST_F(ReadingListServiceTest, GetRecentlyAddedEntries_ZeroMaxMeansAll) {
  AddTestEntries(5);
  auto entries = service_->GetRecentlyAddedEntries(0);
  // Zero max_count means no limit — all entries are returned.
  EXPECT_EQ(entries.size(), 5u);
}

TEST_F(ReadingListServiceTest, GetRecentlyAddedEntries_EmptyListEmpty) {
  auto entries = service_->GetRecentlyAddedEntries(10);
  EXPECT_TRUE(entries.empty());
}

TEST_F(ReadingListServiceTest, GetRecentlyReadEntries_MostRecentFirst) {
  AddTestEntries(5);

  // Mark read in order: 1, 2, 3, 4, 5 (with time gaps).
  for (int i = 1; i <= 5; ++i) {
    service_->MarkEntryRead(TestUrl(i));
    task_environment_.FastForwardBy(base::Milliseconds(10));
  }

  auto entries = service_->GetRecentlyReadEntries(3);
  ASSERT_EQ(entries.size(), 3u);
  // Most recent first: article 5, 4, 3
  EXPECT_TRUE(entries[0].title.find("5") != std::string::npos);
  EXPECT_TRUE(entries[1].title.find("4") != std::string::npos);
  EXPECT_TRUE(entries[2].title.find("3") != std::string::npos);
}

TEST_F(ReadingListServiceTest, GetRecentlyReadEntries_ExcludesUnread) {
  AddTestEntries(5);
  service_->MarkEntryRead(TestUrl(2));
  service_->MarkEntryRead(TestUrl(4));

  auto entries = service_->GetRecentlyReadEntries(10);
  EXPECT_EQ(entries.size(), 2u);
}

// =========================================================================
// Favorites metadata
// =========================================================================

TEST_F(ReadingListServiceTest, SetEntryFavorite_SetsFavoriteTrue) {
  GURL url = AddTestEntry(1);
  EXPECT_TRUE(service_->SetEntryFavorite(url, true));
  EXPECT_TRUE(service_->IsEntryFavorite(url));
}

TEST_F(ReadingListServiceTest, SetEntryFavorite_SetsFavoriteFalse) {
  GURL url = AddTestEntry(1);
  service_->SetEntryFavorite(url, true);
  EXPECT_TRUE(service_->SetEntryFavorite(url, false));
  EXPECT_FALSE(service_->IsEntryFavorite(url));
}

TEST_F(ReadingListServiceTest, SetEntryFavorite_SameValueReturnsFalse) {
  GURL url = AddTestEntry(1);
  // Default is false, setting to false again should return false.
  EXPECT_FALSE(service_->SetEntryFavorite(url, false));
}

TEST_F(ReadingListServiceTest, SetEntryFavorite_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->SetEntryFavorite(TestUrl(999), true));
}

TEST_F(ReadingListServiceTest, SetEntryFavorite_InvalidUrlReturnsFalse) {
  EXPECT_FALSE(service_->SetEntryFavorite(GURL(), true));
}

TEST_F(ReadingListServiceTest, IsEntryFavorite_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->IsEntryFavorite(TestUrl(999)));
}

TEST_F(ReadingListServiceTest, IsEntryFavorite_InvalidUrlReturnsFalse) {
  EXPECT_FALSE(service_->IsEntryFavorite(GURL()));
}

TEST_F(ReadingListServiceTest, GetFavoriteEntries_ReturnsOnlyFavorites) {
  AddTestEntries(5);
  service_->SetEntryFavorite(TestUrl(2), true);
  service_->SetEntryFavorite(TestUrl(4), true);

  auto favorites = service_->GetFavoriteEntries();
  EXPECT_EQ(favorites.size(), 2u);
}

TEST_F(ReadingListServiceTest, GetFavoriteEntries_EmptyWhenNone) {
  AddTestEntries(5);
  auto favorites = service_->GetFavoriteEntries();
  EXPECT_TRUE(favorites.empty());
}

// =========================================================================
// Notes metadata
// =========================================================================

TEST_F(ReadingListServiceTest, SetEntryNote_SetsNote) {
  GURL url = AddTestEntry(1);
  std::string note = "This is a great article.";
  EXPECT_TRUE(service_->SetEntryNote(url, note));
  EXPECT_EQ(service_->GetEntryNote(url), note);
}

TEST_F(ReadingListServiceTest, SetEntryNote_SameNoteReturnsFalse) {
  GURL url = AddTestEntry(1);
  std::string note = "Test note";
  service_->SetEntryNote(url, note);
  EXPECT_FALSE(service_->SetEntryNote(url, note));
}

TEST_F(ReadingListServiceTest, SetEntryNote_ClearsNote) {
  GURL url = AddTestEntry(1);
  service_->SetEntryNote(url, "Test");
  EXPECT_TRUE(service_->SetEntryNote(url, ""));
  EXPECT_TRUE(service_->GetEntryNote(url).empty());
}

TEST_F(ReadingListServiceTest, SetEntryNote_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->SetEntryNote(TestUrl(999), "note"));
}

TEST_F(ReadingListServiceTest, SetEntryNote_InvalidUrlReturnsFalse) {
  EXPECT_FALSE(service_->SetEntryNote(GURL(), "note"));
}

TEST_F(ReadingListServiceTest, GetEntryNote_NonexistentReturnsEmpty) {
  EXPECT_TRUE(service_->GetEntryNote(TestUrl(999)).empty());
}

TEST_F(ReadingListServiceTest, GetEntryNote_InvalidUrlReturnsEmpty) {
  EXPECT_TRUE(service_->GetEntryNote(GURL()).empty());
}

TEST_F(ReadingListServiceTest, SetEntryNote_LongNote) {
  GURL url = AddTestEntry(1);
  std::string long_note(1000, 'x');
  EXPECT_TRUE(service_->SetEntryNote(url, long_note));
  EXPECT_EQ(service_->GetEntryNote(url), long_note);
}

// =========================================================================
// Tags metadata
// =========================================================================

TEST_F(ReadingListServiceTest, AddEntryTag_AddsTag) {
  GURL url = AddTestEntry(1);
  EXPECT_TRUE(service_->AddEntryTag(url, "tech"));

  auto tags = service_->GetEntryTags(url);
  ASSERT_EQ(tags.size(), 1u);
  EXPECT_EQ(tags[0], "tech");
}

TEST_F(ReadingListServiceTest, AddEntryTag_MultipleTags) {
  GURL url = AddTestEntry(1);
  service_->AddEntryTag(url, "tech");
  service_->AddEntryTag(url, "news");
  service_->AddEntryTag(url, "design");

  auto tags = service_->GetEntryTags(url);
  EXPECT_EQ(tags.size(), 3u);
}

TEST_F(ReadingListServiceTest, AddEntryTag_DuplicateIgnored) {
  GURL url = AddTestEntry(1);
  EXPECT_TRUE(service_->AddEntryTag(url, "tech"));
  EXPECT_FALSE(service_->AddEntryTag(url, "tech"));

  auto tags = service_->GetEntryTags(url);
  EXPECT_EQ(tags.size(), 1u);
}

TEST_F(ReadingListServiceTest, AddEntryTag_EmptyTagReturnsFalse) {
  GURL url = AddTestEntry(1);
  EXPECT_FALSE(service_->AddEntryTag(url, ""));
}

TEST_F(ReadingListServiceTest, AddEntryTag_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->AddEntryTag(TestUrl(999), "tech"));
}

TEST_F(ReadingListServiceTest, AddEntryTag_InvalidUrlReturnsFalse) {
  EXPECT_FALSE(service_->AddEntryTag(GURL(), "tech"));
}

TEST_F(ReadingListServiceTest, RemoveEntryTag_RemovesTag) {
  GURL url = AddTestEntry(1);
  service_->AddEntryTag(url, "tech");
  service_->AddEntryTag(url, "news");

  EXPECT_TRUE(service_->RemoveEntryTag(url, "tech"));
  auto tags = service_->GetEntryTags(url);
  EXPECT_EQ(tags.size(), 1u);
  EXPECT_EQ(tags[0], "news");
}

TEST_F(ReadingListServiceTest, RemoveEntryTag_NonexistentTagReturnsFalse) {
  GURL url = AddTestEntry(1);
  service_->AddEntryTag(url, "tech");
  EXPECT_FALSE(service_->RemoveEntryTag(url, "news"));
}

TEST_F(ReadingListServiceTest, RemoveEntryTag_EmptyTagReturnsFalse) {
  GURL url = AddTestEntry(1);
  EXPECT_FALSE(service_->RemoveEntryTag(url, ""));
}

TEST_F(ReadingListServiceTest, RemoveEntryTag_NonexistentEntryReturnsFalse) {
  EXPECT_FALSE(service_->RemoveEntryTag(TestUrl(999), "tech"));
}

TEST_F(ReadingListServiceTest, GetEntryTags_NonexistentReturnsEmpty) {
  auto tags = service_->GetEntryTags(TestUrl(999));
  EXPECT_TRUE(tags.empty());
}

TEST_F(ReadingListServiceTest, GetEntryTags_InvalidUrlReturnsEmpty) {
  auto tags = service_->GetEntryTags(GURL());
  EXPECT_TRUE(tags.empty());
}

TEST_F(ReadingListServiceTest, GetAllTags_ReturnsAllUniqueTags) {
  AddTestEntries(3);
  service_->AddEntryTag(TestUrl(1), "tech");
  service_->AddEntryTag(TestUrl(1), "news");
  service_->AddEntryTag(TestUrl(2), "tech");
  service_->AddEntryTag(TestUrl(3), "design");

  auto tags = service_->GetAllTags();
  EXPECT_EQ(tags.size(), 3u);
  // Should be sorted alphabetically.
  EXPECT_EQ(tags[0], "design");
  EXPECT_EQ(tags[1], "news");
  EXPECT_EQ(tags[2], "tech");
}

TEST_F(ReadingListServiceTest, GetAllTags_EmptyWhenNone) {
  AddTestEntries(3);
  auto tags = service_->GetAllTags();
  EXPECT_TRUE(tags.empty());
}

// =========================================================================
// Folder operations
// =========================================================================

TEST_F(ReadingListServiceTest, CreateFolder_CreatesFolder) {
  std::string folder_id = service_->CreateFolder("Tech Articles");
  EXPECT_FALSE(folder_id.empty());
  EXPECT_EQ(service_->GetFolderCount(), 1u);
}

TEST_F(ReadingListServiceTest, CreateFolder_EmptyNameReturnsEmpty) {
  std::string folder_id = service_->CreateFolder("");
  EXPECT_TRUE(folder_id.empty());
  EXPECT_EQ(service_->GetFolderCount(), 0u);
}

TEST_F(ReadingListServiceTest, CreateFolder_MultipleFolders) {
  std::string id1 = service_->CreateFolder("Folder 1");
  std::string id2 = service_->CreateFolder("Folder 2");
  std::string id3 = service_->CreateFolder("Folder 3");

  EXPECT_FALSE(id1.empty());
  EXPECT_FALSE(id2.empty());
  EXPECT_FALSE(id3.empty());
  EXPECT_NE(id1, id2);
  EXPECT_NE(id2, id3);
  EXPECT_EQ(service_->GetFolderCount(), 3u);
}

TEST_F(ReadingListServiceTest, CreateFolder_IdsAreUnique) {
  std::set<std::string> ids;
  for (int i = 0; i < 100; ++i) {
    std::string id = service_->CreateFolder("Folder " + base::NumberToString(i));
    EXPECT_TRUE(ids.insert(id).second) << "Duplicate folder ID: " << id;
  }
}

TEST_F(ReadingListServiceTest, GetFolder_ExistingReturnsFolder) {
  std::string folder_id = service_->CreateFolder("My Folder");
  const AstraReadingListFolder* folder = service_->GetFolder(folder_id);
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(folder->folder_id, folder_id);
  EXPECT_EQ(folder->name, "My Folder");
  EXPECT_FALSE(folder->is_default);
  EXPECT_EQ(folder->entry_count, 0);
  EXPECT_EQ(folder->unread_count, 0);
}

TEST_F(ReadingListServiceTest, GetFolder_NonexistentReturnsNull) {
  EXPECT_EQ(service_->GetFolder("nonexistent-id"), nullptr);
}

TEST_F(ReadingListServiceTest, GetFolder_EmptyIdReturnsNull) {
  EXPECT_EQ(service_->GetFolder(""), nullptr);
}

TEST_F(ReadingListServiceTest, DeleteFolder_DeletesFolder) {
  std::string folder_id = service_->CreateFolder("To Delete");
  EXPECT_TRUE(service_->DeleteFolder(folder_id));
  EXPECT_EQ(service_->GetFolderCount(), 0u);
  EXPECT_EQ(service_->GetFolder(folder_id), nullptr);
}

TEST_F(ReadingListServiceTest, DeleteFolder_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->DeleteFolder("nonexistent-id"));
}

TEST_F(ReadingListServiceTest, DeleteFolder_EmptyIdReturnsFalse) {
  EXPECT_FALSE(service_->DeleteFolder(""));
}

TEST_F(ReadingListServiceTest, DeleteFolder_MovesEntriesToUncategorized) {
  std::string folder_id = service_->CreateFolder("Temp");
  AddTestEntries(3);
  service_->MoveEntryToFolder(TestUrl(1), folder_id);
  service_->MoveEntryToFolder(TestUrl(2), folder_id);

  EXPECT_TRUE(service_->DeleteFolder(folder_id));

  // Entries should no longer be in the folder.
  EXPECT_TRUE(service_->GetEntryFolder(TestUrl(1)).empty());
  EXPECT_TRUE(service_->GetEntryFolder(TestUrl(2)).empty());
  EXPECT_TRUE(service_->GetEntryFolder(TestUrl(3)).empty());
}

TEST_F(ReadingListServiceTest, RenameFolder_RenamesFolder) {
  std::string folder_id = service_->CreateFolder("Old Name");
  EXPECT_TRUE(service_->RenameFolder(folder_id, "New Name"));

  const AstraReadingListFolder* folder = service_->GetFolder(folder_id);
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(folder->name, "New Name");
}

TEST_F(ReadingListServiceTest, RenameFolder_SameNameReturnsFalse) {
  std::string folder_id = service_->CreateFolder("My Folder");
  EXPECT_FALSE(service_->RenameFolder(folder_id, "My Folder"));
}

TEST_F(ReadingListServiceTest, RenameFolder_NonexistentReturnsFalse) {
  EXPECT_FALSE(service_->RenameFolder("nonexistent", "New Name"));
}

TEST_F(ReadingListServiceTest, RenameFolder_EmptyNameReturnsFalse) {
  std::string folder_id = service_->CreateFolder("My Folder");
  EXPECT_FALSE(service_->RenameFolder(folder_id, ""));
}

TEST_F(ReadingListServiceTest, GetAllFolders_ReturnsAllFolders) {
  service_->CreateFolder("Folder A");
  service_->CreateFolder("Folder B");
  service_->CreateFolder("Folder C");

  auto folders = service_->GetAllFolders();
  EXPECT_EQ(folders.size(), 3u);
}

TEST_F(ReadingListServiceTest, GetAllFolders_SortedByOrderIndex) {
  std::string id1 = service_->CreateFolder("First");
  std::string id2 = service_->CreateFolder("Second");
  std::string id3 = service_->CreateFolder("Third");

  auto folders = service_->GetAllFolders();
  ASSERT_EQ(folders.size(), 3u);
  // Created in order, so order_index should be 0, 1, 2.
  EXPECT_EQ(folders[0].order_index, 0);
  EXPECT_EQ(folders[1].order_index, 1);
  EXPECT_EQ(folders[2].order_index, 2);
}

// =========================================================================
// Entry folder membership
// =========================================================================

TEST_F(ReadingListServiceTest, SetEntryFolder_SetsFolder) {
  std::string folder_id = service_->CreateFolder("Tech");
  GURL url = AddTestEntry(1);

  EXPECT_TRUE(service_->SetEntryFolder(url, folder_id));
  EXPECT_EQ(service_->GetEntryFolder(url), folder_id);
}

TEST_F(ReadingListServiceTest, SetEntryFolder_ClearsFolder) {
  std::string folder_id = service_->CreateFolder("Tech");
  GURL url = AddTestEntry(1);
  service_->SetEntryFolder(url, folder_id);

  EXPECT_TRUE(service_->SetEntryFolder(url, ""));
  EXPECT_TRUE(service_->GetEntryFolder(url).empty());
}

TEST_F(ReadingListServiceTest, SetEntryFolder_SameFolderReturnsFalse) {
  std::string folder_id = service_->CreateFolder("Tech");
  GURL url = AddTestEntry(1);
  service_->SetEntryFolder(url, folder_id);

  EXPECT_FALSE(service_->SetEntryFolder(url, folder_id));
}

TEST_F(ReadingListServiceTest, SetEntryFolder_NonexistentFolderReturnsFalse) {
  GURL url = AddTestEntry(1);
  EXPECT_FALSE(service_->SetEntryFolder(url, "nonexistent-folder"));
}

TEST_F(ReadingListServiceTest, SetEntryFolder_NonexistentEntryReturnsFalse) {
  std::string folder_id = service_->CreateFolder("Tech");
  EXPECT_FALSE(service_->SetEntryFolder(TestUrl(999), folder_id));
}

TEST_F(ReadingListServiceTest, SetEntryFolder_InvalidUrlReturnsFalse) {
  std::string folder_id = service_->CreateFolder("Tech");
  EXPECT_FALSE(service_->SetEntryFolder(GURL(), folder_id));
}

TEST_F(ReadingListServiceTest, GetEntryFolder_NonexistentReturnsEmpty) {
  EXPECT_TRUE(service_->GetEntryFolder(TestUrl(999)).empty());
}

TEST_F(ReadingListServiceTest, GetEntryFolder_InvalidUrlReturnsEmpty) {
  EXPECT_TRUE(service_->GetEntryFolder(GURL()).empty());
}

TEST_F(ReadingListServiceTest, MoveEntryToFolder_SameAsSetEntryFolder) {
  std::string folder_id = service_->CreateFolder("Tech");
  GURL url = AddTestEntry(1);

  EXPECT_TRUE(service_->MoveEntryToFolder(url, folder_id));
  EXPECT_EQ(service_->GetEntryFolder(url), folder_id);
}

TEST_F(ReadingListServiceTest, GetEntriesInFolder_ReturnsEntriesInFolder) {
  std::string folder_id = service_->CreateFolder("Tech");
  AddTestEntries(5);
  service_->MoveEntryToFolder(TestUrl(1), folder_id);
  service_->MoveEntryToFolder(TestUrl(3), folder_id);
  service_->MoveEntryToFolder(TestUrl(5), folder_id);

  auto entries = service_->GetEntriesInFolder(folder_id);
  EXPECT_EQ(entries.size(), 3u);
}

TEST_F(ReadingListServiceTest, GetEntriesInFolder_EmptyFolderEmpty) {
  std::string folder_id = service_->CreateFolder("Empty");
  auto entries = service_->GetEntriesInFolder(folder_id);
  EXPECT_TRUE(entries.empty());
}

TEST_F(ReadingListServiceTest, GetEntriesInFolder_NonexistentFolderEmpty) {
  auto entries = service_->GetEntriesInFolder("nonexistent");
  EXPECT_TRUE(entries.empty());
}

TEST_F(ReadingListServiceTest, FolderEntryCount_UpdatesWhenEntriesMove) {
  std::string folder_id = service_->CreateFolder("Tech");
  AddTestEntries(3);

  const AstraReadingListFolder* folder = service_->GetFolder(folder_id);
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(folder->entry_count, 0);

  service_->MoveEntryToFolder(TestUrl(1), folder_id);
  folder = service_->GetFolder(folder_id);
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(folder->entry_count, 1);

  service_->MoveEntryToFolder(TestUrl(2), folder_id);
  folder = service_->GetFolder(folder_id);
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(folder->entry_count, 2);

  service_->SetEntryFolder(TestUrl(1), "");
  folder = service_->GetFolder(folder_id);
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(folder->entry_count, 1);
}

TEST_F(ReadingListServiceTest, FolderUnreadCount_TracksUnreadEntries) {
  std::string folder_id = service_->CreateFolder("Tech");
  AddTestEntries(3);
  service_->MoveEntryToFolder(TestUrl(1), folder_id);
  service_->MoveEntryToFolder(TestUrl(2), folder_id);
  service_->MoveEntryToFolder(TestUrl(3), folder_id);

  const AstraReadingListFolder* folder = service_->GetFolder(folder_id);
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(folder->unread_count, 3);

  service_->MarkEntryRead(TestUrl(2));
  folder = service_->GetFolder(folder_id);
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(folder->unread_count, 2);
}

// =========================================================================
// Folder reordering
// =========================================================================

TEST_F(ReadingListServiceTest, ReorderFolders_ReordersFolders) {
  std::string id1 = service_->CreateFolder("A");
  std::string id2 = service_->CreateFolder("B");
  std::string id3 = service_->CreateFolder("C");

  // Reorder to C, A, B
  EXPECT_TRUE(service_->ReorderFolders({id3, id1, id2}));

  auto folders = service_->GetAllFolders();
  ASSERT_EQ(folders.size(), 3u);
  EXPECT_EQ(folders[0].folder_id, id3);
  EXPECT_EQ(folders[1].folder_id, id1);
  EXPECT_EQ(folders[2].folder_id, id2);
}

TEST_F(ReadingListServiceTest, ReorderFolders_SameOrderReturnsFalse) {
  std::string id1 = service_->CreateFolder("A");
  std::string id2 = service_->CreateFolder("B");

  EXPECT_FALSE(service_->ReorderFolders({id1, id2}));
}

TEST_F(ReadingListServiceTest, ReorderFolders_EmptyListReturnsFalse) {
  service_->CreateFolder("A");
  EXPECT_FALSE(service_->ReorderFolders({}));
}

TEST_F(ReadingListServiceTest, ReorderFolders_PartialOrder) {
  std::string id1 = service_->CreateFolder("A");
  std::string id2 = service_->CreateFolder("B");
  std::string id3 = service_->CreateFolder("C");
  std::string id4 = service_->CreateFolder("D");

  // Specify only first two; remaining stay in their relative order at the end.
  EXPECT_TRUE(service_->ReorderFolders({id3, id1}));

  auto folders = service_->GetAllFolders();
  ASSERT_EQ(folders.size(), 4u);
  EXPECT_EQ(folders[0].folder_id, id3);
  EXPECT_EQ(folders[1].folder_id, id1);
  // Remaining should be B, D (original relative order).
  EXPECT_EQ(folders[2].folder_id, id2);
  EXPECT_EQ(folders[3].folder_id, id4);
}

TEST_F(ReadingListServiceTest, ReorderFolders_InvalidIdsIgnored) {
  std::string id1 = service_->CreateFolder("A");
  std::string id2 = service_->CreateFolder("B");

  EXPECT_TRUE(service_->ReorderFolders({id2, "nonexistent", id1}));

  auto folders = service_->GetAllFolders();
  ASSERT_EQ(folders.size(), 2u);
  EXPECT_EQ(folders[0].folder_id, id2);
  EXPECT_EQ(folders[1].folder_id, id1);
}

// =========================================================================
// Search
// =========================================================================

TEST_F(ReadingListServiceTest, SearchEntries_FindsByTitle) {
  AddTestEntries(3);
  service_->AddEntry(GURL("https://example.com/alpha"), "Alpha Article");
  service_->AddEntry(GURL("https://example.com/beta"), "Beta Page");

  auto results = service_->SearchEntries("Alpha");
  EXPECT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].title, "Alpha Article");
}

TEST_F(ReadingListServiceTest, SearchEntries_FindsByUrl) {
  service_->AddEntry(GURL("https://tech.example.com/page"), "Page");
  service_->AddEntry(GURL("https://news.example.com/article"), "Article");

  auto results = service_->SearchEntries("tech.example");
  EXPECT_EQ(results.size(), 1u);
}

TEST_F(ReadingListServiceTest, SearchEntries_CaseInsensitive) {
  service_->AddEntry(GURL("https://example.com/a"), "Mixed Case Title");

  auto results = service_->SearchEntries("mixed case");
  EXPECT_EQ(results.size(), 1u);
}

TEST_F(ReadingListServiceTest, SearchEntries_EmptyQueryReturnsAll) {
  AddTestEntries(5);
  auto results = service_->SearchEntries("");
  EXPECT_EQ(results.size(), 5u);
}

TEST_F(ReadingListServiceTest, SearchEntries_NoMatchReturnsEmpty) {
  AddTestEntries(3);
  auto results = service_->SearchEntries("zzznonexistentzzz");
  EXPECT_TRUE(results.empty());
}

TEST_F(ReadingListServiceTest, SearchEntries_SpecialCharacters) {
  service_->AddEntry(GURL("https://example.com/test"), "Article with (special) chars!");

  // Search with special chars should not crash.
  auto results = service_->SearchEntries("(special)");
  EXPECT_EQ(results.size(), 1u);
}

TEST_F(ReadingListServiceTest, SearchEntries_SubstringMatch) {
  service_->AddEntry(GURL("https://example.com/a"), "Programming in C++");
  service_->AddEntry(GURL("https://example.com/b"), "Java Programming");
  service_->AddEntry(GURL("https://example.com/c"), "Python Tutorial");

  auto results = service_->SearchEntries("Programming");
  EXPECT_EQ(results.size(), 2u);
}

TEST_F(ReadingListServiceTest, SearchEntriesByTag_FindsByTag) {
  AddTestEntries(3);
  service_->AddEntryTag(TestUrl(1), "tech");
  service_->AddEntryTag(TestUrl(2), "news");
  service_->AddEntryTag(TestUrl(3), "tech");

  auto results = service_->SearchEntriesByTag("tech");
  EXPECT_EQ(results.size(), 2u);
}

TEST_F(ReadingListServiceTest, SearchEntriesByTag_EmptyTagReturnsEmpty) {
  AddTestEntries(3);
  auto results = service_->SearchEntriesByTag("");
  EXPECT_TRUE(results.empty());
}

TEST_F(ReadingListServiceTest, SearchEntriesByTag_NonexistentTagReturnsEmpty) {
  AddTestEntries(3);
  service_->AddEntryTag(TestUrl(1), "tech");

  auto results = service_->SearchEntriesByTag("sports");
  EXPECT_TRUE(results.empty());
}

TEST_F(ReadingListServiceTest, GetEntriesByStatus_FiltersByStatus) {
  AddTestEntries(5);
  service_->MarkEntryRead(TestUrl(1));
  service_->MarkEntryRead(TestUrl(2));
  service_->MarkEntryUnread(TestUrl(3));
  // 4 and 5 remain unseen.

  auto read_entries = service_->GetEntriesByStatus(AstraReadingListStatus::kRead);
  EXPECT_EQ(read_entries.size(), 2u);

  auto unread_entries = service_->GetEntriesByStatus(AstraReadingListStatus::kUnread);
  EXPECT_EQ(unread_entries.size(), 1u);

  auto unseen_entries = service_->GetEntriesByStatus(AstraReadingListStatus::kUnseen);
  EXPECT_EQ(unseen_entries.size(), 2u);
}

// =========================================================================
// Settings
// =========================================================================

TEST_F(ReadingListServiceTest, Setting_DefaultSortOrder_DefaultValue) {
  EXPECT_EQ(service_->default_sort_order(),
            AstraReadingListSortOrder::kByDateAdded);
}

TEST_F(ReadingListServiceTest, Setting_DefaultSortOrder_SetAndGet) {
  service_->set_default_sort_order(AstraReadingListSortOrder::kByTitle);
  EXPECT_EQ(service_->default_sort_order(),
            AstraReadingListSortOrder::kByTitle);

  service_->set_default_sort_order(AstraReadingListSortOrder::kByDateRead);
  EXPECT_EQ(service_->default_sort_order(),
            AstraReadingListSortOrder::kByDateRead);
}

TEST_F(ReadingListServiceTest, Setting_DefaultSortOrder_PersistsInPrefs) {
  service_->set_default_sort_order(AstraReadingListSortOrder::kByTitle);
  EXPECT_EQ(profile_->GetPrefs()->GetString(
      AstraReadingListService::kPrefDefaultSortOrder), "title");
}

TEST_F(ReadingListServiceTest, Setting_AutoMarkReadOnScroll_DefaultValue) {
  EXPECT_TRUE(service_->auto_mark_read_on_scroll());
}

TEST_F(ReadingListServiceTest, Setting_AutoMarkReadOnScroll_SetAndGet) {
  service_->set_auto_mark_read_on_scroll(false);
  EXPECT_FALSE(service_->auto_mark_read_on_scroll());

  service_->set_auto_mark_read_on_scroll(true);
  EXPECT_TRUE(service_->auto_mark_read_on_scroll());
}

TEST_F(ReadingListServiceTest, Setting_AutoDeleteReadAfterDays_DefaultValue) {
  EXPECT_EQ(service_->auto_delete_read_after_days(), 0);
}

TEST_F(ReadingListServiceTest, Setting_AutoDeleteReadAfterDays_SetAndGet) {
  service_->set_auto_delete_read_after_days(30);
  EXPECT_EQ(service_->auto_delete_read_after_days(), 30);

  service_->set_auto_delete_read_after_days(7);
  EXPECT_EQ(service_->auto_delete_read_after_days(), 7);
}

TEST_F(ReadingListServiceTest, Setting_ShowEstimatedReadTime_DefaultValue) {
  EXPECT_TRUE(service_->show_estimated_read_time());
}

TEST_F(ReadingListServiceTest, Setting_ShowEstimatedReadTime_SetAndGet) {
  service_->set_show_estimated_read_time(false);
  EXPECT_FALSE(service_->show_estimated_read_time());
}

TEST_F(ReadingListServiceTest, Setting_ShowThumbnail_DefaultValue) {
  EXPECT_TRUE(service_->show_thumbnail());
}

TEST_F(ReadingListServiceTest, Setting_ShowThumbnail_SetAndGet) {
  service_->set_show_thumbnail(false);
  EXPECT_FALSE(service_->show_thumbnail());
}

TEST_F(ReadingListServiceTest, Setting_ReaderFontSize_DefaultValue) {
  EXPECT_EQ(service_->reader_font_size(), AstraReadingListFontSize::kMedium);
}

TEST_F(ReadingListServiceTest, Setting_ReaderFontSize_SetAndGet) {
  service_->set_reader_font_size(AstraReadingListFontSize::kLarge);
  EXPECT_EQ(service_->reader_font_size(), AstraReadingListFontSize::kLarge);

  service_->set_reader_font_size(AstraReadingListFontSize::kExtraLarge);
  EXPECT_EQ(service_->reader_font_size(), AstraReadingListFontSize::kExtraLarge);
}

TEST_F(ReadingListServiceTest, Setting_ReaderTheme_DefaultValue) {
  EXPECT_EQ(service_->reader_theme(), AstraReadingListTheme::kSystem);
}

TEST_F(ReadingListServiceTest, Setting_ReaderTheme_SetAndGet) {
  service_->set_reader_theme(AstraReadingListTheme::kDark);
  EXPECT_EQ(service_->reader_theme(), AstraReadingListTheme::kDark);

  service_->set_reader_theme(AstraReadingListTheme::kSepia);
  EXPECT_EQ(service_->reader_theme(), AstraReadingListTheme::kSepia);
}

TEST_F(ReadingListServiceTest, Setting_ReaderLineHeight_DefaultValue) {
  EXPECT_DOUBLE_EQ(service_->reader_line_height(), 1.5);
}

TEST_F(ReadingListServiceTest, Setting_ReaderLineHeight_SetAndGet) {
  service_->set_reader_line_height(1.8);
  EXPECT_DOUBLE_EQ(service_->reader_line_height(), 1.8);

  service_->set_reader_line_height(1.2);
  EXPECT_DOUBLE_EQ(service_->reader_line_height(), 1.2);
}

TEST_F(ReadingListServiceTest, Setting_TextToSpeechEnabled_DefaultValue) {
  EXPECT_FALSE(service_->text_to_speech_enabled());
}

TEST_F(ReadingListServiceTest, Setting_TextToSpeechEnabled_SetAndGet) {
  service_->set_text_to_speech_enabled(true);
  EXPECT_TRUE(service_->text_to_speech_enabled());
}

TEST_F(ReadingListServiceTest, Setting_AutoSyncReadingList_DefaultValue) {
  EXPECT_TRUE(service_->auto_sync_reading_list());
}

TEST_F(ReadingListServiceTest, Setting_AutoSyncReadingList_SetAndGet) {
  service_->set_auto_sync_reading_list(false);
  EXPECT_FALSE(service_->auto_sync_reading_list());
}

TEST_F(ReadingListServiceTest, Setting_SidebarDefaultView_DefaultValue) {
  EXPECT_EQ(service_->sidebar_default_view(), AstraReadingListView::kAll);
}

TEST_F(ReadingListServiceTest, Setting_SidebarDefaultView_SetAndGet) {
  service_->set_sidebar_default_view(AstraReadingListView::kUnread);
  EXPECT_EQ(service_->sidebar_default_view(), AstraReadingListView::kUnread);

  service_->set_sidebar_default_view(AstraReadingListView::kFavorites);
  EXPECT_EQ(service_->sidebar_default_view(), AstraReadingListView::kFavorites);

  service_->set_sidebar_default_view(AstraReadingListView::kFolders);
  EXPECT_EQ(service_->sidebar_default_view(), AstraReadingListView::kFolders);
}

TEST_F(ReadingListServiceTest, Setting_MaxSidebarItemCount_DefaultValue) {
  EXPECT_EQ(service_->max_sidebar_item_count(), 50);
}

TEST_F(ReadingListServiceTest, Setting_MaxSidebarItemCount_SetAndGet) {
  service_->set_max_sidebar_item_count(100);
  EXPECT_EQ(service_->max_sidebar_item_count(), 100);

  service_->set_max_sidebar_item_count(0);
  EXPECT_EQ(service_->max_sidebar_item_count(), 0);
}

TEST_F(ReadingListServiceTest, Settings_PersistAcrossServiceRecreation) {
  // Set several settings.
  service_->set_default_sort_order(AstraReadingListSortOrder::kByTitle);
  service_->set_auto_mark_read_on_scroll(false);
  service_->set_reader_font_size(AstraReadingListFontSize::kLarge);
  service_->set_reader_theme(AstraReadingListTheme::kDark);
  service_->set_sidebar_default_view(AstraReadingListView::kUnread);
  service_->set_max_sidebar_item_count(25);

  // Create a new service instance.
  auto service2 = std::make_unique<AstraReadingListService>(profile_.get());

  EXPECT_EQ(service2->default_sort_order(),
            AstraReadingListSortOrder::kByTitle);
  EXPECT_FALSE(service2->auto_mark_read_on_scroll());
  EXPECT_EQ(service2->reader_font_size(), AstraReadingListFontSize::kLarge);
  EXPECT_EQ(service2->reader_theme(), AstraReadingListTheme::kDark);
  EXPECT_EQ(service2->sidebar_default_view(), AstraReadingListView::kUnread);
  EXPECT_EQ(service2->max_sidebar_item_count(), 25);
}

// =========================================================================
// Observer notifications
// =========================================================================

TEST_F(ReadingListServiceTest, Observer_AddEntryFiresAdded) {
  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  GURL url = AddTestEntry(1);

  EXPECT_EQ(observer.added_count_, 1);
  EXPECT_EQ(observer.last_added_url_, url);
  EXPECT_EQ(observer.last_added_service_, service_.get());

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_RemoveEntryFiresRemoved) {
  GURL url = AddTestEntry(1);

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->RemoveEntry(url);

  EXPECT_EQ(observer.removed_count_, 1);
  EXPECT_EQ(observer.last_removed_url_, url);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_MarkReadFiresChangedAndStatus) {
  GURL url = AddTestEntry(1);

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->MarkEntryRead(url);

  EXPECT_EQ(observer.changed_count_, 1);
  EXPECT_EQ(observer.last_changed_url_, url);
  EXPECT_EQ(observer.status_changed_count_, 1);
  EXPECT_EQ(observer.last_status_changed_url_, url);
  EXPECT_TRUE(observer.last_status_read_);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_MarkUnreadFiresChangedAndStatus) {
  GURL url = AddTestEntry(1);
  service_->MarkEntryRead(url);

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->MarkEntryUnread(url);

  EXPECT_EQ(observer.changed_count_, 1);
  EXPECT_EQ(observer.status_changed_count_, 1);
  EXPECT_FALSE(observer.last_status_read_);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_UpdateTitleFiresChanged) {
  GURL url = AddTestEntry(1);

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->UpdateEntryTitle(url, "New Title");

  EXPECT_EQ(observer.changed_count_, 1);
  EXPECT_EQ(observer.last_changed_url_, url);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_SetFavoriteFiresChanged) {
  GURL url = AddTestEntry(1);

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->SetEntryFavorite(url, true);

  EXPECT_EQ(observer.changed_count_, 1);
  EXPECT_EQ(observer.last_changed_url_, url);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_SetNoteFiresChanged) {
  GURL url = AddTestEntry(1);

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->SetEntryNote(url, "My note");

  EXPECT_EQ(observer.changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_AddTagFiresChanged) {
  GURL url = AddTestEntry(1);

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->AddEntryTag(url, "tech");

  EXPECT_EQ(observer.changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_RemoveTagFiresChanged) {
  GURL url = AddTestEntry(1);
  service_->AddEntryTag(url, "tech");

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->RemoveEntryTag(url, "tech");

  EXPECT_EQ(observer.changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_MoveEntryToFolderFiresChanged) {
  GURL url = AddTestEntry(1);
  std::string folder_id = service_->CreateFolder("Tech");

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->MoveEntryToFolder(url, folder_id);

  EXPECT_EQ(observer.changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_CreateFolderFiresCreated) {
  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  std::string folder_id = service_->CreateFolder("New Folder");

  EXPECT_EQ(observer.folder_created_count_, 1);
  EXPECT_EQ(observer.last_folder_created_id_, folder_id);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_DeleteFolderFiresDeleted) {
  std::string folder_id = service_->CreateFolder("To Delete");

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->DeleteFolder(folder_id);

  EXPECT_EQ(observer.folder_deleted_count_, 1);
  EXPECT_EQ(observer.last_folder_deleted_id_, folder_id);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_MarkAllReadFiresListChanged) {
  AddTestEntries(3);

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->MarkAllRead();

  EXPECT_GE(observer.list_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_DeleteReadFiresListChanged) {
  AddTestEntries(3);
  service_->MarkEntryRead(TestUrl(1));

  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->DeleteRead();

  EXPECT_GE(observer.list_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(ReadingListServiceTest, Observer_ShutdownFiresShutdown) {
  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  EXPECT_EQ(observer.shutdown_count_, 1);
  EXPECT_EQ(observer.last_shutdown_service_, service_.get());

  // Don't call RemoveObserver — it was already cleared during shutdown.
}

TEST_F(ReadingListServiceTest, Observer_MultipleObserversAllNotified) {
  TestReadingListObserver obs1, obs2, obs3;
  service_->AddObserver(&obs1);
  service_->AddObserver(&obs2);
  service_->AddObserver(&obs3);

  AddTestEntry(1);

  EXPECT_EQ(obs1.added_count_, 1);
  EXPECT_EQ(obs2.added_count_, 1);
  EXPECT_EQ(obs3.added_count_, 1);

  service_->RemoveObserver(&obs1);
  service_->RemoveObserver(&obs2);
  service_->RemoveObserver(&obs3);
}

TEST_F(ReadingListServiceTest, Observer_RemoveObserverStopsNotifications) {
  TestReadingListObserver observer;
  service_->AddObserver(&observer);
  service_->RemoveObserver(&observer);

  AddTestEntry(1);

  EXPECT_EQ(observer.added_count_, 0);
}

// =========================================================================
// Sort order
// =========================================================================

TEST_F(ReadingListServiceTest, SortOrder_DateAdded_NewestFirst) {
  service_->set_default_sort_order(AstraReadingListSortOrder::kByDateAdded);

  for (int i = 1; i <= 5; ++i) {
    AddTestEntry(i);
    task_environment_.FastForwardBy(base::Milliseconds(10));
  }

  auto entries = service_->GetAllEntries();
  ASSERT_EQ(entries.size(), 5u);
  // Newest first: 5, 4, 3, 2, 1
  EXPECT_TRUE(entries[0].title.find("5") != std::string::npos);
  EXPECT_TRUE(entries[4].title.find("1") != std::string::npos);
}

TEST_F(ReadingListServiceTest, SortOrder_ByTitle_Alphabetical) {
  service_->set_default_sort_order(AstraReadingListSortOrder::kByTitle);

  service_->AddEntry(GURL("https://example.com/c"), "Charlie");
  service_->AddEntry(GURL("https://example.com/a"), "Alpha");
  service_->AddEntry(GURL("https://example.com/b"), "Bravo");

  auto entries = service_->GetAllEntries();
  ASSERT_EQ(entries.size(), 3u);
  EXPECT_EQ(entries[0].title, "Alpha");
  EXPECT_EQ(entries[1].title, "Bravo");
  EXPECT_EQ(entries[2].title, "Charlie");
}

// =========================================================================
// Edge cases
// =========================================================================

TEST_F(ReadingListServiceTest, EdgeCase_EmptyListAllQueriesSafe) {
  // All query methods should handle empty list without crashing.
  EXPECT_EQ(service_->GetEntryCount(), 0u);
  EXPECT_EQ(service_->GetUnreadCount(), 0u);
  EXPECT_EQ(service_->GetReadCount(), 0u);
  EXPECT_TRUE(service_->GetAllEntries().empty());
  EXPECT_TRUE(service_->GetUnreadEntries().empty());
  EXPECT_TRUE(service_->GetReadEntries().empty());
  EXPECT_TRUE(service_->GetRecentlyAddedEntries(10).empty());
  EXPECT_TRUE(service_->GetRecentlyReadEntries(10).empty());
  EXPECT_TRUE(service_->GetFavoriteEntries().empty());
  EXPECT_TRUE(service_->GetAllTags().empty());
  EXPECT_TRUE(service_->GetAllFolders().empty());
  EXPECT_TRUE(service_->SearchEntries("anything").empty());
  EXPECT_TRUE(service_->SearchEntriesByTag("tag").empty());
  EXPECT_TRUE(service_->GetEntriesByStatus(
      AstraReadingListStatus::kUnread).empty());
  EXPECT_EQ(service_->GetEntryByUrl(GURL("https://example.com/")), nullptr);
  EXPECT_FALSE(service_->HasEntry(GURL("https://example.com/")));
}

TEST_F(ReadingListServiceTest, EdgeCase_EmptyListAllMutationsSafe) {
  // All mutation methods should handle empty list without crashing.
  EXPECT_FALSE(service_->RemoveEntry(TestUrl(1)));
  EXPECT_FALSE(service_->MarkEntryRead(TestUrl(1)));
  EXPECT_FALSE(service_->MarkEntryUnread(TestUrl(1)));
  EXPECT_FALSE(service_->UpdateEntryTitle(TestUrl(1), "new"));
  EXPECT_EQ(service_->MarkAllRead(), 0u);
  EXPECT_EQ(service_->DeleteRead(), 0u);
  EXPECT_FALSE(service_->SetEntryFavorite(TestUrl(1), true));
  EXPECT_FALSE(service_->SetEntryNote(TestUrl(1), "note"));
  EXPECT_FALSE(service_->AddEntryTag(TestUrl(1), "tag"));
  EXPECT_FALSE(service_->RemoveEntryTag(TestUrl(1), "tag"));
  EXPECT_FALSE(service_->SetEntryFolder(TestUrl(1), "folder"));
}

TEST_F(ReadingListServiceTest, EdgeCase_InvalidUrlsAllMethods) {
  GURL invalid_url;

  EXPECT_FALSE(service_->AddEntry(invalid_url, "title"));
  EXPECT_FALSE(service_->HasEntry(invalid_url));
  EXPECT_EQ(service_->GetEntryByUrl(invalid_url), nullptr);
  EXPECT_FALSE(service_->RemoveEntry(invalid_url));
  EXPECT_FALSE(service_->MarkEntryRead(invalid_url));
  EXPECT_FALSE(service_->MarkEntryUnread(invalid_url));
  EXPECT_FALSE(service_->UpdateEntryTitle(invalid_url, "new"));
  EXPECT_FALSE(service_->SetEntryFavorite(invalid_url, true));
  EXPECT_FALSE(service_->IsEntryFavorite(invalid_url));
  EXPECT_FALSE(service_->SetEntryNote(invalid_url, "note"));
  EXPECT_TRUE(service_->GetEntryNote(invalid_url).empty());
  EXPECT_FALSE(service_->AddEntryTag(invalid_url, "tag"));
  EXPECT_FALSE(service_->RemoveEntryTag(invalid_url, "tag"));
  EXPECT_TRUE(service_->GetEntryTags(invalid_url).empty());
  EXPECT_FALSE(service_->SetEntryFolder(invalid_url, "folder"));
  EXPECT_TRUE(service_->GetEntryFolder(invalid_url).empty());
}

TEST_F(ReadingListServiceTest, EdgeCase_LargeNumberOfEntries) {
  const int kNumEntries = 1000;
  AddTestEntries(kNumEntries);
  EXPECT_EQ(service_->GetEntryCount(), static_cast<size_t>(kNumEntries));
  EXPECT_EQ(service_->GetAllEntries().size(),
            static_cast<size_t>(kNumEntries));
}

TEST_F(ReadingListServiceTest, EdgeCase_ManyFolders) {
  const int kNumFolders = 100;
  for (int i = 0; i < kNumFolders; ++i) {
    service_->CreateFolder("Folder " + base::NumberToString(i));
  }
  EXPECT_EQ(service_->GetFolderCount(), static_cast<size_t>(kNumFolders));
  EXPECT_EQ(service_->GetAllFolders().size(),
            static_cast<size_t>(kNumFolders));
}

TEST_F(ReadingListServiceTest, EdgeCase_MoveToNonexistentFolderFails) {
  GURL url = AddTestEntry(1);
  EXPECT_FALSE(service_->MoveEntryToFolder(url, "nonexistent-folder"));
  // Entry should still be uncategorized.
  EXPECT_TRUE(service_->GetEntryFolder(url).empty());
}

TEST_F(ReadingListServiceTest, EdgeCase_DeleteFolderRemovesFromEntries) {
  std::string folder_id = service_->CreateFolder("Temp");
  AddTestEntries(3);
  service_->MoveEntryToFolder(TestUrl(1), folder_id);
  service_->MoveEntryToFolder(TestUrl(2), folder_id);

  EXPECT_TRUE(service_->DeleteFolder(folder_id));

  // All entries should be uncategorized.
  EXPECT_TRUE(service_->GetEntryFolder(TestUrl(1)).empty());
  EXPECT_TRUE(service_->GetEntryFolder(TestUrl(2)).empty());
  EXPECT_TRUE(service_->GetEntryFolder(TestUrl(3)).empty());
}

TEST_F(ReadingListServiceTest, EdgeCase_SearchSpecialCharsNoCrash) {
  service_->AddEntry(GURL("https://example.com/a"), "Test [brackets] (parens) {braces}");

  auto results = service_->SearchEntries("[");
  // Should not crash and should find the entry.
  EXPECT_EQ(results.size(), 1u);
}

TEST_F(ReadingListServiceTest, EdgeCase_TagsWithSpaces) {
  GURL url = AddTestEntry(1);
  EXPECT_TRUE(service_->AddEntryTag(url, "machine learning"));

  auto tags = service_->GetEntryTags(url);
  ASSERT_EQ(tags.size(), 1u);
  EXPECT_EQ(tags[0], "machine learning");
}

TEST_F(ReadingListServiceTest, EdgeCase_DuplicateAddIsNoOp) {
  GURL url = TestUrl(1);
  EXPECT_TRUE(service_->AddEntry(url, "First"));
  EXPECT_FALSE(service_->AddEntry(url, "Second"));

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->title, "First");  // First title should be preserved.
}

TEST_F(ReadingListServiceTest, EdgeCase_RemoveThenAddAgain) {
  GURL url = AddTestEntry(1);
  service_->SetEntryFavorite(url, true);
  service_->AddEntryTag(url, "tag1");

  EXPECT_TRUE(service_->RemoveEntry(url));
  EXPECT_TRUE(service_->AddEntry(url, "New Title"));

  // Metadata should be gone after removal.
  EXPECT_FALSE(service_->IsEntryFavorite(url));
  EXPECT_TRUE(service_->GetEntryTags(url).empty());

  const AstraReadingListEntry* entry = service_->GetEntryByUrl(url);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->title, "New Title");
}

// =========================================================================
// Persistence
// =========================================================================

TEST_F(ReadingListServiceTest, Persistence_EntriesPersistAcrossRecreation) {
  AddTestEntries(5);
  service_->MarkEntryRead(TestUrl(2));
  service_->MarkEntryRead(TestUrl(4));

  // Create a new service instance with the same profile.
  auto service2 = std::make_unique<AstraReadingListService>(profile_.get());

  EXPECT_EQ(service2->GetEntryCount(), 5u);
  EXPECT_EQ(service2->GetReadCount(), 2u);
  EXPECT_EQ(service2->GetUnreadCount(), 3u);
}

TEST_F(ReadingListServiceTest, Persistence_FoldersPersistAcrossRecreation) {
  std::string id1 = service_->CreateFolder("Folder A");
  std::string id2 = service_->CreateFolder("Folder B");

  auto service2 = std::make_unique<AstraReadingListService>(profile_.get());

  EXPECT_EQ(service2->GetFolderCount(), 2u);
  EXPECT_NE(service2->GetFolder(id1), nullptr);
  EXPECT_NE(service2->GetFolder(id2), nullptr);
}

TEST_F(ReadingListServiceTest, Persistence_MetadataPersistsAcrossRecreation) {
  GURL url = AddTestEntry(1);
  service_->SetEntryFavorite(url, true);
  service_->SetEntryNote(url, "My note");
  service_->AddEntryTag(url, "tag1");
  service_->AddEntryTag(url, "tag2");

  std::string folder_id = service_->CreateFolder("Tech");
  service_->MoveEntryToFolder(url, folder_id);

  auto service2 = std::make_unique<AstraReadingListService>(profile_.get());

  EXPECT_TRUE(service2->IsEntryFavorite(url));
  EXPECT_EQ(service2->GetEntryNote(url), "My note");

  auto tags = service2->GetEntryTags(url);
  EXPECT_EQ(tags.size(), 2u);

  EXPECT_EQ(service2->GetEntryFolder(url), folder_id);
}

// =========================================================================
// Observer interface completeness / default implementations
// =========================================================================

TEST_F(ReadingListServiceTest, Observer_DefaultImplementationsExist) {
  // Compile-time check: an observer with no overrides should work.
  class EmptyObserver : public AstraReadingListObserver {};
  EmptyObserver observer;

  service_->AddObserver(&observer);
  AddTestEntry(1);  // Should not crash even with empty observer.
  service_->RemoveObserver(&observer);

  SUCCEED();
}

TEST_F(ReadingListServiceTest, Observer_PartialOverrideWorks) {
  // Observer that only overrides one method.
  class PartialObserver : public AstraReadingListObserver {
   public:
    void OnReadingListEntryAdded(AstraReadingListService* service,
                                 const GURL& url) override {
      added_count_++;
      last_url_ = url;
    }
    int added_count_ = 0;
    GURL last_url_;
  };

  PartialObserver observer;
  service_->AddObserver(&observer);

  GURL url = AddTestEntry(1);
  EXPECT_EQ(observer.added_count_, 1);
  EXPECT_EQ(observer.last_url_, url);

  // Other operations should not crash even though observer only overrides one.
  service_->UpdateEntryTitle(url, "New Title");
  service_->MarkEntryRead(url);
  service_->RemoveEntry(url);

  service_->RemoveObserver(&observer);
  SUCCEED();
}

// =========================================================================
// AstraReadingListEntry struct
// =========================================================================

TEST_F(ReadingListServiceTest, EntryStruct_DefaultValues) {
  AstraReadingListEntry entry;

  EXPECT_TRUE(entry.url.is_empty());
  EXPECT_TRUE(entry.title.empty());
  EXPECT_EQ(entry.status, AstraReadingListStatus::kUnseen);
  EXPECT_TRUE(entry.added_time.is_null());
  EXPECT_TRUE(entry.update_time.is_null());
  EXPECT_TRUE(entry.estimated_read_time.is_zero());
  EXPECT_DOUBLE_EQ(entry.score, -1.0);
  EXPECT_TRUE(entry.thumbnail_url.is_empty());
  EXPECT_FALSE(entry.has_distilled);
  EXPECT_EQ(entry.distill_state, AstraReadingListDistillState::kUnknown);
  EXPECT_EQ(entry.word_count, -1);
  EXPECT_TRUE(entry.first_read_time.is_null());
  EXPECT_TRUE(entry.last_read_time.is_null());
  EXPECT_EQ(entry.read_count, 0);
  EXPECT_FALSE(entry.is_favorite);
  EXPECT_TRUE(entry.tags.empty());
  EXPECT_TRUE(entry.note.empty());
  EXPECT_TRUE(entry.folder_id.empty());
}

TEST_F(ReadingListServiceTest, EntryStruct_CanSetAllFields) {
  AstraReadingListEntry entry;
  entry.url = GURL("https://example.com/article");
  entry.title = "My Article";
  entry.status = AstraReadingListStatus::kRead;
  entry.estimated_read_time = base::Minutes(5);
  entry.score = 0.85;
  entry.thumbnail_url = GURL("https://example.com/thumb.jpg");
  entry.has_distilled = true;
  entry.distill_state = AstraReadingListDistillState::kYes;
  entry.word_count = 1200;
  entry.read_count = 3;
  entry.is_favorite = true;
  entry.tags = {"tech", "news"};
  entry.note = "Great read!";
  entry.folder_id = "folder-123";

  EXPECT_EQ(entry.url, GURL("https://example.com/article"));
  EXPECT_EQ(entry.title, "My Article");
  EXPECT_EQ(entry.status, AstraReadingListStatus::kRead);
  EXPECT_EQ(entry.estimated_read_time, base::Minutes(5));
  EXPECT_DOUBLE_EQ(entry.score, 0.85);
  EXPECT_EQ(entry.thumbnail_url, GURL("https://example.com/thumb.jpg"));
  EXPECT_TRUE(entry.has_distilled);
  EXPECT_EQ(entry.distill_state, AstraReadingListDistillState::kYes);
  EXPECT_EQ(entry.word_count, 1200);
  EXPECT_EQ(entry.read_count, 3);
  EXPECT_TRUE(entry.is_favorite);
  ASSERT_EQ(entry.tags.size(), 2u);
  EXPECT_EQ(entry.tags[0], "tech");
  EXPECT_EQ(entry.tags[1], "news");
  EXPECT_EQ(entry.note, "Great read!");
  EXPECT_EQ(entry.folder_id, "folder-123");
}

// =========================================================================
// AstraReadingListFolder struct
// =========================================================================

TEST_F(ReadingListServiceTest, FolderStruct_DefaultValues) {
  AstraReadingListFolder folder;

  EXPECT_TRUE(folder.folder_id.empty());
  EXPECT_TRUE(folder.name.empty());
  EXPECT_FALSE(folder.is_default);
  EXPECT_EQ(folder.entry_count, 0);
  EXPECT_EQ(folder.unread_count, 0);
  EXPECT_TRUE(folder.created_time.is_null());
  EXPECT_EQ(folder.order_index, 0);
}

TEST_F(ReadingListServiceTest, FolderStruct_CanSetAllFields) {
  AstraReadingListFolder folder;
  folder.folder_id = "folder-abc";
  folder.name = "My Folder";
  folder.is_default = true;
  folder.entry_count = 42;
  folder.unread_count = 17;
  folder.order_index = 3;

  EXPECT_EQ(folder.folder_id, "folder-abc");
  EXPECT_EQ(folder.name, "My Folder");
  EXPECT_TRUE(folder.is_default);
  EXPECT_EQ(folder.entry_count, 42);
  EXPECT_EQ(folder.unread_count, 17);
  EXPECT_EQ(folder.order_index, 3);
}

// =========================================================================
// Enums
// =========================================================================

TEST_F(ReadingListServiceTest, Enums_StatusHasThreeValues) {
  AstraReadingListStatus unread = AstraReadingListStatus::kUnread;
  AstraReadingListStatus read = AstraReadingListStatus::kRead;
  AstraReadingListStatus unseen = AstraReadingListStatus::kUnseen;

  EXPECT_NE(static_cast<int>(unread), static_cast<int>(read));
  EXPECT_NE(static_cast<int>(unread), static_cast<int>(unseen));
  EXPECT_NE(static_cast<int>(read), static_cast<int>(unseen));
}

TEST_F(ReadingListServiceTest, Enums_DistillStateHasFiveValues) {
  auto yes = AstraReadingListDistillState::kYes;
  auto no = AstraReadingListDistillState::kNo;
  auto unknown = AstraReadingListDistillState::kUnknown;
  auto distilling = AstraReadingListDistillState::kDistilling;
  auto error = AstraReadingListDistillState::kDistillError;

  EXPECT_NE(static_cast<int>(yes), static_cast<int>(no));
  EXPECT_NE(static_cast<int>(yes), static_cast<int>(unknown));
  EXPECT_NE(static_cast<int>(yes), static_cast<int>(distilling));
  EXPECT_NE(static_cast<int>(yes), static_cast<int>(error));
}

TEST_F(ReadingListServiceTest, Enums_SortOrderHasFourValues) {
  auto a = AstraReadingListSortOrder::kByDateAdded;
  auto b = AstraReadingListSortOrder::kByDateRead;
  auto c = AstraReadingListSortOrder::kByTitle;
  auto d = AstraReadingListSortOrder::kByEstimatedReadTime;

  EXPECT_NE(static_cast<int>(a), static_cast<int>(b));
  EXPECT_NE(static_cast<int>(a), static_cast<int>(c));
  EXPECT_NE(static_cast<int>(a), static_cast<int>(d));
}

TEST_F(ReadingListServiceTest, Enums_FontSizeHasFourValues) {
  auto small = AstraReadingListFontSize::kSmall;
  auto medium = AstraReadingListFontSize::kMedium;
  auto large = AstraReadingListFontSize::kLarge;
  auto xl = AstraReadingListFontSize::kExtraLarge;

  EXPECT_NE(static_cast<int>(small), static_cast<int>(medium));
  EXPECT_NE(static_cast<int>(small), static_cast<int>(large));
  EXPECT_NE(static_cast<int>(small), static_cast<int>(xl));
}

TEST_F(ReadingListServiceTest, Enums_ThemeHasFourValues) {
  auto light = AstraReadingListTheme::kLight;
  auto dark = AstraReadingListTheme::kDark;
  auto sepia = AstraReadingListTheme::kSepia;
  auto system = AstraReadingListTheme::kSystem;

  EXPECT_NE(static_cast<int>(light), static_cast<int>(dark));
  EXPECT_NE(static_cast<int>(light), static_cast<int>(sepia));
  EXPECT_NE(static_cast<int>(light), static_cast<int>(system));
}

TEST_F(ReadingListServiceTest, Enums_ViewHasFourValues) {
  auto all = AstraReadingListView::kAll;
  auto unread = AstraReadingListView::kUnread;
  auto favorites = AstraReadingListView::kFavorites;
  auto folders = AstraReadingListView::kFolders;

  EXPECT_NE(static_cast<int>(all), static_cast<int>(unread));
  EXPECT_NE(static_cast<int>(all), static_cast<int>(favorites));
  EXPECT_NE(static_cast<int>(all), static_cast<int>(folders));
}

// =========================================================================
// Shutdown
// =========================================================================

TEST_F(ReadingListServiceTest, Shutdown_ClearsObservers) {
  TestReadingListObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  EXPECT_EQ(observer.shutdown_count_, 1);
  EXPECT_EQ(service_->model(), nullptr);
}

TEST_F(ReadingListServiceTest, Shutdown_Idempotent) {
  service_->Shutdown();
  service_->Shutdown();
  SUCCEED() << "Double shutdown completed without crash.";
}

TEST_F(ReadingListServiceTest, Shutdown_NoCrashWithNoObservers) {
  service_->Shutdown();
  SUCCEED();
}

// =========================================================================
// Static pref key constants
// =========================================================================

TEST_F(ReadingListServiceTest, PrefKeyConstants_Defined) {
  // Verify all pref key constants are defined and non-empty.
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefReadingListEntries).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefReadingListFolders).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefReadingListEntryMetadata).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefDefaultSortOrder).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefAutoMarkReadOnScroll).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefAutoDeleteReadAfterDays).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefShowEstimatedReadTime).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefShowThumbnail).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefReaderFontSize).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefReaderTheme).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefReaderLineHeight).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefTextToSpeechEnabled).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefAutoSyncReadingList).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefSidebarDefaultView).empty());
  EXPECT_FALSE(
      std::string(AstraReadingListService::kPrefMaxSidebarItemCount).empty());
}

TEST_F(ReadingListServiceTest, PrefKeyConstants_AllStartWithAstraReadingList) {
  // All pref keys should follow the "astra.reading_list." prefix convention.
  std::vector<const char*> keys = {
      AstraReadingListService::kPrefReadingListEntries,
      AstraReadingListService::kPrefReadingListFolders,
      AstraReadingListService::kPrefReadingListEntryMetadata,
      AstraReadingListService::kPrefDefaultSortOrder,
      AstraReadingListService::kPrefAutoMarkReadOnScroll,
      AstraReadingListService::kPrefAutoDeleteReadAfterDays,
      AstraReadingListService::kPrefShowEstimatedReadTime,
      AstraReadingListService::kPrefShowThumbnail,
      AstraReadingListService::kPrefReaderFontSize,
      AstraReadingListService::kPrefReaderTheme,
      AstraReadingListService::kPrefReaderLineHeight,
      AstraReadingListService::kPrefTextToSpeechEnabled,
      AstraReadingListService::kPrefAutoSyncReadingList,
      AstraReadingListService::kPrefSidebarDefaultView,
      AstraReadingListService::kPrefMaxSidebarItemCount,
  };

  for (const char* key : keys) {
    std::string key_str(key);
    EXPECT_EQ(key_str.find("astra.reading_list."), 0u)
        << "Pref key doesn't start with 'astra.reading_list.': " << key_str;
  }
}

// =========================================================================
// Factory
// =========================================================================

TEST_F(ReadingListServiceTest, Factory_GetInstance) {
  auto* factory = AstraReadingListServiceFactory::GetInstance();
  EXPECT_NE(factory, nullptr);
}

TEST_F(ReadingListServiceTest, Factory_GetForProfile) {
  auto* service = AstraReadingListServiceFactory::GetForProfile(profile_.get());
  EXPECT_NE(service, nullptr);
}

TEST_F(ReadingListServiceTest, Factory_GetForProfileNullProfileReturnsNull) {
  auto* service = AstraReadingListServiceFactory::GetForProfile(nullptr);
  EXPECT_EQ(service, nullptr);
}

}  // namespace astra
