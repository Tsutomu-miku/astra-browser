#ifndef ASTRA_BROWSER_ASTRA_READING_LIST_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_READING_LIST_SERVICE_H_

#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class PrefService;
class Profile;

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace reading_list {
class ReadingListModel;
class ReadingListEntry;
}  // namespace reading_list

namespace astra {

// =========================================================================
// AstraReadingListStatus — read/unread/unseen status of a reading list entry
// =========================================================================
//
// Three-state status for reading list entries.  Chromium's ReadingListModel
// uses a simpler read/unread boolean; the "unseen" state is an Astra-level
// projection for entries that have been added but never opened.
//
// Chromium owner: ReadingListEntry::IsRead()
// (components/reading_list/core/reading_list_entry.h)
// =========================================================================

enum class AstraReadingListStatus {
  kUnread,   // Entry has been seen but marked as unread.
  kRead,     // Entry has been marked as read.
  kUnseen,   // Entry was added but never opened (Astra-projected state).
};

// =========================================================================
// AstraReadingListDistillState — distillation status of a reading list entry
// =========================================================================
//
// Whether the entry has a distilled / reader-mode version available.
// Mirrors the distill state from Chromium's ReadingListEntry.
//
// Chromium owner: ReadingListEntry::DistilledState()
// (components/reading_list/core/reading_list_entry.h)
// =========================================================================

enum class AstraReadingListDistillState {
  kYes,          // Distilled version is available.
  kNo,           // No distilled version.
  kUnknown,      // Distill state not yet determined.
  kDistilling,   // Distillation in progress.
  kDistillError, // Distillation failed.
};

// =========================================================================
// AstraReadingListSortOrder — sort order for reading list query results
// =========================================================================
//
// Controls how entries are ordered when returned by query methods.
// Persisted as a user preference.
//
// Chromium owner: ReadingListModel's internal sort
// (components/reading_list/core/reading_list_model.h)
// =========================================================================

enum class AstraReadingListSortOrder {
  kByDateAdded,          // Most recently added first.
  kByDateRead,           // Most recently read first.
  kByTitle,              // Alphabetical by title (case-insensitive).
  kByEstimatedReadTime,  // Shortest estimated read time first.
};

// =========================================================================
// AstraReadingListFontSize — reader mode font size presets
// =========================================================================

enum class AstraReadingListFontSize {
  kSmall,
  kMedium,
  kLarge,
  kExtraLarge,
};

// =========================================================================
// AstraReadingListTheme — reader mode theme
// =========================================================================

enum class AstraReadingListTheme {
  kLight,
  kDark,
  kSepia,
  kSystem,
};

// =========================================================================
// AstraReadingListView — sidebar default view
// =========================================================================

enum class AstraReadingListView {
  kAll,        // Show all entries.
  kUnread,     // Show only unread entries.
  kFavorites,  // Show only favorite entries.
  kFolders,    // Show folder-based view.
};

// =========================================================================
// AstraReadingListEntry — projected reading list entry data
// =========================================================================
//
// A lightweight projection of a reading list entry for UI consumption.
//
// In production, the source of truth is Chromium's ReadingListModel
// (components/reading_list/core/reading_list_entry.h), and this struct is
// a convenient snapshot for sidebar / command palette presentation.
//
// Astra-owned fields (favorites, tags, note, folder_id) are stored as
// Astra metadata via PrefService and projected onto the entry.
//
// Chromium owner: ReadingListEntry
// (components/reading_list/core/reading_list_entry.h)
// =========================================================================

struct AstraReadingListEntry {
  // The URL of the reading list item (also serves as the unique key).
  GURL url;

  // The title of the page.
  std::string title;

  // Three-state status of the entry.
  AstraReadingListStatus status = AstraReadingListStatus::kUnseen;

  // When the entry was added to the reading list.
  base::Time added_time;

  // When the entry was last updated (e.g., marked read, title change).
  base::Time update_time;

  // Estimated time to read the article.  Zero if unknown.
  base::TimeDelta estimated_read_time;

  // Readability score (0.0 - 1.0).  -1.0 if unknown.
  // Higher scores mean more readable / article-like content.
  double score = -1.0;

  // URL of the thumbnail image for the entry.  May be empty.
  GURL thumbnail_url;

  // Whether the entry has a distilled (reader mode) version.
  bool has_distilled = false;

  // Detailed distillation state.
  AstraReadingListDistillState distill_state =
      AstraReadingListDistillState::kUnknown;

  // Word count of the distilled article.  -1 if unknown.
  int word_count = -1;

  // First time the entry was read.  Null if never read.
  base::Time first_read_time;

  // Most recent time the entry was read.  Null if never read.
  base::Time last_read_time;

  // Number of times the entry has been read.
  int read_count = 0;

  // -- Astra-owned metadata -----------------------------------------------

  // Whether the entry is marked as a favorite (Astra metadata).
  bool is_favorite = false;

  // User-assigned tags for the entry (Astra metadata).
  std::vector<std::string> tags;

  // User-added note for the entry (Astra metadata).
  std::string note;

  // ID of the folder this entry belongs to (Astra metadata).
  // Empty string means "no folder" (uncategorized).
  std::string folder_id;
};

// =========================================================================
// AstraReadingListFolder — Astra-only folder organization
// =========================================================================
//
// Folders are an Astra-level organizational concept on top of Chromium's
// flat reading list.  Folder metadata is stored via PrefService.
//
// Chromium equivalent: ReadingListModel does not have folders natively.
// Astra projects folder membership as entry metadata.
//
// TODO(astra): Consider whether to use ReadingListEntry extra data fields
// instead of a separate pref store.  For the overlay skeleton, PrefService
// storage is simpler.
// =========================================================================

struct AstraReadingListFolder {
  // Unique identifier for the folder.
  std::string folder_id;

  // Display name of the folder.
  std::string name;

  // Whether this is a default / system folder (e.g., "Uncategorized").
  // Default folders cannot be deleted.
  bool is_default = false;

  // Number of entries in this folder.
  int entry_count = 0;

  // Number of unread entries in this folder.
  int unread_count = 0;

  // When the folder was created.
  base::Time created_time;

  // Sort order index for folder display.  Lower numbers come first.
  int order_index = 0;
};

// =========================================================================
// AstraReadingListObserver
// =========================================================================
//
// Observer interface for UI layers (sidebar reading list section, command
// palette, etc.) to react to reading list changes.  The UI is always a
// projection — it never mutates reading list state directly; it calls
// methods on AstraReadingListService.
//
// All methods have empty default implementations so observers can override
// only the events they care about.
//
// Chromium owner: ReadingListModelObserver
// (components/reading_list/core/reading_list_model_observer.h)
// =========================================================================

class AstraReadingListObserver : public base::CheckedObserver {
 public:
  // Called after an entry is added to the reading list.
  virtual void OnReadingListEntryAdded(AstraReadingListService* service,
                                       const GURL& url) {}

  // Called after an entry is removed from the reading list.
  virtual void OnReadingListEntryRemoved(AstraReadingListService* service,
                                         const GURL& url) {}

  // Called after an entry's state changes (e.g., title update,
  // read/unread toggle, metadata change).
  virtual void OnReadingListEntryChanged(AstraReadingListService* service,
                                         const GURL& url) {}

  // Called when the read status of an entry changes.
  // OnReadingListEntryChanged also fires — this is a convenience event.
  virtual void OnReadingListEntryStatusChanged(AstraReadingListService* service,
                                               const GURL& url,
                                               bool read) {}

  // Called after a folder is created.
  virtual void OnReadingListFolderCreated(AstraReadingListService* service,
                                          const std::string& folder_id) {}

  // Called after a folder is deleted.
  virtual void OnReadingListFolderDeleted(AstraReadingListService* service,
                                          const std::string& folder_id) {}

  // Called when the overall reading list has changed in a way that
  // requires a full UI refresh (e.g., bulk operations, sort change).
  virtual void OnReadingListChanged(AstraReadingListService* service) {}

  // Called when the reading list service is shutting down.
  // Observers should remove themselves in this callback.
  virtual void OnReadingListServiceShutdown(AstraReadingListService* service) {}

 protected:
  ~AstraReadingListObserver() override = default;
};

// =========================================================================
// AstraReadingListService
// =========================================================================
//
// Profile-scoped keyed service that projects reading list entries for
// Astra UI surfaces (sidebar, command palette, etc.).
//
// In production: This is a thin projection / observer bridge that wraps
// Chromium's ReadingListModel and translates its observer interface into
// Astra's observer interface.  All mutations delegate to ReadingListModel.
//
// For the overlay skeleton: Entries are stored as Astra metadata via
// PrefService (list of dicts pattern, same as notes and workspaces).
// This lets us develop and test the Astra UI layer before the full
// Chromium reading_list component is wired in.
//
// Why a separate service instead of using ReadingListModel directly?
//   - Astra UI code depends on //astra/browser, not on //components/reading_list.
//   - The service provides Astra-specific projection (e.g., folders, tags,
//     favorites, sidebar grouping).
//   - It isolates Astra from changes to ReadingListModel's API.
//
// Chromium subsystem reused:
//   - ReadingListModel (components/reading_list/core/reading_list_model.h)
//   - ReadingListModelObserver
//   - ReadingListEntry
//   - PrefService (for Astra-level presentation preferences)
//
// Chromium patch points:
//   - None needed for basic integration — ReadingListModel is obtained
//     via its factory from the browser context.
//   - TODO(astra): If we need deep customization (e.g., Astra-specific
//     entry metadata), a small patch to reading_list_model.h or a new
//     ReadingListEntry extra data field would be the way.
// =========================================================================

class AstraReadingListService final : public KeyedService {
 public:
  // -- Pref keys (public static constants) --------------------------------
  //
  // These pref key constants are defined on the service class so that
  // callers and the factory can reference them without depending on
  // a separate pref header.  The factory's RegisterProfilePrefs method
  // uses these constants when registering prefs.

  // Entry metadata prefs (Astra-owned, stored alongside entries).
  // These are part of the entries dict list in prefs.
  static constexpr const char kPrefReadingListEntries[] =
      "astra.reading_list.entries";

  // Folders pref: list of dicts describing all user-created folders.
  static constexpr const char kPrefReadingListFolders[] =
      "astra.reading_list.folders";

  // Entry metadata map: keyed by URL, stores favorite state, tags, notes,
  // and folder membership.  Dict of dicts.
  static constexpr const char kPrefReadingListEntryMetadata[] =
      "astra.reading_list.entry_metadata";

  // -- Settings prefs -----------------------------------------------------

  // Default sort order for reading list queries.
  // String value: "date_added", "date_read", "title", "estimated_read_time".
  static constexpr const char kPrefDefaultSortOrder[] =
      "astra.reading_list.default_sort_order";
  static constexpr const char kDefaultDefaultSortOrder[] = "date_added";

  // Whether to auto-mark entries as read when scrolled past.
  static constexpr const char kPrefAutoMarkReadOnScroll[] =
      "astra.reading_list.auto_mark_read_on_scroll";
  static constexpr bool kDefaultAutoMarkReadOnScroll = true;

  // Number of days after which read entries are auto-deleted.
  // 0 means never auto-delete.
  static constexpr const char kPrefAutoDeleteReadAfterDays[] =
      "astra.reading_list.auto_delete_read_after_days";
  static constexpr int kDefaultAutoDeleteReadAfterDays = 0;

  // Whether to show estimated read time in the reading list UI.
  static constexpr const char kPrefShowEstimatedReadTime[] =
      "astra.reading_list.show_estimated_read_time";
  static constexpr bool kDefaultShowEstimatedReadTime = true;

  // Whether to show thumbnails in the reading list UI.
  static constexpr const char kPrefShowThumbnail[] =
      "astra.reading_list.show_thumbnail";
  static constexpr bool kDefaultShowThumbnail = true;

  // Reader mode font size.
  // String value: "small", "medium", "large", "extra_large".
  static constexpr const char kPrefReaderFontSize[] =
      "astra.reading_list.reader_font_size";
  static constexpr const char kDefaultReaderFontSize[] = "medium";

  // Reader mode theme.
  // String value: "light", "dark", "sepia", "system".
  static constexpr const char kPrefReaderTheme[] =
      "astra.reading_list.reader_theme";
  static constexpr const char kDefaultReaderTheme[] = "system";

  // Reader mode line height multiplier.
  static constexpr const char kPrefReaderLineHeight[] =
      "astra.reading_list.reader_line_height";
  static constexpr double kDefaultReaderLineHeight = 1.5;

  // Whether text-to-speech is enabled for reading list items.
  static constexpr const char kPrefTextToSpeechEnabled[] =
      "astra.reading_list.text_to_speech_enabled";
  static constexpr bool kDefaultTextToSpeechEnabled = false;

  // Whether to auto-sync the reading list.
  static constexpr const char kPrefAutoSyncReadingList[] =
      "astra.reading_list.auto_sync";
  static constexpr bool kDefaultAutoSyncReadingList = true;

  // Default view for the reading list sidebar.
  // String value: "all", "unread", "favorites", "folders".
  static constexpr const char kPrefSidebarDefaultView[] =
      "astra.reading_list.sidebar_default_view";
  static constexpr const char kDefaultSidebarDefaultView[] = "all";

  // Maximum number of items to show in the sidebar reading list section.
  // 0 means no limit.
  static constexpr const char kPrefMaxSidebarItemCount[] =
      "astra.reading_list.max_sidebar_item_count";
  static constexpr int kDefaultMaxSidebarItemCount = 50;

  // -- Construction / destruction ----------------------------------------

  explicit AstraReadingListService(Profile* profile);
  AstraReadingListService(const AstraReadingListService&) = delete;
  AstraReadingListService& operator=(const AstraReadingListService&) = delete;
  ~AstraReadingListService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraReadingListObserver* observer);
  void RemoveObserver(AstraReadingListObserver* observer);

  // -- Entry queries -----------------------------------------------------

  // Returns the total number of entries (read + unread).
  size_t GetEntryCount() const;

  // Returns the number of unread entries.
  size_t GetUnreadCount() const;

  // Returns the number of read entries.
  size_t GetReadCount() const;

  // Returns a pointer to the entry with |url|, or nullptr if not found.
  // The pointer is valid only until the next mutation call.
  const AstraReadingListEntry* GetEntryByUrl(const GURL& url) const;

  // Returns true if |url| is in the reading list.
  bool HasEntry(const GURL& url) const;

  // Returns unread entries.  If |max_count| > 0, returns at most that many.
  // Entries are sorted by the current default sort order.
  std::vector<AstraReadingListEntry> GetUnreadEntries(int max_count = 0) const;

  // Returns read entries.  If |max_count| > 0, returns at most that many.
  std::vector<AstraReadingListEntry> GetReadEntries(int max_count = 0) const;

  // Returns all entries, sorted by the current default sort order.
  std::vector<AstraReadingListEntry> GetAllEntries() const;

  // Returns the most recently added entries (up to |max_count|).
  std::vector<AstraReadingListEntry> GetRecentlyAddedEntries(int max_count) const;

  // Returns the most recently read entries (up to |max_count|).
  std::vector<AstraReadingListEntry> GetRecentlyReadEntries(int max_count) const;

  // -- Entry operations --------------------------------------------------

  // Adds a URL with an explicit title to the reading list.
  // Returns true if the entry was added.
  bool AddEntry(const GURL& url, const std::string& title);

  // Removes the entry with |url| from the reading list.
  // Returns true if an entry was removed.
  bool RemoveEntry(const GURL& url);

  // Marks the entry with |url| as read.
  // Returns true if the state changed.
  bool MarkEntryRead(const GURL& url);

  // Marks the entry with |url| as unread.
  // Returns true if the state changed.
  bool MarkEntryUnread(const GURL& url);

  // Updates the title of an existing entry.
  // Returns true if the entry existed and was updated.
  bool UpdateEntryTitle(const GURL& url, const std::string& title);

  // Marks all entries as read.  Returns the number of entries changed.
  size_t MarkAllRead();

  // Deletes all read entries.  Returns the number of entries removed.
  size_t DeleteRead();

  // -- Astra metadata (favorites, tags, notes, folders) ------------------

  // Sets or clears the favorite flag for an entry.
  // Returns true if the entry existed and the flag changed.
  bool SetEntryFavorite(const GURL& url, bool favorite);

  // Returns true if the entry is marked as a favorite.
  // Returns false if the entry doesn't exist.
  bool IsEntryFavorite(const GURL& url) const;

  // Returns all entries marked as favorites.
  std::vector<AstraReadingListEntry> GetFavoriteEntries() const;

  // Sets a note on an entry.  Pass empty string to clear.
  // Returns true if the entry existed and the note changed.
  bool SetEntryNote(const GURL& url, const std::string& note);

  // Returns the note for an entry, or empty string if none.
  std::string GetEntryNote(const GURL& url) const;

  // Adds a tag to an entry.  Duplicate tags are ignored.
  // Returns true if the tag was added (entry exists and tag is new).
  bool AddEntryTag(const GURL& url, const std::string& tag);

  // Removes a tag from an entry.
  // Returns true if the tag was removed (entry exists and had the tag).
  bool RemoveEntryTag(const GURL& url, const std::string& tag);

  // Returns the tags for an entry.  Returns empty vector if entry not found.
  std::vector<std::string> GetEntryTags(const GURL& url) const;

  // Returns all unique tags across all entries, sorted alphabetically.
  std::vector<std::string> GetAllTags() const;

  // Sets the folder for an entry.  Pass empty string to remove from folder.
  // Returns true if the entry existed and the folder changed.
  bool SetEntryFolder(const GURL& url, const std::string& folder_id);

  // Returns the folder ID for an entry, or empty string if not in a folder.
  std::string GetEntryFolder(const GURL& url) const;

  // -- Folders -----------------------------------------------------------

  // Creates a new folder with the given name.  Returns the new folder ID.
  // Returns empty string if creation fails (e.g., empty name).
  std::string CreateFolder(const std::string& name);

  // Deletes a folder.  Entries in the folder become uncategorized.
  // Returns true if the folder existed and was deleted.
  // Default folders cannot be deleted and return false.
  bool DeleteFolder(const std::string& folder_id);

  // Renames a folder.
  // Returns true if the folder existed and was renamed.
  bool RenameFolder(const std::string& folder_id, const std::string& new_name);

  // Returns the folder with |folder_id|, or nullptr if not found.
  // The pointer is valid only until the next folder mutation.
  const AstraReadingListFolder* GetFolder(const std::string& folder_id) const;

  // Returns all folders, sorted by order_index.
  std::vector<AstraReadingListFolder> GetAllFolders() const;

  // Returns the number of folders.
  size_t GetFolderCount() const;

  // Returns all entries in the given folder.
  std::vector<AstraReadingListEntry> GetEntriesInFolder(
      const std::string& folder_id) const;

  // Moves an entry to a different folder.
  // Equivalent to SetEntryFolder but provided for readability.
  // Returns true if the entry existed and was moved.
  bool MoveEntryToFolder(const GURL& url, const std::string& folder_id);

  // Reorders folders according to the given list of folder IDs.
  // Folders not in the list are placed at the end in their current order.
  // Returns true if the order changed.
  bool ReorderFolders(const std::vector<std::string>& folder_ids_order);

  // -- Search ------------------------------------------------------------

  // Searches entries whose titles or URLs contain |query|
  // (case-insensitive substring match).  Returns matching entries.
  std::vector<AstraReadingListEntry> SearchEntries(
      const std::string& query) const;

  // Returns all entries that have the given tag.
  std::vector<AstraReadingListEntry> SearchEntriesByTag(
      const std::string& tag) const;

  // Returns all entries with the given status.
  std::vector<AstraReadingListEntry> GetEntriesByStatus(
      AstraReadingListStatus status) const;

  // -- Settings ----------------------------------------------------------

  // Default sort order.
  void set_default_sort_order(AstraReadingListSortOrder order);
  AstraReadingListSortOrder default_sort_order() const {
    return default_sort_order_;
  }

  // Auto-mark read on scroll.
  void set_auto_mark_read_on_scroll(bool enabled);
  bool auto_mark_read_on_scroll() const { return auto_mark_read_on_scroll_; }

  // Auto-delete read entries after days (0 = never).
  void set_auto_delete_read_after_days(int days);
  int auto_delete_read_after_days() const {
    return auto_delete_read_after_days_;
  }

  // Show estimated read time.
  void set_show_estimated_read_time(bool show);
  bool show_estimated_read_time() const { return show_estimated_read_time_; }

  // Show thumbnail.
  void set_show_thumbnail(bool show);
  bool show_thumbnail() const { return show_thumbnail_; }

  // Reader mode font size.
  void set_reader_font_size(AstraReadingListFontSize size);
  AstraReadingListFontSize reader_font_size() const {
    return reader_font_size_;
  }

  // Reader mode theme.
  void set_reader_theme(AstraReadingListTheme theme);
  AstraReadingListTheme reader_theme() const { return reader_theme_; }

  // Reader mode line height.
  void set_reader_line_height(double line_height);
  double reader_line_height() const { return reader_line_height_; }

  // Text-to-speech enabled.
  void set_text_to_speech_enabled(bool enabled);
  bool text_to_speech_enabled() const { return text_to_speech_enabled_; }

  // Auto-sync reading list.
  void set_auto_sync_reading_list(bool enabled);
  bool auto_sync_reading_list() const { return auto_sync_reading_list_; }

  // Sidebar default view.
  void set_sidebar_default_view(AstraReadingListView view);
  AstraReadingListView sidebar_default_view() const {
    return sidebar_default_view_;
  }

  // Max sidebar item count (0 = no limit).
  void set_max_sidebar_item_count(int count);
  int max_sidebar_item_count() const { return max_sidebar_item_count_; }

  // -- Model access ------------------------------------------------------

  // Direct access to the underlying Chromium model.  Prefer the typed
  // methods above for Astra UI code.  Exposed for advanced use cases.
  // TODO(astra): Consider whether to expose this at all.  For now, it's
  // useful as an escape hatch and for tests.
  reading_list::ReadingListModel* model() const { return model_; }

  // Returns true if the underlying model has finished loading.
  bool IsModelLoaded() const;

  // -- Pref registration helper ------------------------------------------
  //
  // Registers all reading-list-related profile prefs.
  // Called by AstraReadingListServiceFactory::RegisterProfilePrefs.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  // -- Internal helpers --------------------------------------------------

  // Non-const lookup helper for internal use.
  AstraReadingListEntry* FindEntry(const GURL& url);

  // Apply the default sort order to a vector of entries.
  void ApplyDefaultSort(std::vector<AstraReadingListEntry>& entries) const;

  // Apply a specific sort order to a vector of entries.
  static void ApplySortWithOrder(
      std::vector<AstraReadingListEntry>& entries,
      AstraReadingListSortOrder order);

  // Truncate a vector to |max_count| entries.  max_count <= 0 means no limit.
  static void TruncateEntries(std::vector<AstraReadingListEntry>& entries,
                              int max_count);

  // Generate a unique folder ID.
  static std::string GenerateFolderId();

  // -- Metadata helpers --------------------------------------------------

  // Gets the metadata dict for a given URL.  Returns nullptr if not found.
  const base::Value::Dict* GetMetadataForUrl(const GURL& url) const;

  // Gets or creates the metadata dict for a given URL.
  base::Value::Dict* GetOrCreateMetadataForUrl(const GURL& url);

  // Saves the current metadata map to prefs.
  void SaveMetadataToPrefs();

  // Loads metadata from prefs into the in-memory map.
  void LoadMetadataFromPrefs();

  // -- Folder helpers ----------------------------------------------------

  // Non-const lookup for a folder by ID.
  AstraReadingListFolder* FindFolder(const std::string& folder_id);

  // Recomputes entry_count and unread_count for all folders based on
  // the current entries list and folder membership metadata.
  void RecomputeFolderCounts();

  // Saves the current folder list to prefs.
  void SaveFoldersToPrefs();

  // Loads folders from prefs into the in-memory list.
  void LoadFoldersFromPrefs();

  // -- Persistence -------------------------------------------------------

  // Load all entries from the profile's PrefService.
  void LoadEntriesFromPrefs();

  // Persist all entries to the profile's PrefService.
  void SaveEntriesToPrefs();

  // Load all settings from PrefService.
  void LoadSettingsFromPrefs();

  // Serialize an entry to a Value dict for pref storage.
  static base::Value::Dict EntryToDict(const AstraReadingListEntry& entry);

  // Deserialize an entry from a Value dict.
  // Returns true if deserialization was successful.
  static bool DictToEntry(const base::Value::Dict& dict,
                          AstraReadingListEntry& entry);

  // Serialize a folder to a Value dict for pref storage.
  static base::Value::Dict FolderToDict(const AstraReadingListFolder& folder);

  // Deserialize a folder from a Value dict.
  // Returns true if deserialization was successful.
  static bool DictToFolder(const base::Value::Dict& dict,
                           AstraReadingListFolder& folder);

  // -- Settings conversion helpers ---------------------------------------

  static std::string SortOrderToString(AstraReadingListSortOrder order);
  static AstraReadingListSortOrder StringToSortOrder(const std::string& value);

  static std::string FontSizeToString(AstraReadingListFontSize size);
  static AstraReadingListFontSize StringToFontSize(const std::string& value);

  static std::string ThemeToString(AstraReadingListTheme theme);
  static AstraReadingListTheme StringToTheme(const std::string& value);

  static std::string ViewToString(AstraReadingListView view);
  static AstraReadingListView StringToView(const std::string& value);

  // -- Observer notification helpers -------------------------------------

  void NotifyEntryAdded(const GURL& url);
  void NotifyEntryRemoved(const GURL& url);
  void NotifyEntryChanged(const GURL& url);
  void NotifyEntryStatusChanged(const GURL& url, bool read);
  void NotifyFolderCreated(const std::string& folder_id);
  void NotifyFolderDeleted(const std::string& folder_id);
  void NotifyReadingListChanged();
  void NotifyServiceShutdown();

  // -- Data members ------------------------------------------------------

  raw_ptr<Profile> profile_;
  raw_ptr<PrefService> prefs_ = nullptr;

  // The underlying Chromium reading list model.  Not owned — obtained from
  // the profile's keyed service factory.
  // TODO(astra): Wire up model_ = ReadingListModelFactory::GetForBrowserContext(profile);
  raw_ptr<reading_list::ReadingListModel> model_ = nullptr;

  // In-memory storage for reading list entries (overlay skeleton).
  // In production, all data comes from ReadingListModel.
  // TODO(astra): Remove this when ReadingListModel is fully wired.
  std::vector<AstraReadingListEntry> entries_;

  // In-memory storage for folders.
  std::vector<AstraReadingListFolder> folders_;

  // In-memory metadata map: keyed by URL spec, value is a dict with
  // "favorite", "tags", "note", "folder_id" keys.
  // This is the Astra-owned metadata layer on top of Chromium entries.
  base::Value::Dict entry_metadata_;

  // -- Settings (cached from prefs) --------------------------------------

  AstraReadingListSortOrder default_sort_order_ =
      AstraReadingListSortOrder::kByDateAdded;
  bool auto_mark_read_on_scroll_ = kDefaultAutoMarkReadOnScroll;
  int auto_delete_read_after_days_ = kDefaultAutoDeleteReadAfterDays;
  bool show_estimated_read_time_ = kDefaultShowEstimatedReadTime;
  bool show_thumbnail_ = kDefaultShowThumbnail;
  AstraReadingListFontSize reader_font_size_ = AstraReadingListFontSize::kMedium;
  AstraReadingListTheme reader_theme_ = AstraReadingListTheme::kSystem;
  double reader_line_height_ = kDefaultReaderLineHeight;
  bool text_to_speech_enabled_ = kDefaultTextToSpeechEnabled;
  bool auto_sync_reading_list_ = kDefaultAutoSyncReadingList;
  AstraReadingListView sidebar_default_view_ = AstraReadingListView::kAll;
  int max_sidebar_item_count_ = kDefaultMaxSidebarItemCount;

  // Cached entry used for GetEntryByUrl() return value.  Mutable so
  // GetEntryByUrl (const) can update it.
  // TODO(astra): When the model is fully wired, reconsider whether a cache
  // is appropriate or if we should return value/optional instead.
  mutable AstraReadingListEntry cached_entry_;

  // Cached folder used for GetFolder() return value.
  mutable AstraReadingListFolder cached_folder_;

  base::ObserverList<AstraReadingListObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_READING_LIST_SERVICE_H_
