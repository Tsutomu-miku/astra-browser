// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_reading_list_service.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

namespace astra {

namespace {

// Dictionary keys for entry serialization in prefs.
constexpr char kEntryUrlKey[] = "url";
constexpr char kEntryTitleKey[] = "title";
constexpr char kEntryStatusKey[] = "status";
constexpr char kEntryAddedTimeKey[] = "added_time";
constexpr char kEntryUpdateTimeKey[] = "update_time";
constexpr char kEntryEstimatedReadTimeKey[] = "estimated_read_time";
constexpr char kEntryScoreKey[] = "score";
constexpr char kEntryThumbnailUrlKey[] = "thumbnail_url";
constexpr char kEntryHasDistilledKey[] = "has_distilled";
constexpr char kEntryDistillStateKey[] = "distill_state";
constexpr char kEntryWordCountKey[] = "word_count";
constexpr char kEntryFirstReadTimeKey[] = "first_read_time";
constexpr char kEntryLastReadTimeKey[] = "last_read_time";
constexpr char kEntryReadCountKey[] = "read_count";

// Dictionary keys for folder serialization in prefs.
constexpr char kFolderIdKey[] = "folder_id";
constexpr char kFolderNameKey[] = "name";
constexpr char kFolderIsDefaultKey[] = "is_default";
constexpr char kFolderEntryCountKey[] = "entry_count";
constexpr char kFolderUnreadCountKey[] = "unread_count";
constexpr char kFolderCreatedTimeKey[] = "created_time";
constexpr char kFolderOrderIndexKey[] = "order_index";

// Dictionary keys for entry metadata in prefs.
constexpr char kMetadataFavoriteKey[] = "favorite";
constexpr char kMetadataTagsKey[] = "tags";
constexpr char kMetadataNoteKey[] = "note";
constexpr char kMetadataFolderIdKey[] = "folder_id";

// Status string constants.
constexpr char kStatusUnread[] = "unread";
constexpr char kStatusRead[] = "read";
constexpr char kStatusUnseen[] = "unseen";

// Distill state string constants.
constexpr char kDistillYes[] = "yes";
constexpr char kDistillNo[] = "no";
constexpr char kDistillUnknown[] = "unknown";
constexpr char kDistillDistilling[] = "distilling";
constexpr char kDistillError[] = "distill_error";

// Convert status enum to string.
std::string StatusToString(AstraReadingListStatus status) {
  switch (status) {
    case AstraReadingListStatus::kUnread:
      return kStatusUnread;
    case AstraReadingListStatus::kRead:
      return kStatusRead;
    case AstraReadingListStatus::kUnseen:
      return kStatusUnseen;
  }
  return kStatusUnseen;
}

// Parse status string to enum.
AstraReadingListStatus StringToStatus(const std::string& value) {
  if (value == kStatusRead) {
    return AstraReadingListStatus::kRead;
  }
  if (value == kStatusUnread) {
    return AstraReadingListStatus::kUnread;
  }
  return AstraReadingListStatus::kUnseen;
}

// Convert distill state enum to string.
std::string DistillStateToString(AstraReadingListDistillState state) {
  switch (state) {
    case AstraReadingListDistillState::kYes:
      return kDistillYes;
    case AstraReadingListDistillState::kNo:
      return kDistillNo;
    case AstraReadingListDistillState::kUnknown:
      return kDistillUnknown;
    case AstraReadingListDistillState::kDistilling:
      return kDistillDistilling;
    case AstraReadingListDistillState::kDistillError:
      return kDistillError;
  }
  return kDistillUnknown;
}

// Parse distill state string to enum.
AstraReadingListDistillState StringToDistillState(const std::string& value) {
  if (value == kDistillYes) {
    return AstraReadingListDistillState::kYes;
  }
  if (value == kDistillNo) {
    return AstraReadingListDistillState::kNo;
  }
  if (value == kDistillDistilling) {
    return AstraReadingListDistillState::kDistilling;
  }
  if (value == kDistillError) {
    return AstraReadingListDistillState::kDistillError;
  }
  return AstraReadingListDistillState::kUnknown;
}

// Serialize time to double for pref storage (Windows epoch microseconds).
double TimeToDouble(base::Time time) {
  return time.ToDeltaSinceWindowsEpoch().InMicrosecondsF();
}

// Deserialize time from double.
base::Time DoubleToTime(double value) {
  return base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(static_cast<int64_t>(value)));
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraReadingListService::AstraReadingListService(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
  prefs_ = profile_->GetPrefs();
  DCHECK(prefs_);

  // TODO(astra): Obtain ReadingListModel from the profile's keyed service
  // factory.  The standard Chromium factory is ReadingListModelFactory in
  // chrome/browser/reading_list/reading_list_model_factory.h.
  //
  // Chromium owner: ReadingListModelFactory
  // (chrome/browser/reading_list/reading_list_model_factory.h)

  LoadEntriesFromPrefs();
  LoadFoldersFromPrefs();
  LoadMetadataFromPrefs();
  LoadSettingsFromPrefs();

  // Initial folder count computation based on loaded entries and metadata.
  RecomputeFolderCounts();
}

AstraReadingListService::~AstraReadingListService() = default;

void AstraReadingListService::Shutdown() {
  // Notify observers of shutdown first.
  NotifyServiceShutdown();

  // Clear all observer references before the profile goes away.
  observers_.Clear();
  model_ = nullptr;
  profile_ = nullptr;
  prefs_ = nullptr;
  entries_.clear();
  folders_.clear();
  entry_metadata_ = base::Value::Dict();
}

// =========================================================================
// Observers
// =========================================================================

void AstraReadingListService::AddObserver(AstraReadingListObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraReadingListService::RemoveObserver(AstraReadingListObserver* observer) {
  observers_.RemoveObserver(observer);
}

void AstraReadingListService::AddServiceObserver(
    AstraReadingListServiceObserver* observer) {
  service_observers_.AddObserver(observer);
}

void AstraReadingListService::RemoveServiceObserver(
    AstraReadingListServiceObserver* observer) {
  service_observers_.RemoveObserver(observer);
}

// =========================================================================
// Entry queries
// =========================================================================

size_t AstraReadingListService::GetEntryCount() const {
  return entries_.size();
}

size_t AstraReadingListService::GetUnreadCount() const {
  size_t count = 0;
  for (const auto& entry : entries_) {
    if (entry.status != AstraReadingListStatus::kRead) {
      count++;
    }
  }
  return count;
}

size_t AstraReadingListService::GetReadCount() const {
  size_t count = 0;
  for (const auto& entry : entries_) {
    if (entry.status == AstraReadingListStatus::kRead) {
      count++;
    }
  }
  return count;
}

const AstraReadingListEntry* AstraReadingListService::GetEntryByUrl(
    const GURL& url) const {
  if (!url.is_valid()) {
    return nullptr;
  }
  auto it = base::ranges::find(entries_, url, &AstraReadingListEntry::url);
  if (it == entries_.end()) {
    return nullptr;
  }
  // Cache for pointer stability.
  cached_entry_ = *it;
  return &cached_entry_;
}

bool AstraReadingListService::HasEntry(const GURL& url) const {
  if (!url.is_valid()) {
    return false;
  }
  return base::ranges::find(entries_, url, &AstraReadingListEntry::url) !=
         entries_.end();
}

std::vector<AstraReadingListEntry> AstraReadingListService::GetUnreadEntries(
    int max_count) const {
  std::vector<AstraReadingListEntry> entries;
  for (const auto& entry : entries_) {
    if (entry.status != AstraReadingListStatus::kRead) {
      entries.push_back(entry);
    }
  }
  ApplyDefaultSort(entries);
  TruncateEntries(entries, max_count);
  return entries;
}

std::vector<AstraReadingListEntry> AstraReadingListService::GetReadEntries(
    int max_count) const {
  std::vector<AstraReadingListEntry> entries;
  for (const auto& entry : entries_) {
    if (entry.status == AstraReadingListStatus::kRead) {
      entries.push_back(entry);
    }
  }
  ApplyDefaultSort(entries);
  TruncateEntries(entries, max_count);
  return entries;
}

std::vector<AstraReadingListEntry> AstraReadingListService::GetAllEntries()
    const {
  std::vector<AstraReadingListEntry> entries = entries_;
  ApplyDefaultSort(entries);
  return entries;
}

std::vector<AstraReadingListEntry>
AstraReadingListService::GetRecentlyAddedEntries(int max_count) const {
  std::vector<AstraReadingListEntry> entries = entries_;
  std::sort(entries.begin(), entries.end(),
            [](const AstraReadingListEntry& a,
               const AstraReadingListEntry& b) {
              return a.added_time > b.added_time;
            });
  TruncateEntries(entries, max_count);
  return entries;
}

std::vector<AstraReadingListEntry>
AstraReadingListService::GetRecentlyReadEntries(int max_count) const {
  std::vector<AstraReadingListEntry> entries;
  for (const auto& entry : entries_) {
    if (!entry.last_read_time.is_null()) {
      entries.push_back(entry);
    }
  }
  std::sort(entries.begin(), entries.end(),
            [](const AstraReadingListEntry& a,
               const AstraReadingListEntry& b) {
              return a.last_read_time > b.last_read_time;
            });
  TruncateEntries(entries, max_count);
  return entries;
}

// =========================================================================
// Entry operations
// =========================================================================

bool AstraReadingListService::AddEntry(const GURL& url,
                                       const std::string& title) {
  if (!url.is_valid()) {
    return false;
  }

  // Check for duplicate.
  if (HasEntry(url)) {
    return false;
  }

  AstraReadingListEntry entry;
  entry.url = url;
  entry.title = title.empty() ? url.spec() : title;
  entry.status = AstraReadingListStatus::kUnseen;
  entry.added_time = base::Time::Now();
  entry.update_time = entry.added_time;
  entry.score = -1.0;
  entry.has_distilled = false;
  entry.distill_state = AstraReadingListDistillState::kUnknown;
  entry.word_count = -1;
  entry.read_count = 0;

  entries_.push_back(entry);
  SaveEntriesToPrefs();

  // New entry means folder counts may need updating (for "uncategorized").
  RecomputeFolderCounts();

  NotifyEntryAdded(url);
  return true;
}

bool AstraReadingListService::RemoveEntry(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }

  auto it = base::ranges::find(entries_, url, &AstraReadingListEntry::url);
  if (it == entries_.end()) {
    return false;
  }

  entries_.erase(it);

  // Remove associated metadata.
  entry_metadata_.Remove(url.spec());

  SaveEntriesToPrefs();
  SaveMetadataToPrefs();
  RecomputeFolderCounts();

  NotifyEntryRemoved(url);
  return true;
}

bool AstraReadingListService::MarkEntryRead(const GURL& url) {
  AstraReadingListEntry* entry = FindEntry(url);
  if (!entry) {
    return false;
  }

  if (entry->status == AstraReadingListStatus::kRead) {
    return false;
  }

  bool was_read = (entry->status == AstraReadingListStatus::kRead);
  entry->status = AstraReadingListStatus::kRead;

  base::Time now = base::Time::Now();
  if (entry->first_read_time.is_null()) {
    entry->first_read_time = now;
  }
  entry->last_read_time = now;
  entry->read_count++;
  entry->update_time = now;

  SaveEntriesToPrefs();
  RecomputeFolderCounts();

  NotifyEntryChanged(url);
  NotifyEntryStatusChanged(url, true);

  return true;
}

bool AstraReadingListService::MarkEntryUnread(const GURL& url) {
  AstraReadingListEntry* entry = FindEntry(url);
  if (!entry) {
    return false;
  }

  if (entry->status == AstraReadingListStatus::kUnread) {
    return false;
  }

  entry->status = AstraReadingListStatus::kUnread;
  entry->update_time = base::Time::Now();

  SaveEntriesToPrefs();
  RecomputeFolderCounts();

  NotifyEntryChanged(url);
  NotifyEntryStatusChanged(url, false);

  return true;
}

bool AstraReadingListService::UpdateEntryTitle(const GURL& url,
                                               const std::string& title) {
  AstraReadingListEntry* entry = FindEntry(url);
  if (!entry) {
    return false;
  }

  if (entry->title == title) {
    return false;
  }

  entry->title = title;
  entry->update_time = base::Time::Now();
  SaveEntriesToPrefs();
  NotifyEntryChanged(url);
  return true;
}

size_t AstraReadingListService::MarkAllRead() {
  size_t count = 0;
  base::Time now = base::Time::Now();

  for (auto& entry : entries_) {
    if (entry.status != AstraReadingListStatus::kRead) {
      entry.status = AstraReadingListStatus::kRead;
      entry.update_time = now;
      if (entry.first_read_time.is_null()) {
        entry.first_read_time = now;
      }
      entry.last_read_time = now;
      entry.read_count++;
      count++;
    }
  }

  if (count > 0) {
    SaveEntriesToPrefs();
    RecomputeFolderCounts();
    NotifyReadingListChanged();
  }

  return count;
}

size_t AstraReadingListService::DeleteRead() {
  size_t original_size = entries_.size();

  // Collect URLs of entries to remove (for notification and metadata cleanup).
  std::vector<std::string> urls_to_remove;
  for (const auto& entry : entries_) {
    if (entry.status == AstraReadingListStatus::kRead) {
      urls_to_remove.push_back(entry.url.spec());
    }
  }

  std::erase_if(entries_, [](const AstraReadingListEntry& entry) {
    return entry.status == AstraReadingListStatus::kRead;
  });

  size_t count = original_size - entries_.size();

  if (count > 0) {
    // Remove metadata for deleted entries.
    for (const auto& url_spec : urls_to_remove) {
      entry_metadata_.Remove(url_spec);
    }
    SaveEntriesToPrefs();
    SaveMetadataToPrefs();
    RecomputeFolderCounts();
    NotifyReadingListChanged();
  }

  return count;
}

// =========================================================================
// Astra metadata
// =========================================================================

bool AstraReadingListService::SetEntryFavorite(const GURL& url, bool favorite) {
  if (!url.is_valid() || !HasEntry(url)) {
    return false;
  }

  base::Value::Dict* metadata = GetOrCreateMetadataForUrl(url);
  DCHECK(metadata);

  absl::optional<bool> current = metadata->FindBool(kMetadataFavoriteKey);
  if (current.has_value() && *current == favorite) {
    return false;
  }

  metadata->Set(kMetadataFavoriteKey, favorite);
  SaveMetadataToPrefs();
  NotifyEntryChanged(url);
  return true;
}

bool AstraReadingListService::IsEntryFavorite(const GURL& url) const {
  if (!url.is_valid()) {
    return false;
  }
  const base::Value::Dict* metadata = GetMetadataForUrl(url);
  if (!metadata) {
    return false;
  }
  absl::optional<bool> favorite = metadata->FindBool(kMetadataFavoriteKey);
  return favorite.value_or(false);
}

std::vector<AstraReadingListEntry>
AstraReadingListService::GetFavoriteEntries() const {
  std::vector<AstraReadingListEntry> results;
  for (const auto& entry : entries_) {
    if (IsEntryFavorite(entry.url)) {
      results.push_back(entry);
    }
  }
  ApplyDefaultSort(results);
  return results;
}

bool AstraReadingListService::SetEntryNote(const GURL& url,
                                           const std::string& note) {
  if (!url.is_valid() || !HasEntry(url)) {
    return false;
  }

  base::Value::Dict* metadata = GetOrCreateMetadataForUrl(url);
  DCHECK(metadata);

  const std::string* current = metadata->FindString(kMetadataNoteKey);
  if (current && *current == note) {
    return false;
  }

  metadata->Set(kMetadataNoteKey, note);
  SaveMetadataToPrefs();
  NotifyEntryChanged(url);
  return true;
}

std::string AstraReadingListService::GetEntryNote(const GURL& url) const {
  if (!url.is_valid()) {
    return std::string();
  }
  const base::Value::Dict* metadata = GetMetadataForUrl(url);
  if (!metadata) {
    return std::string();
  }
  const std::string* note = metadata->FindString(kMetadataNoteKey);
  return note ? *note : std::string();
}

bool AstraReadingListService::AddEntryTag(const GURL& url,
                                          const std::string& tag) {
  if (!url.is_valid() || !HasEntry(url) || tag.empty()) {
    return false;
  }

  base::Value::Dict* metadata = GetOrCreateMetadataForUrl(url);
  DCHECK(metadata);

  base::Value::List* tags_list = metadata->FindList(kMetadataTagsKey);
  if (!tags_list) {
    tags_list = metadata->Set(kMetadataTagsKey, base::Value::List())->GetIfList();
    DCHECK(tags_list);
  }

  // Check for duplicate.
  for (const auto& tag_val : *tags_list) {
    if (tag_val.is_string() && tag_val.GetString() == tag) {
      return false;
    }
  }

  tags_list->Append(tag);
  SaveMetadataToPrefs();
  NotifyEntryChanged(url);
  return true;
}

bool AstraReadingListService::RemoveEntryTag(const GURL& url,
                                             const std::string& tag) {
  if (!url.is_valid() || !HasEntry(url) || tag.empty()) {
    return false;
  }

  base::Value::Dict* metadata = GetOrCreateMetadataForUrl(url);
  DCHECK(metadata);

  base::Value::List* tags_list = metadata->FindList(kMetadataTagsKey);
  if (!tags_list) {
    return false;
  }

  bool found = false;
  for (auto it = tags_list->begin(); it != tags_list->end(); ++it) {
    if (it->is_string() && it->GetString() == tag) {
      tags_list->erase(it);
      found = true;
      break;
    }
  }

  if (found) {
    SaveMetadataToPrefs();
    NotifyEntryChanged(url);
  }

  return found;
}

std::vector<std::string> AstraReadingListService::GetEntryTags(
    const GURL& url) const {
  std::vector<std::string> result;
  if (!url.is_valid()) {
    return result;
  }
  const base::Value::Dict* metadata = GetMetadataForUrl(url);
  if (!metadata) {
    return result;
  }
  const base::Value::List* tags_list = metadata->FindList(kMetadataTagsKey);
  if (!tags_list) {
    return result;
  }
  for (const auto& tag_val : *tags_list) {
    if (tag_val.is_string()) {
      result.push_back(tag_val.GetString());
    }
  }
  return result;
}

std::vector<std::string> AstraReadingListService::GetAllTags() const {
  std::set<std::string> all_tags;
  for (const auto [url_spec, metadata_val] : entry_metadata_) {
    if (!metadata_val.is_dict()) {
      continue;
    }
    const base::Value::List* tags_list =
        metadata_val.GetDict().FindList(kMetadataTagsKey);
    if (!tags_list) {
      continue;
    }
    for (const auto& tag_val : *tags_list) {
      if (tag_val.is_string()) {
        all_tags.insert(tag_val.GetString());
      }
    }
  }
  return std::vector<std::string>(all_tags.begin(), all_tags.end());
}

bool AstraReadingListService::SetEntryFolder(const GURL& url,
                                             const std::string& folder_id) {
  if (!url.is_valid() || !HasEntry(url)) {
    return false;
  }

  // If folder_id is non-empty, verify the folder exists.
  if (!folder_id.empty() && !FindFolder(folder_id)) {
    return false;
  }

  std::string current_folder = GetEntryFolder(url);
  if (current_folder == folder_id) {
    return false;
  }

  base::Value::Dict* metadata = GetOrCreateMetadataForUrl(url);
  DCHECK(metadata);

  if (folder_id.empty()) {
    metadata->Remove(kMetadataFolderIdKey);
  } else {
    metadata->Set(kMetadataFolderIdKey, folder_id);
  }

  SaveMetadataToPrefs();
  RecomputeFolderCounts();
  NotifyEntryChanged(url);
  return true;
}

std::string AstraReadingListService::GetEntryFolder(const GURL& url) const {
  if (!url.is_valid()) {
    return std::string();
  }
  const base::Value::Dict* metadata = GetMetadataForUrl(url);
  if (!metadata) {
    return std::string();
  }
  const std::string* folder_id = metadata->FindString(kMetadataFolderIdKey);
  return folder_id ? *folder_id : std::string();
}

// =========================================================================
// Folders
// =========================================================================

std::string AstraReadingListService::CreateFolder(const std::string& name) {
  if (name.empty()) {
    return std::string();
  }

  std::string folder_id = GenerateFolderId();

  AstraReadingListFolder folder;
  folder.folder_id = folder_id;
  folder.name = name;
  folder.is_default = false;
  folder.entry_count = 0;
  folder.unread_count = 0;
  folder.created_time = base::Time::Now();
  folder.order_index = static_cast<int>(folders_.size());

  folders_.push_back(std::move(folder));
  SaveFoldersToPrefs();
  NotifyFolderCreated(folder_id);
  return folder_id;
}

bool AstraReadingListService::DeleteFolder(const std::string& folder_id) {
  if (folder_id.empty()) {
    return false;
  }

  auto it = base::ranges::find(folders_, folder_id,
                               &AstraReadingListFolder::folder_id);
  if (it == folders_.end()) {
    return false;
  }

  if (it->is_default) {
    return false;
  }

  // Remove folder from all entries' metadata.
  for (auto it = entry_metadata_.begin();
       it != entry_metadata_.end();
       ++it) {
    if (!it->second.is_dict()) {
      continue;
    }
    base::Value::Dict& metadata_dict = it->second.GetDict();
    const std::string* current_folder =
        metadata_dict.FindString(kMetadataFolderIdKey);
    if (current_folder && *current_folder == folder_id) {
      metadata_dict.Remove(kMetadataFolderIdKey);
    }
  }

  folders_.erase(it);
  SaveFoldersToPrefs();
  SaveMetadataToPrefs();
  RecomputeFolderCounts();
  NotifyFolderDeleted(folder_id);
  return true;
}

bool AstraReadingListService::RenameFolder(const std::string& folder_id,
                                           const std::string& new_name) {
  if (folder_id.empty() || new_name.empty()) {
    return false;
  }

  AstraReadingListFolder* folder = FindFolder(folder_id);
  if (!folder) {
    return false;
  }

  if (folder->name == new_name) {
    return false;
  }

  folder->name = new_name;
  SaveFoldersToPrefs();
  NotifyReadingListChanged();
  return true;
}

const AstraReadingListFolder* AstraReadingListService::GetFolder(
    const std::string& folder_id) const {
  if (folder_id.empty()) {
    return nullptr;
  }
  auto it = base::ranges::find(folders_, folder_id,
                               &AstraReadingListFolder::folder_id);
  if (it == folders_.end()) {
    return nullptr;
  }
  cached_folder_ = *it;
  return &cached_folder_;
}

std::vector<AstraReadingListFolder>
AstraReadingListService::GetAllFolders() const {
  std::vector<AstraReadingListFolder> result = folders_;
  std::sort(result.begin(), result.end(),
            [](const AstraReadingListFolder& a,
               const AstraReadingListFolder& b) {
              return a.order_index < b.order_index;
            });
  return result;
}

size_t AstraReadingListService::GetFolderCount() const {
  return folders_.size();
}

std::vector<AstraReadingListEntry>
AstraReadingListService::GetEntriesInFolder(const std::string& folder_id) const {
  std::vector<AstraReadingListEntry> result;
  for (const auto& entry : entries_) {
    if (GetEntryFolder(entry.url) == folder_id) {
      result.push_back(entry);
    }
  }
  ApplyDefaultSort(result);
  return result;
}

bool AstraReadingListService::MoveEntryToFolder(const GURL& url,
                                                const std::string& folder_id) {
  return SetEntryFolder(url, folder_id);
}

bool AstraReadingListService::ReorderFolders(
    const std::vector<std::string>& folder_ids_order) {
  if (folder_ids_order.empty()) {
    return false;
  }

  // Build a map of current order indices.
  std::map<std::string, int> old_order;
  for (const auto& folder : folders_) {
    old_order[folder.folder_id] = folder.order_index;
  }

  // Assign new order indices.
  int index = 0;
  bool changed = false;

  // First, folders in the specified order.
  for (const auto& folder_id : folder_ids_order) {
    AstraReadingListFolder* folder = FindFolder(folder_id);
    if (!folder) {
      continue;
    }
    if (folder->order_index != index) {
      folder->order_index = index;
      changed = true;
    }
    index++;
  }

  // Then, remaining folders in their current relative order.
  std::vector<AstraReadingListFolder*> remaining;
  for (auto& folder : folders_) {
    if (base::ranges::find(folder_ids_order, folder.folder_id) ==
        folder_ids_order.end()) {
      remaining.push_back(&folder);
    }
  }
  std::sort(remaining.begin(), remaining.end(),
            [](const AstraReadingListFolder* a,
               const AstraReadingListFolder* b) {
              return a->order_index < b->order_index;
            });

  for (auto* folder : remaining) {
    if (folder->order_index != index) {
      folder->order_index = index;
      changed = true;
    }
    index++;
  }

  if (changed) {
    SaveFoldersToPrefs();
    NotifyReadingListChanged();
  }

  return changed;
}

// =========================================================================
// Search
// =========================================================================

std::vector<AstraReadingListEntry> AstraReadingListService::SearchEntries(
    const std::string& query) const {
  std::vector<AstraReadingListEntry> results;

  if (query.empty()) {
    results = entries_;
  } else {
    std::string lower_query = base::ToLowerASCII(query);

    for (const auto& entry : entries_) {
      std::string lower_title = base::ToLowerASCII(entry.title);
      std::string lower_url = base::ToLowerASCII(entry.url.spec());

      if (lower_title.find(lower_query) != std::string::npos ||
          lower_url.find(lower_query) != std::string::npos) {
        results.push_back(entry);
      }
    }
  }

  ApplyDefaultSort(results);
  return results;
}

std::vector<AstraReadingListEntry> AstraReadingListService::SearchEntriesByTag(
    const std::string& tag) const {
  std::vector<AstraReadingListEntry> results;
  if (tag.empty()) {
    return results;
  }

  for (const auto& entry : entries_) {
    std::vector<std::string> tags = GetEntryTags(entry.url);
    if (base::ranges::find(tags, tag) != tags.end()) {
      results.push_back(entry);
    }
  }

  ApplyDefaultSort(results);
  return results;
}

std::vector<AstraReadingListEntry>
AstraReadingListService::GetEntriesByStatus(
    AstraReadingListStatus status) const {
  std::vector<AstraReadingListEntry> results;
  for (const auto& entry : entries_) {
    if (entry.status == status) {
      results.push_back(entry);
    }
  }
  ApplyDefaultSort(results);
  return results;
}

// =========================================================================
// Settings
// =========================================================================

void AstraReadingListService::set_default_sort_order(
    AstraReadingListSortOrder order) {
  if (default_sort_order_ == order) {
    return;
  }
  default_sort_order_ = order;
  if (prefs_) {
    prefs_->SetString(kPrefDefaultSortOrder, SortOrderToString(order));
  }
  NotifyReadingListChanged();
}

void AstraReadingListService::set_auto_mark_read_on_scroll(bool enabled) {
  if (auto_mark_read_on_scroll_ == enabled) {
    return;
  }
  auto_mark_read_on_scroll_ = enabled;
  if (prefs_) {
    prefs_->SetBoolean(kPrefAutoMarkReadOnScroll, enabled);
  }
}

void AstraReadingListService::set_auto_delete_read_after_days(int days) {
  if (auto_delete_read_after_days_ == days) {
    return;
  }
  auto_delete_read_after_days_ = days;
  if (prefs_) {
    prefs_->SetInteger(kPrefAutoDeleteReadAfterDays, days);
  }
}

void AstraReadingListService::set_show_estimated_read_time(bool show) {
  if (show_estimated_read_time_ == show) {
    return;
  }
  show_estimated_read_time_ = show;
  if (prefs_) {
    prefs_->SetBoolean(kPrefShowEstimatedReadTime, show);
  }
}

void AstraReadingListService::set_show_thumbnail(bool show) {
  if (show_thumbnail_ == show) {
    return;
  }
  show_thumbnail_ = show;
  if (prefs_) {
    prefs_->SetBoolean(kPrefShowThumbnail, show);
  }
}

void AstraReadingListService::set_reader_font_size(
    AstraReadingListFontSize size) {
  if (reader_font_size_ == size) {
    return;
  }
  reader_font_size_ = size;
  if (prefs_) {
    prefs_->SetString(kPrefReaderFontSize, FontSizeToString(size));
  }
}

void AstraReadingListService::set_reader_theme(AstraReadingListTheme theme) {
  if (reader_theme_ == theme) {
    return;
  }
  reader_theme_ = theme;
  if (prefs_) {
    prefs_->SetString(kPrefReaderTheme, ThemeToString(theme));
  }
}

void AstraReadingListService::set_reader_line_height(double line_height) {
  if (reader_line_height_ == line_height) {
    return;
  }
  reader_line_height_ = line_height;
  if (prefs_) {
    prefs_->SetDouble(kPrefReaderLineHeight, line_height);
  }
}

void AstraReadingListService::set_text_to_speech_enabled(bool enabled) {
  if (text_to_speech_enabled_ == enabled) {
    return;
  }
  text_to_speech_enabled_ = enabled;
  if (prefs_) {
    prefs_->SetBoolean(kPrefTextToSpeechEnabled, enabled);
  }
}

void AstraReadingListService::set_auto_sync_reading_list(bool enabled) {
  if (auto_sync_reading_list_ == enabled) {
    return;
  }
  auto_sync_reading_list_ = enabled;
  if (prefs_) {
    prefs_->SetBoolean(kPrefAutoSyncReadingList, enabled);
  }
}

void AstraReadingListService::set_sidebar_default_view(
    AstraReadingListView view) {
  if (sidebar_default_view_ == view) {
    return;
  }
  sidebar_default_view_ = view;
  if (prefs_) {
    prefs_->SetString(kPrefSidebarDefaultView, ViewToString(view));
  }
}

void AstraReadingListService::set_max_sidebar_item_count(int count) {
  if (max_sidebar_item_count_ == count) {
    return;
  }
  max_sidebar_item_count_ = count;
  if (prefs_) {
    prefs_->SetInteger(kPrefMaxSidebarItemCount, count);
  }
}

// =========================================================================
// Model access
// =========================================================================

bool AstraReadingListService::IsModelLoaded() const {
  // For the Astra overlay implementation, entries are loaded synchronously
  // from PrefService in the constructor, so the model is always "loaded"
  // once the service exists.
  //
  // TODO(astra): When ReadingListModel is wired, use model_->loaded().
  // Chromium owner: ReadingListModel::loaded()
  return true;
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraReadingListService::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Entries list — stored as a list of dicts.
  // TODO(astra): This is overlay-skeleton storage.  In production,
  // reading list data is owned by Chromium's ReadingListModel.
  registry->RegisterListPref(kPrefReadingListEntries);

  // Folders list — stored as a list of dicts.
  registry->RegisterListPref(kPrefReadingListFolders);

  // Entry metadata map — keyed by URL spec.
  registry->RegisterDictionaryPref(kPrefReadingListEntryMetadata);

  // Default sort order.
  registry->RegisterStringPref(kPrefDefaultSortOrder, kDefaultDefaultSortOrder);

  // Auto-mark read on scroll.
  registry->RegisterBooleanPref(kPrefAutoMarkReadOnScroll,
                                kDefaultAutoMarkReadOnScroll);

  // Auto-delete read entries after days.
  registry->RegisterIntegerPref(kPrefAutoDeleteReadAfterDays,
                                kDefaultAutoDeleteReadAfterDays);

  // Show estimated read time.
  registry->RegisterBooleanPref(kPrefShowEstimatedReadTime,
                                kDefaultShowEstimatedReadTime);

  // Show thumbnail.
  registry->RegisterBooleanPref(kPrefShowThumbnail, kDefaultShowThumbnail);

  // Reader font size.
  registry->RegisterStringPref(kPrefReaderFontSize, kDefaultReaderFontSize);

  // Reader theme.
  registry->RegisterStringPref(kPrefReaderTheme, kDefaultReaderTheme);

  // Reader line height.
  registry->RegisterDoublePref(kPrefReaderLineHeight, kDefaultReaderLineHeight);

  // Text-to-speech enabled.
  registry->RegisterBooleanPref(kPrefTextToSpeechEnabled,
                                kDefaultTextToSpeechEnabled);

  // Auto-sync reading list.
  registry->RegisterBooleanPref(kPrefAutoSyncReadingList,
                                kDefaultAutoSyncReadingList);

  // Sidebar default view.
  registry->RegisterStringPref(kPrefSidebarDefaultView,
                               kDefaultSidebarDefaultView);

  // Max sidebar item count.
  registry->RegisterIntegerPref(kPrefMaxSidebarItemCount,
                                kDefaultMaxSidebarItemCount);
}

// =========================================================================
// Private helpers
// =========================================================================

AstraReadingListEntry* AstraReadingListService::FindEntry(const GURL& url) {
  auto it = base::ranges::find(entries_, url, &AstraReadingListEntry::url);
  return it == entries_.end() ? nullptr : &(*it);
}

void AstraReadingListService::ApplyDefaultSort(
    std::vector<AstraReadingListEntry>& entries) const {
  ApplySortWithOrder(entries, default_sort_order_);
}

// static
void AstraReadingListService::ApplySortWithOrder(
    std::vector<AstraReadingListEntry>& entries,
    AstraReadingListSortOrder order) {
  switch (order) {
    case AstraReadingListSortOrder::kByDateAdded:
      std::sort(entries.begin(), entries.end(),
                [](const AstraReadingListEntry& a,
                   const AstraReadingListEntry& b) {
                  return a.added_time > b.added_time;
                });
      break;

    case AstraReadingListSortOrder::kByDateRead:
      std::sort(entries.begin(), entries.end(),
                [](const AstraReadingListEntry& a,
                   const AstraReadingListEntry& b) {
                  // Entries with no last_read_time go to the end.
                  if (a.last_read_time.is_null() && b.last_read_time.is_null()) {
                    return a.added_time > b.added_time;
                  }
                  if (a.last_read_time.is_null()) {
                    return false;
                  }
                  if (b.last_read_time.is_null()) {
                    return true;
                  }
                  return a.last_read_time > b.last_read_time;
                });
      break;

    case AstraReadingListSortOrder::kByTitle:
      std::sort(entries.begin(), entries.end(),
                [](const AstraReadingListEntry& a,
                   const AstraReadingListEntry& b) {
                  return base::CompareCaseInsensitiveASCII(a.title, b.title) < 0;
                });
      break;

    case AstraReadingListSortOrder::kByEstimatedReadTime:
      std::sort(entries.begin(), entries.end(),
                [](const AstraReadingListEntry& a,
                   const AstraReadingListEntry& b) {
                  // Unknown times (-1 / zero) go to the end.
                  bool a_known = a.estimated_read_time.is_positive();
                  bool b_known = b.estimated_read_time.is_positive();
                  if (a_known && b_known) {
                    return a.estimated_read_time < b.estimated_read_time;
                  }
                  if (a_known) {
                    return true;
                  }
                  if (b_known) {
                    return false;
                  }
                  return a.added_time > b.added_time;
                });
      break;
  }
}

// static
void AstraReadingListService::TruncateEntries(
    std::vector<AstraReadingListEntry>& entries, int max_count) {
  if (max_count > 0 && static_cast<size_t>(max_count) < entries.size()) {
    entries.resize(static_cast<size_t>(max_count));
  }
}

// static
std::string AstraReadingListService::GenerateFolderId() {
  // Generate a short UUID-based folder ID.
  return base::Uuid::GenerateRandomV4().AsLowercaseString();
}

// =========================================================================
// Metadata helpers
// =========================================================================

const base::Value::Dict* AstraReadingListService::GetMetadataForUrl(
    const GURL& url) const {
  if (!url.is_valid()) {
    return nullptr;
  }
  const base::Value::Dict* metadata =
      entry_metadata_.FindDict(url.spec());
  return metadata;
}

base::Value::Dict* AstraReadingListService::GetOrCreateMetadataForUrl(
    const GURL& url) {
  DCHECK(url.is_valid());
  base::Value::Dict* metadata = entry_metadata_.FindDict(url.spec());
  if (!metadata) {
    metadata = entry_metadata_.Set(url.spec(), base::Value::Dict())->GetIfDict();
    DCHECK(metadata);
  }
  return metadata;
}

void AstraReadingListService::SaveMetadataToPrefs() {
  if (!prefs_) {
    return;
  }
  prefs_->SetDict(kPrefReadingListEntryMetadata,
                  entry_metadata_.Clone());
}

void AstraReadingListService::LoadMetadataFromPrefs() {
  if (!prefs_) {
    return;
  }
  const base::Value::Dict& dict =
      prefs_->GetDict(kPrefReadingListEntryMetadata);
  entry_metadata_ = dict.Clone();
}

// =========================================================================
// Folder helpers
// =========================================================================

AstraReadingListFolder* AstraReadingListService::FindFolder(
    const std::string& folder_id) {
  auto it = base::ranges::find(folders_, folder_id,
                               &AstraReadingListFolder::folder_id);
  return it == folders_.end() ? nullptr : &(*it);
}

void AstraReadingListService::RecomputeFolderCounts() {
  // Reset all folder counts.
  for (auto& folder : folders_) {
    folder.entry_count = 0;
    folder.unread_count = 0;
  }

  // Count entries per folder.
  for (const auto& entry : entries_) {
    std::string folder_id = GetEntryFolder(entry.url);
    if (folder_id.empty()) {
      continue;
    }
    AstraReadingListFolder* folder = FindFolder(folder_id);
    if (!folder) {
      continue;
    }
    folder->entry_count++;
    if (entry.status != AstraReadingListStatus::kRead) {
      folder->unread_count++;
    }
  }
}

void AstraReadingListService::SaveFoldersToPrefs() {
  if (!prefs_) {
    return;
  }

  base::Value::List list;
  for (const auto& folder : folders_) {
    list.Append(FolderToDict(folder));
  }
  prefs_->SetList(kPrefReadingListFolders, std::move(list));
}

void AstraReadingListService::LoadFoldersFromPrefs() {
  if (!prefs_) {
    return;
  }

  const base::Value::List& list = prefs_->GetList(kPrefReadingListFolders);
  folders_.clear();
  for (const auto& item : list) {
    if (!item.is_dict()) {
      continue;
    }
    AstraReadingListFolder folder;
    if (DictToFolder(item.GetDict(), folder)) {
      folders_.push_back(std::move(folder));
    }
  }
}

// =========================================================================
// Persistence
// =========================================================================

void AstraReadingListService::LoadEntriesFromPrefs() {
  if (!prefs_) {
    return;
  }

  const base::Value::List& list = prefs_->GetList(kPrefReadingListEntries);
  entries_.clear();
  for (const auto& item : list) {
    if (!item.is_dict()) {
      continue;
    }
    AstraReadingListEntry entry;
    if (DictToEntry(item.GetDict(), entry)) {
      entries_.push_back(std::move(entry));
    }
  }
}

void AstraReadingListService::SaveEntriesToPrefs() {
  if (!prefs_) {
    return;
  }

  base::Value::List list;
  for (const auto& entry : entries_) {
    list.Append(EntryToDict(entry));
  }
  prefs_->SetList(kPrefReadingListEntries, std::move(list));
}

void AstraReadingListService::LoadSettingsFromPrefs() {
  if (!prefs_) {
    return;
  }

  default_sort_order_ =
      StringToSortOrder(prefs_->GetString(kPrefDefaultSortOrder));
  auto_mark_read_on_scroll_ = prefs_->GetBoolean(kPrefAutoMarkReadOnScroll);
  auto_delete_read_after_days_ =
      prefs_->GetInteger(kPrefAutoDeleteReadAfterDays);
  show_estimated_read_time_ = prefs_->GetBoolean(kPrefShowEstimatedReadTime);
  show_thumbnail_ = prefs_->GetBoolean(kPrefShowThumbnail);
  reader_font_size_ = StringToFontSize(prefs_->GetString(kPrefReaderFontSize));
  reader_theme_ = StringToTheme(prefs_->GetString(kPrefReaderTheme));
  reader_line_height_ = prefs_->GetDouble(kPrefReaderLineHeight);
  text_to_speech_enabled_ = prefs_->GetBoolean(kPrefTextToSpeechEnabled);
  auto_sync_reading_list_ = prefs_->GetBoolean(kPrefAutoSyncReadingList);
  sidebar_default_view_ =
      StringToView(prefs_->GetString(kPrefSidebarDefaultView));
  max_sidebar_item_count_ = prefs_->GetInteger(kPrefMaxSidebarItemCount);
}

// static
base::Value::Dict AstraReadingListService::EntryToDict(
    const AstraReadingListEntry& entry) {
  base::Value::Dict dict;

  dict.Set(kEntryUrlKey, entry.url.spec());
  dict.Set(kEntryTitleKey, entry.title);
  dict.Set(kEntryStatusKey, StatusToString(entry.status));
  dict.Set(kEntryAddedTimeKey, TimeToDouble(entry.added_time));
  dict.Set(kEntryUpdateTimeKey, TimeToDouble(entry.update_time));
  dict.Set(kEntryEstimatedReadTimeKey,
           entry.estimated_read_time.InSecondsF());
  dict.Set(kEntryScoreKey, entry.score);
  dict.Set(kEntryThumbnailUrlKey, entry.thumbnail_url.spec());
  dict.Set(kEntryHasDistilledKey, entry.has_distilled);
  dict.Set(kEntryDistillStateKey, DistillStateToString(entry.distill_state));
  dict.Set(kEntryWordCountKey, entry.word_count);
  dict.Set(kEntryFirstReadTimeKey, TimeToDouble(entry.first_read_time));
  dict.Set(kEntryLastReadTimeKey, TimeToDouble(entry.last_read_time));
  dict.Set(kEntryReadCountKey, entry.read_count);

  return dict;
}

// static
bool AstraReadingListService::DictToEntry(const base::Value::Dict& dict,
                                          AstraReadingListEntry& entry) {
  const std::string* url_str = dict.FindString(kEntryUrlKey);
  if (!url_str) {
    return false;
  }

  GURL url(*url_str);
  if (!url.is_valid()) {
    return false;
  }

  entry.url = url;

  const std::string* title = dict.FindString(kEntryTitleKey);
  entry.title = title ? *title : entry.url.spec();

  const std::string* status_str = dict.FindString(kEntryStatusKey);
  entry.status =
      status_str ? StringToStatus(*status_str) : AstraReadingListStatus::kUnseen;

  absl::optional<double> added_time = dict.FindDouble(kEntryAddedTimeKey);
  if (added_time.has_value()) {
    entry.added_time = DoubleToTime(*added_time);
  }

  absl::optional<double> update_time = dict.FindDouble(kEntryUpdateTimeKey);
  if (update_time.has_value()) {
    entry.update_time = DoubleToTime(*update_time);
  }

  absl::optional<double> est_time = dict.FindDouble(kEntryEstimatedReadTimeKey);
  if (est_time.has_value()) {
    entry.estimated_read_time = base::Seconds(*est_time);
  }

  absl::optional<double> score = dict.FindDouble(kEntryScoreKey);
  entry.score = score.value_or(-1.0);

  const std::string* thumb_str = dict.FindString(kEntryThumbnailUrlKey);
  if (thumb_str) {
    entry.thumbnail_url = GURL(*thumb_str);
  }

  absl::optional<bool> has_distilled = dict.FindBool(kEntryHasDistilledKey);
  entry.has_distilled = has_distilled.value_or(false);

  const std::string* distill_str = dict.FindString(kEntryDistillStateKey);
  entry.distill_state =
      distill_str ? StringToDistillState(*distill_str)
                  : AstraReadingListDistillState::kUnknown;

  absl::optional<int> word_count = dict.FindInt(kEntryWordCountKey);
  entry.word_count = word_count.value_or(-1);

  absl::optional<double> first_read_time =
      dict.FindDouble(kEntryFirstReadTimeKey);
  if (first_read_time.has_value()) {
    entry.first_read_time = DoubleToTime(*first_read_time);
  }

  absl::optional<double> last_read_time =
      dict.FindDouble(kEntryLastReadTimeKey);
  if (last_read_time.has_value()) {
    entry.last_read_time = DoubleToTime(*last_read_time);
  }

  absl::optional<int> read_count = dict.FindInt(kEntryReadCountKey);
  entry.read_count = read_count.value_or(0);

  return true;
}

// static
base::Value::Dict AstraReadingListService::FolderToDict(
    const AstraReadingListFolder& folder) {
  base::Value::Dict dict;

  dict.Set(kFolderIdKey, folder.folder_id);
  dict.Set(kFolderNameKey, folder.name);
  dict.Set(kFolderIsDefaultKey, folder.is_default);
  dict.Set(kFolderEntryCountKey, folder.entry_count);
  dict.Set(kFolderUnreadCountKey, folder.unread_count);
  dict.Set(kFolderCreatedTimeKey, TimeToDouble(folder.created_time));
  dict.Set(kFolderOrderIndexKey, folder.order_index);

  return dict;
}

// static
bool AstraReadingListService::DictToFolder(const base::Value::Dict& dict,
                                           AstraReadingListFolder& folder) {
  const std::string* id = dict.FindString(kFolderIdKey);
  if (!id || id->empty()) {
    return false;
  }

  folder.folder_id = *id;

  const std::string* name = dict.FindString(kFolderNameKey);
  folder.name = name ? *name : std::string();

  absl::optional<bool> is_default = dict.FindBool(kFolderIsDefaultKey);
  folder.is_default = is_default.value_or(false);

  absl::optional<int> entry_count = dict.FindInt(kFolderEntryCountKey);
  folder.entry_count = entry_count.value_or(0);

  absl::optional<int> unread_count = dict.FindInt(kFolderUnreadCountKey);
  folder.unread_count = unread_count.value_or(0);

  absl::optional<double> created_time = dict.FindDouble(kFolderCreatedTimeKey);
  if (created_time.has_value()) {
    folder.created_time = DoubleToTime(*created_time);
  }

  absl::optional<int> order_index = dict.FindInt(kFolderOrderIndexKey);
  folder.order_index = order_index.value_or(0);

  return true;
}

// =========================================================================
// Settings conversion helpers
// =========================================================================

// static
std::string AstraReadingListService::SortOrderToString(
    AstraReadingListSortOrder order) {
  switch (order) {
    case AstraReadingListSortOrder::kByDateAdded:
      return "date_added";
    case AstraReadingListSortOrder::kByDateRead:
      return "date_read";
    case AstraReadingListSortOrder::kByTitle:
      return "title";
    case AstraReadingListSortOrder::kByEstimatedReadTime:
      return "estimated_read_time";
  }
  return "date_added";
}

// static
AstraReadingListSortOrder AstraReadingListService::StringToSortOrder(
    const std::string& value) {
  if (value == "date_read") {
    return AstraReadingListSortOrder::kByDateRead;
  }
  if (value == "title") {
    return AstraReadingListSortOrder::kByTitle;
  }
  if (value == "estimated_read_time") {
    return AstraReadingListSortOrder::kByEstimatedReadTime;
  }
  return AstraReadingListSortOrder::kByDateAdded;
}

// static
std::string AstraReadingListService::FontSizeToString(
    AstraReadingListFontSize size) {
  switch (size) {
    case AstraReadingListFontSize::kSmall:
      return "small";
    case AstraReadingListFontSize::kMedium:
      return "medium";
    case AstraReadingListFontSize::kLarge:
      return "large";
    case AstraReadingListFontSize::kExtraLarge:
      return "extra_large";
  }
  return "medium";
}

// static
AstraReadingListFontSize AstraReadingListService::StringToFontSize(
    const std::string& value) {
  if (value == "small") {
    return AstraReadingListFontSize::kSmall;
  }
  if (value == "large") {
    return AstraReadingListFontSize::kLarge;
  }
  if (value == "extra_large") {
    return AstraReadingListFontSize::kExtraLarge;
  }
  return AstraReadingListFontSize::kMedium;
}

// static
std::string AstraReadingListService::ThemeToString(
    AstraReadingListTheme theme) {
  switch (theme) {
    case AstraReadingListTheme::kLight:
      return "light";
    case AstraReadingListTheme::kDark:
      return "dark";
    case AstraReadingListTheme::kSepia:
      return "sepia";
    case AstraReadingListTheme::kSystem:
      return "system";
  }
  return "system";
}

// static
AstraReadingListTheme AstraReadingListService::StringToTheme(
    const std::string& value) {
  if (value == "light") {
    return AstraReadingListTheme::kLight;
  }
  if (value == "dark") {
    return AstraReadingListTheme::kDark;
  }
  if (value == "sepia") {
    return AstraReadingListTheme::kSepia;
  }
  return AstraReadingListTheme::kSystem;
}

// static
std::string AstraReadingListService::ViewToString(AstraReadingListView view) {
  switch (view) {
    case AstraReadingListView::kAll:
      return "all";
    case AstraReadingListView::kUnread:
      return "unread";
    case AstraReadingListView::kFavorites:
      return "favorites";
    case AstraReadingListView::kFolders:
      return "folders";
  }
  return "all";
}

// static
AstraReadingListView AstraReadingListService::StringToView(
    const std::string& value) {
  if (value == "unread") {
    return AstraReadingListView::kUnread;
  }
  if (value == "favorites") {
    return AstraReadingListView::kFavorites;
  }
  if (value == "folders") {
    return AstraReadingListView::kFolders;
  }
  return AstraReadingListView::kAll;
}

// =========================================================================
// Observer notification helpers
// =========================================================================

void AstraReadingListService::NotifyEntryAdded(const GURL& url) {
  for (auto& observer : observers_) {
    observer.OnReadingListEntryAdded(this, url);
  }
}

void AstraReadingListService::NotifyEntryRemoved(const GURL& url) {
  for (auto& observer : observers_) {
    observer.OnReadingListEntryRemoved(this, url);
  }
}

void AstraReadingListService::NotifyEntryChanged(const GURL& url) {
  for (auto& observer : observers_) {
    observer.OnReadingListEntryChanged(this, url);
  }
}

void AstraReadingListService::NotifyEntryStatusChanged(const GURL& url,
                                                       bool read) {
  for (auto& observer : observers_) {
    observer.OnReadingListEntryStatusChanged(this, url, read);
  }
}

void AstraReadingListService::NotifyFolderCreated(
    const std::string& folder_id) {
  for (auto& observer : observers_) {
    observer.OnReadingListFolderCreated(this, folder_id);
  }
}

void AstraReadingListService::NotifyFolderDeleted(
    const std::string& folder_id) {
  for (auto& observer : observers_) {
    observer.OnReadingListFolderDeleted(this, folder_id);
  }
}

void AstraReadingListService::NotifyReadingListChanged() {
  for (auto& observer : observers_) {
    observer.OnReadingListChanged(this);
  }
}

void AstraReadingListService::NotifyServiceShutdown() {
  for (auto& observer : observers_) {
    observer.OnReadingListServiceShutdown(this);
  }
}

// =========================================================================
// Service observer notification helpers
// =========================================================================

void AstraReadingListService::NotifyServiceEntryAdded(
    const AstraReadingListEntry& entry) {
  for (auto& observer : service_observers_) {
    observer.OnReadingListEntryAdded(entry);
  }
}

void AstraReadingListService::NotifyServiceEntryRemoved(const GURL& url) {
  for (auto& observer : service_observers_) {
    observer.OnReadingListEntryRemoved(url);
  }
}

void AstraReadingListService::NotifyServiceEntryUpdated(
    const AstraReadingListEntry& entry) {
  for (auto& observer : service_observers_) {
    observer.OnReadingListEntryUpdated(entry);
  }
}

void AstraReadingListService::NotifyServiceEntryStatusChanged(
    const GURL& url, bool is_read) {
  for (auto& observer : service_observers_) {
    observer.OnReadingListEntryStatusChanged(url, is_read);
  }
}

void AstraReadingListService::NotifyServiceModelLoaded() {
  for (auto& observer : service_observers_) {
    observer.OnReadingListModelLoaded();
  }
}

void AstraReadingListService::NotifyServiceReordered() {
  for (auto& observer : service_observers_) {
    observer.OnReadingListReordered();
  }
}

void AstraReadingListService::NotifyServiceReloaded() {
  for (auto& observer : service_observers_) {
    observer.OnReadingListReloaded();
  }
}

}  // namespace astra
