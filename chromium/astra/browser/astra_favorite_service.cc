#include "astra/browser/astra_favorite_service.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

#include "astra/browser/astra_tab_features.h"

namespace astra {

namespace {

// Id of the root favorites folder that always exists.
constexpr char kRootFolderId[] = "root";

// Display name of the root favorites folder.
constexpr char kRootFolderName[] = "Favorites";

// Pref key for the folders list.
constexpr char kPrefFolders[] = "astra.favorites.folders";

// Serialization keys for folder dict entries.
constexpr char kFolderIdKey[] = "id";
constexpr char kFolderNameKey[] = "name";
constexpr char kFolderParentIdKey[] = "parent_id";
constexpr char kFolderOrderIndexKey[] = "order_index";
constexpr char kFolderIsExpandedKey[] = "is_expanded";
constexpr char kFolderCreatedTimeKey[] = "created_time";
constexpr char kFolderIsRootKey[] = "is_root";
constexpr char kFolderColorKey[] = "color";
constexpr char kFolderIconKey[] = "icon";
constexpr char kFolderDescriptionKey[] = "description";

// Default folder color (blue).
constexpr char kDefaultFolderColor[] = "#5B8FF9";

// Folder color palette — 10 curated preset colors.
constexpr const char* kFolderColorPalette[] = {
    "#5B8FF9",  // Blue
    "#5AD8A6",  // Green
    "#F6BD16",  // Yellow
    "#E8684A",  // Red
    "#5D7092",  // Slate
    "#6DC8EC",  // Cyan
    "#9270CA",  // Purple
    "#FF9D4D",  // Orange
    "#FF99C3",  // Pink
    "#3E5C76",  // Dark Blue
};

}  // namespace

// ---------------------------------------------------------------------------
// AstraFavoriteService
// ---------------------------------------------------------------------------

AstraFavoriteService::AstraFavoriteService(Profile* profile)
    : profile_(profile) {
  // Load persisted folder state from the profile's PrefService.
  // If no persisted state exists (fresh profile), LoadFromPrefs falls back
  // to creating the root folder.
  // Chromium component: PrefService + profile initialization.
  LoadFromPrefs();
  EnsureRootFolder();
}

AstraFavoriteService::~AstraFavoriteService() = default;

void AstraFavoriteService::Shutdown() {
  // KeyedService shutdown: clear all observer references and drop profile
  // pointer before the profile goes away.  Observers should have removed
  // themselves already (typically in their own Shutdown), but we clear the
  // list here to be safe and to catch dangling observer bugs in debug builds.
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers ---------------------------------------------------------------

void AstraFavoriteService::AddObserver(
    AstraFavoriteServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraFavoriteService::RemoveObserver(
    AstraFavoriteServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Folder query ------------------------------------------------------------

const AstraFavoriteFolder* AstraFavoriteService::GetFolder(
    const std::string& id) const {
  auto it = base::ranges::find(folders_, id, &AstraFavoriteFolder::id);
  return it == folders_.end() ? nullptr : &(*it);
}

const AstraFavoriteFolder& AstraFavoriteService::root_folder() const {
  const AstraFavoriteFolder* folder = GetFolder(kRootFolderId);
  // Invariant: the root folder always exists.
  CHECK(folder);
  return *folder;
}

const std::string& AstraFavoriteService::GetRootFolderId() const {
  // The root folder id is stable — it's always "root".
  // We return a const ref to the string literal via a local static to match
  // the expected string lifetime.
  static const base::NoDestructor<std::string> kId(kRootFolderId);
  return *kId;
}

std::vector<const AstraFavoriteFolder*> AstraFavoriteService::GetChildFolders(
    const std::string& parent_id) const {
  std::vector<const AstraFavoriteFolder*> children;
  const std::string& effective_parent =
      parent_id.empty() ? kRootFolderId : parent_id;

  for (const auto& folder : folders_) {
    if (folder.parent_id == effective_parent) {
      children.push_back(&folder);
    }
  }
  return children;
}

// -- Folder mutation ---------------------------------------------------------

void AstraFavoriteService::EnsureRootFolder() {
  if (GetFolder(kRootFolderId)) {
    return;
  }

  AstraFavoriteFolder root;
  root.id = kRootFolderId;
  root.name = kRootFolderName;
  root.parent_id = std::string();
  root.order_index = 0;
  root.is_expanded = true;
  root.created_time = base::Time::Now();
  root.is_root = true;
  root.color = kDefaultFolderColor;

  folders_.push_back(std::move(root));
  SortFolders();
}

std::string AstraFavoriteService::AddFolder(
    const std::string& name,
    const std::string& parent_id) {
  // Validate parent: if specified, must exist.  If empty, parent is root.
  const std::string& effective_parent =
      parent_id.empty() ? kRootFolderId : parent_id;
  if (!GetFolder(effective_parent)) {
    return std::string();  // Parent not found.
  }

  // Generate a unique id.
  // TODO(astra): Use base::Token or base::UnguessableToken for IDs.
  static int folder_counter = 0;
  std::string new_id = "folder-" + std::to_string(++folder_counter);

  // Determine the next order_index at the end of the parent's children.
  size_t max_index = 0;
  for (const auto& folder : folders_) {
    if (folder.parent_id == effective_parent &&
        folder.order_index >= max_index) {
      max_index = folder.order_index + 1;
    }
  }

  AstraFavoriteFolder folder;
  folder.id = new_id;
  folder.name = name;
  folder.parent_id = effective_parent;
  folder.order_index = max_index;
  folder.is_expanded = true;
  folder.created_time = base::Time::Now();
  folder.is_root = false;
  folder.color = kDefaultFolderColor;

  folders_.push_back(std::move(folder));
  SortFolders();

  // Find the newly added folder in the sorted list for the notification.
  const AstraFavoriteFolder* added = GetFolder(new_id);
  DCHECK(added);
  for (auto& observer : observers_) {
    observer.OnFolderAdded(*added);
  }

  SaveToPrefs();
  return new_id;
}

bool AstraFavoriteService::RenameFolder(const std::string& id,
                                        const std::string& name) {
  // Cannot rename the root folder.
  if (id == kRootFolderId) {
    return false;
  }

  AstraFavoriteFolder* folder = FindFolder(id);
  if (!folder) {
    return false;
  }

  if (folder->name == name) {
    return true;
  }

  folder->name = name;

  for (auto& observer : observers_) {
    observer.OnFolderRenamed(id, name);
  }

  SaveToPrefs();
  return true;
}

bool AstraFavoriteService::DeleteFolder(const std::string& id) {
  // The root folder cannot be deleted.
  if (id == kRootFolderId) {
    return false;
  }

  AstraFavoriteFolder* folder = FindFolder(id);
  if (!folder) {
    return false;
  }

  // Parent id to reassign favorites and child folders to.
  std::string parent_id = folder->parent_id;
  if (parent_id.empty()) {
    parent_id = kRootFolderId;
  }

  // Collect all descendant folders (children, grandchildren, etc.) that
  // will also be removed.  We delete the subtree.
  //
  // Design note: when a folder is deleted, all nested subfolders are also
  // deleted and their favorites are moved up to the target parent folder
  // (the parent of the deleted folder).  This matches the user expectation
  // that deleting a folder "flattens" its contents into the parent.
  //
  // TODO(astra): Consider a "reparent children" mode vs "delete subtree"
  // mode.  For now we delete the subtree and move all favorites to the
  // parent of the deleted folder.
  std::vector<std::string> descendant_ids;
  CollectDescendantIds(id, descendant_ids);

  // ------------------------------------------------------------------
  // Favorite reassignment note:
  //
  // Deleting a folder removes only the folder metadata.  Favorites (tabs
  // whose AstraTabFeatures is_favorite() is true and favorite_folder_id
  // matches |id| or any descendant id) must be reassigned to the parent
  // folder so they remain visible in the sidebar.
  //
  // This service does NOT own TabStripModel or WebContents, so it cannot
  // iterate tabs directly.  The reassignment would happen at the layer
  // that bridges folder metadata to tab features — the sidebar/tab-
  // projection layer handles the AstraTabFeatures update, or we add a
  // helper that iterates all browsers' TabStripModels.
  //
  // TODO(astra): Reassign orphan favorite folder_ids to the parent folder
  // when a folder is deleted.  This requires iterating all WebContents in
  // the profile's Browsers' TabStripModels and checking AstraTabFeatures.
  // Patch point: chrome/browser/ui/browser_list.h + TabStripModel iteration.
  // ------------------------------------------------------------------

  // Remove all descendant folders and the target folder itself.
  // We process in reverse order of depth (deepest first) so that when we
  // erase from the vector, indices of already-processed items don't shift.
  // Easier approach: collect all ids to remove, then erase them.
  std::vector<std::string> all_ids_to_remove = descendant_ids;
  all_ids_to_remove.push_back(id);

  // Remove folders from back to front to avoid iterator invalidation.
  // Actually, simpler: build a new list.
  std::vector<AstraFavoriteFolder> new_folders;
  for (auto& f : folders_) {
    if (base::ranges::find(all_ids_to_remove, f.id) == all_ids_to_remove.end()) {
      new_folders.push_back(std::move(f));
    }
  }
  folders_ = std::move(new_folders);

  // Notify observers of all removed folders (deepest first so UI can
  // remove children before parents if needed).
  for (const auto& removed_id : all_ids_to_remove) {
    for (auto& observer : observers_) {
      observer.OnFolderRemoved(removed_id);
    }
  }

  SortFolders();

  SaveToPrefs();
  return true;
}

bool AstraFavoriteService::ReorderFolders(
    const std::string& parent_id,
    const std::vector<std::string>& ordered_ids) {
  const std::string& effective_parent =
      parent_id.empty() ? kRootFolderId : parent_id;

  // Validate: all ids must be direct children of parent and all children
  // must be included.
  std::vector<const AstraFavoriteFolder*> children =
      GetChildFolders(effective_parent);
  if (ordered_ids.size() != children.size()) {
    return false;
  }

  for (const auto& id : ordered_ids) {
    bool found = false;
    for (const auto* child : children) {
      if (child->id == id) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }

  // Assign order_index based on position in the ordered list.
  for (size_t i = 0; i < ordered_ids.size(); ++i) {
    AstraFavoriteFolder* folder = FindFolder(ordered_ids[i]);
    DCHECK(folder);
    folder->order_index = i;
  }

  SortFolders();

  for (auto& observer : observers_) {
    observer.OnFoldersReordered();
  }

  SaveToPrefs();
  return true;
}

bool AstraFavoriteService::ToggleFolderExpanded(const std::string& id) {
  AstraFavoriteFolder* folder = FindFolder(id);
  if (!folder) {
    return false;
  }

  bool was_expanded = folder->is_expanded;
  folder->is_expanded = !was_expanded;

  if (was_expanded) {
    for (auto& observer : observers_) {
      observer.OnFolderCollapsed(id);
    }
  } else {
    for (auto& observer : observers_) {
      observer.OnFolderExpanded(id);
    }
  }

  SaveToPrefs();
  return true;
}

bool AstraFavoriteService::ExpandFolder(const std::string& id) {
  AstraFavoriteFolder* folder = FindFolder(id);
  if (!folder) {
    return false;
  }
  if (folder->is_expanded) {
    return false;  // Already expanded — no change.
  }

  folder->is_expanded = true;

  for (auto& observer : observers_) {
    observer.OnFolderExpanded(id);
  }

  SaveToPrefs();
  return true;
}

bool AstraFavoriteService::CollapseFolder(const std::string& id) {
  AstraFavoriteFolder* folder = FindFolder(id);
  if (!folder) {
    return false;
  }
  if (!folder->is_expanded) {
    return false;  // Already collapsed — no change.
  }

  folder->is_expanded = false;

  for (auto& observer : observers_) {
    observer.OnFolderCollapsed(id);
  }

  SaveToPrefs();
  return true;
}

bool AstraFavoriteService::SetFolderColor(const std::string& id,
                                          const std::string& color) {
  AstraFavoriteFolder* folder = FindFolder(id);
  if (!folder) {
    return false;
  }
  if (folder->color == color) {
    return true;  // Same color — no change, but folder exists.
  }

  folder->color = color;

  for (auto& observer : observers_) {
    observer.OnFolderColorChanged(id, color);
  }

  SaveToPrefs();
  return true;
}

// static
std::vector<std::string> AstraFavoriteService::GetFolderColorPalette() {
  std::vector<std::string> palette;
  for (const auto& color : kFolderColorPalette) {
    palette.push_back(color);
  }
  return palette;
}

const AstraFavoriteFolder* AstraFavoriteService::FindFolderByName(
    const std::string& name) const {
  for (const auto& folder : folders_) {
    if (base::EqualsCaseInsensitiveASCII(folder.name, name)) {
      return &folder;
    }
  }
  return nullptr;
}

size_t AstraFavoriteService::GetFolderDepth(const std::string& id) const {
  size_t depth = 0;
  std::string current_id = id;

  while (!current_id.empty() && current_id != kRootFolderId) {
    const AstraFavoriteFolder* folder = GetFolder(current_id);
    if (!folder) {
      break;
    }
    current_id = folder->parent_id;
    depth++;
  }

  return depth;
}

std::vector<std::string> AstraFavoriteService::GetAllDescendantIds(
    const std::string& id) const {
  std::vector<std::string> ids;
  CollectDescendantIds(id, ids);
  return ids;
}

// -- Bulk operations ---------------------------------------------------------

size_t AstraFavoriteService::ExpandAllFolders() {
  size_t changed = 0;

  for (auto& folder : folders_) {
    if (!folder.is_expanded) {
      folder.is_expanded = true;
      changed++;
      for (auto& observer : observers_) {
        observer.OnFolderExpanded(folder.id);
      }
    }
  }

  if (changed > 0) {
    SaveToPrefs();
  }

  return changed;
}

size_t AstraFavoriteService::CollapseAllFolders() {
  size_t changed = 0;

  for (auto& folder : folders_) {
    // Root folder is always considered "expanded" (it's the top level).
    if (folder.is_root) {
      continue;
    }
    if (folder.is_expanded) {
      folder.is_expanded = false;
      changed++;
      for (auto& observer : observers_) {
        observer.OnFolderCollapsed(folder.id);
      }
    }
  }

  if (changed > 0) {
    SaveToPrefs();
  }

  return changed;
}

// -- Import / Export ---------------------------------------------------------

std::string AstraFavoriteService::ExportFoldersJson() const {
  // Simple JSON string generation.
  // TODO(astra): Use base::JSONWriter for proper serialization.
  std::string json = "[";
  bool first = true;
  for (const auto& folder : folders_) {
    if (!first) {
      json += ",";
    }
    first = false;
    json += "{";
    json += "\"id\":\"" + folder.id + "\",";
    json += "\"name\":\"" + folder.name + "\",";
    json += "\"parent_id\":\"" + folder.parent_id + "\",";
    json += "\"order_index\":" + std::to_string(folder.order_index) + ",";
    json += "\"is_expanded\":" + std::string(folder.is_expanded ? "true" : "false") + ",";
    json += "\"is_root\":" + std::string(folder.is_root ? "true" : "false") + ",";
    json += "\"color\":\"" + folder.color + "\",";
    if (folder.icon) {
      json += "\"icon\":\"" + *folder.icon + "\",";
    }
    json += "\"description\":\"" + folder.description + "\"";
    json += "}";
  }
  json += "]";
  return json;
}

size_t AstraFavoriteService::ImportFoldersJson(const std::string& json,
                                               bool merge) {
  // Simple JSON parsing.
  // TODO(astra): Use base::JSONReader for proper parsing.
  //
  // For the overlay skeleton, this is a basic parser that handles a
  // list-of-dicts format.  Real implementation should use base::JSONReader.
  if (!merge) {
    // Remove all non-root folders.
    std::vector<AstraFavoriteFolder> kept;
    for (auto& folder : folders_) {
      if (folder.is_root) {
        kept.push_back(std::move(folder));
      }
    }
    folders_ = std::move(kept);
  }

  size_t imported = 0;
  // Very basic parser: find {...} blocks and extract fields.
  // This is intentionally minimal for the overlay skeleton.
  // TODO(astra): Replace with base::JSONReader.
  size_t pos = 0;
  while (pos < json.size()) {
    size_t obj_start = json.find('{', pos);
    if (obj_start == std::string::npos) {
      break;
    }
    size_t obj_end = json.find('}', obj_start);
    if (obj_end == std::string::npos) {
      break;
    }

    std::string obj_str = json.substr(obj_start, obj_end - obj_start + 1);

    // Extract fields.
    auto extract_string = [&](const std::string& key) -> std::string {
      std::string search = "\"" + key + "\":";
      size_t key_pos = obj_str.find(search);
      if (key_pos == std::string::npos) {
        return "";
      }
      size_t val_start = obj_str.find('"', key_pos + search.size());
      if (val_start == std::string::npos) {
        return "";
      }
      size_t val_end = obj_str.find('"', val_start + 1);
      if (val_end == std::string::npos) {
        return "";
      }
      return obj_str.substr(val_start + 1, val_end - val_start - 1);
    };

    auto extract_bool = [&](const std::string& key) -> bool {
      std::string search = "\"" + key + "\":";
      size_t key_pos = obj_str.find(search);
      if (key_pos == std::string::npos) {
        return false;
      }
      return obj_str.compare(key_pos + search.size(), 4, "true") == 0;
    };

    std::string id = extract_string("id");
    std::string name = extract_string("name");
    std::string parent_id = extract_string("parent_id");
    std::string color = extract_string("color");
    std::string description = extract_string("description");

    if (id.empty() || name.empty()) {
      pos = obj_end + 1;
      continue;
    }

    // Skip if this is the root folder (already exists).
    if (extract_bool("is_root") || id == kRootFolderId) {
      pos = obj_end + 1;
      continue;
    }

    // Skip if id already exists and we're merging.
    if (merge && GetFolder(id)) {
      pos = obj_end + 1;
      continue;
    }

    AstraFavoriteFolder folder;
    folder.id = id;
    folder.name = name;
    folder.parent_id = parent_id.empty() ? kRootFolderId : parent_id;
    folder.is_expanded = true;
    folder.created_time = base::Time::Now();
    folder.is_root = false;
    folder.color = color.empty() ? kDefaultFolderColor : color;
    folder.description = description;

    // Set order_index to end of parent's children.
    size_t max_index = 0;
    for (const auto& f : folders_) {
      if (f.parent_id == folder.parent_id && f.order_index >= max_index) {
        max_index = f.order_index + 1;
      }
    }
    folder.order_index = max_index;

    folders_.push_back(std::move(folder));
    imported++;

    pos = obj_end + 1;
  }

  if (imported > 0) {
    SortFolders();
    SaveToPrefs();
    for (auto& observer : observers_) {
      observer.OnFoldersReordered();
    }
  }

  return imported;
}

// -- Favorite operations -----------------------------------------------------

bool AstraFavoriteService::MoveFavoriteToFolder(
    content::WebContents* web_contents,
    const std::string& folder_id) {
  if (!web_contents) {
    return false;
  }

  // Validate the target folder exists.
  if (!GetFolder(folder_id) && folder_id != kRootFolderId) {
    return false;
  }
  // Root always exists via invariant, but check anyway for safety.
  DCHECK(GetFolder(kRootFolderId));

  const std::string& effective_folder_id =
      folder_id.empty() ? kRootFolderId : folder_id;

  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);
  std::string old_folder_id = features->favorite_folder_id();

  if (old_folder_id == effective_folder_id) {
    // Already in this folder — still ensure is_favorite is true.
    if (!features->is_favorite()) {
      features->set_is_favorite(true);
    }
    return true;
  }

  features->set_favorite_folder_id(effective_folder_id);
  features->set_is_favorite(true);

  // TODO(astra): Update favorite_order_index when moving.  For now the
  // caller or UI can adjust ordering separately.

  for (auto& observer : observers_) {
    observer.OnFavoriteMoved(web_contents, old_folder_id, effective_folder_id);
  }

  return true;
}

bool AstraFavoriteService::ReorderFavoritesInFolder(
    const std::string& folder_id,
    const std::vector<content::WebContents*>& ordered_tabs) {
  // Validate the target folder exists.
  if (!GetFolder(folder_id) && folder_id != kRootFolderId) {
    return false;
  }

  const std::string& effective_folder_id =
      folder_id.empty() ? kRootFolderId : folder_id;

  // Update favorite_order_index on each tab's AstraTabFeatures.
  for (size_t i = 0; i < ordered_tabs.size(); ++i) {
    content::WebContents* web_contents = ordered_tabs[i];
    if (!web_contents) {
      continue;
    }
    AstraTabFeatures* features =
        AstraTabFeatures::GetOrCreateForWebContents(web_contents);
    if (features->is_favorite() &&
        features->favorite_folder_id() == effective_folder_id) {
      features->set_favorite_order_index(i);
    }
  }

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnFavoritesReordered(effective_folder_id);
  }

  // TODO(astra): Persist the reorder via PrefService once persistence
  // is wired up for favorite order. Note: favorite_order_index is on
  // AstraTabFeatures (WebContentsUserData), not in prefs, so it travels
  // with the tab through session restore.
  //
  // TODO(astra): This is a partial implementation. For a full reorder,
  // we need to iterate ALL tabs in the profile (across all browser
  // windows) that are in this folder, not just the ones passed in.
  // The ordered_tabs parameter is the subset of favorites being reordered.
  // We should also handle tabs not in the ordered list by shifting their
  // indices appropriately.

  return true;
}

size_t AstraFavoriteService::GetFavoriteCountInFolder(
    const std::string& folder_id) const {
  // TODO(astra): Implement by iterating all Browser instances for this
  // profile and counting tabs where AstraTabFeatures::is_favorite() and
  // favorite_folder_id() match.
  //
  // Chromium subsystem: BrowserList + TabStripModel.
  // Patch point: none needed — use public BrowserList and TabStripModel APIs.
  //
  // For now we return 0 as a stub.
  return 0;
}

// -- Private helpers ---------------------------------------------------------

AstraFavoriteFolder* AstraFavoriteService::FindFolder(const std::string& id) {
  auto it = base::ranges::find(folders_, id, &AstraFavoriteFolder::id);
  return it == folders_.end() ? nullptr : &(*it);
}

void AstraFavoriteService::SortFolders() {
  // Sort: root folder first, then all other folders grouped by parent
  // and ordered by order_index within each parent group.
  //
  // We use a two-level sort: first by a depth-like key (parent chain),
  // then by order_index.  For simplicity and correctness with nested
  // folders, we compute a sort key string like "0000.0001.0002" where
  // each segment is the order_index padded at each level.
  //
  // For a flat list (no nesting), just sorting by order_index works.
  // For nested folders, we need a more stable sort.  We use a breadth-
  // first approach: root first, then children in order, then their children.

  // Build a map from id to its sort rank for deterministic ordering.
  // We do a BFS from root: root = rank 0, then each child gets the
  // next available rank in order_index order, recursively.
  //
  // This gives us a pre-order traversal order which is a reasonable
  // presentation order for a tree flattened into a list.

  std::vector<std::string> ordered_ids;
  ordered_ids.reserve(folders_.size());

  // Helper: recursively add children in order_index order.
  std::function<void(const std::string&)> add_children =
      [&](const std::string& parent_id) {
        // Collect children of this parent.
        std::vector<AstraFavoriteFolder*> children;
        for (auto& folder : folders_) {
          if (folder.parent_id == parent_id) {
            children.push_back(&folder);
          }
        }
        // Sort by order_index.
        base::ranges::sort(children, std::less<>(),
                           &AstraFavoriteFolder::order_index);
        // Add in order, then recurse.
        for (auto* child : children) {
          ordered_ids.push_back(child->id);
          add_children(child->id);
        }
      };

  // Root always comes first.
  ordered_ids.push_back(kRootFolderId);
  add_children(kRootFolderId);

  // Reorder folders_ to match ordered_ids.
  std::vector<AstraFavoriteFolder> sorted_folders;
  sorted_folders.reserve(folders_.size());
  for (const auto& id : ordered_ids) {
    auto it = base::ranges::find(folders_, id, &AstraFavoriteFolder::id);
    if (it != folders_.end()) {
      sorted_folders.push_back(std::move(*it));
    }
  }

  // Add any remaining folders (orphans or invalid ones) at the end.
  for (auto& folder : folders_) {
    if (!folder.id.empty()) {
      sorted_folders.push_back(std::move(folder));
    }
  }

  folders_ = std::move(sorted_folders);
}

void AstraFavoriteService::CollectDescendantIds(
    const std::string& folder_id,
    std::vector<std::string>& out_ids) const {
  for (const auto& folder : folders_) {
    if (folder.parent_id == folder_id) {
      out_ids.push_back(folder.id);
      CollectDescendantIds(folder.id, out_ids);
    }
  }
}

void AstraFavoriteService::LoadFromPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  const PrefService* prefs = profile_->GetPrefs();
  const base::Value::List& folder_list = prefs->GetList(kPrefFolders);

  if (folder_list.empty()) {
    // No persisted state — fresh profile.  EnsureRootFolder() will handle it.
    return;
  }

  folders_.clear();

  for (const auto& item : folder_list) {
    if (!item.is_dict()) {
      continue;
    }
    const base::Value::Dict& dict = item.GetDict();

    AstraFavoriteFolder folder;

    const std::string* id = dict.FindString(kFolderIdKey);
    if (!id || id->empty()) {
      continue;
    }
    folder.id = *id;

    const std::string* name = dict.FindString(kFolderNameKey);
    folder.name = name ? *name : std::string();

    const std::string* parent_id = dict.FindString(kFolderParentIdKey);
    folder.parent_id = parent_id ? *parent_id : std::string();

    absl::optional<int> order_index = dict.FindInt(kFolderOrderIndexKey);
    folder.order_index = order_index.value_or(0);

    absl::optional<bool> is_expanded = dict.FindBool(kFolderIsExpandedKey);
    folder.is_expanded = is_expanded.value_or(true);

    const std::string* created_time_str = dict.FindString(kFolderCreatedTimeKey);
    if (created_time_str && !created_time_str->empty()) {
      int64_t time_us;
      if (base::StringToInt64(*created_time_str, &time_us)) {
        folder.created_time = base::Time::FromDeltaSinceWindowsEpoch(
            base::Microseconds(time_us));
      }
    }
    if (folder.created_time.is_null()) {
      folder.created_time = base::Time::Now();
    }

    absl::optional<bool> is_root = dict.FindBool(kFolderIsRootKey);
    folder.is_root = is_root.value_or(false);

    const std::string* color = dict.FindString(kFolderColorKey);
    folder.color = color ? *color : std::string(kDefaultFolderColor);

    const std::string* icon = dict.FindString(kFolderIconKey);
    if (icon && !icon->empty()) {
      folder.icon = *icon;
    }

    const std::string* description = dict.FindString(kFolderDescriptionKey);
    folder.description = description ? *description : std::string();

    folders_.push_back(std::move(folder));
  }

  SortFolders();
}

void AstraFavoriteService::SaveToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  base::Value::List list;
  for (const auto& folder : folders_) {
    base::Value::Dict dict;

    dict.Set(kFolderIdKey, folder.id);
    dict.Set(kFolderNameKey, folder.name);
    dict.Set(kFolderParentIdKey, folder.parent_id);
    dict.Set(kFolderOrderIndexKey, static_cast<int>(folder.order_index));
    dict.Set(kFolderIsExpandedKey, folder.is_expanded);
    dict.Set(
        kFolderCreatedTimeKey,
        base::NumberToString(folder.created_time.ToDeltaSinceWindowsEpoch()
                                 .InMicroseconds()));
    dict.Set(kFolderIsRootKey, folder.is_root);
    dict.Set(kFolderColorKey, folder.color);
    if (folder.icon) {
      dict.Set(kFolderIconKey, *folder.icon);
    }
    dict.Set(kFolderDescriptionKey, folder.description);

    list.Append(std::move(dict));
  }

  prefs->SetList(kPrefFolders, std::move(list));
}

// ---------------------------------------------------------------------------
// AstraFavoriteServiceFactory
// ---------------------------------------------------------------------------

// static
AstraFavoriteService* AstraFavoriteServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AstraFavoriteService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AstraFavoriteServiceFactory* AstraFavoriteServiceFactory::GetInstance() {
  static base::NoDestructor<AstraFavoriteServiceFactory> instance;
  return instance.get();
}

// static
void AstraFavoriteServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Favorite folders list — list of dictionaries with folder metadata.
  // Default: empty list (the root folder is created at service init time).
  //
  // These prefs live on the regular profile.  Incognito redirects to the
  // original profile (see factory constructor), so incognito windows share
  // the same favorite folder set — only browsing context is isolated.
  //
  // Chromium component: PrefService / PrefRegistry.
  // Patch point: profile keyed service registration (chrome/browser/profiles/).
  registry->RegisterListPref(kPrefFolders);
}

AstraFavoriteServiceFactory::AstraFavoriteServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraFavoriteService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito uses kRedirectedToOriginal because favorite
              // folder metadata is product-level state, not browsing
              // context state.  An incognito window is still the same
              // user with the same favorite folders — only the browsing
              // session is isolated.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest sessions have no original profile to redirect to,
              // so they get their own ephemeral favorite service instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile has no user favorites.
              .Build()) {}

AstraFavoriteServiceFactory::~AstraFavoriteServiceFactory() = default;

KeyedService*
AstraFavoriteServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return new AstraFavoriteService(Profile::FromBrowserContext(context));
}

}  // namespace astra
