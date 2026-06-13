#include "astra/browser/astra_note_service.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <utility>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/unguessable_token.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace astra {

namespace {

// Dictionary keys for note serialization in prefs.
constexpr char kNoteIdKey[] = "id";
constexpr char kNoteTitleKey[] = "title";
constexpr char kNoteContentKey[] = "content";
constexpr char kNoteWorkspaceIdKey[] = "workspace_id";
constexpr char kNoteTabUrlKey[] = "tab_url";
constexpr char kNoteTabTitleKey[] = "tab_title";
constexpr char kNoteTagsKey[] = "tags";
constexpr char kNoteColorKey[] = "color";
constexpr char kNotePinnedKey[] = "is_pinned";
constexpr char kNoteFavoriteKey[] = "is_favorite";
constexpr char kNoteCreatedTimeKey[] = "created_time";
constexpr char kNoteModifiedTimeKey[] = "modified_time";
constexpr char kNoteLastAccessedKey[] = "last_accessed";
constexpr char kNoteWordCountKey[] = "word_count";
constexpr char kNoteSizeBytesKey[] = "size_bytes";

// Preset note color palette.
// Curated set of distinct, readable colors for note categorization.
const char* kNoteColorPalette[] = {
    "#FFD93D",  // Yellow (default)
    "#FF8A65",  // Orange
    "#F06292",  // Pink
    "#BA68C8",  // Purple
    "#7986CB",  // Indigo
    "#4FC3F7",  // Light blue
    "#4DB6AC",  // Teal
    "#81C784",  // Green
    "#AED581",  // Lime
    "#FFB74D",  // Amber
    "#90A4AE",  // Blue grey
    "#E0E0E0",  // Light grey
};

// Helper: count words in a string.
// A "word" is a sequence of non-whitespace characters.
int CountWords(const std::string& text) {
  if (text.empty()) {
    return 0;
  }

  int count = 0;
  bool in_word = false;
  for (char c : text) {
    if (base::IsAsciiWhitespace(c)) {
      in_word = false;
    } else if (!in_word) {
      in_word = true;
      count++;
    }
  }
  return count;
}

}  // namespace

// =========================================================================
// AstraNote struct methods
// =========================================================================

bool AstraNote::IsEmpty() const {
  return title.empty() && content.empty();
}

bool AstraNote::MatchesQuery(const std::string& query) const {
  if (query.empty()) {
    return true;
  }

  std::string lower_query = base::ToLowerASCII(query);

  // Search in title.
  std::string lower_title = base::ToLowerASCII(title);
  if (lower_title.find(lower_query) != std::string::npos) {
    return true;
  }

  // Search in content.
  std::string lower_content = base::ToLowerASCII(content);
  if (lower_content.find(lower_query) != std::string::npos) {
    return true;
  }

  // Search in tags.
  for (const auto& tag : tags) {
    std::string lower_tag = base::ToLowerASCII(tag);
    if (lower_tag.find(lower_query) != std::string::npos) {
      return true;
    }
  }

  return false;
}

// =========================================================================
// AstraNoteService — Construction / destruction
// =========================================================================

AstraNoteService::AstraNoteService(Profile* profile) : profile_(profile) {
  LoadFromPrefs();
}

AstraNoteService::~AstraNoteService() = default;

void AstraNoteService::Shutdown() {
  // Notify all observers of shutdown.
  for (auto& observer : observers_) {
    observer.OnNoteServiceShutdown(this);
  }

  // Clear all observer references before the profile goes away.
  observers_.Clear();
  service_observers_.Clear();
  profile_ = nullptr;
}

// =========================================================================
// Observers
// =========================================================================

void AstraNoteService::AddObserver(AstraNoteObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraNoteService::RemoveObserver(AstraNoteObserver* observer) {
  observers_.RemoveObserver(observer);
}

void AstraNoteService::AddServiceObserver(AstraNoteServiceObserver* observer) {
  service_observers_.AddObserver(observer);
}

void AstraNoteService::RemoveServiceObserver(AstraNoteServiceObserver* observer) {
  service_observers_.RemoveObserver(observer);
}

// =========================================================================
// Note CRUD
// =========================================================================

std::string AstraNoteService::CreateNote(const std::string& title,
                                          const std::string& content) {
  AstraNote note;
  note.id = GenerateNoteId();
  note.title = title;
  note.content = content;
  note.workspace_id = default_workspace();
  note.color = default_note_color();
  note.created_time = base::Time::Now();
  note.modified_time = note.created_time;
  note.last_accessed = note.created_time;
  ComputeNoteStats(&note);

  notes_[note.id] = std::move(note);

  const AstraNote& added = notes_[note.id];
  NotifyNoteCreated(note.id);
  NotifyServiceNoteAdded(added);

  SaveToPrefs();
  return note.id;
}

const AstraNote* AstraNoteService::GetNote(const std::string& note_id) const {
  auto it = notes_.find(note_id);
  if (it == notes_.end()) {
    return nullptr;
  }
  // Note: in a real implementation we might update last_accessed here,
  // but GetNote is const. A separate RecordNoteAccess() method could be
  // added if needed.
  return &it->second;
}

bool AstraNoteService::UpdateNoteTitle(const std::string& note_id,
                                        const std::string& title) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  if (note->title == title) {
    return true;  // No change.
  }

  note->title = title;
  note->modified_time = base::Time::Now();
  ComputeNoteStats(note);

  NotifyNoteChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

bool AstraNoteService::UpdateNoteContent(const std::string& note_id,
                                          const std::string& content) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  if (note->content == content) {
    return true;  // No change.
  }

  note->content = content;
  note->modified_time = base::Time::Now();
  ComputeNoteStats(note);

  NotifyNoteChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

bool AstraNoteService::DeleteNote(const std::string& note_id) {
  auto it = notes_.find(note_id);
  if (it == notes_.end()) {
    return false;
  }

  notes_.erase(it);

  NotifyNoteDeleted(note_id);
  NotifyServiceNoteRemoved(note_id);

  SaveToPrefs();
  return true;
}

bool AstraNoteService::DoesNoteExist(const std::string& note_id) const {
  return notes_.find(note_id) != notes_.end();
}

size_t AstraNoteService::GetNoteCount() const {
  return notes_.size();
}

// =========================================================================
// Note association
// =========================================================================

bool AstraNoteService::SetNoteWorkspace(const std::string& note_id,
                                         const std::string& workspace_id) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  if (note->workspace_id == workspace_id) {
    return true;  // No change.
  }

  note->workspace_id = workspace_id;
  note->modified_time = base::Time::Now();

  NotifyNoteChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

std::string AstraNoteService::GetNoteWorkspace(const std::string& note_id) const {
  const AstraNote* note = GetNote(note_id);
  return note ? note->workspace_id : std::string();
}

bool AstraNoteService::SetNoteTabUrl(const std::string& note_id,
                                      const GURL& url,
                                      const std::string& page_title) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  if (note->tab_url == url && note->tab_title == page_title) {
    return true;  // No change.
  }

  note->tab_url = url;
  note->tab_title = page_title;
  note->modified_time = base::Time::Now();

  NotifyNoteChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

GURL AstraNoteService::GetNoteTabUrl(const std::string& note_id) const {
  const AstraNote* note = GetNote(note_id);
  return note ? note->tab_url : GURL();
}

bool AstraNoteService::ClearNoteTab(const std::string& note_id) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  if (note->tab_url.is_empty() && note->tab_title.empty()) {
    return true;  // Already cleared.
  }

  note->tab_url = GURL();
  note->tab_title.clear();
  note->modified_time = base::Time::Now();

  NotifyNoteChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

// =========================================================================
// Tags
// =========================================================================

bool AstraNoteService::AddNoteTag(const std::string& note_id,
                                   const std::string& tag) {
  AstraNote* note = FindNote(note_id);
  if (!note || tag.empty()) {
    return false;
  }

  // Check if tag already exists.
  auto it = base::ranges::find(note->tags, tag);
  if (it != note->tags.end()) {
    return false;  // Already present.
  }

  note->tags.push_back(tag);
  std::sort(note->tags.begin(), note->tags.end());
  note->modified_time = base::Time::Now();

  NotifyNoteChanged(note_id);
  NotifyNoteTagsChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

bool AstraNoteService::RemoveNoteTag(const std::string& note_id,
                                      const std::string& tag) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  auto it = base::ranges::find(note->tags, tag);
  if (it == note->tags.end()) {
    return false;  // Not found.
  }

  note->tags.erase(it);
  note->modified_time = base::Time::Now();

  NotifyNoteChanged(note_id);
  NotifyNoteTagsChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

bool AstraNoteService::SetNoteTags(const std::string& note_id,
                                    const std::vector<std::string>& tags) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  // Sort and deduplicate the input tags.
  std::vector<std::string> sorted_tags = tags;
  std::sort(sorted_tags.begin(), sorted_tags.end());
  auto last = std::unique(sorted_tags.begin(), sorted_tags.end());
  sorted_tags.erase(last, sorted_tags.end());
  // Remove empty tags.
  sorted_tags.erase(
      std::remove(sorted_tags.begin(), sorted_tags.end(), std::string()),
      sorted_tags.end());

  if (note->tags == sorted_tags) {
    return true;  // No change.
  }

  note->tags = std::move(sorted_tags);
  note->modified_time = base::Time::Now();

  NotifyNoteChanged(note_id);
  NotifyNoteTagsChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

std::vector<std::string> AstraNoteService::GetNoteTags(
    const std::string& note_id) const {
  const AstraNote* note = GetNote(note_id);
  return note ? note->tags : std::vector<std::string>();
}

std::vector<std::string> AstraNoteService::GetAllTags() const {
  std::set<std::string> tag_set;

  for (const auto& [id, note] : notes_) {
    for (const auto& tag : note.tags) {
      tag_set.insert(tag);
    }
  }

  return std::vector<std::string>(tag_set.begin(), tag_set.end());
}

size_t AstraNoteService::GetTagCount(const std::string& tag) const {
  if (tag.empty()) {
    return 0;
  }

  size_t count = 0;
  for (const auto& [id, note] : notes_) {
    if (base::ranges::find(note.tags, tag) != note.tags.end()) {
      count++;
    }
  }
  return count;
}

// =========================================================================
// Note organization
// =========================================================================

bool AstraNoteService::SetNoteColor(const std::string& note_id,
                                     const std::string& color) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  if (note->color == color) {
    return true;  // No change.
  }

  note->color = color;
  note->modified_time = base::Time::Now();

  NotifyNoteChanged(note_id);
  NotifyServiceNoteUpdated(*note);
  NotifyServiceNoteColorChanged(note_id, color);

  SaveToPrefs();
  return true;
}

std::string AstraNoteService::GetNoteColor(const std::string& note_id) const {
  const AstraNote* note = GetNote(note_id);
  return note ? note->color : std::string();
}

bool AstraNoteService::SetNotePinned(const std::string& note_id, bool pinned) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  if (note->is_pinned == pinned) {
    return true;  // No change.
  }

  note->is_pinned = pinned;
  note->modified_time = base::Time::Now();

  NotifyNoteChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

bool AstraNoteService::IsNotePinned(const std::string& note_id) const {
  const AstraNote* note = GetNote(note_id);
  return note ? note->is_pinned : false;
}

bool AstraNoteService::SetNoteFavorite(const std::string& note_id,
                                        bool favorite) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }

  if (note->is_favorite == favorite) {
    return true;  // No change.
  }

  note->is_favorite = favorite;
  note->modified_time = base::Time::Now();

  NotifyNoteChanged(note_id);
  NotifyServiceNoteUpdated(*note);

  SaveToPrefs();
  return true;
}

bool AstraNoteService::IsNoteFavorite(const std::string& note_id) const {
  const AstraNote* note = GetNote(note_id);
  return note ? note->is_favorite : false;
}

// =========================================================================
// Queries
// =========================================================================

std::vector<AstraNote> AstraNoteService::GetAllNotes() const {
  std::vector<AstraNote> result;
  result.reserve(notes_.size());
  for (const auto& [id, note] : notes_) {
    result.push_back(note);
  }
  return SortNotesBy(result, sort_order_);
}

std::vector<AstraNote> AstraNoteService::GetNotesByWorkspace(
    const std::string& workspace_id) const {
  std::vector<AstraNote> result;
  for (const auto& [id, note] : notes_) {
    if (note.workspace_id == workspace_id) {
      result.push_back(note);
    }
  }
  return SortNotesBy(result, sort_order_);
}

std::vector<AstraNote> AstraNoteService::GetNotesByTag(
    const std::string& tag) const {
  if (tag.empty()) {
    return {};
  }

  std::vector<AstraNote> result;
  for (const auto& [id, note] : notes_) {
    if (base::ranges::find(note.tags, tag) != note.tags.end()) {
      result.push_back(note);
    }
  }
  return SortNotesBy(result, sort_order_);
}

std::vector<AstraNote> AstraNoteService::GetPinnedNotes() const {
  std::vector<AstraNote> result;
  for (const auto& [id, note] : notes_) {
    if (note.is_pinned) {
      result.push_back(note);
    }
  }
  return SortNotesBy(result, sort_order_);
}

std::vector<AstraNote> AstraNoteService::GetFavoriteNotes() const {
  std::vector<AstraNote> result;
  for (const auto& [id, note] : notes_) {
    if (note.is_favorite) {
      result.push_back(note);
    }
  }
  return SortNotesBy(result, sort_order_);
}

std::vector<AstraNote> AstraNoteService::GetNotesByUrl(const GURL& url) const {
  if (!url.is_valid()) {
    return {};
  }

  std::vector<AstraNote> result;
  for (const auto& [id, note] : notes_) {
    if (note.tab_url == url) {
      result.push_back(note);
    }
  }
  return SortNotesBy(result, sort_order_);
}

std::vector<AstraNote> AstraNoteService::SearchNotes(
    const std::string& query) const {
  if (query.empty()) {
    return GetAllNotes();
  }

  std::vector<AstraNote> result;
  for (const auto& [id, note] : notes_) {
    if (note.MatchesQuery(query)) {
      result.push_back(note);
    }
  }

  result = SortNotesBy(result, sort_order_);

  // Apply max search results limit.
  int max = max_search_results();
  if (max > 0 && static_cast<int>(result.size()) > max) {
    result.resize(static_cast<size_t>(max));
  }

  return result;
}

std::vector<AstraNote> AstraNoteService::GetRecentlyModifiedNotes(
    int max_count) const {
  if (max_count <= 0) {
    return {};
  }

  std::vector<AstraNote> result;
  result.reserve(notes_.size());
  for (const auto& [id, note] : notes_) {
    result.push_back(note);
  }

  // Sort by modified time descending.
  std::sort(result.begin(), result.end(),
            [](const AstraNote& a, const AstraNote& b) {
              return a.modified_time > b.modified_time;
            });

  if (static_cast<int>(result.size()) > max_count) {
    result.resize(static_cast<size_t>(max_count));
  }

  return result;
}

std::vector<AstraNote> AstraNoteService::GetNotesByColor(
    const std::string& color) const {
  if (color.empty()) {
    return {};
  }

  std::vector<AstraNote> result;
  for (const auto& [id, note] : notes_) {
    if (note.color == color) {
      result.push_back(note);
    }
  }
  return SortNotesBy(result, sort_order_);
}

// =========================================================================
// Bulk operations
// =========================================================================

size_t AstraNoteService::DeleteNotesByWorkspace(
    const std::string& workspace_id) {
  if (workspace_id.empty()) {
    return 0;
  }

  // Collect IDs to delete (can't erase while iterating).
  std::vector<std::string> ids_to_delete;
  for (const auto& [id, note] : notes_) {
    if (note.workspace_id == workspace_id) {
      ids_to_delete.push_back(id);
    }
  }

  for (const auto& id : ids_to_delete) {
    notes_.erase(id);
    NotifyNoteDeleted(id);
    NotifyServiceNoteRemoved(id);
  }

  if (!ids_to_delete.empty()) {
    SaveToPrefs();
  }

  return ids_to_delete.size();
}

void AstraNoteService::DeleteAllNotes() {
  // Collect IDs for notification.
  std::vector<std::string> ids;
  ids.reserve(notes_.size());
  for (const auto& [id, note] : notes_) {
    ids.push_back(id);
  }

  notes_.clear();

  // Notify for each deleted note.
  for (const auto& id : ids) {
    NotifyNoteDeleted(id);
    NotifyServiceNoteRemoved(id);
  }

  // Also fire the bulk reload notification.
  NotifyServiceNotesReloaded();

  SaveToPrefs();
}

bool AstraNoteService::MergeNotes(const std::string& note_id1,
                                   const std::string& note_id2,
                                   const std::string& title) {
  AstraNote* note1 = FindNote(note_id1);
  AstraNote* note2 = FindNote(note_id2);

  if (!note1 || !note2) {
    return false;
  }

  if (note_id1 == note_id2) {
    return false;  // Can't merge a note with itself.
  }

  // Merge content: note1 content + separator + note2 content.
  std::string merged_content;
  if (!note1->content.empty() && !note2->content.empty()) {
    merged_content = note1->content + "\n\n" + note2->content;
  } else {
    merged_content = note1->content + note2->content;
  }

  // Merge tags (union).
  std::set<std::string> tag_set;
  for (const auto& tag : note1->tags) {
    tag_set.insert(tag);
  }
  for (const auto& tag : note2->tags) {
    tag_set.insert(tag);
  }
  std::vector<std::string> merged_tags(tag_set.begin(), tag_set.end());

  // Update note1 with merged data.
  note1->title = title;
  note1->content = merged_content;
  note1->tags = std::move(merged_tags);
  note1->is_pinned = note1->is_pinned || note2->is_pinned;
  note1->is_favorite = note1->is_favorite || note2->is_favorite;
  note1->modified_time = base::Time::Now();
  ComputeNoteStats(note1);

  // Delete note2.
  notes_.erase(note_id2);

  NotifyNoteChanged(note_id1);
  NotifyServiceNoteUpdated(*note1);
  NotifyNoteDeleted(note_id2);
  NotifyServiceNoteRemoved(note_id2);

  SaveToPrefs();
  return true;
}

std::string AstraNoteService::DuplicateNote(const std::string& note_id) {
  const AstraNote* source = GetNote(note_id);
  if (!source) {
    return std::string();
  }

  AstraNote copy;
  copy.id = GenerateNoteId();
  copy.title = source->title + " (Copy)";
  copy.content = source->content;
  copy.workspace_id = source->workspace_id;
  copy.tab_url = source->tab_url;
  copy.tab_title = source->tab_title;
  copy.tags = source->tags;
  copy.color = source->color;
  copy.is_pinned = false;  // Duplicates start unpinned.
  copy.is_favorite = false;  // Duplicates start not favorite.
  copy.created_time = base::Time::Now();
  copy.modified_time = copy.created_time;
  copy.last_accessed = copy.created_time;
  ComputeNoteStats(&copy);

  std::string new_id = copy.id;
  notes_[new_id] = std::move(copy);

  NotifyNoteCreated(new_id);
  NotifyServiceNoteAdded(notes_[new_id]);

  SaveToPrefs();
  return new_id;
}

// =========================================================================
// Import / Export
// =========================================================================

std::string AstraNoteService::ExportNotesToJson() const {
  base::Value::List note_list;
  note_list.reserve(notes_.size());

  for (const auto& [id, note] : notes_) {
    note_list.Append(base::Value(ExportNoteToDict(note)));
  }

  std::string json;
  if (!base::JSONWriter::WriteWithOptions(
          base::Value(std::move(note_list)),
          base::JSONWriter::OPTIONS_PRETTY_PRINT,
          &json)) {
    return std::string();
  }
  return json;
}

size_t AstraNoteService::ImportNotesFromJson(const std::string& json,
                                              bool merge) {
  // Parse the JSON string into a Value.
  auto parsed = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSONParserOptions::JSON_PARSE_RFC);

  if (!parsed.has_value()) {
    // Invalid JSON — nothing imported.
    return 0;
  }

  if (!parsed->is_list()) {
    // Not a list — nothing imported.
    return 0;
  }

  const base::Value::List& list = parsed->GetList();

  if (!merge) {
    // Clear existing notes, but collect IDs for notifications.
    std::vector<std::string> old_ids;
    old_ids.reserve(notes_.size());
    for (const auto& [id, note] : notes_) {
      old_ids.push_back(id);
    }
    notes_.clear();
    for (const auto& id : old_ids) {
      NotifyNoteDeleted(id);
      NotifyServiceNoteRemoved(id);
    }
  }

  size_t imported = 0;

  for (const auto& entry : list) {
    if (!entry.is_dict()) {
      continue;
    }
    const base::Value::Dict& dict = entry.GetDict();

    absl::optional<AstraNote> parsed_note = NoteFromDict(dict);
    if (!parsed_note.has_value()) {
      continue;
    }

    AstraNote note = std::move(parsed_note.value());

    // If ID is empty or already exists, generate a new one.
    if (note.id.empty() || notes_.find(note.id) != notes_.end()) {
      note.id = GenerateNoteId();
    }

    // Ensure stats are computed.
    ComputeNoteStats(&note);

    std::string note_id = note.id;
    notes_[note_id] = std::move(note);

    NotifyNoteCreated(note_id);
    NotifyServiceNoteAdded(notes_[note_id]);
    imported++;
  }

  if (imported > 0) {
    NotifyNotesImported(static_cast<int>(imported));
    NotifyServiceNotesReloaded();
    SaveToPrefs();
  } else if (!merge) {
    // We cleared notes but imported nothing. Still save and notify.
    NotifyServiceNotesReloaded();
    SaveToPrefs();
  }

  return imported;
}

// static
base::Value::Dict AstraNoteService::ExportNoteToDict(const AstraNote& note) {
  base::Value::Dict dict;

  dict.Set(kNoteIdKey, note.id);
  dict.Set(kNoteTitleKey, note.title);
  dict.Set(kNoteContentKey, note.content);
  dict.Set(kNoteWorkspaceIdKey, note.workspace_id);

  if (note.tab_url.is_valid()) {
    dict.Set(kNoteTabUrlKey, note.tab_url.spec());
  } else {
    dict.Set(kNoteTabUrlKey, std::string());
  }
  dict.Set(kNoteTabTitleKey, note.tab_title);

  // Tags.
  base::Value::List tags_list;
  for (const auto& tag : note.tags) {
    tags_list.Append(tag);
  }
  dict.Set(kNoteTagsKey, std::move(tags_list));

  dict.Set(kNoteColorKey, note.color);
  dict.Set(kNotePinnedKey, note.is_pinned);
  dict.Set(kNoteFavoriteKey, note.is_favorite);

  dict.Set(kNoteCreatedTimeKey,
           static_cast<double>(note.created_time
                                   .ToDeltaSinceWindowsEpoch()
                                   .InMicroseconds()));
  dict.Set(kNoteModifiedTimeKey,
           static_cast<double>(note.modified_time
                                   .ToDeltaSinceWindowsEpoch()
                                   .InMicroseconds()));
  dict.Set(kNoteLastAccessedKey,
           static_cast<double>(note.last_accessed
                                   .ToDeltaSinceWindowsEpoch()
                                   .InMicroseconds()));

  dict.Set(kNoteWordCountKey, note.word_count);
  dict.Set(kNoteSizeBytesKey, note.size_bytes);

  return dict;
}

// static
absl::optional<AstraNote> AstraNoteService::NoteFromDict(
    const base::Value::Dict& dict) {
  AstraNote note;

  // ID is optional — caller can generate one.
  const std::string* id_ptr = dict.FindString(kNoteIdKey);
  if (id_ptr) {
    note.id = *id_ptr;
  }

  // Title.
  const std::string* title_ptr = dict.FindString(kNoteTitleKey);
  if (title_ptr) {
    note.title = *title_ptr;
  }

  // Content.
  const std::string* content_ptr = dict.FindString(kNoteContentKey);
  if (content_ptr) {
    note.content = *content_ptr;
  }

  // Workspace ID.
  const std::string* workspace_ptr = dict.FindString(kNoteWorkspaceIdKey);
  if (workspace_ptr) {
    note.workspace_id = *workspace_ptr;
  }

  // Tab URL.
  const std::string* url_ptr = dict.FindString(kNoteTabUrlKey);
  if (url_ptr && !url_ptr->empty()) {
    note.tab_url = GURL(*url_ptr);
  }

  // Tab title.
  const std::string* tab_title_ptr = dict.FindString(kNoteTabTitleKey);
  if (tab_title_ptr) {
    note.tab_title = *tab_title_ptr;
  }

  // Tags.
  const base::Value::List* tags_list = dict.FindList(kNoteTagsKey);
  if (tags_list) {
    for (const auto& tag_val : *tags_list) {
      if (tag_val.is_string()) {
        note.tags.push_back(tag_val.GetString());
      }
    }
    std::sort(note.tags.begin(), note.tags.end());
  }

  // Color.
  const std::string* color_ptr = dict.FindString(kNoteColorKey);
  if (color_ptr) {
    note.color = *color_ptr;
  }

  // Pinned.
  absl::optional<bool> pinned = dict.FindBool(kNotePinnedKey);
  if (pinned.has_value()) {
    note.is_pinned = pinned.value();
  }

  // Favorite.
  absl::optional<bool> favorite = dict.FindBool(kNoteFavoriteKey);
  if (favorite.has_value()) {
    note.is_favorite = favorite.value();
  }

  // Created time.
  absl::optional<double> created_time = dict.FindDouble(kNoteCreatedTimeKey);
  if (created_time.has_value()) {
    note.created_time =
        base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(
            static_cast<int64_t>(created_time.value())));
  } else {
    note.created_time = base::Time::Now();
  }

  // Modified time.
  absl::optional<double> modified_time =
      dict.FindDouble(kNoteModifiedTimeKey);
  if (modified_time.has_value()) {
    note.modified_time =
        base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(
            static_cast<int64_t>(modified_time.value())));
  } else {
    note.modified_time = note.created_time;
  }

  // Last accessed.
  absl::optional<double> last_accessed =
      dict.FindDouble(kNoteLastAccessedKey);
  if (last_accessed.has_value()) {
    note.last_accessed =
        base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(
            static_cast<int64_t>(last_accessed.value())));
  } else {
    note.last_accessed = note.modified_time;
  }

  // Word count.
  absl::optional<int> word_count = dict.FindInt(kNoteWordCountKey);
  if (word_count.has_value()) {
    note.word_count = word_count.value();
  }

  // Size bytes.
  absl::optional<int> size_bytes = dict.FindInt(kNoteSizeBytesKey);
  if (size_bytes.has_value()) {
    note.size_bytes = size_bytes.value();
  }

  return note;
}

// =========================================================================
// Settings
// =========================================================================

std::string AstraNoteService::default_note_color() const {
  if (!profile_) {
    return kDefaultNoteColor;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return kDefaultNoteColor;
  }
  return prefs->GetString(kPrefDefaultNoteColor);
}

void AstraNoteService::SetDefaultNoteColor(const std::string& color) {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetString(kPrefDefaultNoteColor, color);
}

int AstraNoteService::auto_save_interval_seconds() const {
  if (!profile_) {
    return kDefaultAutoSaveIntervalSeconds;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return kDefaultAutoSaveIntervalSeconds;
  }
  return prefs->GetInteger(kPrefAutoSaveIntervalSeconds);
}

void AstraNoteService::SetAutoSaveIntervalSeconds(int seconds) {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }
  if (seconds < 0) {
    seconds = 0;
  }
  prefs->SetInteger(kPrefAutoSaveIntervalSeconds, seconds);
}

bool AstraNoteService::show_word_count() const {
  if (!profile_) {
    return kDefaultShowWordCount;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return kDefaultShowWordCount;
  }
  return prefs->GetBoolean(kPrefShowWordCount);
}

void AstraNoteService::SetShowWordCount(bool show) {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(kPrefShowWordCount, show);
}

NoteSortOrder AstraNoteService::sort_order() const {
  return sort_order_;
}

void AstraNoteService::SetSortOrder(NoteSortOrder order) {
  if (sort_order_ == order) {
    return;
  }

  sort_order_ = order;

  if (profile_) {
    PrefService* prefs = profile_->GetPrefs();
    if (prefs) {
      prefs->SetInteger(kPrefNoteSortOrder, static_cast<int>(sort_order_));
    }
  }

  NotifyServiceNotesReordered();
}

std::string AstraNoteService::default_workspace() const {
  if (!profile_) {
    return std::string(kDefaultWorkspace);
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return std::string(kDefaultWorkspace);
  }
  return prefs->GetString(kPrefDefaultWorkspace);
}

void AstraNoteService::SetDefaultWorkspace(const std::string& workspace_id) {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetString(kPrefDefaultWorkspace, workspace_id);
}

int AstraNoteService::max_search_results() const {
  if (!profile_) {
    return kDefaultMaxSearchResults;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return kDefaultMaxSearchResults;
  }
  return prefs->GetInteger(kPrefMaxSearchResults);
}

void AstraNoteService::SetMaxSearchResults(int max) {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }
  if (max < 0) {
    max = 0;
  }
  prefs->SetInteger(kPrefMaxSearchResults, max);
}

bool AstraNoteService::trash_enabled() const {
  if (!profile_) {
    return kDefaultTrashEnabled;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return kDefaultTrashEnabled;
  }
  return prefs->GetBoolean(kPrefTrashEnabled);
}

void AstraNoteService::SetTrashEnabled(bool enabled) {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(kPrefTrashEnabled, enabled);
}

bool AstraNoteService::auto_tag_from_page() const {
  if (!profile_) {
    return kDefaultAutoTagFromPage;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return kDefaultAutoTagFromPage;
  }
  return prefs->GetBoolean(kPrefAutoTagFromPage);
}

void AstraNoteService::SetAutoTagFromPage(bool enabled) {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(kPrefAutoTagFromPage, enabled);
}

int AstraNoteService::note_font_size() const {
  if (!profile_) {
    return kDefaultNoteFontSize;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return kDefaultNoteFontSize;
  }
  return prefs->GetInteger(kPrefNoteFontSize);
}

void AstraNoteService::SetNoteFontSize(int size) {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }
  if (size < 8) {
    size = 8;
  }
  if (size > 72) {
    size = 72;
  }
  prefs->SetInteger(kPrefNoteFontSize, size);
}

double AstraNoteService::note_line_height() const {
  if (!profile_) {
    return kDefaultNoteLineHeight;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return kDefaultNoteLineHeight;
  }
  return prefs->GetDouble(kPrefNoteLineHeight);
}

void AstraNoteService::SetNoteLineHeight(double line_height) {
  if (!profile_) {
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }
  if (line_height < 0.8) {
    line_height = 0.8;
  }
  if (line_height > 3.0) {
    line_height = 3.0;
  }
  prefs->SetDouble(kPrefNoteLineHeight, line_height);
}

// =========================================================================
// Color palette
// =========================================================================

// static
std::vector<std::string> AstraNoteService::GetNoteColorPalette() {
  std::vector<std::string> palette;
  for (const char* color : kNoteColorPalette) {
    palette.push_back(color);
  }
  return palette;
}

// =========================================================================
// Sort helpers (extended API / legacy)
// =========================================================================

std::vector<AstraNote> AstraNoteService::GetNotesSortedBy(
    NoteSortOrder order) const {
  std::vector<AstraNote> result;
  result.reserve(notes_.size());
  for (const auto& [id, note] : notes_) {
    result.push_back(note);
  }
  return SortNotesBy(result, order);
}

// Legacy AddNote — delegates to CreateNote plus sets URL and color.
std::string AstraNoteService::AddNote(const std::string& title,
                                       const std::string& content,
                                       const GURL& url,
                                       const std::string& workspace_id,
                                       const std::string& color) {
  // Use CreateNote for the basic creation.
  std::string id = CreateNote(title, content);

  // Then set additional fields.
  AstraNote* note = FindNote(id);
  if (!note) {
    return id;
  }

  if (!url.is_empty()) {
    note->tab_url = url;
  }
  if (!workspace_id.empty()) {
    note->workspace_id = workspace_id;
  }
  if (!color.empty()) {
    note->color = color;
  }
  note->modified_time = base::Time::Now();

  SaveToPrefs();

  // Note: CreateNote already fired notifications. Since we're modifying
  // immediately after, fire updated notifications too.
  NotifyNoteChanged(id);
  NotifyServiceNoteUpdated(*note);

  return id;
}

// Legacy UpdateNote.
bool AstraNoteService::UpdateNote(const AstraNote& note) {
  AstraNote* existing = FindNote(note.id);
  if (!existing) {
    return false;
  }

  // Copy all fields except id and created_time (those are immutable).
  existing->title = note.title;
  existing->content = note.content;
  existing->tab_url = note.tab_url;
  existing->tab_title = note.tab_title;
  existing->workspace_id = note.workspace_id;
  existing->color = note.color;
  existing->tags = note.tags;
  existing->is_pinned = note.is_pinned;
  existing->is_favorite = note.is_favorite;
  existing->modified_time = base::Time::Now();
  ComputeNoteStats(existing);

  NotifyNoteChanged(note.id);
  NotifyNoteTagsChanged(note.id);
  NotifyServiceNoteUpdated(*existing);

  SaveToPrefs();
  return true;
}

// Legacy tag methods.
bool AstraNoteService::AddTagToNote(const std::string& note_id,
                                     const std::string& tag) {
  return AddNoteTag(note_id, tag);
}

bool AstraNoteService::RemoveTagFromNote(const std::string& note_id,
                                          const std::string& tag) {
  return RemoveNoteTag(note_id, tag);
}

std::vector<AstraNote> AstraNoteService::GetNotesWithTag(
    const std::string& tag) const {
  return GetNotesByTag(tag);
}

std::vector<AstraNote> AstraNoteService::GetNotesWithAllTags(
    const std::vector<std::string>& tags) const {
  if (tags.empty()) {
    return {};
  }

  std::vector<AstraNote> result;
  for (const auto& [id, note] : notes_) {
    bool has_all = true;
    for (const auto& tag : tags) {
      if (base::ranges::find(note.tags, tag) == note.tags.end()) {
        has_all = false;
        break;
      }
    }
    if (has_all) {
      result.push_back(note);
    }
  }
  return SortNotesBy(result, sort_order_);
}

std::vector<AstraNote> AstraNoteService::GetNotesForUrl(
    const GURL& url) const {
  return GetNotesByUrl(url);
}

std::vector<AstraNote> AstraNoteService::GetNotesForWorkspace(
    const std::string& workspace_id) const {
  return GetNotesByWorkspace(workspace_id);
}

// Legacy pinned helpers.
bool AstraNoteService::ToggleNotePinned(const std::string& note_id) {
  AstraNote* note = FindNote(note_id);
  if (!note) {
    return false;
  }
  return SetNotePinned(note_id, !note->is_pinned);
}

// Legacy import/export.
size_t AstraNoteService::ImportNotesJson(const std::string& json, bool merge) {
  return ImportNotesFromJson(json, merge);
}

std::string AstraNoteService::ExportNotesJson() const {
  return ExportNotesToJson();
}

// =========================================================================
// Private helpers
// =========================================================================

AstraNote* AstraNoteService::FindNote(const std::string& note_id) {
  auto it = notes_.find(note_id);
  return it == notes_.end() ? nullptr : &it->second;
}

void AstraNoteService::ComputeNoteStats(AstraNote* note) const {
  DCHECK(note);
  note->word_count = CountWords(note->content);
  note->size_bytes = static_cast<int>(note->title.size() + note->content.size());
}

// static
std::vector<AstraNote> AstraNoteService::SortNotesBy(
    const std::vector<AstraNote>& notes,
    NoteSortOrder order) {
  std::vector<AstraNote> result = notes;

  auto compare = [order](const AstraNote& a, const AstraNote& b) {
    // Pinned notes always come first, regardless of sort order.
    if (a.is_pinned != b.is_pinned) {
      return a.is_pinned;  // Pinned < unpinned => pinned comes first.
    }

    switch (order) {
      case NoteSortOrder::kDateDescending:
        return a.modified_time > b.modified_time;
      case NoteSortOrder::kDateAscending:
        return a.modified_time < b.modified_time;
      case NoteSortOrder::kTitleAscending:
        return base::CompareCaseInsensitiveASCII(a.title, b.title) < 0;
      case NoteSortOrder::kTitleDescending:
        return base::CompareCaseInsensitiveASCII(a.title, b.title) > 0;
      case NoteSortOrder::kColorAscending:
        return a.color < b.color;
      case NoteSortOrder::kColorDescending:
        return a.color > b.color;
      case NoteSortOrder::kCreatedDateDescending:
        return a.created_time > b.created_time;
      case NoteSortOrder::kCreatedDateAscending:
        return a.created_time < b.created_time;
    }
    return a.modified_time > b.modified_time;  // Fallback.
  };

  std::sort(result.begin(), result.end(), compare);
  return result;
}

void AstraNoteService::LoadFromPrefs() {
  if (!profile_) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }

  // Load sort order.
  int sort_order_int = prefs->GetInteger(kPrefNoteSortOrder);
  if (sort_order_int >= 0 &&
      sort_order_int <=
          static_cast<int>(NoteSortOrder::kCreatedDateAscending)) {
    sort_order_ = static_cast<NoteSortOrder>(sort_order_int);
  } else {
    sort_order_ = NoteSortOrder::kDateDescending;
  }

  // Load notes.
  const base::Value::List& note_list = prefs->GetList(kPrefNotes);
  notes_.clear();

  for (const auto& entry : note_list) {
    if (!entry.is_dict()) {
      continue;
    }
    const base::Value::Dict& dict = entry.GetDict();

    absl::optional<AstraNote> parsed = NoteFromDict(dict);
    if (!parsed.has_value()) {
      continue;
    }

    AstraNote note = std::move(parsed.value());
    if (note.id.empty()) {
      continue;
    }

    // Ensure stats are computed (in case old data doesn't have them).
    if (note.word_count == 0 && !note.content.empty()) {
      ComputeNoteStats(&note);
    }
    if (note.size_bytes == 0 && (!note.title.empty() || !note.content.empty())) {
      ComputeNoteStats(&note);
    }

    notes_[note.id] = std::move(note);
  }

  // Notify service observers of reload.
  NotifyServiceNotesReloaded();
}

void AstraNoteService::SaveToPrefs() {
  if (!profile_) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  if (!prefs) {
    return;
  }

  base::Value::List note_list;
  note_list.reserve(notes_.size());

  for (const auto& [id, note] : notes_) {
    note_list.Append(base::Value(ExportNoteToDict(note)));
  }

  prefs->SetList(kPrefNotes, std::move(note_list));
}

std::string AstraNoteService::GenerateNoteId() const {
  return base::UnguessableToken::Create().ToString();
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraNoteService::NotifyNoteCreated(const std::string& note_id) {
  for (auto& observer : observers_) {
    observer.OnNoteCreated(this, note_id);
  }
}

void AstraNoteService::NotifyNoteDeleted(const std::string& note_id) {
  for (auto& observer : observers_) {
    observer.OnNoteDeleted(this, note_id);
  }
}

void AstraNoteService::NotifyNoteChanged(const std::string& note_id) {
  for (auto& observer : observers_) {
    observer.OnNoteChanged(this, note_id);
  }
}

void AstraNoteService::NotifyNoteTagsChanged(const std::string& note_id) {
  for (auto& observer : observers_) {
    observer.OnNoteTagsChanged(this, note_id);
  }
}

void AstraNoteService::NotifyNotesImported(int count) {
  for (auto& observer : observers_) {
    observer.OnNotesImported(this, count);
  }
}

void AstraNoteService::NotifyServiceNoteAdded(const AstraNote& note) {
  for (auto& observer : service_observers_) {
    observer.OnNoteAdded(note);
  }
}

void AstraNoteService::NotifyServiceNoteUpdated(const AstraNote& note) {
  for (auto& observer : service_observers_) {
    observer.OnNoteUpdated(note);
  }
}

void AstraNoteService::NotifyServiceNoteRemoved(const std::string& note_id) {
  for (auto& observer : service_observers_) {
    observer.OnNoteRemoved(note_id);
  }
}

void AstraNoteService::NotifyServiceNoteColorChanged(
    const std::string& note_id,
    const std::string& new_color) {
  for (auto& observer : service_observers_) {
    observer.OnNoteColorChanged(note_id, new_color);
  }
}

void AstraNoteService::NotifyServiceNotesReordered() {
  for (auto& observer : service_observers_) {
    observer.OnNotesReordered();
  }
}

void AstraNoteService::NotifyServiceNotesReloaded() {
  for (auto& observer : service_observers_) {
    observer.OnNotesReloaded();
  }
}

}  // namespace astra
