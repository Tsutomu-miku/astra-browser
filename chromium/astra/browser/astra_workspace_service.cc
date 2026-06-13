#include "astra/browser/astra_workspace_service.h"

#include <algorithm>
#include <utility>

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_workspace_window_manager.h"
#include "base/check.h"
#include "base/check_op.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// Id of the default workspace that always exists.
constexpr char kDefaultWorkspaceId[] = "default";

// Default accent color for the default workspace.
constexpr char kDefaultAccentColor[] = "#5B8FF9";

// Display name of the default workspace.
constexpr char kDefaultWorkspaceName[] = "Personal";

// Dictionary keys for workspace serialization in prefs.
constexpr char kWorkspaceIdKey[] = "id";
constexpr char kWorkspaceNameKey[] = "name";
constexpr char kWorkspaceAccentColorKey[] = "accent_color";
constexpr char kWorkspaceCreatedTimeKey[] = "created_time";
constexpr char kWorkspaceIsDefaultKey[] = "is_default";
constexpr char kWorkspaceOrderIndexKey[] = "order_index";
constexpr char kWorkspaceIconKey[] = "icon";
constexpr char kWorkspaceDescriptionKey[] = "description";
constexpr char kWorkspaceLastUsedTimeKey[] = "last_used_time";
constexpr char kWorkspaceIsHibernatedKey[] = "is_hibernated";
constexpr char kWorkspaceIsPinnedKey[] = "is_pinned";

}  // namespace

// ---------------------------------------------------------------------------
// AstraWorkspaceService
// ---------------------------------------------------------------------------

AstraWorkspaceService::AstraWorkspaceService(Profile* profile)
    : profile_(profile) {
  // Load workspace state from the profile's PrefService.  If no persisted
  // state exists (fresh profile), LoadFromPrefs falls back to creating the
  // default workspace.
  //
  // Chromium component: PrefService + profile initialization.
  // All persistence goes through PrefService — no custom file I/O.
  LoadFromPrefs();
}

AstraWorkspaceService::~AstraWorkspaceService() = default;

void AstraWorkspaceService::Shutdown() {
  // KeyedService shutdown: clear all observer references and drop profile
  // pointer before the profile goes away.  Observers should have removed
  // themselves already (typically in their own Shutdown), but we clear the
  // list here to be safe and to catch dangling observer bugs in debug builds.
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers ---------------------------------------------------------------

void AstraWorkspaceService::AddObserver(
    AstraWorkspaceServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraWorkspaceService::RemoveObserver(
    AstraWorkspaceServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Workspace query ---------------------------------------------------------

const AstraWorkspace* AstraWorkspaceService::GetWorkspace(
    const std::string& id) const {
  auto it = base::ranges::find(workspaces_, id, &AstraWorkspace::id);
  return it == workspaces_.end() ? nullptr : &(*it);
}

const AstraWorkspace& AstraWorkspaceService::active_workspace() const {
  const AstraWorkspace* ws = GetWorkspace(active_workspace_id_);
  // Invariant: the active workspace id always refers to an existing workspace.
  CHECK(ws);
  return *ws;
}

const std::string& AstraWorkspaceService::GetDefaultWorkspaceId() const {
  // The default workspace id is stable — it's always "default".
  // We return a const ref to the string literal via a local static to match
  // the expected string lifetime.
  static const base::NoDestructor<std::string> kId(kDefaultWorkspaceId);
  return *kId;
}

const AstraWorkspace& AstraWorkspaceService::GetDefaultWorkspace() const {
  const AstraWorkspace* ws = GetWorkspace(kDefaultWorkspaceId);
  CHECK(ws);
  return *ws;
}

const AstraWorkspace* AstraWorkspaceService::GetWorkspaceAtIndex(
    size_t index) const {
  if (index >= workspaces_.size()) {
    return nullptr;
  }
  return &workspaces_[index];
}

const AstraWorkspace* AstraWorkspaceService::FindWorkspaceByName(
    const std::string& name) const {
  auto it = base::ranges::find(workspaces_, name, &AstraWorkspace::name);
  return it == workspaces_.end() ? nullptr : &(*it);
}

// -- Workspace mutation ------------------------------------------------------

void AstraWorkspaceService::EnsureDefaultWorkspace() {
  if (GetWorkspace(kDefaultWorkspaceId)) {
    return;
  }

  AstraWorkspace default_ws;
  default_ws.id = kDefaultWorkspaceId;
  default_ws.name = kDefaultWorkspaceName;
  default_ws.accent_color = kDefaultAccentColor;
  default_ws.created_time = base::Time::Now();
  default_ws.is_default = true;
  default_ws.order_index = 0;

  workspaces_.push_back(std::move(default_ws));
  active_workspace_id_ = kDefaultWorkspaceId;
}

void AstraWorkspaceService::ActivateWorkspace(
    const std::string& workspace_id) {
  // Ignore activation of a non-existent workspace.
  if (!GetWorkspace(workspace_id)) {
    return;
  }

  if (active_workspace_id_ == workspace_id) {
    return;
  }

  std::string old_id = active_workspace_id_;

  // Push previous workspace onto back history stack.
  back_history_.push_back(old_id);
  if (back_history_.size() > kMaxHistorySize) {
    back_history_.erase(back_history_.begin());
  }
  // Clear forward history on a new navigation.
  forward_history_.clear();

  active_workspace_id_ = workspace_id;

  // Update last used time for the newly activated workspace.
  TouchWorkspace(workspace_id);

  // TODO(astra): Project Chrome TabStripModel contents by workspace metadata
  // instead of moving WebContents ownership outside Chromium.
  // The sidebar/UI observes OnActiveWorkspaceChanged and re-filters the
  // TabStripModel by workspace_id on AstraTabFeatures.  No WebContents are
  // created, destroyed, or moved — this is purely a visibility projection.

  for (auto& observer : observers_) {
    observer.OnActiveWorkspaceChanged(old_id, active_workspace_id_);
  }

  SaveToPrefs();
}

// -- Workspace switch history ------------------------------------------------

bool AstraWorkspaceService::NavigateSwitchHistory(SwitchDirection direction) {
  if (direction == SwitchDirection::kBack) {
    if (back_history_.empty()) {
      return false;
    }
    std::string prev_id = back_history_.back();
    back_history_.pop_back();
    if (!GetWorkspace(prev_id)) {
      // Workspace no longer exists, skip it.
      return NavigateSwitchHistory(direction);
    }
    forward_history_.push_back(active_workspace_id_);
    std::string old_id = active_workspace_id_;
    active_workspace_id_ = prev_id;
    TouchWorkspace(active_workspace_id_);
    for (auto& observer : observers_) {
      observer.OnActiveWorkspaceChanged(old_id, active_workspace_id_);
    }
    SaveToPrefs();
    return true;
  } else {
    if (forward_history_.empty()) {
      return false;
    }
    std::string next_id = forward_history_.back();
    forward_history_.pop_back();
    if (!GetWorkspace(next_id)) {
      return NavigateSwitchHistory(direction);
    }
    back_history_.push_back(active_workspace_id_);
    std::string old_id = active_workspace_id_;
    active_workspace_id_ = next_id;
    TouchWorkspace(active_workspace_id_);
    for (auto& observer : observers_) {
      observer.OnActiveWorkspaceChanged(old_id, active_workspace_id_);
    }
    SaveToPrefs();
    return true;
  }
}

bool AstraWorkspaceService::CanGoBackInHistory() const {
  // Check if there's any valid (existing) workspace in back history.
  for (auto it = back_history_.rbegin(); it != back_history_.rend(); ++it) {
    if (GetWorkspace(*it)) {
      return true;
    }
  }
  return false;
}

bool AstraWorkspaceService::CanGoForwardInHistory() const {
  for (auto it = forward_history_.rbegin(); it != forward_history_.rend();
       ++it) {
    if (GetWorkspace(*it)) {
      return true;
    }
  }
  return false;
}

std::string AstraWorkspaceService::PeekBackHistory() const {
  for (auto it = back_history_.rbegin(); it != back_history_.rend(); ++it) {
    if (GetWorkspace(*it)) {
      return *it;
    }
  }
  return std::string();
}

std::string AstraWorkspaceService::PeekForwardHistory() const {
  for (auto it = forward_history_.rbegin(); it != forward_history_.rend();
       ++it) {
    if (GetWorkspace(*it)) {
      return *it;
    }
  }
  return std::string();
}

void AstraWorkspaceService::ClearSwitchHistory() {
  back_history_.clear();
  forward_history_.clear();
}

void AstraWorkspaceService::AddWorkspace(AstraWorkspace workspace) {
  // Disallow duplicate ids.
  if (GetWorkspace(workspace.id)) {
    return;
  }

  // Auto-assign order_index at the end if not set by caller.
  if (workspace.order_index == 0 && !workspaces_.empty()) {
    size_t max_index = 0;
    for (const auto& ws : workspaces_) {
      if (ws.order_index > max_index) {
        max_index = ws.order_index;
      }
    }
    workspace.order_index = max_index + 1;
  }

  // Default created_time to now if not set.
  if (workspace.created_time.is_null()) {
    workspace.created_time = base::Time::Now();
  }

  workspaces_.push_back(std::move(workspace));
  SortWorkspaces();

  for (auto& observer : observers_) {
    observer.OnWorkspaceAdded(workspaces_.back());
  }

  SaveToPrefs();
}

bool AstraWorkspaceService::RenameWorkspace(const std::string& id,
                                            const std::string& name) {
  AstraWorkspace* ws = FindWorkspace(id);
  if (!ws) {
    return false;
  }

  if (ws->name == name) {
    return true;
  }

  ws->name = name;

  for (auto& observer : observers_) {
    observer.OnWorkspaceRenamed(id, name);
  }

  SaveToPrefs();
  return true;
}

bool AstraWorkspaceService::DeleteWorkspace(const std::string& id) {
  // The default workspace cannot be deleted.
  if (id == kDefaultWorkspaceId) {
    return false;
  }

  auto it = base::ranges::find(workspaces_, id, &AstraWorkspace::id);
  if (it == workspaces_.end()) {
    return false;
  }

  // ------------------------------------------------------------------
  // Tab-to-workspace projection note:
  //
  // Deleting a workspace removes only the metadata.  Tabs that were in this
  // workspace (i.e., whose AstraTabFeatures workspace_id matches |id|) must
  // be reassigned to the default workspace so they remain visible somewhere.
  //
  // This service does NOT own TabStripModel or WebContents, so it cannot
  // iterate tabs directly.  The reassignment happens at the layer that
  // bridges workspace metadata to tab features — the workspace service only
  // announces the deletion, and the sidebar/tab-projection layer handles
  // the AstraTabFeatures update.
  //
  // TODO(astra): Reassign orphan tab workspace_ids to the default workspace
  // when a workspace is deleted.  This requires iterating all WebContents in
  // the profile's Browsers' TabStripModels and checking AstraTabFeatures.
  // Patch point: chrome/browser/ui/browser_list.h + TabStripModel iteration.
  // ------------------------------------------------------------------

  bool was_active = (active_workspace_id_ == id);
  workspaces_.erase(it);

  if (was_active) {
    // Switch active workspace to the default.  This fires
    // OnActiveWorkspaceChanged so the UI updates.
    ActivateWorkspace(kDefaultWorkspaceId);
  }

  for (auto& observer : observers_) {
    observer.OnWorkspaceRemoved(id);
  }

  SaveToPrefs();
  return true;
}

bool AstraWorkspaceService::ReorderWorkspaces(
    const std::vector<std::string>& ordered_ids) {
  // Validate: all ids must exist and all workspaces must be included.
  if (ordered_ids.size() != workspaces_.size()) {
    return false;
  }

  for (const auto& id : ordered_ids) {
    if (!GetWorkspace(id)) {
      return false;
    }
  }

  // Assign order_index based on position in the ordered list.
  for (size_t i = 0; i < ordered_ids.size(); ++i) {
    AstraWorkspace* ws = FindWorkspace(ordered_ids[i]);
    DCHECK(ws);
    ws->order_index = i;
  }

  SortWorkspaces();

  for (auto& observer : observers_) {
    observer.OnWorkspacesReordered();
  }

  SaveToPrefs();
  return true;
}

bool AstraWorkspaceService::SetWorkspaceAccentColor(const std::string& id,
                                                    const std::string& color) {
  AstraWorkspace* ws = FindWorkspace(id);
  if (!ws) {
    return false;
  }

  ws->accent_color = color;

  for (auto& observer : observers_) {
    observer.OnWorkspaceAccentColorChanged(id, color);
  }

  SaveToPrefs();
  return true;
}

bool AstraWorkspaceService::SetWorkspaceDescription(
    const std::string& id,
    const std::string& description) {
  AstraWorkspace* ws = FindWorkspace(id);
  if (!ws) {
    return false;
  }

  if (ws->description == description) {
    return true;
  }

  ws->description = description;
  ws->last_used_time = base::Time::Now();

  SaveToPrefs();
  return true;
}

bool AstraWorkspaceService::SetWorkspacePinned(const std::string& id,
                                                bool pinned) {
  AstraWorkspace* ws = FindWorkspace(id);
  if (!ws) {
    return false;
  }

  if (ws->is_pinned == pinned) {
    return true;
  }

  ws->is_pinned = pinned;

  for (auto& observer : observers_) {
    observer.OnWorkspacePinnedChanged(id, pinned);
  }

  SaveToPrefs();
  return true;
}

std::vector<AstraWorkspace>
AstraWorkspaceService::GetPinnedWorkspaces() const {
  std::vector<AstraWorkspace> result;
  for (const auto& ws : workspaces_) {
    if (ws.is_pinned) {
      result.push_back(ws);
    }
  }
  return result;
}

bool AstraWorkspaceService::MoveWorkspaceUp(const std::string& id) {
  size_t index = GetWorkspaceIndex(id);
  if (index == 0 || index >= workspaces_.size()) {
    return false;
  }
  return MoveWorkspaceToPosition(id, index - 1);
}

bool AstraWorkspaceService::MoveWorkspaceDown(const std::string& id) {
  size_t index = GetWorkspaceIndex(id);
  if (index >= workspaces_.size() - 1) {
    return false;
  }
  return MoveWorkspaceToPosition(id, index + 1);
}

bool AstraWorkspaceService::MoveWorkspaceToPosition(const std::string& id,
                                                     size_t position) {
  AstraWorkspace* ws = FindWorkspace(id);
  if (!ws || position >= workspaces_.size()) {
    return false;
  }

  size_t old_index = GetWorkspaceIndex(id);
  if (old_index == position) {
    return true;
  }

  // Build ordered list with the workspace moved to new position.
  std::vector<std::string> ordered_ids;
  ordered_ids.reserve(workspaces_.size());
  bool inserted = false;
  for (size_t i = 0; i < workspaces_.size(); ++i) {
    if (i == position) {
      ordered_ids.push_back(id);
      inserted = true;
    }
    if (workspaces_[i].id != id) {
      ordered_ids.push_back(workspaces_[i].id);
    }
  }
  if (!inserted) {
    ordered_ids.push_back(id);
  }

  bool result = ReorderWorkspaces(ordered_ids);
  if (result) {
    for (auto& observer : observers_) {
      observer.OnWorkspaceMoved(id, old_index, position);
    }
  }
  return result;
}

std::string AstraWorkspaceService::CloneWorkspace(
    const std::string& source_id) {
  const AstraWorkspace* source = GetWorkspace(source_id);
  if (!source) {
    return std::string();
  }

  // Generate a new unique ID (use a simple counter-based approach
  // since we don't have base::GenerateGUID available in all contexts).
  // TODO(astra): Use base::GenerateGUID() for proper unique IDs.
  static int clone_counter = 0;
  std::string new_id = source->id + "-clone-" +
                       base::NumberToString(++clone_counter);

  // Ensure uniqueness.
  if (GetWorkspace(new_id)) {
    // If somehow there's a collision, append timestamp.
    new_id += "-" + base::NumberToString(base::Time::Now().ToTimeT());
  }

  AstraWorkspace cloned;
  cloned.id = new_id;
  cloned.name = source->name + " copy";
  cloned.accent_color = source->accent_color;
  cloned.description = source->description;
  cloned.created_time = base::Time::Now();
  cloned.last_used_time = base::Time::Now();
  cloned.is_default = false;
  cloned.is_hibernated = false;
  cloned.is_pinned = source->is_pinned;
  cloned.icon = source->icon;

  // Place at the end of the list.
  size_t max_index = 0;
  for (const auto& ws : workspaces_) {
    if (ws.order_index > max_index) {
      max_index = ws.order_index;
    }
  }
  cloned.order_index = max_index + 1;

  workspaces_.push_back(std::move(cloned));
  SortWorkspaces();

  const AstraWorkspace* new_ws = GetWorkspace(new_id);
  DCHECK(new_ws);

  for (auto& observer : observers_) {
    observer.OnWorkspaceAdded(*new_ws);
    observer.OnWorkspaceCloned(*new_ws);
  }

  SaveToPrefs();
  return new_id;
}

bool AstraWorkspaceService::MergeWorkspaces(const std::string& source_id,
                                             const std::string& target_id) {
  if (source_id == target_id) {
    return false;
  }

  const AstraWorkspace* source = GetWorkspace(source_id);
  const AstraWorkspace* target = GetWorkspace(target_id);
  if (!source || !target) {
    return false;
  }

  // TODO(astra): Move all tabs from source to target workspace.
  // This requires iterating TabStripModel + AstraTabFeatures for each
  // Browser in the profile, and updating workspace_id on matching tabs.
  // For now, this is a metadata-only merge — the target workspace's
  // metadata remains, and the source workspace is deleted.
  //
  // Chromium owner: BrowserList + TabStripModel.
  // Patch point: chrome/browser/ui/browser_list.h + TabStripModel iteration.

  std::string source_name = source->name;
  (void)source_name;  // Referenced in note below.

  // Delete the source workspace.  Tabs (if any) would need to be moved first.
  bool deleted = DeleteWorkspace(source_id);
  if (!deleted) {
    return false;
  }

  for (auto& observer : observers_) {
    observer.OnWorkspacesMerged(source_id, target_id);
  }

  // TODO(astra): Consider appending source workspace name to target
  // description, or adding a "merged from" metadata field.

  return true;
}

size_t AstraWorkspaceService::ClearWorkspace(const std::string& id) {
  if (!GetWorkspace(id)) {
    return 0;
  }

  // TODO(astra): Move all tabs from this workspace to the default workspace.
  // Like MergeWorkspaces, this requires TabStripModel + AstraTabFeatures.
  // For now, this is a no-op that returns 0.
  //
  // Chromium owner: BrowserList + TabStripModel.
  // Patch point: chrome/browser/ui/browser_list.h.

  return 0;
}

bool AstraWorkspaceService::TouchWorkspace(const std::string& id) {
  AstraWorkspace* ws = FindWorkspace(id);
  if (!ws) {
    return false;
  }

  ws->last_used_time = base::Time::Now();

  SaveToPrefs();
  return true;
}

std::vector<AstraWorkspace>
AstraWorkspaceService::GetRecentlyUsedWorkspaces() const {
  std::vector<AstraWorkspace> result = workspaces_;

  // Sort by last_used_time descending (most recent first).
  // Workspaces with null last_used_time go to the end.
  std::sort(result.begin(), result.end(),
            [](const AstraWorkspace& a, const AstraWorkspace& b) {
              if (a.last_used_time.is_null() && b.last_used_time.is_null()) {
                return a.order_index < b.order_index;
              }
              if (a.last_used_time.is_null()) {
                return false;
              }
              if (b.last_used_time.is_null()) {
                return true;
              }
              return a.last_used_time > b.last_used_time;
            });

  return result;
}

bool AstraWorkspaceService::SetWorkspaceHibernated(const std::string& id,
                                                    bool hibernated) {
  AstraWorkspace* ws = FindWorkspace(id);
  if (!ws) {
    return false;
  }

  if (ws->is_hibernated == hibernated) {
    return true;
  }

  ws->is_hibernated = hibernated;

  // TODO(astra): When hibernating, unload all tabs in the workspace.
  // When waking from hibernation, restore the tabs.
  // For now, this is just a metadata flag — the actual tab
  // suspend/restore is handled by AstraWorkspaceWindowManager or by
  // Chromium's tab discard system.
  //
  // Chromium owner: resource_coordinator::TabManager / tab discarding.
  // Patch point: tab discard / reload via WebContents::DiscardPage().

  SaveToPrefs();
  return true;
}

std::vector<AstraWorkspace>
AstraWorkspaceService::GetHibernatedWorkspaces() const {
  std::vector<AstraWorkspace> result;
  for (const auto& ws : workspaces_) {
    if (ws.is_hibernated) {
      result.push_back(ws);
    }
  }
  return result;
}

// -- Templates ---------------------------------------------------------------

std::string AstraWorkspaceService::CreateWorkspaceFromTemplate(
    const std::string& template_id) {
  // Look up the template.
  const AstraWorkspaceTemplate* temp = FindTemplate(template_id);
  if (!temp) {
    return std::string();
  }

  // Build a new workspace from template metadata.
  AstraWorkspace ws;

  // Generate a unique workspace id based on the template id.
  // TODO(astra): Use base::GenerateGUID() for proper unique IDs.
  // See the note in CloneWorkspace for the same pattern.
  static int template_counter = 0;
  ws.id = temp->id + "-" + base::NumberToString(++template_counter);

  // Ensure uniqueness against existing workspaces.
  if (GetWorkspace(ws.id)) {
    ws.id += "-" + base::NumberToString(base::Time::Now().ToTimeT());
  }

  ws.name = temp->name;
  ws.accent_color = temp->color;
  ws.description = temp->description;
  ws.created_time = base::Time::Now();
  ws.last_used_time = base::Time::Now();
  ws.is_default = false;
  ws.is_hibernated = false;
  if (!temp->icon.empty()) {
    ws.icon = temp->icon;
  }

  // Place at the end of the list (order_index will be auto-assigned
  // by AddWorkspace if it's 0).
  ws.order_index = 0;

  // Use AddWorkspace to add it to the list and fire OnWorkspaceAdded.
  AddWorkspace(std::move(ws));

  // Look up the newly added workspace to pass to the template-specific
  // observer notification.
  const AstraWorkspace* new_ws = GetWorkspace(ws.id);
  DCHECK(new_ws);

  // Fire the template-specific observer notification. UI layers that
  // handle tab creation should listen for this and open the template's
  // default tabs via AstraWorkspaceWindowManager.
  //
  // TODO(astra): Consider opening default tabs directly here via
  // AstraWorkspaceWindowManager, rather than requiring UI to observe.
  // Chromium owner: Browser + TabStripModel for tab creation.
  // Patch point: AstraWorkspaceWindowManager::AddTabsForWorkspace.
  for (auto& observer : observers_) {
    observer.OnWorkspaceCreatedFromTemplate(*new_ws, template_id);
  }

  return ws.id;
}

std::vector<AstraWorkspaceTemplate>
AstraWorkspaceService::GetAvailableTemplates() const {
  // Currently returns built-in templates only.
  // TODO(astra): Merge with user-created templates from PrefService.
  // Chromium owner: PrefService (user template persistence).
  // Patch point: add user template prefs to astra_prefs.h.
  return GetBuiltInTemplates();
}

// -- Navigation helpers ------------------------------------------------------

size_t AstraWorkspaceService::GetWorkspaceIndex(const std::string& id) const {
  for (size_t i = 0; i < workspaces_.size(); ++i) {
    if (workspaces_[i].id == id) {
      return i;
    }
  }
  // Fall back to the default workspace index (should be at index 0 after
  // sort, but we look it up to be safe).
  return GetWorkspaceIndex(kDefaultWorkspaceId);
}

std::string AstraWorkspaceService::GetNextWorkspaceId() const {
  if (workspaces_.size() <= 1) {
    return active_workspace_id_;
  }
  size_t current = GetWorkspaceIndex(active_workspace_id_);
  size_t next = (current + 1) % workspaces_.size();
  return workspaces_[next].id;
}

std::string AstraWorkspaceService::GetPreviousWorkspaceId() const {
  if (workspaces_.size() <= 1) {
    return active_workspace_id_;
  }
  size_t current = GetWorkspaceIndex(active_workspace_id_);
  size_t prev = (current + workspaces_.size() - 1) % workspaces_.size();
  return workspaces_[prev].id;
}

// -- Window and tab aggregation ----------------------------------------------

size_t AstraWorkspaceService::GetWindowCount(
    const std::string& workspace_id) const {
  if (!profile_) {
    return 0;
  }
  // Delegate to the window manager, which iterates BrowserList and
  // filters by profile + workspace_id.
  //
  // Chromium owner: BrowserList — we project from Chromium-owned data.
  return AstraWorkspaceWindowManager::GetInstance()->GetWindowCount(
      profile_, workspace_id);
}

size_t AstraWorkspaceService::GetTabCount(
    const std::string& workspace_id) const {
  if (!profile_) {
    return 0;
  }
  // Delegate to the window manager, which sums tab counts from each
  // Browser's TabStripModel for windows in the workspace.
  //
  // Chromium owner: TabStripModel — we aggregate from Chromium-owned data.
  return AstraWorkspaceWindowManager::GetInstance()->GetTabCount(
      profile_, workspace_id);
}

// -- Incognito compatibility -----------------------------------------------

bool AstraWorkspaceService::IsIncognito() const {
  // Since the factory uses kRedirectedToOriginal for incognito profiles,
  // the service instance always belongs to the original (non-OTR) profile.
  // Therefore this method will always return false, even when called from
  // an incognito context.
  //
  // This is intentionally a trivial implementation — it exists to:
  //   1. Document the incognito behavior of this service explicitly.
  //   2. Provide a stable API for callers that may not know about the
  //      kRedirectedToOriginal pattern.
  //
  // To check if the CALLER is in incognito mode (e.g. a sidebar view
  // attached to an incognito browser window), use
  // AstraIncognitoHandler::IsIncognitoProfile(browser->profile()) instead.
  //
  // Chromium owner: Profile::IsOffTheRecord()
  // (chrome/browser/profiles/profile.h)
  if (!profile_) {
    return false;
  }
  return profile_->IsOffTheRecord();
}

// -- Private helpers ---------------------------------------------------------

AstraWorkspace* AstraWorkspaceService::FindWorkspace(const std::string& id) {
  auto it = base::ranges::find(workspaces_, id, &AstraWorkspace::id);
  return it == workspaces_.end() ? nullptr : &(*it);
}

void AstraWorkspaceService::SortWorkspaces() {
  base::ranges::sort(workspaces_, std::less<>(), &AstraWorkspace::order_index);
}

void AstraWorkspaceService::LoadFromPrefs() {
  // Load workspace list and active workspace id from the profile's
  // PrefService.  If no persisted state exists (fresh profile), we fall
  // back to creating the default workspace.
  //
  // Chromium component: PrefService.
  // All persistence goes through PrefService — no custom file I/O, no JSON
  // files, no localStorage for browser state.

  DCHECK(profile_);
  PrefService* prefs = profile_->GetPrefs();
  DCHECK(prefs);

  const base::Value::List& list = prefs->GetList(prefs::kPrefWorkspaces);
  workspaces_.reserve(list.size());

  for (const auto& item : list) {
    if (!item.is_dict()) {
      continue;
    }
    const base::Value::Dict& dict = item.GetDict();

    AstraWorkspace ws;

    const std::string* id = dict.FindString(kWorkspaceIdKey);
    if (!id || id->empty()) {
      continue;
    }
    ws.id = *id;

    const std::string* name = dict.FindString(kWorkspaceNameKey);
    ws.name = name ? *name : std::string();

    const std::string* accent_color = dict.FindString(kWorkspaceAccentColorKey);
    ws.accent_color = accent_color ? *accent_color : std::string();

    absl::optional<double> created_time_us =
        dict.FindDouble(kWorkspaceCreatedTimeKey);
    if (created_time_us.has_value()) {
      ws.created_time = base::Time::FromDeltaSinceWindowsEpoch(
          base::Microseconds(static_cast<int64_t>(created_time_us.value())));
    }

    absl::optional<bool> is_default = dict.FindBool(kWorkspaceIsDefaultKey);
    ws.is_default = is_default.value_or(false);

    absl::optional<int> order_index = dict.FindInt(kWorkspaceOrderIndexKey);
    ws.order_index = static_cast<size_t>(order_index.value_or(0));

    const std::string* icon = dict.FindString(kWorkspaceIconKey);
    if (icon) {
      ws.icon = *icon;
    }

    const std::string* description = dict.FindString(kWorkspaceDescriptionKey);
    if (description) {
      ws.description = *description;
    }

    absl::optional<double> last_used_us =
        dict.FindDouble(kWorkspaceLastUsedTimeKey);
    if (last_used_us.has_value()) {
      ws.last_used_time = base::Time::FromDeltaSinceWindowsEpoch(
          base::Microseconds(static_cast<int64_t>(last_used_us.value())));
    }

    absl::optional<bool> is_hibernated = dict.FindBool(kWorkspaceIsHibernatedKey);
    ws.is_hibernated = is_hibernated.value_or(false);

    absl::optional<bool> is_pinned = dict.FindBool(kWorkspaceIsPinnedKey);
    ws.is_pinned = is_pinned.value_or(false);

    workspaces_.push_back(std::move(ws));
  }

  SortWorkspaces();

  // Load active workspace id.
  active_workspace_id_ = prefs->GetString(prefs::kPrefActiveWorkspaceId);

  // Invariants: always have at least the default workspace, and the active
  // workspace id must refer to an existing workspace.
  EnsureDefaultWorkspace();

  if (!GetWorkspace(active_workspace_id_)) {
    active_workspace_id_ = kDefaultWorkspaceId;
  }
}

void AstraWorkspaceService::SaveToPrefs() {
  // Persist current workspaces_ and active_workspace_id_ to the profile's
  // PrefService.
  //
  // Called on every mutation (Add, Rename, Delete, Reorder, Activate,
  // SetAccentColor).  PrefService handles deferred writes to disk
  // internally, so we do not need to batch or throttle writes here.
  //
  // Chromium component: PrefService + PrefRegistry.
  // All persistence goes through PrefService — no custom file I/O.

  if (!profile_) {
    // Shutdown path: profile_ may be nulled in Shutdown().
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  DCHECK(prefs);

  base::Value::List list;
  list.reserve(workspaces_.size());

  for (const auto& ws : workspaces_) {
    base::Value::Dict dict;

    dict.Set(kWorkspaceIdKey, ws.id);
    dict.Set(kWorkspaceNameKey, ws.name);
    dict.Set(kWorkspaceAccentColorKey, ws.accent_color);

    // Serialize base::Time as microseconds since Windows epoch, matching
    // Chromium's standard time serialization pattern in prefs.
    int64_t us_since_epoch =
        ws.created_time.ToDeltaSinceWindowsEpoch().InMicroseconds();
    dict.Set(kWorkspaceCreatedTimeKey, static_cast<double>(us_since_epoch));

    dict.Set(kWorkspaceIsDefaultKey, ws.is_default);
    dict.Set(kWorkspaceOrderIndexKey, static_cast<int>(ws.order_index));

    if (ws.icon.has_value()) {
      dict.Set(kWorkspaceIconKey, *ws.icon);
    }

    if (!ws.description.empty()) {
      dict.Set(kWorkspaceDescriptionKey, ws.description);
    }

    if (!ws.last_used_time.is_null()) {
      int64_t last_used_us =
          ws.last_used_time.ToDeltaSinceWindowsEpoch().InMicroseconds();
      dict.Set(kWorkspaceLastUsedTimeKey, static_cast<double>(last_used_us));
    }

    if (ws.is_hibernated) {
      dict.Set(kWorkspaceIsHibernatedKey, true);
    }

    if (ws.is_pinned) {
      dict.Set(kWorkspaceIsPinnedKey, true);
    }

    list.Append(std::move(dict));
  }

  prefs->Set(prefs::kPrefWorkspaces, base::Value(std::move(list)));
  prefs->SetString(prefs::kPrefActiveWorkspaceId, active_workspace_id_);
}

// ---------------------------------------------------------------------------
// AstraWorkspaceServiceFactory
// ---------------------------------------------------------------------------

// static
AstraWorkspaceService* AstraWorkspaceServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AstraWorkspaceService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AstraWorkspaceServiceFactory* AstraWorkspaceServiceFactory::GetInstance() {
  static base::NoDestructor<AstraWorkspaceServiceFactory> instance;
  return instance.get();
}

// static
void AstraWorkspaceServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Delegate to the shared astra_prefs module, which registers all Astra
  // profile pref keys (workspaces, sidebar, split view, etc.).
  //
  // These prefs live on the regular profile.  Incognito redirects to the
  // original profile (see factory constructor), so incognito windows share
  // the same workspace set — only browsing context is isolated.
  //
  // Chromium component: PrefService / PrefRegistry.
  // Patch point: profile keyed service registration (chrome/browser/profiles/).
  //
  // TODO(astra): Wire this into Chromium's profile pref registration
  // pipeline so prefs are registered at profile creation time.  Two
  // standard Chromium patterns:
  //   1. Factory-based: RegisterProfilePrefs is called automatically when
  //      the factory is registered with the BrowserContextKeyedServiceFactory
  //      system (via DependsOn or explicit registration).  This is the
  //      preferred pattern for keyed services.
  //   2. Explicit: Call RegisterAstraProfilePrefs from
  //      chrome/browser/prefs/browser_prefs.cc (RegisterUserPrefs).
  //
  // Owner: chrome/browser/prefs/browser_prefs.cc or
  //        chrome/browser/profiles/profile_keyed_service_factory*
  prefs::RegisterProfilePrefs(registry);
}

AstraWorkspaceServiceFactory::AstraWorkspaceServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraWorkspaceService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito uses kRedirectedToOriginal because workspace
              // definitions are product-level state that should reflect the
              // user's main profile.  An incognito window is still the same
              // user with the same workspace set — only the browsing session
              // is isolated.
              //
              // === Incognito workspace behavior ===
              //   - Workspace LIST is shared with the original profile.
              //     Incognito windows see the same workspaces.
              //   - Workspace MUTATIONS (add/rename/delete/reorder) are
              //     DISABLED in incognito (see AstraIncognitoHandler).
              //     They are disabled because mutations would persist and
              //     affect the main profile, which violates user expectations
              //     about incognito being ephemeral.
              //   - ACTIVE workspace in incognito is LOCAL to the incognito
              //     window/session.  It does NOT affect the original profile's
              //     active workspace (which is persisted).
              //     The sidebar tracks its own active workspace id when the
              //     browser's profile is incognito.
              //
              // See AstraIncognitoHandler for the centralized incognito
              // policy and design rationale.
              //
              // Chromium pattern: this is the same approach used by
              // BookmarkService, which also redirects to original for
              // incognito so bookmarks are visible but read-only in OTR.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest sessions have no original profile to redirect to,
              // so they get their own ephemeral workspace service instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile has no user workspaces.
              .Build()) {}

AstraWorkspaceServiceFactory::~AstraWorkspaceServiceFactory() = default;

KeyedService*
AstraWorkspaceServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return new AstraWorkspaceService(Profile::FromBrowserContext(context));
}

// ---------------------------------------------------------------------------
// Registration entry point
// ---------------------------------------------------------------------------

void RegisterAstraProfileKeyedServices() {
  // TODO(astra): Wire this function into Chromium's profile keyed service
  // registration pipeline so Astra services are created alongside other
  // profile-keyed services.
  //
  // Chromium patch point:
  //   chrome/browser/profiles/profile_keyed_service_factory* (desktop)
  //   or a ChromeBrowserMainExtraParts PreProfileInit/PostProfileInit hook.
  //
  // For now, services are lazily created when GetForProfile is first called.
  // The factory singleton is already alive via base::NoDestructor.
  //
  // This function is also where RegisterProfilePrefs would be connected —
  // in Chromium, pref registration typically happens through the
  // ProfileKeyedServiceFactory::RegisterProfilePrefs mechanism or via
  // chrome/browser/prefs/browser_prefs.cc.
}

}  // namespace astra
