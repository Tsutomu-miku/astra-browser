// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/notes_page/astra_notes_page_model.h"

#include <algorithm>
#include <set>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"

namespace astra {

namespace {

// Case-insensitive substring match of |needle| in |haystack|.
bool CaseInsensitiveContains(const std::u16string& haystack,
                             const std::u16string& needle) {
  if (needle.empty()) {
    return true;
  }
  return base::StrContains(base::ToLowerASCII(haystack),
                           base::ToLowerASCII(needle));
}

// Count words in a UTF-16 string (space-delimited).
int CountWords(const std::u16string& text) {
  if (text.empty()) {
    return 0;
  }
  int count = 0;
  bool in_word = false;
  for (char16_t c : text) {
    if (c == u' ' || c == u'\n' || c == u'\t' || c == u'\r') {
      in_word = false;
    } else {
      if (!in_word) {
        count++;
        in_word = true;
      }
    }
  }
  return count;
}

}  // namespace

// ===========================================================================
// AstraNotesPageModel
// ===========================================================================

AstraNotesPageModel::AstraNotesPageModel() = default;

AstraNotesPageModel::~AstraNotesPageModel() {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnNotesPageModelShutdown();
  }
}

// -- Observer management ----------------------------------------------------

void AstraNotesPageModel::AddObserver(AstraNotesPageObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraNotesPageModel::RemoveObserver(AstraNotesPageObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Notes data -------------------------------------------------------------

const std::vector<AstraNoteEntry>& AstraNotesPageModel::GetNotes() const {
  return all_notes_;
}

const AstraNoteEntry* AstraNotesPageModel::GetNote(
    const std::string& id) const {
  for (const auto& note : all_notes_) {
    if (note.id == id) {
      return &note;
    }
  }
  return nullptr;
}

size_t AstraNotesPageModel::GetCount() const {
  return all_notes_.size();
}

size_t AstraNotesPageModel::GetArchivedCount() const {
  size_t count = 0;
  for (const auto& note : all_notes_) {
    if (note.is_archived) {
      count++;
    }
  }
  return count;
}

size_t AstraNotesPageModel::GetPinnedCount() const {
  size_t count = 0;
  for (const auto& note : all_notes_) {
    if (note.is_pinned) {
      count++;
    }
  }
  return count;
}

// -- Active note ------------------------------------------------------------

void AstraNotesPageModel::SetActiveNote(const std::string& id) {
  if (active_note_id_ == id) {
    return;
  }
  active_note_id_ = id;
  NotifyActiveNoteChanged(id);
}

const std::string& AstraNotesPageModel::GetActiveNoteId() const {
  return active_note_id_;
}

// -- Search -----------------------------------------------------------------

void AstraNotesPageModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  ApplyFilters();
  NotifySearchChanged();
  NotifyNotesChanged();
}

const std::u16string& AstraNotesPageModel::GetSearchQuery() const {
  return search_query_;
}

// -- Filter -----------------------------------------------------------------

void AstraNotesPageModel::SetFilter(AstraNotesFilter filter) {
  if (filter_ == filter) {
    return;
  }
  filter_ = filter;
  ApplyFilters();
  NotifyFilterChanged();
  NotifyNotesChanged();
}

AstraNotesFilter AstraNotesPageModel::GetFilter() const {
  return filter_;
}

// -- Folder filter ----------------------------------------------------------

void AstraNotesPageModel::SetFolderFilter(const std::string& folder) {
  if (folder_filter_ == folder) {
    return;
  }
  folder_filter_ = folder;
  ApplyFilters();
  NotifyFilterChanged();
  NotifyNotesChanged();
}

const std::string& AstraNotesPageModel::GetFolderFilter() const {
  return folder_filter_;
}

// -- Tag filter -------------------------------------------------------------

void AstraNotesPageModel::SetTagFilter(const std::string& tag) {
  if (tag_filter_ == tag) {
    return;
  }
  tag_filter_ = tag;
  ApplyFilters();
  NotifyFilterChanged();
  NotifyNotesChanged();
}

const std::string& AstraNotesPageModel::GetTagFilter() const {
  return tag_filter_;
}

// -- Sorting ----------------------------------------------------------------

void AstraNotesPageModel::SetSortType(AstraNotesSortType sort_type) {
  if (sort_type_ == sort_type) {
    return;
  }
  sort_type_ = sort_type;
  ApplyFilters();
  NotifyNotesChanged();
}

AstraNotesSortType AstraNotesPageModel::GetSortType() const {
  return sort_type_;
}

// -- Folders ----------------------------------------------------------------

const std::vector<AstraNoteFolder>& AstraNotesPageModel::GetFolders() const {
  return folders_;
}

// -- Tags -------------------------------------------------------------------

std::vector<std::string> AstraNotesPageModel::GetAllTags() const {
  std::set<std::string> unique_tags;
  for (const auto& note : all_notes_) {
    for (const auto& tag : note.tags) {
      if (!tag.empty()) {
        unique_tags.insert(tag);
      }
    }
  }
  return std::vector<std::string>(unique_tags.begin(), unique_tags.end());
}

// -- Note CRUD --------------------------------------------------------------

std::string AstraNotesPageModel::AddNote(const std::u16string& title,
                                         const std::u16string& content,
                                         const std::string& folder,
                                         const std::string& color) {
  AstraNoteEntry entry;
  entry.id = GenerateNoteId();
  entry.title = title;
  entry.content = content;
  entry.folder = folder;
  entry.color = color;
  entry.date_created = base::Time::Now();
  entry.date_modified = base::Time::Now();
  entry.is_pinned = false;
  entry.is_archived = false;
  UpdateWordCount(entry);

  all_notes_.push_back(std::move(entry));
  UpdateFolderNoteCounts();
  ApplyFilters();

  const std::string& new_id = all_notes_.back().id;
  NotifyNoteAdded(new_id);
  NotifyNotesChanged();
  return new_id;
}

void AstraNotesPageModel::RemoveNote(const std::string& id) {
  auto it = std::remove_if(all_notes_.begin(), all_notes_.end(),
                           [&id](const AstraNoteEntry& n) {
                             return n.id == id;
                           });
  if (it == all_notes_.end()) {
    return;
  }
  all_notes_.erase(it, all_notes_.end());

  if (active_note_id_ == id) {
    active_note_id_.clear();
  }

  UpdateFolderNoteCounts();
  ApplyFilters();
  NotifyNoteRemoved(id);
  NotifyNotesChanged();
}

void AstraNotesPageModel::UpdateNoteTitle(const std::string& id,
                                          const std::u16string& title) {
  AstraNoteEntry* entry = FindNote(id);
  if (!entry) {
    return;
  }
  entry->title = title;
  entry->date_modified = base::Time::Now();
  ApplyFilters();
  NotifyNoteUpdated(id);
  NotifyNotesChanged();
}

void AstraNotesPageModel::UpdateNoteContent(const std::string& id,
                                            const std::u16string& content) {
  AstraNoteEntry* entry = FindNote(id);
  if (!entry) {
    return;
  }
  entry->content = content;
  entry->date_modified = base::Time::Now();
  UpdateWordCount(*entry);
  ApplyFilters();
  NotifyNoteUpdated(id);
  NotifyNotesChanged();
}

void AstraNotesPageModel::UpdateNoteColor(const std::string& id,
                                          const std::string& color) {
  AstraNoteEntry* entry = FindNote(id);
  if (!entry) {
    return;
  }
  entry->color = color;
  entry->date_modified = base::Time::Now();
  ApplyFilters();
  NotifyNoteUpdated(id);
  NotifyNotesChanged();
}

void AstraNotesPageModel::TogglePin(const std::string& id) {
  AstraNoteEntry* entry = FindNote(id);
  if (!entry) {
    return;
  }
  entry->is_pinned = !entry->is_pinned;
  entry->date_modified = base::Time::Now();
  ApplyFilters();
  NotifyNoteUpdated(id);
  NotifyNotesChanged();
}

void AstraNotesPageModel::ArchiveNote(const std::string& id) {
  AstraNoteEntry* entry = FindNote(id);
  if (!entry || entry->is_archived) {
    return;
  }
  entry->is_archived = true;
  entry->date_modified = base::Time::Now();
  UpdateFolderNoteCounts();
  ApplyFilters();
  NotifyNoteUpdated(id);
  NotifyNotesChanged();
}

void AstraNotesPageModel::UnarchiveNote(const std::string& id) {
  AstraNoteEntry* entry = FindNote(id);
  if (!entry || !entry->is_archived) {
    return;
  }
  entry->is_archived = false;
  entry->date_modified = base::Time::Now();
  UpdateFolderNoteCounts();
  ApplyFilters();
  NotifyNoteUpdated(id);
  NotifyNotesChanged();
}

// -- Tags on notes ----------------------------------------------------------

void AstraNotesPageModel::AddTagToNote(const std::string& id,
                                       const std::string& tag) {
  AstraNoteEntry* entry = FindNote(id);
  if (!entry || tag.empty()) {
    return;
  }
  // Check if tag already exists.
  for (const auto& t : entry->tags) {
    if (t == tag) {
      return;
    }
  }
  entry->tags.push_back(tag);
  entry->date_modified = base::Time::Now();
  ApplyFilters();
  NotifyNoteUpdated(id);
  NotifyNotesChanged();
}

void AstraNotesPageModel::RemoveTagFromNote(const std::string& id,
                                            const std::string& tag) {
  AstraNoteEntry* entry = FindNote(id);
  if (!entry) {
    return;
  }
  auto it = std::remove(entry->tags.begin(), entry->tags.end(), tag);
  if (it == entry->tags.end()) {
    return;
  }
  entry->tags.erase(it, entry->tags.end());
  entry->date_modified = base::Time::Now();
  ApplyFilters();
  NotifyNoteUpdated(id);
  NotifyNotesChanged();
}

// -- Move notes between folders ---------------------------------------------

void AstraNotesPageModel::MoveNoteToFolder(const std::string& id,
                                           const std::string& folder) {
  AstraNoteEntry* entry = FindNote(id);
  if (!entry) {
    return;
  }
  if (entry->folder == folder) {
    return;
  }
  entry->folder = folder;
  entry->date_modified = base::Time::Now();
  UpdateFolderNoteCounts();
  ApplyFilters();
  NotifyNoteUpdated(id);
  NotifyNotesChanged();
}

// -- Folder CRUD ------------------------------------------------------------

std::string AstraNotesPageModel::AddFolder(const std::u16string& name,
                                           const std::string& color) {
  AstraNoteFolder folder;
  folder.id = GenerateFolderId();
  folder.name = name;
  folder.color = color;
  folder.note_count = 0;

  folders_.push_back(std::move(folder));
  const std::string& new_id = folders_.back().id;
  NotifyFolderAdded(new_id);
  return new_id;
}

void AstraNotesPageModel::RemoveFolder(const std::string& id) {
  // Move notes in this folder to the default (empty) folder.
  for (auto& note : all_notes_) {
    // Note: folder field stores folder name, not id, for simplicity.
    // If the folder matching this id has the same name as the note's
    // folder, clear it.
    AstraNoteFolder* folder = FindFolder(id);
    if (folder && note.folder == base::UTF16ToUTF8(folder->name)) {
      note.folder.clear();
    }
  }

  auto it = std::remove_if(folders_.begin(), folders_.end(),
                           [&id](const AstraNoteFolder& f) {
                             return f.id == id;
                           });
  if (it == folders_.end()) {
    return;
  }
  folders_.erase(it, folders_.end());

  if (folder_filter_ == id) {
    folder_filter_.clear();
  }

  UpdateFolderNoteCounts();
  ApplyFilters();
  NotifyFolderRemoved(id);
  NotifyNotesChanged();
}

void AstraNotesPageModel::RenameFolder(const std::string& id,
                                       const std::u16string& name) {
  AstraNoteFolder* folder = FindFolder(id);
  if (!folder) {
    return;
  }
  std::string old_name = base::UTF16ToUTF8(folder->name);
  folder->name = name;
  std::string new_name = base::UTF16ToUTF8(name);

  // Update notes that reference this folder by name.
  for (auto& note : all_notes_) {
    if (note.folder == old_name) {
      note.folder = new_name;
    }
  }

  ApplyFilters();
  NotifyFolderAdded(id);  // No "folder updated" observer method; reuse add.
  NotifyNotesChanged();
}

// -- Sample data ------------------------------------------------------------

void AstraNotesPageModel::PopulateSampleNotes() {
  all_notes_.clear();
  folders_.clear();

  base::Time now = base::Time::Now();

  // Add sample folders.
  struct AstraNoteFolderSample {
    const char* name;
    const char* color;
  };
  AstraNoteFolderSample folder_samples[] = {
      {"Personal", "blue"},
      {"Work", "green"},
      {"Ideas", "yellow"},
      {"Research", "purple"},
      {"Shopping", "pink"},
  };
  for (const auto& f : folder_samples) {
    AddFolder(base::UTF8ToUTF16(f.name), f.color);
  }

  // 20+ sample notes across folders, colors, and tags.
  struct AstraNoteSample {
    const char* title;
    const char* content;
    const char* folder;
    const char* color;
    bool is_pinned;
    bool is_archived;
    int days_ago_modified;
    int hours_offset;
    std::vector<const char*> tags;
    const char* associated_url;
    const char* associated_tab_title;
  };

  AstraNoteSample samples[] = {
      // Pinned work notes
      {"Q3 Planning Notes",
       "Goals for Q3:\n"
       "- Launch notes feature\n"
       "- Improve sidebar UX\n"
       "- Add split view improvements\n"
       "- Performance optimizations",
       "Work", "default", true, false, 0, 10,
       {"planning", "q3", "important"},
       "https://docs.google.com/spreadsheets/d/astra-q3", "Q3 Planning Sheet"},

      {"Design Review Feedback",
       "Review from Tuesday's design review:\n"
       "1. Tab strip needs more contrast\n"
       "2. Sidebar icons should be 20px\n"
       "3. Add hover state to note cards\n"
       "4. Consider a compact mode",
       "Work", "green", true, false, 1, 14,
       {"design", "feedback"},
       "https://www.figma.com/file/astra-design", "Astra Design System"},

      // Regular work notes
      {"Meeting Notes - Sync with Engineering",
       "Attendees: Alex, Jordan, Sam\n"
       "Topics:\n"
       "- Chromium migration progress\n"
       "- Build system issues\n"
       "- Test coverage goals\n"
       "Action items:\n"
       "- Alex: fix GN build on Windows\n"
       "- Jordan: add more unit tests\n"
       "- Sam: review architecture doc",
       "Work", "default", false, false, 2, 11,
       {"meeting", "engineering"},
       "https://meet.google.com/astra-sync", "Engineering Sync"},

      {"Chromium Architecture Research",
       "Key Chromium subsystems to understand:\n"
       " - content module: multi-process architecture\n"
       " - chrome module: browser UI and features\n"
       " - components: reusable feature modules\n"
       " - ui/views: native UI toolkit\n"
       " - Profile: user data container\n"
       " - WebContents: a tab's web state\n"
       " - TabStripModel: tab ordering and state",
       "Research", "blue", false, false, 3, 9,
       {"chromium", "architecture", "research"},
       "https://www.chromium.org/developers/design-documents",
       "Chromium Design Documents"},

      {"API Design - Notes Service",
       "Proposal for AstraNotesService KeyedService:\n"
       "\n"
       "class AstraNotesService : public KeyedService {\n"
       " public:\n"
       "   void AddNote(...);\n"
       "   void UpdateNote(...);\n"
       "   void DeleteNote(...);\n"
       "   std::vector<AstraNoteEntry> GetAllNotes();\n"
       "   void AddObserver(AstraNotesPageObserver*);\n"
       " private:\n"
       "   // leveldb or PrefService backed storage\n"
       "};",
       "Work", "purple", false, false, 4, 16,
       {"api", "design", "engineering"},
       "", ""},

      {"Bug Triage - Week 24",
       "P0 bugs:\n"
       "- Crash on startup with certain profiles\n"
       "- Memory leak in tab hover cards\n"
       "\n"
       "P1 bugs:\n"
       "- Sidebar doesn't resize properly\n"
       "- Bookmark import fails silently\n"
       "- Search field loses focus on tab switch",
       "Work", "yellow", false, false, 5, 13,
       {"bugs", "triage"},
       "", ""},

      // Personal notes
      {"Grocery List",
       "Milk\n"
       "Eggs\n"
       "Bread\n"
       "Avocados\n"
       "Coffee beans\n"
       "Oat milk\n"
       "Berries\n"
       "Almonds",
       "Personal", "yellow", false, false, 0, 8,
       {"shopping", "food"},
       "", ""},

      {"Book Recommendations",
       "Books to read:\n"
       "- Designing Data-Intensive Applications\n"
       "- The Pragmatic Programmer\n"
       "- Clean Code\n"
       "- Refactoring\n"
       "- Domain-Driven Design\n"
       "- The Mythical Man-Month",
       "Personal", "default", false, false, 7, 20,
       {"books", "reading"},
       "https://www.goodreads.com/user/astra", "My Books | Goodreads"},

      {"Workout Routine",
       "Monday: Chest + Triceps\n"
       "  - Bench press 4x8\n"
       "  - Incline dumbbell 3x10\n"
       "  - Tricep pushdowns 3x12\n"
       "\n"
       "Wednesday: Back + Biceps\n"
       "  - Pull-ups 4x10\n"
       "  - Rows 3x10\n"
       "  - Bicep curls 3x12\n"
       "\n"
       "Friday: Legs + Shoulders",
       "Personal", "green", false, false, 1, 7,
       {"fitness", "routine"},
       "", ""},

      {"Recipe - Homemade Pasta",
       "Ingredients:\n"
       "- 2 cups flour\n"
       "- 3 eggs\n"
       "- 1 tbsp olive oil\n"
       "- 1 tsp salt\n"
       "\n"
       "Steps:\n"
       "1. Mound flour on a board\n"
       "2. Make a well, add eggs\n"
       "3. Gradually mix in flour\n"
       "4. Knead for 10 minutes\n"
       "5. Rest 30 minutes\n"
       "6. Roll and cut",
       "Personal", "pink", false, false, 10, 18,
       {"recipe", "food", "cooking"},
       "https://www.example.com/recipes/homemade-pasta", "Homemade Pasta Recipe"},

      {"Trip Planning - Tokyo",
       "Places to visit:\n"
       "- Shibuya Crossing\n"
       "- Senso-ji Temple\n"
       "- Meiji Shrine\n"
       "- Tokyo Tower\n"
       "- Akihabara\n"
       "- TeamLab Borderless\n"
       "\n"
       "Food to try:\n"
       "- Sushi at Tsukiji\n"
       "- Ramen\n"
       "- Yakitori\n"
       "- Matcha desserts",
       "Personal", "blue", true, false, 14, 21,
       {"travel", "japan", "planning"},
       "https://www.example.com/travel/tokyo-guide", "Tokyo Travel Guide"},

      {"Budget Tracker",
       "Monthly budget:\n"
       "- Rent: $1800\n"
       "- Groceries: $400\n"
       "- Dining out: $300\n"
       "- Transportation: $150\n"
       "- Entertainment: $200\n"
       "- Subscriptions: $80\n"
       "- Savings: $1000\n"
       "\n"
       "Total: $3930 / month",
       "Personal", "default", false, false, 20, 12,
       {"finance", "budget"},
       "", ""},

      // Idea notes
      {"Browser Feature Ideas",
       "Ideas for Astra Browser:\n"
       "- AI-powered tab grouping\n"
       "- Note-taking directly on pages\n"
       "- Split view with up to 4 tabs\n"
       "- Workspace profiles with separate cookies\n"
       "- Built-in screenshot annotation\n"
       "- Reading list with offline support\n"
       "- Tab suspender for memory savings\n"
       "- Command palette for quick actions",
       "Ideas", "yellow", true, false, 2, 16,
       {"ideas", "features", "brainstorm"},
       "", ""},

      {"Side Project Ideas",
       "Project ideas:\n"
       "1. A minimalist markdown note-taking app\n"
       "2. A habit tracker with streaks\n"
       "3. A Pomodoro timer with website blocker\n"
       "4. A local-first bookmark manager\n"
       "5. A daily journal with mood tracking\n"
       "\n"
       "Pick one and ship it in 2 weeks.",
       "Ideas", "purple", false, false, 6, 22,
       {"ideas", "projects", "side-project"},
       "", ""},

      {"UI Inspiration",
       "Design patterns to explore:\n"
       "- Glassmorphism for modals\n"
       "- Neumorphism for buttons\n"
       "- Micro-interactions on hover\n"
       "- Subtle parallax effects\n"
       "- Dark mode with warm grays\n"
       "- Staggered animations on load",
       "Ideas", "pink", false, false, 8, 15,
       {"design", "ui", "inspiration"},
       "https://dribbble.com/shots/browser-ui", "Browser UI Shots - Dribbble"},

      {"App Feature - Focus Mode",
       "Focus mode concept:\n"
       "- Single tab view with minimal chrome\n"
       "- Distracting sites blocked temporarily\n"
       "- Timer with 25/5 Pomodoro cycle\n"
       "- Stats: focus sessions per day\n"
       "- Ambient sound option\n"
       "- Integrates with workspaces",
       "Ideas", "green", false, false, 12, 11,
       {"focus", "productivity", "feature-idea"},
       "", ""},

      // Research notes
      {"Web Performance Techniques",
       "Key performance optimization techniques:\n"
       "\n"
       "Loading:\n"
       "- Code splitting and lazy loading\n"
       "- Preload critical resources\n"
       "- HTTP/2 server push\n"
       "- Compress assets (gzip, brotli)\n"
       "\n"
       "Rendering:\n"
       "- Avoid layout thrashing\n"
       "- Use CSS containment\n"
       "- Virtualize long lists\n"
       "- Defer non-critical JS",
       "Research", "default", false, false, 5, 9,
       {"performance", "web", "research"},
       "https://web.dev/vitals/", "Web Vitals | web.dev"},

      {"Accessibility Checklist",
       "A11y checklist for UI components:\n"
       "\n"
       "Keyboard:\n"
       "- All interactive elements reachable via Tab\n"
       "- Focus indicator visible\n"
       "- Arrow keys for lists/menus\n"
       "\n"
       "Screen reader:\n"
       "- Proper ARIA labels\n"
       "- Semantic HTML\n"
       "- Live regions for dynamic content\n"
       "\n"
       "Color:\n"
       "- 4.5:1 contrast for text\n"
       "- Don't rely on color alone",
       "Research", "blue", false, false, 9, 10,
       {"accessibility", "a11y", "checklist"},
       "https://www.w3.org/WAI/", "Web Accessibility Initiative"},

      // Archived note
      {"Old Project Notes - Legacy WebUI",
       "WebUI architecture notes (archived for reference):\n"
       "- Polymer components\n"
       "- JS module system\n"
       "- Content scripts\n"
       "\n"
       "Migrated to direct Views in M2.\n"
       "Kept for historical reference.",
       "Research", "default", false, true, 30, 14,
       {"legacy", "archive", "reference"},
       "", ""},

      {"V1.0 Release Notes",
       "Astra Browser v1.0 released! \n"
       "\n"
       "Features:\n"
       "- Tabbed browsing with multi-process\n"
       "- Bookmark management\n"
       "- History with search\n"
       "- Sidebar with tools\n"
       "- Workspace organization\n"
       "\n"
       "Release date: April 1, 2025\n"
       "Team: 8 engineers, 2 designers",
       "Work", "green", false, true, 60, 16,
       {"release", "archive", "milestone"},
       "", ""},

      // Shopping list note
      {"Gift Ideas - Birthday",
       "Gift ideas for birthday:\n"
       "- New headphones (noise cancelling)\n"
       "- Mechanical keyboard\n"
       "- Desk lamp\n"
       "- Plant for desk\n"
       "- Art print\n"
       "- Coffee subscription\n"
       "- Book about design systems",
       "Shopping", "pink", false, false, 15, 17,
       {"gifts", "shopping", "birthday"},
       "https://www.amazon.com/hz/wishlist", "My Wish List"},

      {"Tech Wishlist 2025",
       "Tech to buy this year:\n"
       "- Ultra-wide monitor\n"
       "- Standing desk converter\n"
       "- Wireless charger\n"
       "- Webcam upgrade\n"
       "- Mechanical numpad\n"
       "- USB-C hub\n"
       "- Laptop stand",
       "Shopping", "purple", false, false, 25, 20,
       {"shopping", "tech", "wishlist"},
       "", ""},

      // Additional note to ensure 20+
      {"Journal - Random Thoughts",
       "Thoughts on productivity systems:\n"
       "\n"
       "I've been trying different productivity methods lately. "
       "GTD feels too rigid, but bullet journaling is too free-form. "
       "Maybe the answer is a hybrid approach?\n"
       "\n"
       "The key insight seems to be: capture everything, review weekly, "
       "and trust the system. The tool doesn't matter as much as the habit.",
       "Personal", "yellow", false, false, 18, 23,
       {"journal", "productivity", "thoughts"},
       "", ""},
  };

  int id_counter = 1;
  for (const auto& sample : samples) {
    AstraNoteEntry entry;
    entry.id = base::StringPrintf("n%03d", id_counter++);
    entry.title = base::UTF8ToUTF16(sample.title);
    entry.content = base::UTF8ToUTF16(sample.content);
    entry.folder = sample.folder;
    entry.color = sample.color;
    entry.is_pinned = sample.is_pinned;
    entry.is_archived = sample.is_archived;
    entry.associated_url = sample.associated_url;
    entry.associated_tab_title = base::UTF8ToUTF16(sample.associated_tab_title);
    entry.workspace = "Default";

    for (const auto* tag : sample.tags) {
      entry.tags.push_back(tag);
    }

    base::Time modified_time = now - base::Days(sample.days_ago_modified) +
                               base::Hours(sample.hours_offset);
    entry.date_modified = modified_time;
    // Created is a bit before modified for realism.
    entry.date_created = modified_time - base::Minutes(30);

    UpdateWordCount(entry);

    all_notes_.push_back(std::move(entry));
  }

  UpdateFolderNoteCounts();
  ApplyFilters();
  NotifyNotesChanged();
}

// -- State ------------------------------------------------------------------

bool AstraNotesPageModel::IsLoading() const {
  return loading_;
}

void AstraNotesPageModel::SetLoading(bool loading) {
  loading_ = loading;
  // TODO(astra): Add a loading state notification method to observer.
}

std::vector<AstraNoteEntry> AstraNotesPageModel::GetFilteredNotes() const {
  std::vector<AstraNoteEntry> result;

  for (const auto& note : all_notes_) {
    if (!MatchesFilter(note)) {
      continue;
    }
    if (!MatchesSearch(note)) {
      continue;
    }
    if (!MatchesFolder(note)) {
      continue;
    }
    if (!MatchesTag(note)) {
      continue;
    }
    result.push_back(note);
  }

  // Sort - note: we sort pinned notes first, then apply sort_type within.
  // TODO(astra): Consider making pinned-first configurable.
  // The SortNotes method sorts, and we separate pinned notes to the top.
  std::vector<AstraNoteEntry> pinned;
  std::vector<AstraNoteEntry> unpinned;
  for (auto& note : result) {
    if (note.is_pinned && !note.is_archived) {
      pinned.push_back(note);
    } else {
      unpinned.push_back(note);
    }
  }

  SortNotes(pinned);
  SortNotes(unpinned);

  std::vector<AstraNoteEntry> combined;
  combined.reserve(pinned.size() + unpinned.size());
  combined.insert(combined.end(), pinned.begin(), pinned.end());
  combined.insert(combined.end(), unpinned.begin(), unpinned.end());

  return combined;
}

// -- Filter helpers ---------------------------------------------------------

bool AstraNotesPageModel::MatchesSearch(const AstraNoteEntry& entry) const {
  if (search_query_.empty()) {
    return true;
  }
  if (CaseInsensitiveContains(entry.title, search_query_)) {
    return true;
  }
  if (CaseInsensitiveContains(entry.content, search_query_)) {
    return true;
  }
  return false;
}

bool AstraNotesPageModel::MatchesFilter(const AstraNoteEntry& entry) const {
  switch (filter_) {
    case AstraNotesFilter::kAll:
      return !entry.is_archived;
    case AstraNotesFilter::kPinned:
      return entry.is_pinned && !entry.is_archived;
    case AstraNotesFilter::kArchived:
      return entry.is_archived;
    case AstraNotesFilter::kRecent:
      // Notes modified in the last 7 days.
      return !entry.is_archived &&
             (base::Time::Now() - entry.date_modified) <= base::Days(7);
  }
  return true;
}

bool AstraNotesPageModel::MatchesFolder(const AstraNoteEntry& entry) const {
  if (folder_filter_.empty()) {
    return true;
  }
  // Match by folder name (folder_filter_ stores name in this simple model).
  return entry.folder == folder_filter_;
}

bool AstraNotesPageModel::MatchesTag(const AstraNoteEntry& entry) const {
  if (tag_filter_.empty()) {
    return true;
  }
  for (const auto& tag : entry.tags) {
    if (tag == tag_filter_) {
      return true;
    }
  }
  return false;
}

// -- Sorting ----------------------------------------------------------------

void AstraNotesPageModel::SortNotes(std::vector<AstraNoteEntry>& notes) const {
  switch (sort_type_) {
    case AstraNotesSortType::kNewestModified:
      std::sort(notes.begin(), notes.end(),
                [](const AstraNoteEntry& a, const AstraNoteEntry& b) {
                  return a.date_modified > b.date_modified;
                });
      break;
    case AstraNotesSortType::kOldestModified:
      std::sort(notes.begin(), notes.end(),
                [](const AstraNoteEntry& a, const AstraNoteEntry& b) {
                  return a.date_modified < b.date_modified;
                });
      break;
    case AstraNotesSortType::kNewestCreated:
      std::sort(notes.begin(), notes.end(),
                [](const AstraNoteEntry& a, const AstraNoteEntry& b) {
                  return a.date_created > b.date_created;
                });
      break;
    case AstraNotesSortType::kTitle:
      std::sort(notes.begin(), notes.end(),
                [](const AstraNoteEntry& a, const AstraNoteEntry& b) {
                  return base::CompareCaseInsensitiveASCII(a.title, b.title) < 0;
                });
      break;
    case AstraNotesSortType::kColor:
      std::sort(notes.begin(), notes.end(),
                [](const AstraNoteEntry& a, const AstraNoteEntry& b) {
                  return a.color < b.color;
                });
      break;
  }
}

void AstraNotesPageModel::ApplyFilters() {
  // filtered_notes_ is not stored; it's computed on demand via
  // GetFilteredNotes(). This method is a no-op in terms of cached state,
  // but kept for API consistency with other page models.
  // In the future, we could cache filtered results here for performance.
}

// -- Word count -------------------------------------------------------------

void AstraNotesPageModel::UpdateWordCount(AstraNoteEntry& entry) const {
  entry.word_count = CountWords(entry.content);
}

// -- Find helpers -----------------------------------------------------------

AstraNoteEntry* AstraNotesPageModel::FindNote(const std::string& id) {
  for (auto& note : all_notes_) {
    if (note.id == id) {
      return &note;
    }
  }
  return nullptr;
}

AstraNoteFolder* AstraNotesPageModel::FindFolder(const std::string& id) {
  for (auto& folder : folders_) {
    if (folder.id == id) {
      return &folder;
    }
  }
  return nullptr;
}

// -- ID generation ----------------------------------------------------------

std::string AstraNotesPageModel::GenerateNoteId() const {
  static int next_id = 1;
  return base::StringPrintf("note_%d", next_id++);
}

std::string AstraNotesPageModel::GenerateFolderId() const {
  static int next_id = 1;
  return base::StringPrintf("folder_%d", next_id++);
}

// -- Folder note counts -----------------------------------------------------

void AstraNotesPageModel::UpdateFolderNoteCounts() {
  for (auto& folder : folders_) {
    int count = 0;
    for (const auto& note : all_notes_) {
      if (!note.is_archived && note.folder == base::UTF16ToUTF8(folder.name)) {
        count++;
      }
    }
    folder.note_count = count;
  }
}

// -- Notify helpers ---------------------------------------------------------

void AstraNotesPageModel::NotifyNotesChanged() {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnNotesChanged();
  }
}

void AstraNotesPageModel::NotifyNoteAdded(const std::string& id) {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnNoteAdded(id);
  }
}

void AstraNotesPageModel::NotifyNoteRemoved(const std::string& id) {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnNoteRemoved(id);
  }
}

void AstraNotesPageModel::NotifyNoteUpdated(const std::string& id) {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnNoteUpdated(id);
  }
}

void AstraNotesPageModel::NotifyFolderAdded(const std::string& id) {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnFolderAdded(id);
  }
}

void AstraNotesPageModel::NotifyFolderRemoved(const std::string& id) {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnFolderRemoved(id);
  }
}

void AstraNotesPageModel::NotifyActiveNoteChanged(const std::string& id) {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnActiveNoteChanged(id);
  }
}

void AstraNotesPageModel::NotifySearchChanged() {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnSearchChanged(search_query_);
  }
}

void AstraNotesPageModel::NotifyFilterChanged() {
  for (AstraNotesPageObserver& observer : observers_) {
    observer.OnFilterChanged();
  }
}

}  // namespace astra
