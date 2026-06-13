#ifndef ASTRA_BROWSER_ASTRA_WORKSPACE_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_WORKSPACE_SERVICE_H_

#include <optional>
#include <string>
#include <vector>

#include "astra/browser/astra_workspace_template.h"
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
}

namespace astra {

// Product metadata representing an Astra workspace (Arc-style Space).
//
// Chromium owns all WebContents, Browser, and TabStripModel state.
// A workspace is a metadata projection: it does not own tabs — tabs carry
// a workspace_id via AstraTabFeatures (WebContentsUserData), and the Astra
// sidebar projects TabStripModel contents filtered by workspace_id.
//
// Deleting a workspace removes only this metadata; affected tabs are
// reassigned to the default workspace so no WebContents is destroyed.
struct AstraWorkspace {
  std::string id;
  std::string name;
  std::string accent_color;
  base::Time created_time;
  bool is_default = false;
  size_t order_index = 0;
  // Optional icon identifier, e.g. a Chrome side panel icon name.
  std::optional<std::string> icon;
  // Optional description of the workspace's purpose.
  std::string description;
  // When the workspace was last used (activated).
  // Used for "recent workspaces" sorting and recommendations.
  base::Time last_used_time;
  // Whether the workspace is pinned (stays at top of list, etc.).
  bool is_pinned = false;
  // Whether the workspace is hibernated (tabs unloaded from memory).
  // Hibernated workspaces keep their metadata but their tabs are
  // discarded to save memory. Tabs reload when the workspace is activated.
  bool is_hibernated = false;
};

// Observer interface for AstraWorkspaceService.
//
// UI layers (sidebar, workspace switcher) should observe this service to
// update their presentation.  UI must never be the source of truth for
// workspace state — this service is.
class AstraWorkspaceServiceObserver : public base::CheckedObserver {
 public:
  // Called after a new workspace is added to the service.
  virtual void OnWorkspaceAdded(const AstraWorkspace& workspace) {}

  // Called after a workspace is removed.  Tabs belonging to the removed
  // workspace have already been reassigned to the default workspace in
  // AstraTabFeatures metadata.
  virtual void OnWorkspaceRemoved(const std::string& workspace_id) {}

  // Called after a workspace has been renamed.
  virtual void OnWorkspaceRenamed(const std::string& workspace_id,
                                  const std::string& new_name) {}

  // Called when the active workspace changes.  This is a projection change:
  // no WebContents are created or destroyed; the sidebar/UI should re-filter
  // TabStripModel by the new active workspace_id.
  virtual void OnActiveWorkspaceChanged(const std::string& old_id,
                                        const std::string& new_id) {}

  // Called after workspaces are reordered.  Observers should refresh the
  // order of workspace items in their presentation.
  virtual void OnWorkspacesReordered() {}

  // Called after a workspace's accent color changes.
  virtual void OnWorkspaceAccentColorChanged(const std::string& workspace_id,
                                             const std::string& new_color) {}

  // Called after a workspace has been cloned (duplicated).
  // |new_workspace| is the newly created workspace.
  virtual void OnWorkspaceCloned(const AstraWorkspace& new_workspace) {}

  // Called after a workspace has been created from a template.
  // |new_workspace| is the newly created workspace.
  // |template_id| identifies the template that was used.
  virtual void OnWorkspaceCreatedFromTemplate(
      const AstraWorkspace& new_workspace,
      const std::string& template_id) {}

  // Called after a workspace is pinned or unpinned.
  virtual void OnWorkspacePinnedChanged(const std::string& workspace_id,
                                        bool is_pinned) {}

  // Called after a workspace is moved (reordered).
  virtual void OnWorkspaceMoved(const std::string& workspace_id,
                                size_t old_index,
                                size_t new_index) {}

  // Called after workspaces are merged.
  // |source_id| is the workspace that was merged into |target_id|.
  virtual void OnWorkspacesMerged(const std::string& source_id,
                                   const std::string& target_id) {}

 protected:
  ~AstraWorkspaceServiceObserver() override = default;
};

// Profile-scoped keyed service that owns all Astra workspace metadata.
//
// Truth source for:
//   - Workspace definitions (name, color, icon, order, creation time).
//   - Which workspace is currently active (the one shown in the sidebar).
//
// Not owned here:
//   - Tabs / WebContents (Chromium TabStripModel owns them).
//   - Profile, history, downloads, passwords, extensions (all Chromium).
//   - Session restore state (Chromium session service owns it).
//
// Persistence:
//   Workspace state persists through Chromium's PrefService, registered
//   via astra::prefs::RegisterProfilePrefs (see astra_prefs.h).  No custom
//   file I/O, no JSON files, no localStorage — everything goes through the
//   profile's PrefService so workspace state participates in profile
//   lifecycle, sync, and policy correctly.
//
//   Per-tab workspace membership (AstraTabFeatures) does NOT persist via
//   PrefService — it travels with the tab through Chromium's session
//   restore pipeline.  See astra_tab_features.h for details.
class AstraWorkspaceService final : public KeyedService {
 public:
  // Direction for workspace switch history navigation.
  enum class SwitchDirection {
    kBack,
    kForward,
  };

  explicit AstraWorkspaceService(Profile* profile);
  AstraWorkspaceService(const AstraWorkspaceService&) = delete;
  AstraWorkspaceService& operator=(const AstraWorkspaceService&) = delete;
  ~AstraWorkspaceService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraWorkspaceServiceObserver* observer);
  void RemoveObserver(AstraWorkspaceServiceObserver* observer);

  // -- Workspace query ---------------------------------------------------

  // All workspaces, in user-defined order (sorted by order_index).
  const std::vector<AstraWorkspace>& workspaces() const { return workspaces_; }

  // Number of workspaces.
  size_t workspace_count() const { return workspaces_.size(); }

  // Returns the workspace with the given id, or nullptr if not found.
  const AstraWorkspace* GetWorkspace(const std::string& id) const;

  // Returns the workspace at the given index, or nullptr if out of bounds.
  const AstraWorkspace* GetWorkspaceAtIndex(size_t index) const;

  // Finds a workspace by name (case-sensitive). Returns nullptr if not found.
  const AstraWorkspace* FindWorkspaceByName(const std::string& name) const;

  // Returns the currently active workspace.  There is always at least the
  // default workspace, so this is never null.
  const AstraWorkspace& active_workspace() const;

  // Convenience accessor for the active workspace id.
  const std::string& active_workspace_id() const {
    return active_workspace_id_;
  }

  // Returns the id of the default workspace.
  const std::string& GetDefaultWorkspaceId() const;

  // Returns the default workspace.  Always exists.
  const AstraWorkspace& GetDefaultWorkspace() const;

  // -- Workspace mutation ------------------------------------------------

  // Ensures at least the default workspace exists.  Idempotent.
  void EnsureDefaultWorkspace();

  // Switches the active workspace.  This changes which tab projection is
  // shown in the Astra sidebar; it does NOT create, destroy, or move any
  // WebContents.  TabStripModel owns all tabs at all times.
  // Pushes the previous workspace onto the back history stack.
  void ActivateWorkspace(const std::string& workspace_id);

  // -- Workspace switch history ------------------------------------------

  // Navigates backward or forward in the workspace switch history.
  // Returns true if navigation succeeded (there was history in that direction).
  bool NavigateSwitchHistory(SwitchDirection direction);

  // Returns whether there is back navigation history available.
  bool CanGoBackInHistory() const;

  // Returns whether there is forward navigation history available.
  bool CanGoForwardInHistory() const;

  // Returns the id of the workspace we would go to if we navigated back.
  // Returns empty string if no back history.
  std::string PeekBackHistory() const;

  // Returns the id of the workspace we would go to if we navigated forward.
  // Returns empty string if no forward history.
  std::string PeekForwardHistory() const;

  // Clears the switch history stack.
  void ClearSwitchHistory();

  // Returns the current size of the back history stack.
  size_t back_history_size() const { return back_history_.size(); }

  // Returns the current size of the forward history stack.
  size_t forward_history_size() const { return forward_history_.size(); }

  // Adds a new workspace.  The new workspace gets an order_index at the end
  // of the current list.  Fires OnWorkspaceAdded.
  void AddWorkspace(AstraWorkspace workspace);

  // Renames the workspace with the given id.  Fires OnWorkspaceRenamed.
  // Returns true if the workspace existed and was renamed.
  bool RenameWorkspace(const std::string& id, const std::string& name);

  // Deletes the workspace with the given id.  The default workspace cannot
  // be deleted — returns false if attempted.
  //
  // Deletion removes only the workspace metadata.  Tabs that were in the
  // deleted workspace are reassigned to the default workspace via their
  // AstraTabFeatures workspace_id.  No WebContents are destroyed.
  // Fires OnWorkspaceRemoved.
  bool DeleteWorkspace(const std::string& id);

  // Reorders workspaces to match the order of the given id list.
  // All ids must exist and all workspaces must be included.
  // Returns true on success.
  bool ReorderWorkspaces(const std::vector<std::string>& ordered_ids);

  // Sets the accent color for a workspace.  Returns true if the workspace
  // existed and the color was updated.
  bool SetWorkspaceAccentColor(const std::string& id,
                               const std::string& color);

  // Sets the description for a workspace.  Returns true if the workspace
  // existed and the description was updated.
  bool SetWorkspaceDescription(const std::string& id,
                               const std::string& description);

  // Sets the pinned state of a workspace.  Pinned workspaces stay at the
  // top of the list.  Returns true if the workspace existed and state was
  // changed.
  bool SetWorkspacePinned(const std::string& id, bool pinned);

  // Returns all pinned workspaces, in order.
  std::vector<AstraWorkspace> GetPinnedWorkspaces() const;

  // Moves a workspace up by one position.  Returns true on success.
  bool MoveWorkspaceUp(const std::string& id);

  // Moves a workspace down by one position.  Returns true on success.
  bool MoveWorkspaceDown(const std::string& id);

  // Moves a workspace to a specific position.  Returns true on success.
  bool MoveWorkspaceToPosition(const std::string& id, size_t position);

  // Clones (duplicates) a workspace.  Creates a new workspace with the
  // same name (with " copy" suffix), color, icon, and description.
  // The new workspace gets a fresh ID and current timestamps.
  // Returns the ID of the new workspace, or empty string if the source
  // workspace doesn't exist.
  //
  // Note: this only clones metadata — it does not clone tabs.
  // To clone tabs too, use AstraWorkspaceWindowManager or import/export.
  // TODO(astra): Consider adding a CloneWorkspaceWithTabs variant that
  // duplicates all tabs in the workspace.
  std::string CloneWorkspace(const std::string& source_id);

  // Merges two workspaces.  All tabs from |source_id| are moved to
  // |target_id|, then |source_id| is deleted.  Returns true on success.
  // Returns false if either workspace doesn't exist or if source == target.
  bool MergeWorkspaces(const std::string& source_id,
                       const std::string& target_id);

  // Clears all tabs from a workspace (moves them to the default workspace).
  // Returns the number of tabs that were moved.
  // TODO(astra): Implement with real TabStripModel integration.
  size_t ClearWorkspace(const std::string& id);

  // Updates the last_used_time of a workspace to now.
  // Called when a workspace is activated or interacted with.
  // Returns true if the workspace existed.
  bool TouchWorkspace(const std::string& id);

  // Returns workspaces sorted by last used time (most recent first).
  // Useful for "recent workspaces" features.
  std::vector<AstraWorkspace> GetRecentlyUsedWorkspaces() const;

  // Sets the hibernation state of a workspace.
  // Hibernated workspaces keep their metadata but their tabs are
  // unloaded to save memory.
  // Returns true if the workspace existed and state was changed.
  bool SetWorkspaceHibernated(const std::string& id, bool hibernated);

  // Returns all hibernated workspaces.
  std::vector<AstraWorkspace> GetHibernatedWorkspaces() const;

  // -- Templates -----------------------------------------------------------

  // Creates a new workspace from the given template.
  //
  // The new workspace inherits the template's name, accent color, icon,
  // and description. Default tabs from the template are NOT created by
  // this method — workspace metadata only. UI layers (e.g. sidebar,
  // workspace switcher) should observe OnWorkspaceCreatedFromTemplate
  // and open the template's default tabs via AstraWorkspaceWindowManager.
  //
  // Returns the id of the new workspace, or an empty string if the
  // template was not found.
  //
  // Chromium owner: TabStripModel / Browser (tab creation).
  // We create metadata only — tab creation is delegated to Chromium's
  // browser and tab infrastructure triggered by the UI layer.
  std::string CreateWorkspaceFromTemplate(const std::string& template_id);

  // Returns all available workspace templates.
  //
  // Currently returns built-in templates only.
  // TODO(astra): Combine built-in templates with user-created templates
  // from PrefService. Chromium patch point: astra_prefs.h + PrefService.
  std::vector<AstraWorkspaceTemplate> GetAvailableTemplates() const;

  // -- Navigation helpers -------------------------------------------------

  // Returns the index of the workspace with the given id, or the index of
  // the default workspace if not found.
  size_t GetWorkspaceIndex(const std::string& id) const;

  // Returns the id of the next workspace in order.  Wraps around from last
  // to first.  Always returns a valid workspace id (at least the default).
  std::string GetNextWorkspaceId() const;

  // Returns the id of the previous workspace in order.  Wraps around from
  // first to last.  Always returns a valid workspace id (at least the
  // default).
  std::string GetPreviousWorkspaceId() const;

  // -- Window and tab aggregation -----------------------------------------

  // Returns the number of browser windows in the given workspace.
  // Delegates to AstraWorkspaceWindowManager to count windows across all
  // Browser instances belonging to this profile.
  //
  // Chromium owner: BrowserList + TabStripModel.
  // We aggregate by iterating Chromium-owned windows and tabs.
  size_t GetWindowCount(const std::string& workspace_id) const;

  // Returns the total number of tabs across all windows in the given
  // workspace.  Delegates to AstraWorkspaceWindowManager to sum tab counts
  // from each Browser's TabStripModel.
  //
  // Chromium owner: BrowserList + TabStripModel.
  size_t GetTabCount(const std::string& workspace_id) const;

  // -- Incognito compatibility -------------------------------------------

  // Returns true if this service instance is associated with an incognito
  // (off-the-record) profile.
  //
  // Note: because the factory uses kRedirectedToOriginal for incognito,
  // this will return false even when called from an incognito context —
  // the service instance belongs to the original profile.  Use
  // AstraIncognitoHandler::IsIncognitoProfile() on the browser's profile
  // to determine if the caller is in incognito mode.
  //
  // See the class comment on AstraWorkspaceServiceFactory for the full
  // incognito redirect explanation.
  bool IsIncognito() const;

 private:
  // Non-const lookup helper for internal use.
  AstraWorkspace* FindWorkspace(const std::string& id);

  // Sorts workspaces_ by order_index in ascending order.
  void SortWorkspaces();

  // Loads workspace state from the profile's PrefService.  Called from the
  // constructor.  If no persisted state exists (fresh profile), falls back
  // to creating the default workspace.
  //
  // Chromium component: PrefService.
  // All persistence goes through PrefService — no custom file I/O.
  void LoadFromPrefs();

  // Persists current workspace state to the profile's PrefService.
  // Called from every mutation method (Add, Rename, Delete, Reorder,
  // Activate, SetAccentColor).  PrefService handles deferred disk writes
  // internally.
  //
  // Chromium component: PrefService + PrefRegistry.
  void SaveToPrefs();

  raw_ptr<Profile> profile_;
  std::vector<AstraWorkspace> workspaces_;
  std::string active_workspace_id_;
  base::ObserverList<AstraWorkspaceServiceObserver> observers_;

  // Switch history stacks for back/forward navigation.
  // back_history_ contains previous workspace ids (most recent last).
  // forward_history_ contains workspace ids we navigated back from.
  std::vector<std::string> back_history_;
  std::vector<std::string> forward_history_;

  // Maximum size of each history stack (prevents unbounded growth).
  static constexpr size_t kMaxHistorySize = 50;
};

// Factory for AstraWorkspaceService.
//
// Incognito behavior: the factory uses kRedirectedToOriginal for regular
// incognito profiles because workspace metadata is a product-level concern
// that should reflect the user's main profile state.  A separate incognito
// window still uses the same workspace set; only the browsing context is
// isolated.  Guest sessions get their own instance (kOwnInstance) because
// they have no backing profile to redirect to.
class AstraWorkspaceServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  static AstraWorkspaceService* GetForProfile(Profile* profile);
  static AstraWorkspaceServiceFactory* GetInstance();

  // Registers all Astra profile prefs (workspaces, sidebar, split view)
  // on the profile's PrefRegistry.  Delegates to astra::prefs::RegisterProfilePrefs.
  //
  // TODO(astra): Wire this into Chromium's profile pref registration
  // pipeline so prefs are registered at profile creation time.  Two
  // standard patterns:
  //   1. Factory-based: auto-called when the factory is registered with
  //      the BrowserContextKeyedServiceFactory system.
  //   2. Explicit: called from chrome/browser/prefs/browser_prefs.cc.
  //
  // Chromium patch point: chrome/browser/prefs/browser_prefs.cc or
  // chrome/browser/profiles/profile_keyed_service_factory registrations.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  AstraWorkspaceServiceFactory();
  ~AstraWorkspaceServiceFactory() override;

  // ProfileKeyedServiceFactory:
  KeyedService* BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

// Registers all Astra browser-layer ProfileKeyedServices.
//
// TODO(astra): Call this from a Chrome browser main patch point (e.g.
// chrome/browser/profiles/profile_keyed_service_factory_android.cc or
// equivalent desktop hook) so Astra services are created alongside other
// profile-keyed services.
void RegisterAstraProfileKeyedServices();

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_WORKSPACE_SERVICE_H_
