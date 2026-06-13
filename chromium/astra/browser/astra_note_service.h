#ifndef ASTRA_BROWSER_ASTRA_NOTE_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_NOTE_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace astra {

class AstraNoteService;

// =========================================================================
// AstraNote — note metadata structure
// =========================================================================
//
// A single note entry. Notes are Astra-specific productivity metadata
// associated with a web page, a workspace, or standing alone.
//
// Truth model:
//   - Note definitions live in AstraNoteService (profile-scoped,
//     persisted via PrefService).
//   - Per-page notes are linked by URL, not by tab identity.
//   - Notes can optionally be linked to workspaces.
//
// Chromium subsystems reused:
//   - PrefService (for note persistence).
//   - ProfileKeyedServiceFactory pattern.
//
// Chromium patch points:
//   - Profile keyed service registration: to wire up the factory.
//     Patch point: chrome/browser/profiles/profile_keyed_service_factory*.
//
// TODO(astra): Consider using a dedicated notes storage backend
// (LevelDB, SQLite) for performance with many notes. PrefService list-of-dicts
// is simple and works for a few hundred notes, but at scale a dedicated store
// would be more efficient for queries and partial updates.
// Chromium owner: PrefService (for small-scale) / LevelDB (for large-scale)
// =========================================================================

struct AstraNote {
  // Unique identifier for this note.
  std::string id;

  // Note title (short, single line).
  std::string title;

  // Note body content (plain text, multi-line).
  std::string content;

  // Associated workspace ID. Empty if not linked to a workspace (global).
  std::string workspace_id;

  // Associated URL for per-page notes. Empty if not linked to a page.
  GURL tab_url;

  // Page title for per-page notes. Empty if not linked to a page.
  std::string tab_title;

  // Tags for categorizing and filtering notes.
  std::vector<std::string> tags;

  // Accent color for the note (hex string, e.g. "#FFD93D").
  // Empty string means use the default color.
  std::string color;

  // Whether the note is pinned. Pinned notes appear at the top.
  bool is_pinned = false;

  // Whether the note is marked as favorite.
  bool is_favorite = false;

  // When the note was created.
  base::Time created_time;

  // When the note was last modified.
  base::Time modified_time;

  // When the note was last accessed (viewed or edited).
  base::Time last_accessed;

  // Cached word count of the note content.
  // Computed automatically when content changes.
  int word_count = 0;

  // Cached size of the note in bytes (title + content).
  // Computed automatically when title or content changes.
  int size_bytes = 0;

  // Returns true if both title and content are empty.
  bool IsEmpty() const;

  // Returns true if the note matches the given search query.
  // Search covers title, content, and tags (case-insensitive substring match).
  bool MatchesQuery(const std::string& query) const;
};

// =========================================================================
// NoteSortOrder
// =========================================================================
//
// Sorting criteria for note lists.  Used by GetNotesSortedBy() and the
// default sort order preference.
//
// Pinned notes always come first within any sort order — pinning is a
// priority overlay, not a sort criterion.
// =========================================================================

enum class NoteSortOrder {
  // Most recently modified first (default).
  kDateDescending,
  // Least recently modified first.
  kDateAscending,
  // Title A-Z.
  kTitleAscending,
  // Title Z-A.
  kTitleDescending,
  // Color (hex string, alphabetical).
  kColorAscending,
  // Color (hex string, reverse alphabetical).
  kColorDescending,
  // Most recently created first.
  kCreatedDateDescending,
  // Least recently created first.
  kCreatedDateAscending,
};

// =========================================================================
// AstraNoteObserver
// =========================================================================
//
// Observer interface for UI layers (sidebar notes view, note editor) to
// react to note changes. UI must never be the source of truth —
// AstraNoteService is.
//
// All methods have empty default implementations so observers can override
// only the events they care about.
//
// Events are intentionally simple: they pass the note ID rather than the
// full note struct, so observers can decide whether to fetch the full
// note data.
// =========================================================================

class AstraNoteObserver : public base::CheckedObserver {
 public:
  // Called after a new note is created.
  virtual void OnNoteCreated(AstraNoteService* service,
                             const std::string& note_id) {}

  // Called after a note is deleted.
  virtual void OnNoteDeleted(AstraNoteService* service,
                             const std::string& note_id) {}

  // Called after a note's core data (title, content, workspace, URL,
  // color, pinned, favorite) is changed.
  virtual void OnNoteChanged(AstraNoteService* service,
                             const std::string& note_id) {}

  // Called when a note's tags change.
  // OnNoteChanged also fires — this is a convenience event for UI that
  // only cares about tag changes.
  virtual void OnNoteTagsChanged(AstraNoteService* service,
                                 const std::string& note_id) {}

  // Called after a batch of notes has been imported.
  virtual void OnNotesImported(AstraNoteService* service, int count) {}

  // Called when the note service is shutting down.
  // Observers should remove themselves and release any references.
  virtual void OnNoteServiceShutdown(AstraNoteService* service) {}

 protected:
  ~AstraNoteObserver() override = default;
};

// =========================================================================
// AstraNoteServiceObserver
// =========================================================================
//
// Extended observer interface with more granular events.
// This provides richer notifications for UI that needs to distinguish
// between different types of changes.
//
// All methods have empty default implementations so observers can override
// only the events they care about.
// =========================================================================

class AstraNoteServiceObserver : public base::CheckedObserver {
 public:
  // Called after a new note is created.
  virtual void OnNoteAdded(const AstraNote& note) {}

  // Called after a note is updated (title, content, tags, pinned state, etc.).
  virtual void OnNoteUpdated(const AstraNote& note) {}

  // Called after a note is deleted.
  virtual void OnNoteRemoved(const std::string& note_id) {}

  // Called when a note's color is changed.  OnNoteUpdated also fires.
  // This is a convenience event for UI that only cares about color changes.
  virtual void OnNoteColorChanged(const std::string& note_id,
                                  const std::string& new_color) {}

  // Called when the overall note ordering changes (e.g., sort order changed,
  // or bulk reorder).  Observers should refresh their list ordering.
  virtual void OnNotesReordered() {}

  // Called when the entire notes set has changed (e.g., bulk import,
  // or after a full sync). Observers should do a full rebuild.
  virtual void OnNotesReloaded() {}

 protected:
  ~AstraNoteServiceObserver() override = default;
};

// =========================================================================
// AstraNoteService
// =========================================================================
//
// Profile-scoped keyed service that owns all Astra note metadata.
//
// Truth source for:
//   - Note definitions (id, title, content, url, workspace_id, timestamps,
//     color, tags, pinned, favorite, word_count, size_bytes).
//
// Persistence:
//   - Notes are stored in the profile's PrefService as a list of
//     dictionaries under the kPrefNotes key.
//   - All mutations are persisted immediately.
//
// Not owned here:
//   - Tabs / WebContents (Chromium TabStripModel owns them).
//   - Workspaces (AstraWorkspaceService owns them).
//   - Profile, history, downloads, etc. (all Chromium).
//
// TODO(astra): Add full-text search with a proper index (e.g. using
// base::i18n::StringSearch or a dedicated search index). For now,
// SearchNotes does a simple substring match over all note titles and content.
// =========================================================================

class AstraNoteService final : public KeyedService {
 public:
  // -- Pref keys (public for factory registration) -------------------------
  //
  // These pref key constants are exposed so the factory and tests can
  // register and read them by name.

  // List of note dictionaries.
  static constexpr const char kPrefNotes[] = "astra.notes.list";

  // Default sort order for note lists (int, maps to NoteSortOrder).
  static constexpr const char kPrefNoteSortOrder[] = "astra.notes.sort_order";

  // Default note color (string, hex color code).
  static constexpr const char kPrefDefaultNoteColor[] =
      "astra.notes.default_color";

  // Auto-save interval in seconds (int).
  // 0 means save immediately on every change.
  static constexpr const char kPrefAutoSaveIntervalSeconds[] =
      "astra.notes.auto_save_interval_seconds";

  // Whether to show word count in the note editor (bool).
  static constexpr const char kPrefShowWordCount[] = "astra.notes.show_word_count";

  // Default workspace ID for new notes (string).
  // Empty means new notes are global (not attached to any workspace).
  static constexpr const char kPrefDefaultWorkspace[] =
      "astra.notes.default_workspace";

  // Maximum number of search results to return (int).
  // 0 means no limit.
  static constexpr const char kPrefMaxSearchResults[] =
      "astra.notes.max_search_results";

  // Whether trash / soft-delete is enabled (bool).
  // When false, deletions are permanent.
  static constexpr const char kPrefTrashEnabled[] = "astra.notes.trash_enabled";

  // Whether to auto-tag notes from page content (bool).
  // When true, notes attached to a page get tags inferred from the page.
  static constexpr const char kPrefAutoTagFromPage[] =
      "astra.notes.auto_tag_from_page";

  // Default note font size in points (int).
  static constexpr const char kPrefNoteFontSize[] = "astra.notes.font_size";

  // Default note line height multiplier (double).
  // 1.0 = single spaced, 1.5 = 1.5x, 2.0 = double spaced.
  static constexpr const char kPrefNoteLineHeight[] =
      "astra.notes.line_height";

  // -- Default values ------------------------------------------------------

  // Default sort order: most recently modified first.
  static constexpr int kDefaultNoteSortOrder =
      static_cast<int>(NoteSortOrder::kDateDescending);

  // Default note color (amber/yellow, classic note-style).
  static constexpr const char kDefaultNoteColor[] = "#FFD93D";

  // Default auto-save interval: 0 = immediate save.
  static constexpr int kDefaultAutoSaveIntervalSeconds = 0;

  // Default: show word count.
  static constexpr bool kDefaultShowWordCount = true;

  // Default: no default workspace (global notes).
  static constexpr const char kDefaultWorkspace[] = "";

  // Default: no limit on search results.
  static constexpr int kDefaultMaxSearchResults = 0;

  // Default: trash disabled (permanent delete).
  static constexpr bool kDefaultTrashEnabled = false;

  // Default: auto-tag from page disabled.
  static constexpr bool kDefaultAutoTagFromPage = false;

  // Default font size: 14pt.
  static constexpr int kDefaultNoteFontSize = 14;

  // Default line height: 1.5x.
  static constexpr double kDefaultNoteLineHeight = 1.5;

  // -----------------------------------------------------------------------
  // Construction / destruction
  // -----------------------------------------------------------------------

  explicit AstraNoteService(Profile* profile);
  AstraNoteService(const AstraNoteService&) = delete;
  AstraNoteService& operator=(const AstraNoteService&) = delete;
  ~AstraNoteService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraNoteObserver* observer);
  void RemoveObserver(AstraNoteObserver* observer);

  void AddServiceObserver(AstraNoteServiceObserver* observer);
  void RemoveServiceObserver(AstraNoteServiceObserver* observer);

  // -- Note CRUD -----------------------------------------------------------

  // Creates a new note with the given title and content.
  // Generates a unique ID and sets timestamps to now.
  // Returns the ID of the new note.
  // Fires OnNoteCreated (AstraNoteObserver) and OnNoteAdded (ServiceObserver).
  std::string CreateNote(const std::string& title,
                         const std::string& content = "");

  // Returns the note with the given ID, or nullptr if not found.
  // The returned pointer is valid only until the next mutation call.
  const AstraNote* GetNote(const std::string& note_id) const;

  // Updates the title of a note.
  // Returns true if the note existed and was updated.
  bool UpdateNoteTitle(const std::string& note_id, const std::string& title);

  // Updates the content of a note.
  // Recomputes word_count and size_bytes.
  // Returns true if the note existed and was updated.
  bool UpdateNoteContent(const std::string& note_id,
                         const std::string& content);

  // Deletes the note with the given ID.
  // Returns true if the note existed and was deleted.
  bool DeleteNote(const std::string& note_id);

  // Returns whether a note with the given ID exists.
  bool DoesNoteExist(const std::string& note_id) const;

  // Returns the total number of notes.
  size_t GetNoteCount() const;

  // -- Note association ----------------------------------------------------

  // Sets the workspace for a note. Empty string = global (no workspace).
  // Returns true if the note existed and was updated.
  bool SetNoteWorkspace(const std::string& note_id,
                        const std::string& workspace_id);

  // Returns the workspace ID of a note, or empty string if not set or
  // the note doesn't exist.
  std::string GetNoteWorkspace(const std::string& note_id) const;

  // Sets the tab URL and page title for a note.
  // Returns true if the note existed and was updated.
  bool SetNoteTabUrl(const std::string& note_id,
                     const GURL& url,
                     const std::string& page_title);

  // Returns the URL of a note, or empty GURL if not set or note doesn't exist.
  GURL GetNoteTabUrl(const std::string& note_id) const;

  // Clears the tab association (URL and title) from a note.
  // Returns true if the note existed and was updated.
  bool ClearNoteTab(const std::string& note_id);

  // -- Tags ----------------------------------------------------------------

  // Adds a tag to a note. No-op if the tag already exists.
  // Returns true if the tag was added.
  bool AddNoteTag(const std::string& note_id, const std::string& tag);

  // Removes a tag from a note. No-op if the tag doesn't exist.
  // Returns true if the tag was removed.
  bool RemoveNoteTag(const std::string& note_id, const std::string& tag);

  // Replaces all tags on a note with the given set.
  // Returns true if the note existed and tags were set.
  bool SetNoteTags(const std::string& note_id,
                   const std::vector<std::string>& tags);

  // Returns all tags for a note. Empty vector if note doesn't exist.
  std::vector<std::string> GetNoteTags(const std::string& note_id) const;

  // Returns all unique tags across all notes, sorted alphabetically.
  std::vector<std::string> GetAllTags() const;

  // Returns the number of notes that have the given tag.
  size_t GetTagCount(const std::string& tag) const;

  // -- Note organization ---------------------------------------------------

  // Sets the color of a note.
  // Returns true if the note existed and was updated.
  bool SetNoteColor(const std::string& note_id, const std::string& color);

  // Returns the color of a note, or empty string if note doesn't exist.
  std::string GetNoteColor(const std::string& note_id) const;

  // Sets the pinned state of a note.
  // Returns true if the note existed and was updated.
  bool SetNotePinned(const std::string& note_id, bool pinned);

  // Returns whether a note is pinned. Returns false if note doesn't exist.
  bool IsNotePinned(const std::string& note_id) const;

  // Sets the favorite state of a note.
  // Returns true if the note existed and was updated.
  bool SetNoteFavorite(const std::string& note_id, bool favorite);

  // Returns whether a note is favorite. Returns false if note doesn't exist.
  bool IsNoteFavorite(const std::string& note_id) const;

  // -- Queries -------------------------------------------------------------

  // Returns all notes, sorted by modified time (most recent first).
  // Pinned notes come first.
  std::vector<AstraNote> GetAllNotes() const;

  // Returns notes associated with the given workspace.
  // Sorted by modified time (most recent first), pinned first.
  std::vector<AstraNote> GetNotesByWorkspace(
      const std::string& workspace_id) const;

  // Returns notes that have the given tag.
  // Sorted by modified time (most recent first), pinned first.
  std::vector<AstraNote> GetNotesByTag(const std::string& tag) const;

  // Returns all pinned notes, sorted by modified time (most recent first).
  std::vector<AstraNote> GetPinnedNotes() const;

  // Returns all favorite notes, sorted by modified time (most recent first).
  std::vector<AstraNote> GetFavoriteNotes() const;

  // Returns notes attached to the given URL.
  // Sorted by modified time (most recent first), pinned first.
  std::vector<AstraNote> GetNotesByUrl(const GURL& url) const;

  // Full-text search across title, content, and tags.
  // Case-insensitive substring matching.
  // Sorted by modified time (most recent first).
  // Limited by max_search_results pref (0 = no limit).
  std::vector<AstraNote> SearchNotes(const std::string& query) const;

  // Returns the N most recently modified notes.
  std::vector<AstraNote> GetRecentlyModifiedNotes(int max_count) const;

  // Returns notes with the given accent color.
  std::vector<AstraNote> GetNotesByColor(const std::string& color) const;

  // -- Bulk operations -----------------------------------------------------

  // Deletes all notes in the given workspace.
  // Returns the number of notes deleted.
  size_t DeleteNotesByWorkspace(const std::string& workspace_id);

  // Deletes all notes.
  void DeleteAllNotes();

  // Merges two notes into one.
  // The merged note retains note_id1's ID, workspace, URL, and color.
  // Title is set to |title|.
  // Content is concatenated: note1 content + "\n\n" + note2 content.
  // Tags are unioned.
  // Pinned state: true if either note is pinned.
  // Favorite state: true if either note is favorite.
  // note_id2 is deleted after the merge.
  // Returns true if both notes existed and the merge succeeded.
  bool MergeNotes(const std::string& note_id1,
                  const std::string& note_id2,
                  const std::string& title);

  // Creates a copy of a note with a new ID.
  // The duplicate has " (Copy)" appended to the title and current timestamps.
  // Tags, color, workspace, and URL are copied.
  // Pinned and favorite states are NOT copied (duplicate starts unpinned,
  // not favorite).
  // Returns the ID of the new note, or empty string if the source note
  // doesn't exist.
  std::string DuplicateNote(const std::string& note_id);

  // -- Import / Export -----------------------------------------------------

  // Exports all notes as a JSON string (JSON array of note objects).
  std::string ExportNotesToJson() const;

  // Imports notes from a JSON string.
  // If |merge| is true, imported notes are added to existing notes.
  // If |merge| is false, existing notes are replaced.
  // Returns the number of notes successfully imported.
  size_t ImportNotesFromJson(const std::string& json, bool merge = false);

  // Exports a single note to a Value::Dict.
  static base::Value::Dict ExportNoteToDict(const AstraNote& note);

  // Parses a note from a Value::Dict.
  // Returns absl::nullopt if the dict is missing required fields or is
  // otherwise invalid.
  // Note: the returned note's id is taken from the dict if present,
  // otherwise an empty id is returned (caller should generate one).
  static absl::optional<AstraNote> NoteFromDict(const base::Value::Dict& dict);

  // -- Settings ------------------------------------------------------------
  //
  // User-configurable note settings, persisted via PrefService.

  // Default note color.
  std::string default_note_color() const;
  void SetDefaultNoteColor(const std::string& color);

  // Auto-save interval in seconds (0 = immediate).
  int auto_save_interval_seconds() const;
  void SetAutoSaveIntervalSeconds(int seconds);

  // Whether to show word count.
  bool show_word_count() const;
  void SetShowWordCount(bool show);

  // Default sort order.
  NoteSortOrder sort_order() const;
  void SetSortOrder(NoteSortOrder order);

  // Default workspace for new notes (empty = global).
  std::string default_workspace() const;
  void SetDefaultWorkspace(const std::string& workspace_id);

  // Maximum search results (0 = no limit).
  int max_search_results() const;
  void SetMaxSearchResults(int max);

  // Whether trash (soft delete) is enabled.
  bool trash_enabled() const;
  void SetTrashEnabled(bool enabled);

  // Whether to auto-tag notes from page content.
  bool auto_tag_from_page() const;
  void SetAutoTagFromPage(bool enabled);

  // Note font size in points.
  int note_font_size() const;
  void SetNoteFontSize(int size);

  // Note line height multiplier.
  double note_line_height() const;
  void SetNoteLineHeight(double line_height);

  // -- Color palette -------------------------------------------------------

  // Returns the list of available note colors (hex color strings).
  static std::vector<std::string> GetNoteColorPalette();

  // -- Sort helpers (extended API) -----------------------------------------

  // Returns all notes sorted by the given order.
  // Pinned notes come first regardless of sort order.
  std::vector<AstraNote> GetNotesSortedBy(NoteSortOrder order) const;

  // Legacy AddNote (preserved for backward compatibility).
  std::string AddNote(const std::string& title,
                      const std::string& content,
                      const GURL& url = GURL(),
                      const std::string& workspace_id = std::string(),
                      const std::string& color = std::string());

  // Legacy UpdateNote (preserved for backward compatibility).
  bool UpdateNote(const AstraNote& note);

  // Legacy tag methods.
  bool AddTagToNote(const std::string& note_id, const std::string& tag);
  bool RemoveTagFromNote(const std::string& note_id, const std::string& tag);
  std::vector<AstraNote> GetNotesWithTag(const std::string& tag) const;
  std::vector<AstraNote> GetNotesWithAllTags(
      const std::vector<std::string>& tags) const;
  std::vector<AstraNote> GetNotesForUrl(const GURL& url) const;
  std::vector<AstraNote> GetNotesForWorkspace(
      const std::string& workspace_id) const;

  // Legacy pinned helpers.
  bool ToggleNotePinned(const std::string& note_id);

  // Legacy import/export.
  size_t ImportNotesJson(const std::string& json, bool merge = false);
  std::string ExportNotesJson() const;

 private:
  // Non-const lookup helper for internal use.
  AstraNote* FindNote(const std::string& note_id);

  // Recomputes word_count and size_bytes for a note.
  // Called whenever title or content changes.
  void ComputeNoteStats(AstraNote* note) const;

  // Sorts a list of notes by the given order (pinned first, then by order).
  static std::vector<AstraNote> SortNotesBy(
      const std::vector<AstraNote>& notes,
      NoteSortOrder order);

  // Load all notes from the profile's PrefService.
  // Called from the constructor.
  void LoadFromPrefs();

  // Persist all notes to the profile's PrefService.
  // Called after every mutation.
  // TODO(astra): Consider batching pref writes with a delayed task to
  // avoid excessive disk I/O when typing in the note editor (auto-save).
  // Chromium pattern: Use base::OneShotTimer for debounced saves.
  void SaveToPrefs();

  // Generate a unique note ID using base::UnguessableToken.
  std::string GenerateNoteId() const;

  // -- Notification helpers ------------------------------------------------

  // Notify all observers that a note was created.
  void NotifyNoteCreated(const std::string& note_id);

  // Notify all observers that a note was deleted.
  void NotifyNoteDeleted(const std::string& note_id);

  // Notify all observers that a note was changed (core fields).
  void NotifyNoteChanged(const std::string& note_id);

  // Notify all observers that a note's tags changed.
  void NotifyNoteTagsChanged(const std::string& note_id);

  // Notify all observers that notes were imported.
  void NotifyNotesImported(int count);

  // Notify all service observers with the full note struct (added).
  void NotifyServiceNoteAdded(const AstraNote& note);

  // Notify all service observers with the full note struct (updated).
  void NotifyServiceNoteUpdated(const AstraNote& note);

  // Notify all service observers of note removal.
  void NotifyServiceNoteRemoved(const std::string& note_id);

  // Notify all service observers of color change.
  void NotifyServiceNoteColorChanged(const std::string& note_id,
                                     const std::string& new_color);

  // Notify all service observers of reorder.
  void NotifyServiceNotesReordered();

  // Notify all service observers of reload.
  void NotifyServiceNotesReloaded();

  // -- Member variables ----------------------------------------------------

  raw_ptr<Profile> profile_;

  // Notes stored by ID for O(1) lookup.
  std::map<std::string, AstraNote> notes_;

  // Default sort order.
  NoteSortOrder sort_order_ = NoteSortOrder::kDateDescending;

  // Observers (simple AstraNoteObserver).
  base::ObserverList<AstraNoteObserver> observers_;

  // Service observers (extended AstraNoteServiceObserver).
  base::ObserverList<AstraNoteServiceObserver> service_observers_;
};

// =========================================================================
// AstraNoteServiceFactory
// =========================================================================
//
// Factory for AstraNoteService.
//
// Incognito behavior: the factory uses kRedirectedToOriginal for regular
// incognito profiles because notes are user-level state that persists
// across sessions. An incognito window still has access to the user's
// notes — only the browsing context is isolated.
//
// Guest: kOwnInstance — guest sessions get their own ephemeral notes.
// System: kNone — system profile has no user notes.
// =========================================================================

class AstraNoteServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  static AstraNoteService* GetForProfile(Profile* profile);
  static AstraNoteServiceFactory* GetInstance();

  // Registers note-related prefs on the profile's PrefRegistry.
  // TODO(astra): Wire this into profile keyed service registration so it
  // runs during profile creation. Chromium patch point:
  // chrome/browser/profiles/profile_keyed_service_factory registrations.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  AstraNoteServiceFactory();
  ~AstraNoteServiceFactory() override;

  // ProfileKeyedServiceFactory:
  KeyedService* BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_NOTE_SERVICE_H_
