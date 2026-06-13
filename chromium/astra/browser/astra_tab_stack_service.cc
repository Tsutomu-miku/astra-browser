#include "astra/browser/astra_tab_stack_service.h"

#include <algorithm>
#include <utility>

#include "astra/browser/astra_prefs.h"
#include "base/check.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/unguessable_token.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// Dictionary keys for stack serialization in prefs.
constexpr char kStackIdKey[] = "id";
constexpr char kStackNameKey[] = "name";
constexpr char kStackColorKey[] = "color";
constexpr char kStackCollapsedKey[] = "collapsed";
constexpr char kStackPinnedKey[] = "pinned";
constexpr char kStackNoteKey[] = "note";
constexpr char kStackOrderIndexKey[] = "order_index";
constexpr char kStackCreatedTimeKey[] = "created_time";
constexpr char kStackLastAccessedKey[] = "last_accessed";
constexpr char kStackTabIndicesKey[] = "tab_indices";

// Default color for new stacks (blue accent).
constexpr char kDefaultStackColor[] = "#5B8FF9";

// Preset color palette for stack color picker UI.
// Curated set of accessible, distinct colors matching Astra's design system.
//
// TODO(astra): Sync this palette with the Astra theme system when the
//   theme service has design tokens.  For now, use a fixed set of
//   well-tested accent colors.
constexpr const char* kStackColorPalette[] = {
    "#5B8FF9",  // Blue — default
    "#5AD8A6",  // Green
    "#F6BD16",  // Yellow
    "#E86452",  // Red
    "#6DC8EC",  // Cyan
    "#9270CA",  // Purple
    "#FF9D4D",  // Orange
    "#269A99",  // Teal
    "#FF99C3",  // Pink
    "#1E5CAB",  // Dark Blue
};

constexpr size_t kStackColorPaletteSize =
    sizeof(kStackColorPalette) / sizeof(kStackColorPalette[0]);

// Converts a base::Time to a double for pref serialization.
double TimeToDouble(base::Time time) {
  return time.is_null() ? 0.0 : time.ToDoubleT();
}

// Converts a double from pref serialization to base::Time.
base::Time DoubleToTime(double value) {
  if (value == 0.0) {
    return base::Time();
  }
  return base::Time::FromDoubleT(value);
}

}  // namespace

// =========================================================================
// Static pref registration
// =========================================================================

// static
void AstraTabStackService::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Stack default color.
  registry->RegisterStringPref(kPrefStackDefaultColor,
                               kDefaultStackColor);

  // Auto-collapse inactive stacks.
  registry->RegisterBooleanPref(kPrefStackAutoCollapseInactive,
                                kDefaultAutoCollapseInactive);

  // Auto-collapse after seconds.
  registry->RegisterIntegerPref(kPrefStackAutoCollapseAfterSeconds,
                                kDefaultAutoCollapseAfterSeconds);

  // Show stack count badge.
  registry->RegisterBooleanPref(kPrefStackShowCountBadge,
                                kDefaultShowCountBadge);

  // Show stack tab count.
  registry->RegisterBooleanPref(kPrefStackShowTabCount,
                                kDefaultShowTabCount);

  // Stack creation behavior.
  registry->RegisterIntegerPref(kPrefStackCreationBehavior,
                                kDefaultStackCreationBehavior);

  // Auto-stack related tabs.
  registry->RegisterBooleanPref(kPrefStackAutoStackRelated,
                                kDefaultAutoStackRelated);

  // Minimum tabs per stack.
  registry->RegisterIntegerPref(kPrefStackMinimumTabsPerStack,
                                kDefaultMinimumTabsPerStack);
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraTabStackService::AstraTabStackService(Profile* profile)
    : profile_(profile) {
  // Load stack metadata from the profile's PrefService.
  //
  // Chromium component: PrefService + profile initialization.
  // All persistence goes through PrefService — no custom file I/O.
  LoadFromPrefs();
}

AstraTabStackService::~AstraTabStackService() = default;

void AstraTabStackService::Shutdown() {
  // Notify observers of shutdown.
  for (auto& observer : observers_) {
    observer.OnTabStackServiceShutdown(this);
  }

  // Clear observer list and drop profile pointer before the profile goes away.
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers -------------------------------------------------------------

void AstraTabStackService::AddObserver(AstraTabStackObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraTabStackService::RemoveObserver(AstraTabStackObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Stack management
// =========================================================================

AstraTabStackId AstraTabStackService::CreateStack(
    const std::string& name,
    const std::string& color) {
  AstraTabStackInfo stack;
  stack.stack_id = GenerateStackId();
  stack.name = name.empty() ? "New Stack" : name;
  stack.color = color.empty() ? GetDefaultStackColor() : color;
  stack.is_collapsed = false;
  stack.is_pinned = false;
  stack.order_index = static_cast<int>(stacks_.size());
  stack.created_time = base::Time::Now();
  stack.last_accessed = base::Time::Now();
  stack.tab_count = 0;

  // Save ID before moving the stack object.
  const std::string stack_id = stack.stack_id;

  stacks_[stack_id] = std::move(stack);

  for (auto& observer : observers_) {
    observer.OnStackCreated(this, stack_id);
  }

  SaveToPrefs();
  return stack_id;
}

bool AstraTabStackService::DeleteStack(const std::string& stack_id) {
  auto it = stacks_.find(stack_id);
  if (it == stacks_.end()) {
    return false;
  }

  // Remove all tabs from this stack first (metadata cleanup).
  // In production, this would also update AstraTabFeatures on WebContents.
  //
  // TODO(astra): When integrating with TabStripModel / AstraTabFeatures,
  //   remove stack membership from actual WebContents here.
  //   Chromium owner: AstraTabFeatures (WebContentsUserData)
  std::vector<int> tabs_to_remove = it->second.tab_indices;
  for (int tab_index : tabs_to_remove) {
    for (auto& observer : observers_) {
      observer.OnTabRemovedFromStack(this, tab_index, stack_id);
    }
  }

  stacks_.erase(it);

  // Recompute order indices for remaining stacks.
  RecomputeOrderIndices();

  for (auto& observer : observers_) {
    observer.OnStackDeleted(this, stack_id);
  }

  SaveToPrefs();
  return true;
}

bool AstraTabStackService::RenameStack(const std::string& stack_id,
                                       const std::string& new_name) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }

  if (stack->name == new_name) {
    return true;
  }

  stack->name = new_name;
  TouchStack(stack);

  for (auto& observer : observers_) {
    observer.OnStackChanged(this, stack_id);
  }

  SaveToPrefs();
  return true;
}

bool AstraTabStackService::SetStackColor(const std::string& stack_id,
                                         const std::string& color) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }

  if (stack->color == color) {
    return true;
  }

  stack->color = color;
  TouchStack(stack);

  for (auto& observer : observers_) {
    observer.OnStackChanged(this, stack_id);
  }

  SaveToPrefs();
  return true;
}

const AstraTabStackInfo* AstraTabStackService::GetStack(
    const std::string& stack_id) const {
  return FindStack(stack_id);
}

std::vector<AstraTabStackInfo> AstraTabStackService::GetAllStacks() const {
  std::vector<AstraTabStackInfo> result;
  result.reserve(stacks_.size());

  for (const auto& [id, stack] : stacks_) {
    result.push_back(stack);
  }

  // Sort: pinned stacks first, then by order_index.
  base::ranges::sort(result, [](const AstraTabStackInfo& a,
                                const AstraTabStackInfo& b) {
    if (a.is_pinned != b.is_pinned) {
      return a.is_pinned;  // pinned comes first
    }
    return a.order_index < b.order_index;
  });

  return result;
}

size_t AstraTabStackService::GetStackCount() const {
  return stacks_.size();
}

bool AstraTabStackService::DoesStackExist(const std::string& stack_id) const {
  return stacks_.find(stack_id) != stacks_.end();
}

// =========================================================================
// Tab-to-stack membership
// =========================================================================

bool AstraTabStackService::AddTabToStack(int tab_index,
                                         const std::string& stack_id) {
  if (tab_index < 0) {
    return false;
  }

  // Validate that the stack exists.
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }

  // If already in this stack, nothing to do.
  if (stack->HasTab(tab_index)) {
    return false;
  }

  // If already in another stack, remove first.
  std::string old_stack_id = GetStackForTab(tab_index);
  if (!old_stack_id.empty()) {
    RemoveTabFromStack(tab_index, old_stack_id);
  }

  // Add to the new stack (at the end).
  stack->tab_indices.push_back(tab_index);
  stack->tab_count = stack->tab_indices.size();
  TouchStack(stack);

  for (auto& observer : observers_) {
    observer.OnTabAddedToStack(this, tab_index, stack_id);
  }

  SaveToPrefs();
  return true;
}

bool AstraTabStackService::RemoveTabFromStack(int tab_index,
                                              const std::string& stack_id) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }

  auto it = base::ranges::find(stack->tab_indices, tab_index);
  if (it == stack->tab_indices.end()) {
    return false;
  }

  stack->tab_indices.erase(it);
  stack->tab_count = stack->tab_indices.size();
  TouchStack(stack);

  for (auto& observer : observers_) {
    observer.OnTabRemovedFromStack(this, tab_index, stack_id);
  }

  SaveToPrefs();
  return true;
}

bool AstraTabStackService::RemoveTabFromAllStacks(int tab_index) {
  std::string stack_id = GetStackForTab(tab_index);
  if (stack_id.empty()) {
    return false;
  }

  return RemoveTabFromStack(tab_index, stack_id);
}

std::vector<int> AstraTabStackService::GetTabsInStack(
    const std::string& stack_id) const {
  const AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return {};
  }
  return stack->tab_indices;
}

size_t AstraTabStackService::GetTabCountInStack(
    const std::string& stack_id) const {
  const AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return 0;
  }
  return stack->tab_indices.size();
}

std::string AstraTabStackService::GetStackForTab(int tab_index) const {
  for (const auto& [id, stack] : stacks_) {
    if (stack.HasTab(tab_index)) {
      return id;
    }
  }
  return std::string();
}

bool AstraTabStackService::IsTabInStack(int tab_index,
                                        const std::string& stack_id) const {
  const AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }
  return stack->HasTab(tab_index);
}

bool AstraTabStackService::MoveTabInStack(int tab_index, int new_position) {
  std::string stack_id = GetStackForTab(tab_index);
  if (stack_id.empty()) {
    return false;
  }

  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }

  // Find current position.
  auto it = base::ranges::find(stack->tab_indices, tab_index);
  if (it == stack->tab_indices.end()) {
    return false;
  }

  int current_position = static_cast<int>(it - stack->tab_indices.begin());

  // Clamp new_position.
  if (new_position < 0) {
    new_position = 0;
  }
  if (new_position >= static_cast<int>(stack->tab_indices.size())) {
    new_position = static_cast<int>(stack->tab_indices.size()) - 1;
  }

  if (current_position == new_position) {
    return false;
  }

  // Move the tab.
  int value = *it;
  stack->tab_indices.erase(it);
  stack->tab_indices.insert(stack->tab_indices.begin() + new_position, value);

  TouchStack(stack);
  SaveToPrefs();
  return true;
}

size_t AstraTabStackService::MoveTabsToStack(
    const std::vector<int>& tab_indices,
    const std::string& stack_id) {
  if (!DoesStackExist(stack_id)) {
    return 0;
  }

  size_t added = 0;
  for (int tab_index : tab_indices) {
    if (AddTabToStack(tab_index, stack_id)) {
      ++added;
    }
  }

  return added;
}

// =========================================================================
// Stack operations
// =========================================================================

void AstraTabStackService::CollapseStack(const std::string& stack_id) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack || stack->is_collapsed) {
    return;
  }

  stack->is_collapsed = true;
  TouchStack(stack);

  for (auto& observer : observers_) {
    observer.OnStackCollapsedChanged(this, stack_id, true);
  }

  SaveToPrefs();
}

void AstraTabStackService::ExpandStack(const std::string& stack_id) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack || !stack->is_collapsed) {
    return;
  }

  stack->is_collapsed = false;
  TouchStack(stack);

  for (auto& observer : observers_) {
    observer.OnStackCollapsedChanged(this, stack_id, false);
  }

  SaveToPrefs();
}

bool AstraTabStackService::ToggleStackCollapsed(const std::string& stack_id) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }

  if (stack->is_collapsed) {
    ExpandStack(stack_id);
    return false;
  } else {
    CollapseStack(stack_id);
    return true;
  }
}

bool AstraTabStackService::IsStackCollapsed(const std::string& stack_id) const {
  const AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }
  return stack->is_collapsed;
}

size_t AstraTabStackService::CloseStack(const std::string& stack_id) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return 0;
  }

  size_t tab_count = stack->tab_indices.size();

  // Save for undo.
  closed_stacks_undo_[stack_id] = *stack;

  // Remove all tabs from the stack.
  std::vector<int> tabs = stack->tab_indices;
  for (int tab_index : tabs) {
    RemoveTabFromStack(tab_index, stack_id);
  }

  // TODO(astra): Actually close the tabs via TabStripModel.
  //   Chromium owner: TabStripModel::CloseWebContentsAt()
  //   For now, we just remove them from the stack (metadata only).

  return tab_count;
}

bool AstraTabStackService::UndoCloseStack(const std::string& stack_id) {
  auto undo_it = closed_stacks_undo_.find(stack_id);
  if (undo_it == closed_stacks_undo_.end()) {
    return false;
  }

  // If the stack still exists, restore its tabs.
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (stack) {
    // Restore tabs from the undo snapshot.
    for (int tab_index : undo_it->second.tab_indices) {
      AddTabToStack(tab_index, stack_id);
    }
  } else {
    // Stack was deleted too — recreate it.
    stacks_[stack_id] = undo_it->second;

    for (auto& observer : observers_) {
      observer.OnStackCreated(this, stack_id);
    }

    // Notify tab additions.
    for (int tab_index : undo_it->second.tab_indices) {
      for (auto& observer : observers_) {
        observer.OnTabAddedToStack(this, tab_index, stack_id);
      }
    }
  }

  closed_stacks_undo_.erase(undo_it);
  SaveToPrefs();
  return true;
}

// =========================================================================
// Stack metadata
// =========================================================================

bool AstraTabStackService::SetStackNote(const std::string& stack_id,
                                        const std::string& note) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }

  if (stack->note == note) {
    return true;
  }

  stack->note = note;
  TouchStack(stack);

  for (auto& observer : observers_) {
    observer.OnStackChanged(this, stack_id);
  }

  SaveToPrefs();
  return true;
}

std::string AstraTabStackService::GetStackNote(
    const std::string& stack_id) const {
  const AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return std::string();
  }
  return stack->note;
}

bool AstraTabStackService::SetStackPinned(const std::string& stack_id,
                                          bool pinned) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }

  if (stack->is_pinned == pinned) {
    return true;
  }

  stack->is_pinned = pinned;
  TouchStack(stack);

  // Recompute order since pinning affects sort order.
  RecomputeOrderIndices();

  for (auto& observer : observers_) {
    observer.OnStackChanged(this, stack_id);
    observer.OnStacksReordered(this);
  }

  SaveToPrefs();
  return true;
}

bool AstraTabStackService::IsStackPinned(const std::string& stack_id) const {
  const AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }
  return stack->is_pinned;
}

base::Time AstraTabStackService::GetStackLastAccessed(
    const std::string& stack_id) const {
  const AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return base::Time();
  }
  return stack->last_accessed;
}

bool AstraTabStackService::SetStackOrderIndex(const std::string& stack_id,
                                              int order_index) {
  AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return false;
  }

  if (stack->order_index == order_index) {
    return true;
  }

  stack->order_index = order_index;
  TouchStack(stack);

  // Normalize all order indices.
  auto all_stacks = GetAllStacks();
  int idx = 0;
  for (const auto& s : all_stacks) {
    AstraTabStackInfo* mutable_stack = FindStack(s.stack_id);
    if (mutable_stack) {
      mutable_stack->order_index = idx++;
    }
  }

  for (auto& observer : observers_) {
    observer.OnStacksReordered(this);
  }

  SaveToPrefs();
  return true;
}

int AstraTabStackService::GetStackOrderIndex(
    const std::string& stack_id) const {
  const AstraTabStackInfo* stack = FindStack(stack_id);
  if (!stack) {
    return -1;
  }
  return stack->order_index;
}

// =========================================================================
// Bulk operations
// =========================================================================

size_t AstraTabStackService::CloseAllStacks() {
  size_t count = stacks_.size();

  // Collect all stack IDs since we'll be modifying the map.
  std::vector<std::string> stack_ids;
  stack_ids.reserve(stacks_.size());
  for (const auto& [id, stack] : stacks_) {
    stack_ids.push_back(id);
  }

  for (const auto& id : stack_ids) {
    DeleteStack(id);
  }

  return count;
}

std::vector<AstraTabStackInfo> AstraTabStackService::GetStacksByColor(
    const std::string& color) const {
  std::vector<AstraTabStackInfo> result;

  for (const auto& [id, stack] : stacks_) {
    if (base::EqualsCaseInsensitiveASCII(stack.color, color)) {
      result.push_back(stack);
    }
  }

  // Sort by order_index.
  base::ranges::sort(result, [](const AstraTabStackInfo& a,
                                const AstraTabStackInfo& b) {
    return a.order_index < b.order_index;
  });

  return result;
}

std::vector<AstraTabStackInfo>
AstraTabStackService::GetRecentlyAccessedStacks(int max_count) const {
  std::vector<AstraTabStackInfo> result;
  result.reserve(stacks_.size());

  for (const auto& [id, stack] : stacks_) {
    result.push_back(stack);
  }

  // Sort by last_accessed descending.
  base::ranges::sort(result, [](const AstraTabStackInfo& a,
                                const AstraTabStackInfo& b) {
    return a.last_accessed > b.last_accessed;
  });

  if (max_count > 0 && static_cast<size_t>(max_count) < result.size()) {
    result.resize(static_cast<size_t>(max_count));
  }

  return result;
}

std::vector<AstraTabStackInfo> AstraTabStackService::SearchStacks(
    const std::string& query) const {
  if (query.empty()) {
    return GetAllStacks();
  }

  std::vector<AstraTabStackInfo> result;
  std::string lower_query = base::ToLowerASCII(query);

  for (const auto& [id, stack] : stacks_) {
    std::string lower_name = base::ToLowerASCII(stack.name);
    std::string lower_note = base::ToLowerASCII(stack.note);

    if (lower_name.find(lower_query) != std::string::npos ||
        lower_note.find(lower_query) != std::string::npos) {
      result.push_back(stack);
    }
  }

  // Sort by order_index.
  base::ranges::sort(result, [](const AstraTabStackInfo& a,
                                const AstraTabStackInfo& b) {
    return a.order_index < b.order_index;
  });

  return result;
}

bool AstraTabStackService::MergeStacks(const std::string& source_stack_id,
                                       const std::string& target_stack_id) {
  if (source_stack_id == target_stack_id) {
    return false;
  }

  AstraTabStackInfo* source = FindStack(source_stack_id);
  AstraTabStackInfo* target = FindStack(target_stack_id);
  if (!source || !target) {
    return false;
  }

  // Move all tabs from source to target.
  std::vector<int> tabs_to_move = source->tab_indices;
  for (int tab_index : tabs_to_move) {
    // Remove from source.
    RemoveTabFromStack(tab_index, source_stack_id);
    // Add to target.
    AddTabToStack(tab_index, target_stack_id);
  }

  // Delete the source stack.
  DeleteStack(source_stack_id);

  return true;
}

AstraTabStackId AstraTabStackService::DuplicateStack(
    const std::string& stack_id) {
  const AstraTabStackInfo* source = FindStack(stack_id);
  if (!source) {
    return std::string();
  }

  // Create new stack with same properties.
  AstraTabStackInfo dup;
  dup.stack_id = GenerateStackId();
  dup.name = source->name + " copy";
  dup.color = source->color;
  dup.is_collapsed = source->is_collapsed;
  dup.is_pinned = source->is_pinned;
  dup.note = source->note;
  dup.tab_indices = source->tab_indices;
  dup.tab_count = dup.tab_indices.size();
  dup.order_index = static_cast<int>(stacks_.size());
  dup.created_time = base::Time::Now();
  dup.last_accessed = base::Time::Now();

  // Save ID before moving the stack object.
  const std::string dup_id = dup.stack_id;

  stacks_[dup_id] = std::move(dup);

  for (auto& observer : observers_) {
    observer.OnStackCreated(this, dup_id);
  }

  SaveToPrefs();
  return dup_id;
}

// =========================================================================
// Settings
// =========================================================================

std::string AstraTabStackService::GetDefaultStackColor() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultStackColor;
  }
  return prefs->GetString(kPrefStackDefaultColor);
}

void AstraTabStackService::SetDefaultStackColor(const std::string& color) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetString(kPrefStackDefaultColor, color);
}

bool AstraTabStackService::GetAutoCollapseInactive() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultAutoCollapseInactive;
  }
  return prefs->GetBoolean(kPrefStackAutoCollapseInactive);
}

void AstraTabStackService::SetAutoCollapseInactive(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(kPrefStackAutoCollapseInactive, enabled);
}

int AstraTabStackService::GetAutoCollapseAfterSeconds() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultAutoCollapseAfterSeconds;
  }
  return prefs->GetInteger(kPrefStackAutoCollapseAfterSeconds);
}

void AstraTabStackService::SetAutoCollapseAfterSeconds(int seconds) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetInteger(kPrefStackAutoCollapseAfterSeconds, seconds);
}

bool AstraTabStackService::GetShowStackCountBadge() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultShowCountBadge;
  }
  return prefs->GetBoolean(kPrefStackShowCountBadge);
}

void AstraTabStackService::SetShowStackCountBadge(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(kPrefStackShowCountBadge, show);
}

bool AstraTabStackService::GetShowStackTabCount() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultShowTabCount;
  }
  return prefs->GetBoolean(kPrefStackShowTabCount);
}

void AstraTabStackService::SetShowStackTabCount(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(kPrefStackShowTabCount, show);
}

int AstraTabStackService::GetStackCreationBehavior() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultStackCreationBehavior;
  }
  return prefs->GetInteger(kPrefStackCreationBehavior);
}

void AstraTabStackService::SetStackCreationBehavior(int behavior) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetInteger(kPrefStackCreationBehavior, behavior);
}

bool AstraTabStackService::GetAutoStackRelated() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultAutoStackRelated;
  }
  return prefs->GetBoolean(kPrefStackAutoStackRelated);
}

void AstraTabStackService::SetAutoStackRelated(bool enabled) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(kPrefStackAutoStackRelated, enabled);
}

int AstraTabStackService::GetMinimumTabsPerStack() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultMinimumTabsPerStack;
  }
  return prefs->GetInteger(kPrefStackMinimumTabsPerStack);
}

void AstraTabStackService::SetMinimumTabsPerStack(int minimum) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }
  prefs->SetInteger(kPrefStackMinimumTabsPerStack, minimum);
}

// =========================================================================
// Stack utility
// =========================================================================

// static
std::vector<std::string> AstraTabStackService::GetStackColorPalette() {
  std::vector<std::string> palette;
  palette.reserve(kStackColorPaletteSize);
  for (size_t i = 0; i < kStackColorPaletteSize; ++i) {
    palette.emplace_back(kStackColorPalette[i]);
  }
  return palette;
}

const AstraTabStackInfo* AstraTabStackService::FindStackByName(
    const std::string& name) const {
  for (const auto& [id, stack] : stacks_) {
    if (base::EqualsCaseInsensitiveASCII(stack.name, name)) {
      return &stack;
    }
  }
  return nullptr;
}

size_t AstraTabStackService::CollapseAllStacks() {
  size_t changed_count = 0;

  for (auto& [id, stack] : stacks_) {
    if (!stack.is_collapsed) {
      stack.is_collapsed = true;
      ++changed_count;

      for (auto& observer : observers_) {
        observer.OnStackCollapsedChanged(this, id, true);
      }
    }
  }

  if (changed_count > 0) {
    SaveToPrefs();
  }

  return changed_count;
}

size_t AstraTabStackService::ExpandAllStacks() {
  size_t changed_count = 0;

  for (auto& [id, stack] : stacks_) {
    if (stack.is_collapsed) {
      stack.is_collapsed = false;
      ++changed_count;

      for (auto& observer : observers_) {
        observer.OnStackCollapsedChanged(this, id, false);
      }
    }
  }

  if (changed_count > 0) {
    SaveToPrefs();
  }

  return changed_count;
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraTabStackService::LoadFromPrefs() {
  DCHECK(profile_);
  PrefService* prefs = profile_->GetPrefs();
  DCHECK(prefs);

  const base::Value::List& list = prefs->GetList(prefs::kPrefTabStacks);
  stacks_.clear();

  for (const auto& item : list) {
    if (!item.is_dict()) {
      continue;
    }
    const base::Value::Dict& dict = item.GetDict();

    AstraTabStackInfo stack;

    const std::string* id = dict.FindString(kStackIdKey);
    if (!id || id->empty()) {
      continue;
    }
    stack.stack_id = *id;

    const std::string* name = dict.FindString(kStackNameKey);
    if (name) {
      stack.name = *name;
    }

    const std::string* color = dict.FindString(kStackColorKey);
    if (color) {
      stack.color = *color;
    } else {
      stack.color = kDefaultStackColor;
    }

    absl::optional<bool> collapsed = dict.FindBool(kStackCollapsedKey);
    if (collapsed.has_value()) {
      stack.is_collapsed = collapsed.value();
    }

    absl::optional<bool> pinned = dict.FindBool(kStackPinnedKey);
    if (pinned.has_value()) {
      stack.is_pinned = pinned.value();
    }

    const std::string* note = dict.FindString(kStackNoteKey);
    if (note) {
      stack.note = *note;
    }

    absl::optional<int> order_index = dict.FindInt(kStackOrderIndexKey);
    if (order_index.has_value()) {
      stack.order_index = order_index.value();
    }

    absl::optional<double> created_time =
        dict.FindDouble(kStackCreatedTimeKey);
    if (created_time.has_value()) {
      stack.created_time = DoubleToTime(created_time.value());
    }

    absl::optional<double> last_accessed =
        dict.FindDouble(kStackLastAccessedKey);
    if (last_accessed.has_value()) {
      stack.last_accessed = DoubleToTime(last_accessed.value());
    }

    const base::Value::List* tab_indices = dict.FindList(kStackTabIndicesKey);
    if (tab_indices) {
      for (const auto& val : *tab_indices) {
        if (val.is_int()) {
          stack.tab_indices.push_back(val.GetInt());
        }
      }
    }
    stack.tab_count = stack.tab_indices.size();

    stacks_[stack.stack_id] = std::move(stack);
  }

  RecomputeOrderIndices();
}

void AstraTabStackService::SaveToPrefs() {
  if (!profile_) {
    // Shutdown path: profile_ may be nulled in Shutdown().
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  DCHECK(prefs);

  // Get sorted list.
  auto sorted_stacks = GetAllStacks();

  base::Value::List list;
  list.reserve(sorted_stacks.size());

  for (const auto& stack : sorted_stacks) {
    base::Value::Dict dict;
    dict.Set(kStackIdKey, stack.stack_id);
    dict.Set(kStackNameKey, stack.name);
    dict.Set(kStackColorKey, stack.color);
    dict.Set(kStackCollapsedKey, stack.is_collapsed);
    dict.Set(kStackPinnedKey, stack.is_pinned);
    dict.Set(kStackNoteKey, stack.note);
    dict.Set(kStackOrderIndexKey, stack.order_index);
    dict.Set(kStackCreatedTimeKey, TimeToDouble(stack.created_time));
    dict.Set(kStackLastAccessedKey, TimeToDouble(stack.last_accessed));

    base::Value::List tab_indices_list;
    for (int idx : stack.tab_indices) {
      tab_indices_list.Append(idx);
    }
    dict.Set(kStackTabIndicesKey, std::move(tab_indices_list));

    list.Append(std::move(dict));
  }

  prefs->SetList(prefs::kPrefTabStacks, std::move(list));
}

AstraTabStackInfo* AstraTabStackService::FindStack(
    const std::string& stack_id) {
  auto it = stacks_.find(stack_id);
  return it == stacks_.end() ? nullptr : &it->second;
}

const AstraTabStackInfo* AstraTabStackService::FindStack(
    const std::string& stack_id) const {
  auto it = stacks_.find(stack_id);
  return it == stacks_.end() ? nullptr : &it->second;
}

// static
AstraTabStackId AstraTabStackService::GenerateStackId() {
  return base::UnguessableToken::Create().ToString();
}

void AstraTabStackService::TouchStack(AstraTabStackInfo* stack) {
  DCHECK(stack);
  stack->last_accessed = base::Time::Now();
}

void AstraTabStackService::RecomputeOrderIndices() {
  auto sorted = GetAllStacks();
  for (size_t i = 0; i < sorted.size(); ++i) {
    AstraTabStackInfo* stack = FindStack(sorted[i].stack_id);
    if (stack) {
      stack->order_index = static_cast<int>(i);
    }
  }
}

PrefService* AstraTabStackService::GetPrefs() const {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

}  // namespace astra
