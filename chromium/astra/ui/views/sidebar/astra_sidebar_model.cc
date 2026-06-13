#include "astra/ui/views/sidebar/astra_sidebar_model.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/observer_list.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Section ID constants
// =========================================================================

const char AstraSidebarModel::kSectionWorkspaces[] = "workspaces";
const char AstraSidebarModel::kSectionFavorites[] = "favorites";
const char AstraSidebarModel::kSectionPinnedTabs[] = "pinned_tabs";
const char AstraSidebarModel::kSectionOpenTabs[] = "open_tabs";
const char AstraSidebarModel::kSectionTabGroups[] = "tab_groups";
const char AstraSidebarModel::kSectionBookmarks[] = "bookmarks";
const char AstraSidebarModel::kSectionHistory[] = "history";
const char AstraSidebarModel::kSectionRecentlyClosed[] = "recently_closed";
const char AstraSidebarModel::kSectionReadingList[] = "reading_list";
const char AstraSidebarModel::kSectionNotes[] = "notes";
const char AstraSidebarModel::kSectionDownloads[] = "downloads";
const char AstraSidebarModel::kSectionPasswords[] = "passwords";
const char AstraSidebarModel::kSectionExtensions[] = "extensions";
const char AstraSidebarModel::kSectionDevTools[] = "devtools";

namespace {

// Helper: find a section in a vector by ID.
// Returns an iterator to the section, or sections->end() if not found.
std::vector<AstraSidebarSection>::iterator FindSectionById(
    std::vector<AstraSidebarSection>* sections,
    const std::string& section_id) {
  return std::find_if(
      sections->begin(), sections->end(),
      [&section_id](const AstraSidebarSection& s) { return s.id == section_id; });
}

std::vector<AstraSidebarSection>::const_iterator FindSectionByIdConst(
    const std::vector<AstraSidebarSection>& sections,
    const std::string& section_id) {
  return std::find_if(
      sections.begin(), sections.end(),
      [&section_id](const AstraSidebarSection& s) { return s.id == section_id; });
}

// Helper: update position indices in a sections vector to match their
// order in the vector.
void UpdatePositions(std::vector<AstraSidebarSection>& sections) {
  for (size_t i = 0; i < sections.size(); ++i) {
    sections[i].position = static_cast<int>(i);
  }
}

}  // namespace

// =========================================================================
// AstraSidebarModel
// =========================================================================

AstraSidebarModel::AstraSidebarModel(PrefService* pref_service)
    : pref_service_(pref_service), sections_(GetDefaultSections()) {
  if (pref_service_) {
    LoadFromPrefs();
  }
  // Set the initial active section.
  if (remember_last_section() && !last_active_section().empty()) {
    active_section_id_ = last_active_section();
  } else if (!default_active_section().empty()) {
    active_section_id_ = default_active_section();
  } else {
    active_section_id_ = kSectionOpenTabs;
  }
}

AstraSidebarModel::~AstraSidebarModel() = default;

// -- Observers -------------------------------------------------------------

void AstraSidebarModel::AddObserver(AstraSidebarModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraSidebarModel::RemoveObserver(AstraSidebarModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Sidebar state ---------------------------------------------------------

bool AstraSidebarModel::is_visible() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarVisible;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarVisible);
}

void AstraSidebarModel::SetVisible(bool visible) {
  if (is_visible() == visible) {
    return;
  }
  if (pref_service_) {
    pref_service_->SetBoolean(prefs::kPrefSidebarVisible, visible);
  }
  if (visible) {
    NotifySidebarShown();
  } else {
    NotifySidebarHidden();
  }
}

void AstraSidebarModel::ToggleVisible() {
  SetVisible(!is_visible());
}

bool AstraSidebarModel::is_pinned() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarPinned;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarPinned);
}

void AstraSidebarModel::SetPinned(bool pinned) {
  if (is_pinned() == pinned) {
    return;
  }
  if (pref_service_) {
    pref_service_->SetBoolean(prefs::kPrefSidebarPinned, pinned);
  }
  NotifySidebarPinnedChanged(pinned);
}

void AstraSidebarModel::TogglePinned() {
  SetPinned(!is_pinned());
}

int AstraSidebarModel::width() const {
  if (!pref_service_) {
    return kDefaultWidth;
  }
  return ClampWidth(pref_service_->GetInteger(prefs::kPrefSidebarWidth));
}

void AstraSidebarModel::SetWidth(int width) {
  int clamped = ClampWidth(width);
  if (this->width() == clamped) {
    return;
  }
  if (pref_service_) {
    pref_service_->SetInteger(prefs::kPrefSidebarWidth, clamped);
  }
  NotifySidebarWidthChanged(clamped);
}

AstraSidebarPosition AstraSidebarModel::position() const {
  if (!pref_service_) {
    return AstraSidebarPosition::kLeft;
  }
  std::string pos = pref_service_->GetString(prefs::kPrefSidebarPosition);
  if (pos == "right") {
    return AstraSidebarPosition::kRight;
  }
  return AstraSidebarPosition::kLeft;
}

void AstraSidebarModel::SetPosition(AstraSidebarPosition position) {
  if (this->position() == position) {
    return;
  }
  if (pref_service_) {
    std::string pos_str =
        position == AstraSidebarPosition::kRight ? "right" : "left";
    pref_service_->SetString(prefs::kPrefSidebarPosition, pos_str);
  }
  NotifySidebarPositionChanged(position);
}

// -- Active section --------------------------------------------------------

void AstraSidebarModel::SetActiveSection(const std::string& section_id) {
  if (active_section_id_ == section_id) {
    return;
  }
  // Validate that the section exists.
  auto it = FindSectionByIdConst(sections_, section_id);
  if (it == sections_.end()) {
    return;
  }
  active_section_id_ = section_id;

  // Persist as last active section if remember_last_section is enabled.
  if (remember_last_section() && pref_service_) {
    pref_service_->SetString(prefs::kPrefSidebarLastActiveSection, section_id);
  }

  NotifyActiveSectionChanged(section_id);
}

// -- Section management ----------------------------------------------------

std::vector<AstraSidebarSection> AstraSidebarModel::GetAllSections() const {
  return sections_;
}

std::vector<AstraSidebarSection> AstraSidebarModel::GetVisibleSections() const {
  std::vector<AstraSidebarSection> visible;
  for (const auto& section : sections_) {
    if (section.is_visible) {
      visible.push_back(section);
    }
  }
  return visible;
}

std::vector<std::string> AstraSidebarModel::GetCollapsedSectionIds() const {
  std::vector<std::string> collapsed;
  for (const auto& section : sections_) {
    if (section.is_collapsed) {
      collapsed.push_back(section.id);
    }
  }
  return collapsed;
}

absl::optional<AstraSidebarSection> AstraSidebarModel::GetSectionById(
    const std::string& section_id) const {
  auto it = FindSectionByIdConst(sections_, section_id);
  if (it == sections_.end()) {
    return absl::nullopt;
  }
  return *it;
}

bool AstraSidebarModel::SetSectionVisible(const std::string& section_id,
                                           bool visible) {
  auto it = FindSectionById(&sections_, section_id);
  if (it == sections_.end()) {
    return false;
  }
  if (it->is_visible == visible) {
    return true;
  }
  it->is_visible = visible;

  // Update corresponding pref if there's one for this section.
  if (pref_service_) {
    if (section_id == kSectionTabGroups) {
      pref_service_->SetBoolean(prefs::kPrefSidebarShowTabGroupsSection,
                                visible);
    } else if (section_id == kSectionHistory) {
      pref_service_->SetBoolean(prefs::kPrefSidebarShowHistorySection, visible);
    } else if (section_id == kSectionRecentlyClosed) {
      pref_service_->SetBoolean(prefs::kPrefSidebarShowRecentlyClosedSection,
                                visible);
    } else if (section_id == kSectionReadingList) {
      pref_service_->SetBoolean(prefs::kPrefSidebarShowReadingListSection,
                                visible);
    } else if (section_id == kSectionNotes) {
      pref_service_->SetBoolean(prefs::kPrefSidebarShowNotesSection, visible);
    } else if (section_id == kSectionDownloads) {
      pref_service_->SetBoolean(prefs::kPrefSidebarShowDownloadsSection,
                                visible);
    } else if (section_id == kSectionPasswords) {
      pref_service_->SetBoolean(prefs::kPrefSidebarShowPasswordsSection,
                                visible);
    } else if (section_id == kSectionExtensions) {
      pref_service_->SetBoolean(prefs::kPrefSidebarShowExtensionsSection,
                                visible);
    }
  }

  NotifySectionVisibilityChanged(section_id, visible);
  return true;
}

bool AstraSidebarModel::ToggleSectionVisible(const std::string& section_id) {
  auto it = FindSectionByIdConst(sections_, section_id);
  if (it == sections_.end()) {
    return false;
  }
  return SetSectionVisible(section_id, !it->is_visible);
}

bool AstraSidebarModel::ToggleSectionCollapsed(const std::string& section_id) {
  auto it = FindSectionById(&sections_, section_id);
  if (it == sections_.end() || !it->is_collapsible) {
    return false;
  }
  return SetSectionCollapsed(section_id, !it->is_collapsed);
}

bool AstraSidebarModel::SetSectionCollapsed(const std::string& section_id,
                                             bool collapsed) {
  auto it = FindSectionById(&sections_, section_id);
  if (it == sections_.end() || !it->is_collapsible) {
    return false;
  }
  if (it->is_collapsed == collapsed) {
    return true;
  }
  it->is_collapsed = collapsed;
  SaveCollapsedSectionsToPrefs();
  NotifySectionCollapsedChanged(section_id, collapsed);
  return true;
}

bool AstraSidebarModel::MoveSection(const std::string& section_id,
                                     int new_position) {
  auto it = FindSectionById(&sections_, section_id);
  if (it == sections_.end()) {
    return false;
  }

  int old_position = it->position;
  if (old_position == new_position) {
    return true;
  }

  // Clamp new_position to valid range.
  int max_position = static_cast<int>(sections_.size()) - 1;
  if (new_position < 0) {
    new_position = 0;
  }
  if (new_position > max_position) {
    new_position = max_position;
  }

  // Extract the section and reinsert at the new position.
  AstraSidebarSection section = *it;
  sections_.erase(it);
  sections_.insert(sections_.begin() + new_position, section);
  UpdatePositions(sections_);

  SaveSectionOrderToPrefs();
  NotifySectionOrderChanged();
  return true;
}

bool AstraSidebarModel::ReorderSections(
    const std::vector<std::string>& section_ids_in_order) {
  // Validate: all IDs must exist.
  for (const auto& id : section_ids_in_order) {
    if (FindSectionByIdConst(sections_, id) == sections_.end()) {
      return false;
    }
  }
  // Validate: number of IDs matches number of sections.
  if (section_ids_in_order.size() != sections_.size()) {
    return false;
  }

  // Reorder sections based on the provided IDs.
  std::vector<AstraSidebarSection> new_order;
  new_order.reserve(sections_.size());
  for (const auto& id : section_ids_in_order) {
    auto it = FindSectionById(&sections_, id);
    DCHECK(it != sections_.end());
    new_order.push_back(*it);
  }

  sections_ = std::move(new_order);
  UpdatePositions(sections_);

  SaveSectionOrderToPrefs();
  NotifySectionOrderChanged();
  return true;
}

size_t AstraSidebarModel::GetSectionCount() const {
  return sections_.size();
}

size_t AstraSidebarModel::GetVisibleSectionCount() const {
  size_t count = 0;
  for (const auto& section : sections_) {
    if (section.is_visible) {
      ++count;
    }
  }
  return count;
}

// -- Bulk operations -------------------------------------------------------

void AstraSidebarModel::SetAllSectionsVisible(bool visible) {
  bool changed = false;
  for (auto& section : sections_) {
    if (section.is_visible != visible) {
      section.is_visible = visible;
      changed = true;
    }
  }
  if (!changed) {
    return;
  }
  // Update per-section prefs.
  if (pref_service_) {
    pref_service_->SetBoolean(prefs::kPrefSidebarShowTabGroupsSection,
                              visible);
    pref_service_->SetBoolean(prefs::kPrefSidebarShowHistorySection, visible);
    pref_service_->SetBoolean(prefs::kPrefSidebarShowRecentlyClosedSection,
                              visible);
    pref_service_->SetBoolean(prefs::kPrefSidebarShowReadingListSection,
                              visible);
    pref_service_->SetBoolean(prefs::kPrefSidebarShowNotesSection, visible);
    pref_service_->SetBoolean(prefs::kPrefSidebarShowDownloadsSection,
                              visible);
    pref_service_->SetBoolean(prefs::kPrefSidebarShowPasswordsSection,
                              visible);
    pref_service_->SetBoolean(prefs::kPrefSidebarShowExtensionsSection,
                              visible);
  }
  // Notify for each section (views may need per-section updates).
  for (const auto& section : sections_) {
    NotifySectionVisibilityChanged(section.id, visible);
  }
}

void AstraSidebarModel::ToggleMultipleSections(
    const std::vector<std::string>& section_ids) {
  for (const auto& id : section_ids) {
    ToggleSectionVisible(id);
  }
}

void AstraSidebarModel::CollapseAllSections() {
  bool changed = false;
  for (auto& section : sections_) {
    if (section.is_collapsible && !section.is_collapsed) {
      section.is_collapsed = true;
      changed = true;
    }
  }
  if (!changed) {
    return;
  }
  SaveCollapsedSectionsToPrefs();
  for (const auto& section : sections_) {
    if (section.is_collapsible) {
      NotifySectionCollapsedChanged(section.id, true);
    }
  }
}

void AstraSidebarModel::ExpandAllSections() {
  bool changed = false;
  for (auto& section : sections_) {
    if (section.is_collapsible && section.is_collapsed) {
      section.is_collapsed = false;
      changed = true;
    }
  }
  if (!changed) {
    return;
  }
  SaveCollapsedSectionsToPrefs();
  for (const auto& section : sections_) {
    if (section.is_collapsible) {
      NotifySectionCollapsedChanged(section.id, false);
    }
  }
}

void AstraSidebarModel::ResetAllSettings() {
  if (!pref_service_) {
    return;
  }
  // Reset all sidebar prefs to their default values.
  pref_service_->ClearPref(prefs::kPrefSidebarVisible);
  pref_service_->ClearPref(prefs::kPrefSidebarWidth);
  pref_service_->ClearPref(prefs::kPrefSidebarPinned);
  pref_service_->ClearPref(prefs::kPrefSidebarPosition);
  pref_service_->ClearPref(prefs::kPrefSidebarAutoHide);
  pref_service_->ClearPref(prefs::kPrefSidebarPinnedSections);
  pref_service_->ClearPref(prefs::kPrefSidebarShowSectionIcons);
  pref_service_->ClearPref(prefs::kPrefSidebarShowSectionLabels);
  pref_service_->ClearPref(prefs::kPrefSidebarCompactMode);
  pref_service_->ClearPref(prefs::kPrefSidebarAutoHideOnTabClick);
  pref_service_->ClearPref(prefs::kPrefSidebarShowTabCountBadges);
  pref_service_->ClearPref(prefs::kPrefSidebarShowWorkspaceBadge);
  pref_service_->ClearPref(prefs::kPrefSidebarShowTabGroupsSection);
  pref_service_->ClearPref(prefs::kPrefSidebarShowHistorySection);
  pref_service_->ClearPref(prefs::kPrefSidebarShowRecentlyClosedSection);
  pref_service_->ClearPref(prefs::kPrefSidebarShowReadingListSection);
  pref_service_->ClearPref(prefs::kPrefSidebarShowNotesSection);
  pref_service_->ClearPref(prefs::kPrefSidebarShowDownloadsSection);
  pref_service_->ClearPref(prefs::kPrefSidebarShowPasswordsSection);
  pref_service_->ClearPref(prefs::kPrefSidebarShowExtensionsSection);
  pref_service_->ClearPref(prefs::kPrefSidebarAnimationEnabled);
  pref_service_->ClearPref(prefs::kPrefSidebarRememberLastSection);
  pref_service_->ClearPref(prefs::kPrefSidebarLastActiveSection);
  pref_service_->ClearPref(prefs::kPrefSidebarDefaultActiveSection);
  pref_service_->ClearPref(prefs::kPrefSidebarSectionOrder);
  pref_service_->ClearPref(prefs::kPrefSidebarCollapsedSections);

  // Re-sync section visibility with defaults.
  SyncSectionVisibilityWithPrefs();

  NotifySidebarSettingsChanged();
}

void AstraSidebarModel::ResetSectionsToDefaults() {
  sections_ = GetDefaultSections();
  SyncSectionVisibilityWithPrefs();
  SaveSectionOrderToPrefs();
  SaveCollapsedSectionsToPrefs();
  NotifySectionOrderChanged();
  // Notify visibility changes for all sections.
  for (const auto& section : sections_) {
    NotifySectionVisibilityChanged(section.id, section.is_visible);
    NotifySectionCollapsedChanged(section.id, section.is_collapsed);
  }
}

// -- Presentation settings -------------------------------------------------

bool AstraSidebarModel::show_section_icons() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowSectionIcons;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowSectionIcons);
}

void AstraSidebarModel::SetShowSectionIcons(bool show) {
  if (!pref_service_ || show_section_icons() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefSidebarShowSectionIcons, show);
  NotifySidebarSettingsChanged();
}

bool AstraSidebarModel::show_section_labels() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowSectionLabels;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowSectionLabels);
}

void AstraSidebarModel::SetShowSectionLabels(bool show) {
  if (!pref_service_ || show_section_labels() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefSidebarShowSectionLabels, show);
  NotifySidebarSettingsChanged();
}

bool AstraSidebarModel::compact_mode() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarCompactMode;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarCompactMode);
}

void AstraSidebarModel::SetCompactMode(bool enabled) {
  if (!pref_service_ || compact_mode() == enabled) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefSidebarCompactMode, enabled);
  NotifySidebarSettingsChanged();
}

bool AstraSidebarModel::auto_hide_on_tab_click() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarAutoHideOnTabClick;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarAutoHideOnTabClick);
}

void AstraSidebarModel::SetAutoHideOnTabClick(bool enabled) {
  if (!pref_service_ || auto_hide_on_tab_click() == enabled) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefSidebarAutoHideOnTabClick, enabled);
  NotifySidebarSettingsChanged();
}

bool AstraSidebarModel::show_tab_count_badges() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowTabCountBadges;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowTabCountBadges);
}

void AstraSidebarModel::SetShowTabCountBadges(bool show) {
  if (!pref_service_ || show_tab_count_badges() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefSidebarShowTabCountBadges, show);
  NotifySidebarSettingsChanged();
}

bool AstraSidebarModel::show_workspace_badge() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowWorkspaceBadge;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowWorkspaceBadge);
}

void AstraSidebarModel::SetShowWorkspaceBadge(bool show) {
  if (!pref_service_ || show_workspace_badge() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefSidebarShowWorkspaceBadge, show);
  NotifySidebarSettingsChanged();
}

bool AstraSidebarModel::animation_enabled() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarAnimationEnabled;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarAnimationEnabled);
}

void AstraSidebarModel::SetAnimationEnabled(bool enabled) {
  if (!pref_service_ || animation_enabled() == enabled) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefSidebarAnimationEnabled, enabled);
  NotifySidebarSettingsChanged();
}

bool AstraSidebarModel::remember_last_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarRememberLastSection;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarRememberLastSection);
}

void AstraSidebarModel::SetRememberLastSection(bool remember) {
  if (!pref_service_ || remember_last_section() == remember) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefSidebarRememberLastSection, remember);
  NotifySidebarSettingsChanged();
}

std::string AstraSidebarModel::default_active_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarDefaultActiveSection;
  }
  return pref_service_->GetString(prefs::kPrefSidebarDefaultActiveSection);
}

void AstraSidebarModel::SetDefaultActiveSection(const std::string& section_id) {
  if (!pref_service_ || default_active_section() == section_id) {
    return;
  }
  pref_service_->SetString(prefs::kPrefSidebarDefaultActiveSection, section_id);
  NotifySidebarSettingsChanged();
}

std::string AstraSidebarModel::last_active_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarLastActiveSection;
  }
  return pref_service_->GetString(prefs::kPrefSidebarLastActiveSection);
}

void AstraSidebarModel::SetLastActiveSection(const std::string& section_id) {
  if (!pref_service_ || last_active_section() == section_id) {
    return;
  }
  pref_service_->SetString(prefs::kPrefSidebarLastActiveSection, section_id);
  NotifySidebarSettingsChanged();
}

// -- Section visibility settings -------------------------------------------

bool AstraSidebarModel::show_tab_groups_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowTabGroupsSection;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowTabGroupsSection);
}

void AstraSidebarModel::SetShowTabGroupsSection(bool show) {
  SetSectionVisible(kSectionTabGroups, show);
}

bool AstraSidebarModel::show_history_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowHistorySection;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowHistorySection);
}

void AstraSidebarModel::SetShowHistorySection(bool show) {
  SetSectionVisible(kSectionHistory, show);
}

bool AstraSidebarModel::show_recently_closed_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowRecentlyClosedSection;
  }
  return pref_service_->GetBoolean(
      prefs::kPrefSidebarShowRecentlyClosedSection);
}

void AstraSidebarModel::SetShowRecentlyClosedSection(bool show) {
  SetSectionVisible(kSectionRecentlyClosed, show);
}

bool AstraSidebarModel::show_reading_list_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowReadingListSection;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowReadingListSection);
}

void AstraSidebarModel::SetShowReadingListSection(bool show) {
  SetSectionVisible(kSectionReadingList, show);
}

bool AstraSidebarModel::show_notes_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowNotesSection;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowNotesSection);
}

void AstraSidebarModel::SetShowNotesSection(bool show) {
  SetSectionVisible(kSectionNotes, show);
}

bool AstraSidebarModel::show_downloads_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowDownloadsSection;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowDownloadsSection);
}

void AstraSidebarModel::SetShowDownloadsSection(bool show) {
  SetSectionVisible(kSectionDownloads, show);
}

bool AstraSidebarModel::show_passwords_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowPasswordsSection;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowPasswordsSection);
}

void AstraSidebarModel::SetShowPasswordsSection(bool show) {
  SetSectionVisible(kSectionPasswords, show);
}

bool AstraSidebarModel::show_extensions_section() const {
  if (!pref_service_) {
    return prefs::kDefaultSidebarShowExtensionsSection;
  }
  return pref_service_->GetBoolean(prefs::kPrefSidebarShowExtensionsSection);
}

void AstraSidebarModel::SetShowExtensionsSection(bool show) {
  SetSectionVisible(kSectionExtensions, show);
}

// -- Utility methods -------------------------------------------------------

// static
std::vector<AstraSidebarSection> AstraSidebarModel::GetDefaultSections() {
  std::vector<AstraSidebarSection> sections;

  sections.push_back({
      .id = kSectionWorkspaces,
      .name = u"Workspaces",
      .icon_id = "workspaces",
      .is_visible = true,
      .position = 0,
      .is_collapsible = false,
      .is_collapsed = false,
  });

  sections.push_back({
      .id = kSectionFavorites,
      .name = u"Favorites",
      .icon_id = "favorites",
      .is_visible = true,
      .position = 1,
      .is_collapsible = true,
      .is_collapsed = false,
  });

  sections.push_back({
      .id = kSectionPinnedTabs,
      .name = u"Pinned",
      .icon_id = "pinned",
      .is_visible = true,
      .position = 2,
      .is_collapsible = true,
      .is_collapsed = false,
  });

  sections.push_back({
      .id = kSectionOpenTabs,
      .name = u"Tabs",
      .icon_id = "tabs",
      .is_visible = true,
      .position = 3,
      .is_collapsible = true,
      .is_collapsed = false,
  });

  sections.push_back({
      .id = kSectionTabGroups,
      .name = u"Tab Groups",
      .icon_id = "tab_groups",
      .is_visible = prefs::kDefaultSidebarShowTabGroupsSection,
      .position = 4,
      .is_collapsible = true,
      .is_collapsed = false,
  });

  sections.push_back({
      .id = kSectionBookmarks,
      .name = u"Bookmarks",
      .icon_id = "bookmarks",
      .is_visible = true,
      .position = 5,
      .is_collapsible = true,
      .is_collapsed = true,
  });

  sections.push_back({
      .id = kSectionHistory,
      .name = u"History",
      .icon_id = "history",
      .is_visible = prefs::kDefaultSidebarShowHistorySection,
      .position = 6,
      .is_collapsible = true,
      .is_collapsed = true,
  });

  sections.push_back({
      .id = kSectionRecentlyClosed,
      .name = u"Recently Closed",
      .icon_id = "recently_closed",
      .is_visible = prefs::kDefaultSidebarShowRecentlyClosedSection,
      .position = 7,
      .is_collapsible = true,
      .is_collapsed = true,
  });

  sections.push_back({
      .id = kSectionReadingList,
      .name = u"Reading List",
      .icon_id = "reading_list",
      .is_visible = prefs::kDefaultSidebarShowReadingListSection,
      .position = 8,
      .is_collapsible = true,
      .is_collapsed = true,
  });

  sections.push_back({
      .id = kSectionNotes,
      .name = u"Notes",
      .icon_id = "notes",
      .is_visible = prefs::kDefaultSidebarShowNotesSection,
      .position = 9,
      .is_collapsible = true,
      .is_collapsed = true,
  });

  sections.push_back({
      .id = kSectionDownloads,
      .name = u"Downloads",
      .icon_id = "downloads",
      .is_visible = prefs::kDefaultSidebarShowDownloadsSection,
      .position = 10,
      .is_collapsible = true,
      .is_collapsed = true,
  });

  sections.push_back({
      .id = kSectionPasswords,
      .name = u"Passwords",
      .icon_id = "passwords",
      .is_visible = prefs::kDefaultSidebarShowPasswordsSection,
      .position = 11,
      .is_collapsible = true,
      .is_collapsed = true,
  });

  sections.push_back({
      .id = kSectionExtensions,
      .name = u"Extensions",
      .icon_id = "extensions",
      .is_visible = prefs::kDefaultSidebarShowExtensionsSection,
      .position = 12,
      .is_collapsible = true,
      .is_collapsed = true,
  });

  sections.push_back({
      .id = kSectionDevTools,
      .name = u"DevTools",
      .icon_id = "devtools",
      .is_visible = true,
      .position = 13,
      .is_collapsible = true,
      .is_collapsed = true,
  });

  UpdatePositions(sections);
  return sections;
}

// static
int AstraSidebarModel::ClampWidth(int width) {
  if (width < kMinWidth) {
    return kMinWidth;
  }
  if (width > kMaxWidth) {
    return kMaxWidth;
  }
  return width;
}

// -- Private helpers -------------------------------------------------------

void AstraSidebarModel::LoadFromPrefs() {
  DCHECK(pref_service_);

  LoadSectionOrderFromPrefs();
  LoadCollapsedSectionsFromPrefs();
  SyncSectionVisibilityWithPrefs();
}

void AstraSidebarModel::SaveSectionOrderToPrefs() {
  if (!pref_service_) {
    return;
  }
  base::Value::List order;
  for (const auto& section : sections_) {
    order.Append(section.id);
  }
  pref_service_->SetList(prefs::kPrefSidebarSectionOrder, std::move(order));
}

void AstraSidebarModel::SaveCollapsedSectionsToPrefs() {
  if (!pref_service_) {
    return;
  }
  base::Value::List collapsed;
  for (const auto& section : sections_) {
    if (section.is_collapsed) {
      collapsed.Append(section.id);
    }
  }
  pref_service_->SetList(prefs::kPrefSidebarCollapsedSections,
                         std::move(collapsed));
}

void AstraSidebarModel::LoadSectionOrderFromPrefs() {
  DCHECK(pref_service_);
  const base::Value::List& order_list =
      pref_service_->GetList(prefs::kPrefSidebarSectionOrder);
  if (order_list.empty()) {
    return;
  }

  // Build a new order from the pref list, skipping unknown IDs.
  std::vector<AstraSidebarSection> new_order;
  new_order.reserve(sections_.size());

  std::vector<std::string> ordered_ids;
  for (const auto& val : order_list) {
    if (val.is_string()) {
      ordered_ids.push_back(val.GetString());
    }
  }

  // Add sections in the order specified by the pref.
  for (const auto& id : ordered_ids) {
    auto it = FindSectionById(&sections_, id);
    if (it != sections_.end()) {
      new_order.push_back(*it);
    }
  }

  // Add any remaining sections that weren't in the order list.
  for (const auto& section : sections_) {
    auto it = std::find(ordered_ids.begin(), ordered_ids.end(), section.id);
    if (it == ordered_ids.end()) {
      new_order.push_back(section);
    }
  }

  if (new_order.size() == sections_.size()) {
    sections_ = std::move(new_order);
    UpdatePositions(sections_);
  }
}

void AstraSidebarModel::LoadCollapsedSectionsFromPrefs() {
  DCHECK(pref_service_);
  const base::Value::List& collapsed_list =
      pref_service_->GetList(prefs::kPrefSidebarCollapsedSections);
  if (collapsed_list.empty()) {
    return;
  }

  // First uncollapse everything.
  for (auto& section : sections_) {
    section.is_collapsed = false;
  }

  // Collapse the sections listed in the pref.
  for (const auto& val : collapsed_list) {
    if (!val.is_string()) {
      continue;
    }
    std::string id = val.GetString();
    auto it = FindSectionById(&sections_, id);
    if (it != sections_.end() && it->is_collapsible) {
      it->is_collapsed = true;
    }
  }
}

void AstraSidebarModel::SyncSectionVisibilityWithPrefs() {
  if (!pref_service_) {
    return;
  }
  // Update section visibility from per-section prefs.
  auto update_visible = [&](const std::string& id, bool visible) {
    auto it = FindSectionById(&sections_, id);
    if (it != sections_.end()) {
      it->is_visible = visible;
    }
  };

  update_visible(kSectionTabGroups, show_tab_groups_section());
  update_visible(kSectionHistory, show_history_section());
  update_visible(kSectionRecentlyClosed, show_recently_closed_section());
  update_visible(kSectionReadingList, show_reading_list_section());
  update_visible(kSectionNotes, show_notes_section());
  update_visible(kSectionDownloads, show_downloads_section());
  update_visible(kSectionPasswords, show_passwords_section());
  update_visible(kSectionExtensions, show_extensions_section());
}

// -- Observer notification helpers -----------------------------------------

void AstraSidebarModel::NotifySidebarShown() {
  for (auto& observer : observers_) {
    observer.OnSidebarShown();
  }
}

void AstraSidebarModel::NotifySidebarHidden() {
  for (auto& observer : observers_) {
    observer.OnSidebarHidden();
  }
}

void AstraSidebarModel::NotifySidebarPinnedChanged(bool pinned) {
  for (auto& observer : observers_) {
    observer.OnSidebarPinnedChanged(pinned);
  }
}

void AstraSidebarModel::NotifyActiveSectionChanged(
    const std::string& section_id) {
  for (auto& observer : observers_) {
    observer.OnActiveSectionChanged(section_id);
  }
}

void AstraSidebarModel::NotifySectionVisibilityChanged(
    const std::string& section_id,
    bool visible) {
  for (auto& observer : observers_) {
    observer.OnSectionVisibilityChanged(section_id, visible);
  }
}

void AstraSidebarModel::NotifySectionOrderChanged() {
  for (auto& observer : observers_) {
    observer.OnSectionOrderChanged();
  }
}

void AstraSidebarModel::NotifySectionCollapsedChanged(
    const std::string& section_id,
    bool collapsed) {
  for (auto& observer : observers_) {
    observer.OnSectionCollapsedChanged(section_id, collapsed);
  }
}

void AstraSidebarModel::NotifySidebarWidthChanged(int width) {
  for (auto& observer : observers_) {
    observer.OnSidebarWidthChanged(width);
  }
}

void AstraSidebarModel::NotifySidebarPositionChanged(
    AstraSidebarPosition position) {
  for (auto& observer : observers_) {
    observer.OnSidebarPositionChanged(position);
  }
}

void AstraSidebarModel::NotifySidebarSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnSidebarSettingsChanged();
  }
}

}  // namespace astra
