#ifndef ASTRA_BROWSER_ASTRA_FAVORITE_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_FAVORITE_SERVICE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace astra {

// =========================================================================
// AstraFavoriteFolder — favorite folder metadata
// =========================================================================
//
// Favorite folders are an Astra-specific sidebar organization concept.
// They are NOT Chrome bookmarks — bookmarks are a separate Chromium feature
// that Astra reuses as-is.  Favorites mark which tabs appear in the
// sidebar's favorites section; folders organize those favorites into
// groups (like Arc's favorites bar folders).
//
// Truth model:
//   - Folder definitions (name, hierarchy, order) live in this service
//     (profile-scoped, persisted via PrefService).
//   - "Is this tab a favorite?" and "which folder is it in?" live on
//     AstraTabFeatures (WebContentsUserData attached to each tab).
//   - The sidebar projects favorites by iterating TabStripModel, checking
//     AstraTabFeatures::is_favorite(), and grouping by
//     AstraTabFeatures::favorite_folder_id().
//
// Chromium subsystems reused:
//   - PrefService (for folder persistence).
//   - TabStripModel / WebContents (tabs are truth; favorites are metadata).
//   - ProfileKeyedServiceFactory pattern.
//
// Chromium patch points:
//   - Session restore: to restore AstraTabFeatures favorite state alongside
//     tab restore.  Patch point: chrome/browser/sessions/session_restore.cc
//     or equivalent tab restore path.
//   - Profile keyed service registration: to wire up the factory.
//     Patch point: chrome/browser/profiles/profile_keyed_service_factory*.
// =========================================================================

struct AstraFavoriteFolder {
  // Unique identifier for this folder.  "root" is the top-level folder
  // that always exists and cannot be deleted.
  std::string id;

  // Display name shown in the sidebar.
  std::string name;

  // Id of the parent folder, or empty if this is a top-level folder
  // (direct child of root).  Root itself has an empty parent_id.
  std::string parent_id;

  // 0-based position within the parent folder's children list.
  size_t order_index = 0;

  // Whether the folder is expanded in the sidebar (shows its children
  // and favorite tabs).
  bool is_expanded = true;

  // Creation timestamp.
  base::Time created_time;

  // Whether this is the root folder (immutable).
  bool is_root = false;

  // Accent color for the folder (hex string, e.g. "#5B8FF9").
  // Empty string means use the default color.
  // Used as a visual hint in the sidebar and workspace switcher.
  std::string color;

  // Optional icon identifier for the folder (e.g. a Chrome side panel
  // icon name or a custom emoji).  Empty if no custom icon.
  std::optional<std::string> icon;

  // Description of the folder's purpose.  Empty if not set.
  std::string description;
};

// =========================================================================
// AstraFavoriteServiceObserver
// =========================================================================
//
// Observer interface for UI layers (sidebar, favorites view) to react to
// folder and favorite changes.  UI must never be the source of truth —
// AstraFavoriteService and AstraTabFeatures are.
//
// Favorite state changes (is_favorite flag on a tab) are observed
// indirectly: the sidebar re-reads AstraTabFeatures when notified of a
// folder change, or via TabStripModelObserver.  This avoids adding an
// observer on every WebContents.
// TODO(astra): Evaluate adding an AstraTabFeatures change notification
// mechanism if the sidebar needs fine-grained favorite state updates.
// Patch point: TabStripModelObserver with Astra hook.
// =========================================================================

class AstraFavoriteServiceObserver : public base::CheckedObserver {
 public:
  // Called after a new folder is created.
  virtual void OnFolderAdded(const AstraFavoriteFolder& folder) {}

  // Called after a folder is removed.  Favorites that were in the deleted
  // folder have already been reassigned to the parent folder (or root).
  virtual void OnFolderRemoved(const std::string& folder_id) {}

  // Called after a folder is renamed.
  virtual void OnFolderRenamed(const std::string& folder_id,
                               const std::string& new_name) {}

  // Called after folders are reordered (within the same parent).
  virtual void OnFoldersReordered() {}

  // Called after a favorite is moved to a different folder.
  // |web_contents| is the tab whose favorite folder changed.
  // Note: this is a convenience notification — the real state change is
  // on AstraTabFeatures (WebContentsUserData).  Observers should read
  // the current state from there.
  // TODO(astra): Consider whether this notification belongs here or on
  // a tab-level observer.  For now we fire it from the service when
  // MoveFavoriteToFolder is called, as that's the main entry point.
  virtual void OnFavoriteMoved(content::WebContents* web_contents,
                               const std::string& old_folder_id,
                               const std::string& new_folder_id) {}

  // Called after favorites are reordered within a folder.
  // Observers should refresh their favorite order display.
  virtual void OnFavoritesReordered(const std::string& folder_id) {}

  // Called after a folder is expanded.
  virtual void OnFolderExpanded(const std::string& folder_id) {}

  // Called after a folder is collapsed.
  virtual void OnFolderCollapsed(const std::string& folder_id) {}

  // Called after a folder's accent color changes.
  virtual void OnFolderColorChanged(const std::string& folder_id,
                                    const std::string& new_color) {}

 protected:
  ~AstraFavoriteServiceObserver() override = default;
};

// =========================================================================
// AstraFavoriteService
// =========================================================================
//
// Profile-scoped keyed service that owns all Astra favorite folder
// metadata.
//
// Truth source for:
//   - Folder definitions (id, name, parent, order, expanded state).
//   - Folder hierarchy (tree structure of favorite folders).
//
// Not owned here:
//   - Per-tab favorite state (lives on AstraTabFeatures / WebContentsUserData).
//   - Tabs / WebContents (Chromium TabStripModel owns them).
//   - Chrome bookmarks (entirely separate — reused as-is from Chromium).
//   - Profile, history, downloads, etc. (all Chromium).
//
// Persistence:
//   TODO(astra): Persist folders via Chromium PrefService (PrefRegistry).
//   Do NOT add a custom file-based storage layer — always go through the
//   profile's PrefService so folder state participates in profile
//   lifecycle, sync, and policy correctly.
// =========================================================================

class AstraFavoriteService final : public KeyedService {
 public:
  explicit AstraFavoriteService(Profile* profile);
  AstraFavoriteService(const AstraFavoriteService&) = delete;
  AstraFavoriteService& operator=(const AstraFavoriteService&) = delete;
  ~AstraFavoriteService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraFavoriteServiceObserver* observer);
  void RemoveObserver(AstraFavoriteServiceObserver* observer);

  // -- Folder query ------------------------------------------------------

  // Returns all folders, sorted by order_index within each parent level.
  // The root folder is always first in the list.
  const std::vector<AstraFavoriteFolder>& folders() const { return folders_; }

  // Number of folders (including the root folder).
  size_t folder_count() const { return folders_.size(); }

  // Returns the folder with the given id, or nullptr if not found.
  const AstraFavoriteFolder* GetFolder(const std::string& id) const;

  // Returns the root folder.  Always exists.
  const AstraFavoriteFolder& root_folder() const;

  // Returns the id of the root folder.
  const std::string& GetRootFolderId() const;

  // Returns the direct child folders of |parent_id|, ordered by
  // order_index.  If |parent_id| is empty, returns top-level folders
  // (children of root).
  std::vector<const AstraFavoriteFolder*> GetChildFolders(
      const std::string& parent_id) const;

  // -- Folder mutation ---------------------------------------------------

  // Ensures at least the root folder exists.  Idempotent.
  void EnsureRootFolder();

  // Adds a new folder.  The new folder is placed at the end of its
  // parent's children.  Fires OnFolderAdded.
  // Returns the id of the new folder, or empty string on failure.
  std::string AddFolder(const std::string& name,
                        const std::string& parent_id = std::string());

  // Renames the folder with the given id.  Fires OnFolderRenamed.
  // Returns true if the folder existed and was renamed.
  bool RenameFolder(const std::string& id, const std::string& name);

  // Deletes the folder with the given id.  The root folder cannot be
  // deleted — returns false if attempted.
  //
  // Deletion removes only the folder metadata.  Favorites that were in
  // the deleted folder are reassigned to the parent folder (or root) via
  // AstraTabFeatures favorite_folder_id.  No WebContents are destroyed.
  // Fires OnFolderRemoved.
  bool DeleteFolder(const std::string& id);

  // Reorders child folders of |parent_id| to match the given id order.
  // All ids must be direct children of |parent_id| and all children must
  // be included.  Returns true on success.
  // Fires OnFoldersReordered.
  bool ReorderFolders(const std::string& parent_id,
                      const std::vector<std::string>& ordered_ids);

  // Toggles the expanded state of a folder.  Returns true if the folder
  // existed and the state changed.
  // Fires OnFolderExpanded or OnFolderCollapsed if the state changed.
  bool ToggleFolderExpanded(const std::string& id);

  // Expands a folder.  Returns true if the folder existed and was expanded
  // (was previously collapsed).
  // Fires OnFolderExpanded if the state changed.
  bool ExpandFolder(const std::string& id);

  // Collapses a folder.  Returns true if the folder existed and was collapsed
  // (was previously expanded).
  // Fires OnFolderCollapsed if the state changed.
  bool CollapseFolder(const std::string& id);

  // Sets the accent color for a folder.  Returns true if the folder
  // existed and the color was updated.
  // Fires OnFolderColorChanged if the color changed.
  bool SetFolderColor(const std::string& id, const std::string& color);

  // Returns the list of available folder colors (hex color strings).
  // These are the preset colors users can choose from for folders.
  static std::vector<std::string> GetFolderColorPalette();

  // Finds a folder by name (case-insensitive).  Returns nullptr if not found.
  // If multiple folders have the same name, returns the first one found
  // (in sort order).
  const AstraFavoriteFolder* FindFolderByName(const std::string& name) const;

  // Returns the depth of a folder in the tree.
  // Root has depth 0, its children have depth 1, etc.
  // Returns 0 if the folder is not found (safe fallback).
  size_t GetFolderDepth(const std::string& id) const;

  // Returns all descendant folder IDs of the given folder (children,
  // grandchildren, etc.).  The folder itself is NOT included.
  // Returns an empty vector if the folder is not found.
  std::vector<std::string> GetAllDescendantIds(const std::string& id) const;

  // -- Bulk operations ----------------------------------------------------

  // Expands all folders.  Returns the number of folders whose state changed.
  // Fires OnFolderExpanded for each folder that was expanded.
  size_t ExpandAllFolders();

  // Collapses all folders (except the root, which is always "expanded").
  // Returns the number of folders whose state changed.
  // Fires OnFolderCollapsed for each folder that was collapsed.
  size_t CollapseAllFolders();

  // -- Import / Export ---------------------------------------------------

  // Exports all folder metadata as a JSON string.
  // Returns a JSON string with folder hierarchy data.
  // TODO(astra): Use base::JSONWriter for proper JSON serialization.
  // Chromium component: base/json/json_writer.h
  std::string ExportFoldersJson() const;

  // Imports folders from a JSON string.
  // If |merge| is true, imported folders are added to existing folders
  // (with unique IDs generated to avoid conflicts).
  // If |merge| is false, existing folders (except root) are replaced.
  // Returns the number of folders imported.
  // Fires OnFoldersReordered and OnFolderAdded notifications as appropriate.
  // TODO(astra): Use base::JSONReader for proper JSON parsing.
  // Chromium component: base/json/json_reader.h
  size_t ImportFoldersJson(const std::string& json, bool merge = false);

  // -- Favorite operations ----------------------------------------------

  // Moves the favorite tab identified by |web_contents| to |folder_id|.
  // Also sets is_favorite to true if it wasn't already.
  // Returns true on success (tab exists and folder exists).
  // Fires OnFavoriteMoved.
  //
  // Note: this is the service-level helper.  The canonical truth is on
  // AstraTabFeatures (WebContentsUserData).  This method updates
  // AstraTabFeatures and notifies observers.
  bool MoveFavoriteToFolder(content::WebContents* web_contents,
                            const std::string& folder_id);

  // Reorders favorites within |folder_id| to match the given order.
  // Takes a list of WebContents pointers in the desired order.
  // Updates favorite_order_index on each tab's AstraTabFeatures.
  // Fires OnFavoritesReordered.
  // TODO(astra): Implement this properly by iterating all tabs in the
  // profile and updating their favorite_order_index. For now it's a stub.
  // Chromium subsystem: BrowserList + TabStripModel for tab iteration.
  bool ReorderFavoritesInFolder(
      const std::string& folder_id,
      const std::vector<content::WebContents*>& ordered_tabs);

  // Returns the number of favorite tabs in |folder_id|.
  // TODO(astra): This requires iterating TabStripModel across all
  // browsers for the profile.  For now we return 0 as a stub.
  // Patch point: use BrowserList + TabStripModel iteration.
  size_t GetFavoriteCountInFolder(const std::string& folder_id) const;

 private:
  // Non-const lookup helper for internal use.
  AstraFavoriteFolder* FindFolder(const std::string& id);

  // Sorts folders_ by parent_id grouping then order_index.
  // Root folder is always first.
  void SortFolders();

  // Recursively collects all descendant folder ids of |folder_id|.
  // Used by DeleteFolder to find all folders that must also be removed.
  void CollectDescendantIds(const std::string& folder_id,
                            std::vector<std::string>& out_ids) const;

  // TODO(astra): Load folder state from the profile's PrefService.
  // Hook point: call this from the constructor once pref keys are registered.
  // PrefService is the Chromium subsystem; use RegisterProfilePrefs on the
  // factory to declare keys.
  void LoadFromPrefs();

  // TODO(astra): Persist current folder state to the profile's PrefService.
  // Should be called whenever folder metadata changes.  Do NOT write to
  // disk directly — always go through PrefService so profile lifecycle,
  // policy, and sync can participate.
  void SaveToPrefs();

  raw_ptr<Profile> profile_;
  std::vector<AstraFavoriteFolder> folders_;
  base::ObserverList<AstraFavoriteServiceObserver> observers_;
};

// =========================================================================
// AstraFavoriteServiceFactory
// =========================================================================
//
// Factory for AstraFavoriteService.
//
// Incognito behavior: the factory uses kRedirectedToOriginal for regular
// incognito profiles because favorite folder metadata is a product-level
// concern that should reflect the user's main profile state.  A separate
// incognito window still uses the same favorite folders — only the
// browsing context is isolated.  Guest sessions get their own instance
// (kOwnInstance) because they have no backing profile to redirect to.
// =========================================================================

class AstraFavoriteServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  static AstraFavoriteService* GetForProfile(Profile* profile);
  static AstraFavoriteServiceFactory* GetInstance();

  // Registers favorite-related prefs on the profile's PrefRegistry.
  // TODO(astra): Wire this into profile keyed service registration so it
  // runs during profile creation.  Chromium patch point:
  // chrome/browser/profiles/profile_keyed_service_factory registrations.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  AstraFavoriteServiceFactory();
  ~AstraFavoriteServiceFactory() override;

  // ProfileKeyedServiceFactory:
  KeyedService* BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_FAVORITE_SERVICE_H_
