// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_NOTES_PAGE_ASTRA_NOTES_PAGE_MODEL_H_
#define ASTRA_UI_VIEWS_NOTES_PAGE_ASTRA_NOTES_PAGE_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"

namespace astra {

// Filter options for notes.
enum class AstraNotesFilter {
  kAll,
  kPinned,
  kArchived,
  kRecent,
};

// Sort options for notes.
enum class AstraNotesSortType {
  kNewestModified,
  kOldestModified,
  kNewestCreated,
  kTitle,
  kColor,
};

// A single note entry.
struct AstraNoteEntry {
  std::string id;
  std::u16string title;
  std::u16string content;
  base::Time date_created;
  base::Time date_modified;
  std::vector<std::string> tags;
  std::string workspace;
  std::string folder;
  std::string color;  // default, yellow, green, blue, pink, purple
  bool is_pinned = false;
  bool is_archived = false;
  int word_count = 0;
  std::string associated_url;
  std::u16string associated_tab_title;
};

// A note folder / notebook.
struct AstraNoteFolder {
  std::string id;
  std::u16string name;
  std::string color;
  int note_count = 0;
};

// Observer for AstraNotesPageModel.
class AstraNotesPageObserver : public base::CheckedObserver {
 public:
  // Called when the overall notes list changes.
  virtual void OnNotesChanged() {}

  // Called when a note is added.
  virtual void OnNoteAdded(const std::string& id) {}

  // Called when a note is removed.
  virtual void OnNoteRemoved(const std::string& id) {}

  // Called when a note is updated (title, content, color, tags, etc.).
  virtual void OnNoteUpdated(const std::string& id) {}

  // Called when a folder is added.
  virtual void OnFolderAdded(const std::string& id) {}

  // Called when a folder is removed.
  virtual void OnFolderRemoved(const std::string& id) {}

  // Called when the active/selected note changes.
  virtual void OnActiveNoteChanged(const std::string& id) {}

  // Called when the search query changes.
  virtual void OnSearchChanged(const std::u16string& query) {}

  // Called when a filter (status, folder, tag, sort) changes.
  virtual void OnFilterChanged() {}

  // Called when the model is about to be destroyed.
  virtual void OnNotesPageModelShutdown() {}

 protected:
  ~AstraNotesPageObserver() override = default;
};

// Model for the notes page.
//
// Owns the note entries, folders, and filtering/search logic.  Notes data
// is Astra-specific metadata that augments Chromium's browsing state with
// user-created notes tied to pages, workspaces, and folders.
//
// Chromium owner: No direct Chromium owner yet — this is Astra-only
//   metadata.  Long-term, notes should be stored as part of an
//   Astra-specific KeyedService and persisted via leveldb or similar.
//
// TODO(astra): Persist notes via an Astra KeyedService backed by
// leveldb / PrefService.  Patch point:
// chrome/browser/profiles/profile_keyed_service_factory.cc.
// TODO(astra): Wire up associated_url / associated_tab_title to
// the current WebContents via TabHelpers.
class AstraNotesPageModel {
 public:
  AstraNotesPageModel();
  ~AstraNotesPageModel();

  AstraNotesPageModel(const AstraNotesPageModel&) = delete;
  AstraNotesPageModel& operator=(const AstraNotesPageModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraNotesPageObserver* observer);
  void RemoveObserver(AstraNotesPageObserver* observer);

  // -- Note data ------------------------------------------------------------

  // Get all notes (unfiltered).
  const std::vector<AstraNoteEntry>& GetNotes() const;

  // Get a specific note by ID. Returns nullptr if not found.
  const AstraNoteEntry* GetNote(const std::string& id) const;

  // Get total number of notes (unfiltered).
  size_t GetCount() const;

  // Get number of archived notes.
  size_t GetArchivedCount() const;

  // Get number of pinned notes.
  size_t GetPinnedCount() const;

  // -- Active note ----------------------------------------------------------

  void SetActiveNote(const std::string& id);
  const std::string& GetActiveNoteId() const;

  // -- Search ---------------------------------------------------------------

  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const;

  // -- Filters --------------------------------------------------------------

  void SetFilter(AstraNotesFilter filter);
  AstraNotesFilter GetFilter() const;

  void SetFolderFilter(const std::string& folder);
  const std::string& GetFolderFilter() const;

  void SetTagFilter(const std::string& tag);
  const std::string& GetTagFilter() const;

  // -- Sorting --------------------------------------------------------------

  void SetSortType(AstraNotesSortType sort_type);
  AstraNotesSortType GetSortType() const;

  // -- Folders --------------------------------------------------------------

  const std::vector<AstraNoteFolder>& GetFolders() const;

  void AddFolder(const std::u16string& name, const std::string& color);
  void RemoveFolder(const std::string& id);
  void RenameFolder(const std::string& id, const std::u16string& name);

  // -- Tags -----------------------------------------------------------------

  // Get all unique tags across all notes.
  std::vector<std::string> GetAllTags() const;

  // -- Note manipulation ----------------------------------------------------

  // Add a new note. Returns the generated ID.
  std::string AddNote(const std::u16string& title,
                      const std::u16string& content,
                      const std::string& folder,
                      const std::string& color);

  // Remove a note by ID.
  void RemoveNote(const std::string& id);

  // Update note title.
  void UpdateNoteTitle(const std::string& id, const std::u16string& title);

  // Update note content.
  void UpdateNoteContent(const std::string& id, const std::u16string& content);

  // Update note color.
  void UpdateNoteColor(const std::string& id, const std::string& color);

  // Toggle pin state of a note.
  void TogglePin(const std::string& id);

  // Archive a note.
  void ArchiveNote(const std::string& id);

  // Unarchive a note.
  void UnarchiveNote(const std::string& id);

  // Add a tag to a note.
  void AddTagToNote(const std::string& id, const std::string& tag);

  // Remove a tag from a note.
  void RemoveTagFromNote(const std::string& id, const std::string& tag);

  // Move a note to a different folder.
  void MoveNoteToFolder(const std::string& id, const std::string& folder);

  // -- Sample data ----------------------------------------------------------

  // Populate with 20+ sample notes across folders, colors, and tags.
  void PopulateSampleNotes();

  // -- State ----------------------------------------------------------------

  bool IsLoading() const;
  void SetLoading(bool loading);

  // Get filtered and sorted notes (what the UI should display).
  std::vector<AstraNoteEntry> GetFilteredNotes() const;

 private:
  // Check if a note matches the current search query (title AND content).
  bool MatchesSearch(const AstraNoteEntry& entry) const;

  // Check if a note matches the current status filter.
  bool MatchesFilter(const AstraNoteEntry& entry) const;

  // Check if a note matches the current folder filter.
  bool MatchesFolder(const AstraNoteEntry& entry) const;

  // Check if a note matches the current tag filter.
  bool MatchesTag(const AstraNoteEntry& entry) const;

  // Sort a list of notes by the current sort type.
  void SortNotes(std::vector<AstraNoteEntry>& notes) const;

  // Apply all filters and update folder note counts.
  void ApplyFilters();

  // Update the word count for a note based on its content.
  void UpdateWordCount(AstraNoteEntry& entry) const;

  // Find a note by ID (mutable). Returns nullptr if not found.
  AstraNoteEntry* FindNote(const std::string& id);

  // Find a folder by ID (mutable). Returns nullptr if not found.
  AstraNoteFolder* FindFolder(const std::string& id);

  // Generate a unique note ID.
  std::string GenerateNoteId() const;

  // Generate a unique folder ID.
  std::string GenerateFolderId() const;

  // Update note counts on all folders.
  void UpdateFolderNoteCounts();

  // -- Notify helpers -------------------------------------------------------

  void NotifyNotesChanged();
  void NotifyNoteAdded(const std::string& id);
  void NotifyNoteRemoved(const std::string& id);
  void NotifyNoteUpdated(const std::string& id);
  void NotifyFolderAdded(const std::string& id);
  void NotifyFolderRemoved(const std::string& id);
  void NotifyActiveNoteChanged(const std::string& id);
  void NotifySearchChanged(const std::u16string& query);
  void NotifyFilterChanged();

  // -- Data -----------------------------------------------------------------

  // All notes (unfiltered).
  std::vector<AstraNoteEntry> all_notes_;

  // All folders.
  std::vector<AstraNoteFolder> folders_;

  // Currently active / selected note ID.
  std::string active_note_id_;

  // Current search query.
  std::u16string search_query_;

  // Current status filter.
  AstraNotesFilter filter_ = AstraNotesFilter::kAll;

  // Current folder filter (empty = all).
  std::string folder_filter_;

  // Current tag filter (empty = all).
  std::string tag_filter_;

  // Current sort type.
  AstraNotesSortType sort_type_ = AstraNotesSortType::kNewestModified;

  // Loading state.
  bool loading_ = false;

  // Observers.
  base::ObserverList<AstraNotesPageObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_NOTES_PAGE_ASTRA_NOTES_PAGE_MODEL_H_
