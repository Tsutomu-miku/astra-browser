#include "astra/ui/views/omnibox/astra_omnibox_decoration_model.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/prefs/pref_service.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Helper to create a default decoration item for a given type.
AstraOmniboxDecorationItem MakeDefaultDecoration(AstraOmniboxDecorationType type,
                                                  int order_index) {
  AstraOmniboxDecorationItem item;
  item.type = type;
  item.is_visible = true;
  item.is_active = false;
  item.order_index = order_index;
  item.has_bubble = false;
  item.badge_color = SK_ColorTRANSPARENT;

  switch (type) {
    case AstraOmniboxDecorationType::kWorkspaceIndicator:
      item.icon = "workspace";
      item.tooltip = u"Current workspace";
      item.accessibility_label = u"Workspace indicator";
      item.has_bubble = true;
      break;
    case AstraOmniboxDecorationType::kFocusModeBadge:
      item.icon = "focus_mode";
      item.tooltip = u"Focus mode is active";
      item.accessibility_label = u"Focus mode badge";
      break;
    case AstraOmniboxDecorationType::kTabStackIndicator:
      item.icon = "tab_stack";
      item.tooltip = u"Tab stack";
      item.accessibility_label = u"Tab stack indicator";
      item.has_bubble = true;
      break;
    case AstraOmniboxDecorationType::kReadingListBadge:
      item.icon = "reading_list";
      item.tooltip = u"Add to reading list";
      item.accessibility_label = u"Reading list button";
      break;
    case AstraOmniboxDecorationType::kNoteBadge:
      item.icon = "note";
      item.tooltip = u"Add note";
      item.accessibility_label = u"Note button";
      item.has_bubble = true;
      break;
    case AstraOmniboxDecorationType::kFavoriteStar:
      item.icon = "star";
      item.tooltip = u"Bookmark this page";
      item.accessibility_label = u"Bookmark star";
      break;
    case AstraOmniboxDecorationType::kSidebarToggle:
      item.icon = "sidebar";
      item.tooltip = u"Toggle sidebar";
      item.accessibility_label = u"Sidebar toggle";
      break;
    case AstraOmniboxDecorationType::kSplitViewToggle:
      item.icon = "split_view";
      item.tooltip = u"Toggle split view";
      item.accessibility_label = u"Split view button";
      break;
    case AstraOmniboxDecorationType::kTranslateButton:
      item.icon = "translate";
      item.tooltip = u"Translate page";
      item.accessibility_label = u"Translate button";
      break;
    case AstraOmniboxDecorationType::kAstraActionButton:
      item.icon = "astra_action";
      item.tooltip = u"Astra action";
      item.accessibility_label = u"Astra action button";
      item.has_bubble = true;
      break;
    case AstraOmniboxDecorationType::kNone:
      break;
  }

  return item;
}

}  // namespace

// =========================================================================
// AstraOmniboxDecorationModel — construction / destruction
// =========================================================================

AstraOmniboxDecorationModel::AstraOmniboxDecorationModel() {
  InitializeDefaultDecorations();
}

AstraOmniboxDecorationModel::~AstraOmniboxDecorationModel() {
  NotifyShutdown();
}

// =========================================================================
// Default decoration order
// =========================================================================

std::vector<AstraOmniboxDecorationType>
AstraOmniboxDecorationModel::GetDefaultDecorationOrder() {
  return {
      AstraOmniboxDecorationType::kWorkspaceIndicator,
      AstraOmniboxDecorationType::kFocusModeBadge,
      AstraOmniboxDecorationType::kTabStackIndicator,
      AstraOmniboxDecorationType::kReadingListBadge,
      AstraOmniboxDecorationType::kNoteBadge,
      AstraOmniboxDecorationType::kFavoriteStar,
      AstraOmniboxDecorationType::kSidebarToggle,
      AstraOmniboxDecorationType::kSplitViewToggle,
      AstraOmniboxDecorationType::kTranslateButton,
      AstraOmniboxDecorationType::kAstraActionButton,
  };
}

void AstraOmniboxDecorationModel::InitializeDefaultDecorations() {
  decorations_.clear();
  auto order = GetDefaultDecorationOrder();
  for (size_t i = 0; i < order.size(); ++i) {
    decorations_.push_back(MakeDefaultDecoration(order[i], static_cast<int>(i)));
  }
}

// =========================================================================
// Decoration item access
// =========================================================================

const std::vector<AstraOmniboxDecorationItem>&
AstraOmniboxDecorationModel::GetDecorations() const {
  return decorations_;
}

int AstraOmniboxDecorationModel::GetDecorationCount() const {
  return static_cast<int>(decorations_.size());
}

const AstraOmniboxDecorationItem*
AstraOmniboxDecorationModel::GetDecorationAt(int index) const {
  if (index < 0 || index >= static_cast<int>(decorations_.size())) {
    return nullptr;
  }
  return &decorations_[static_cast<size_t>(index)];
}

const AstraOmniboxDecorationItem*
AstraOmniboxDecorationModel::GetDecorationByType(
    AstraOmniboxDecorationType type) const {
  int index = FindDecorationIndex(type);
  if (index < 0) {
    return nullptr;
  }
  return &decorations_[static_cast<size_t>(index)];
}

int AstraOmniboxDecorationModel::FindDecorationIndex(
    AstraOmniboxDecorationType type) const {
  for (size_t i = 0; i < decorations_.size(); ++i) {
    if (decorations_[i].type == type) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

AstraOmniboxDecorationItem* AstraOmniboxDecorationModel::GetMutableDecoration(
    AstraOmniboxDecorationType type) {
  int index = FindDecorationIndex(type);
  if (index < 0) {
    return nullptr;
  }
  return &decorations_[static_cast<size_t>(index)];
}

// =========================================================================
// Decoration visibility
// =========================================================================

void AstraOmniboxDecorationModel::SetDecorationVisible(
    AstraOmniboxDecorationType type,
    bool visible) {
  AstraOmniboxDecorationItem* item = GetMutableDecoration(type);
  if (!item) {
    return;
  }
  if (item->is_visible == visible) {
    return;
  }
  item->is_visible = visible;
  NotifyVisibilityChanged(type, visible);
}

bool AstraOmniboxDecorationModel::IsDecorationVisible(
    AstraOmniboxDecorationType type) const {
  const AstraOmniboxDecorationItem* item = GetDecorationByType(type);
  return item ? item->is_visible : false;
}

// =========================================================================
// Decoration active state
// =========================================================================

void AstraOmniboxDecorationModel::SetDecorationActive(
    AstraOmniboxDecorationType type,
    bool active) {
  AstraOmniboxDecorationItem* item = GetMutableDecoration(type);
  if (!item) {
    return;
  }
  if (item->is_active == active) {
    return;
  }
  item->is_active = active;
  NotifyActiveChanged(type, active);
}

bool AstraOmniboxDecorationModel::IsDecorationActive(
    AstraOmniboxDecorationType type) const {
  const AstraOmniboxDecorationItem* item = GetDecorationByType(type);
  return item ? item->is_active : false;
}

// =========================================================================
// Decoration tooltip
// =========================================================================

void AstraOmniboxDecorationModel::SetDecorationTooltip(
    AstraOmniboxDecorationType type,
    const std::u16string& tooltip) {
  AstraOmniboxDecorationItem* item = GetMutableDecoration(type);
  if (!item) {
    return;
  }
  if (item->tooltip == tooltip) {
    return;
  }
  item->tooltip = tooltip;
}

// =========================================================================
// Decoration badges
// =========================================================================

void AstraOmniboxDecorationModel::SetDecorationBadge(
    AstraOmniboxDecorationType type,
    const std::u16string& badge_text,
    SkColor color) {
  AstraOmniboxDecorationItem* item = GetMutableDecoration(type);
  if (!item) {
    return;
  }
  if (item->badge_text == badge_text && item->badge_color == color) {
    return;
  }
  item->badge_text = badge_text;
  item->badge_color = color;
  NotifyBadgeChanged(type);
}

void AstraOmniboxDecorationModel::ClearDecorationBadge(
    AstraOmniboxDecorationType type) {
  SetDecorationBadge(type, std::u16string(), SK_ColorTRANSPARENT);
}

// =========================================================================
// Decoration ordering
// =========================================================================

void AstraOmniboxDecorationModel::ReorderDecorations(
    const std::vector<AstraOmniboxDecorationType>& order) {
  if (order.empty()) {
    return;
  }

  std::vector<AstraOmniboxDecorationItem> new_order;
  std::vector<bool> used(decorations_.size(), false);

  // First, add items in the specified order.
  for (auto type : order) {
    int index = FindDecorationIndex(type);
    if (index >= 0 && !used[static_cast<size_t>(index)]) {
      new_order.push_back(decorations_[static_cast<size_t>(index)]);
      used[static_cast<size_t>(index)] = true;
    }
  }

  // Then add remaining items in their original order.
  for (size_t i = 0; i < decorations_.size(); ++i) {
    if (!used[i]) {
      new_order.push_back(decorations_[i]);
    }
  }

  // Update order_index values.
  for (size_t i = 0; i < new_order.size(); ++i) {
    new_order[i].order_index = static_cast<int>(i);
  }

  decorations_ = std::move(new_order);
  NotifyReordered();
}

std::vector<AstraOmniboxDecorationType>
AstraOmniboxDecorationModel::GetDecorationOrder() const {
  std::vector<AstraOmniboxDecorationType> order;
  order.reserve(decorations_.size());
  for (const auto& item : decorations_) {
    order.push_back(item.type);
  }
  return order;
}

void AstraOmniboxDecorationModel::ResetDecorationOrder() {
  InitializeDefaultDecorations();
  // Re-apply individual visibility settings.
  SetDecorationVisible(AstraOmniboxDecorationType::kWorkspaceIndicator,
                       show_workspace_indicator_);
  SetDecorationVisible(AstraOmniboxDecorationType::kFocusModeBadge,
                       show_focus_mode_badge_);
  SetDecorationVisible(AstraOmniboxDecorationType::kTabStackIndicator,
                       show_tab_stack_indicator_);
  SetDecorationVisible(AstraOmniboxDecorationType::kReadingListBadge,
                       show_reading_list_button_);
  SetDecorationVisible(AstraOmniboxDecorationType::kNoteBadge,
                       show_note_button_);
  SetDecorationVisible(AstraOmniboxDecorationType::kFavoriteStar,
                       show_favorite_star_);
  SetDecorationVisible(AstraOmniboxDecorationType::kSidebarToggle,
                       show_sidebar_toggle_);
  SetDecorationVisible(AstraOmniboxDecorationType::kSplitViewToggle,
                       show_split_view_button_);
  SetDecorationVisible(AstraOmniboxDecorationType::kTranslateButton,
                       show_translate_button_);

  NotifyReordered();
}

// =========================================================================
// Decoration execution
// =========================================================================

void AstraOmniboxDecorationModel::ExecuteDecoration(
    AstraOmniboxDecorationType type) {
  if (GetDecorationByType(type) == nullptr) {
    return;
  }
  NotifyExecuted(type);
}

// =========================================================================
// Bubble management
// =========================================================================

void AstraOmniboxDecorationModel::ShowDecorationBubble(
    AstraOmniboxDecorationType type) {
  if (type == AstraOmniboxDecorationType::kNone) {
    return;
  }
  const AstraOmniboxDecorationItem* item = GetDecorationByType(type);
  if (!item || !item->has_bubble) {
    return;
  }
  if (open_bubble_type_ == type) {
    return;  // Already open.
  }
  // Hide the currently open bubble first, if any.
  if (open_bubble_type_ != AstraOmniboxDecorationType::kNone) {
    AstraOmniboxDecorationType old_type = open_bubble_type_;
    open_bubble_type_ = AstraOmniboxDecorationType::kNone;
    NotifyBubbleHidden(old_type);
  }
  open_bubble_type_ = type;
  NotifyBubbleShown(type);
}

void AstraOmniboxDecorationModel::HideDecorationBubble(
    AstraOmniboxDecorationType type) {
  if (open_bubble_type_ != type) {
    return;
  }
  open_bubble_type_ = AstraOmniboxDecorationType::kNone;
  NotifyBubbleHidden(type);
}

AstraOmniboxDecorationType AstraOmniboxDecorationModel::GetOpenBubbleType()
    const {
  return open_bubble_type_;
}

void AstraOmniboxDecorationModel::HideAllBubbles() {
  if (open_bubble_type_ == AstraOmniboxDecorationType::kNone) {
    return;
  }
  AstraOmniboxDecorationType old_type = open_bubble_type_;
  open_bubble_type_ = AstraOmniboxDecorationType::kNone;
  NotifyBubbleHidden(old_type);
}

// =========================================================================
// Workspace decoration state
// =========================================================================

void AstraOmniboxDecorationModel::SetCurrentWorkspaceName(
    const std::u16string& name) {
  if (workspace_name_ == name) {
    return;
  }
  workspace_name_ = name;

  // Update the workspace decoration tooltip.
  AstraOmniboxDecorationItem* item = GetMutableDecoration(
      AstraOmniboxDecorationType::kWorkspaceIndicator);
  if (item && !name.empty()) {
    item->tooltip = name;
  }

  NotifyWorkspaceChanged(name);
}

const std::u16string& AstraOmniboxDecorationModel::GetCurrentWorkspaceName()
    const {
  return workspace_name_;
}

void AstraOmniboxDecorationModel::SetWorkspaceColor(SkColor color) {
  workspace_color_ = color;
}

SkColor AstraOmniboxDecorationModel::GetWorkspaceColor() const {
  return workspace_color_;
}

void AstraOmniboxDecorationModel::SetWorkspaceBadgeCount(int count) {
  if (workspace_badge_count_ == count) {
    return;
  }
  workspace_badge_count_ = count;
  if (count > 0) {
    SetDecorationBadge(AstraOmniboxDecorationType::kWorkspaceIndicator,
                       base::NumberToString16(count),
                       SK_ColorRED);
  } else {
    ClearDecorationBadge(AstraOmniboxDecorationType::kWorkspaceIndicator);
  }
}

int AstraOmniboxDecorationModel::GetWorkspaceBadgeCount() const {
  return workspace_badge_count_;
}

void AstraOmniboxDecorationModel::SetShowWorkspaceIndicator(bool show) {
  if (show_workspace_indicator_ == show) {
    return;
  }
  show_workspace_indicator_ = show;
  SetDecorationVisible(AstraOmniboxDecorationType::kWorkspaceIndicator, show);
}

bool AstraOmniboxDecorationModel::GetShowWorkspaceIndicator() const {
  return show_workspace_indicator_;
}

// =========================================================================
// Focus mode decoration state
// =========================================================================

void AstraOmniboxDecorationModel::SetFocusModeActive(bool active) {
  if (focus_mode_active_ == active) {
    return;
  }
  focus_mode_active_ = active;
  SetDecorationActive(AstraOmniboxDecorationType::kFocusModeBadge, active);
  NotifyFocusModeChanged(active);
}

bool AstraOmniboxDecorationModel::IsFocusModeActive() const {
  return focus_mode_active_;
}

void AstraOmniboxDecorationModel::SetFocusModeTimeRemaining(
    base::TimeDelta remaining) {
  focus_mode_time_remaining_ = remaining;
}

base::TimeDelta AstraOmniboxDecorationModel::GetFocusModeTimeRemaining()
    const {
  return focus_mode_time_remaining_;
}

void AstraOmniboxDecorationModel::SetShowFocusModeBadge(bool show) {
  if (show_focus_mode_badge_ == show) {
    return;
  }
  show_focus_mode_badge_ = show;
  SetDecorationVisible(AstraOmniboxDecorationType::kFocusModeBadge, show);
}

bool AstraOmniboxDecorationModel::GetShowFocusModeBadge() const {
  return show_focus_mode_badge_;
}

void AstraOmniboxDecorationModel::SetFocusModeColor(SkColor color) {
  focus_mode_color_ = color;
}

SkColor AstraOmniboxDecorationModel::GetFocusModeColor() const {
  return focus_mode_color_;
}

// =========================================================================
// Tab stack decoration state
// =========================================================================

void AstraOmniboxDecorationModel::SetTabStackName(const std::u16string& name) {
  tab_stack_name_ = name;

  // Update the tab stack decoration tooltip.
  AstraOmniboxDecorationItem* item = GetMutableDecoration(
      AstraOmniboxDecorationType::kTabStackIndicator);
  if (item && !name.empty()) {
    item->tooltip = name;
  }
}

const std::u16string& AstraOmniboxDecorationModel::GetTabStackName() const {
  return tab_stack_name_;
}

void AstraOmniboxDecorationModel::SetTabStackColor(SkColor color) {
  tab_stack_color_ = color;
}

SkColor AstraOmniboxDecorationModel::GetTabStackColor() const {
  return tab_stack_color_;
}

void AstraOmniboxDecorationModel::SetTabStackTabCount(int count) {
  if (tab_stack_tab_count_ == count) {
    return;
  }
  tab_stack_tab_count_ = count;
  if (count > 0) {
    SetDecorationBadge(AstraOmniboxDecorationType::kTabStackIndicator,
                       base::NumberToString16(count),
                       tab_stack_color_);
  } else {
    ClearDecorationBadge(AstraOmniboxDecorationType::kTabStackIndicator);
  }
}

int AstraOmniboxDecorationModel::GetTabStackTabCount() const {
  return tab_stack_tab_count_;
}

void AstraOmniboxDecorationModel::SetShowTabStackIndicator(bool show) {
  if (show_tab_stack_indicator_ == show) {
    return;
  }
  show_tab_stack_indicator_ = show;
  SetDecorationVisible(AstraOmniboxDecorationType::kTabStackIndicator, show);
}

bool AstraOmniboxDecorationModel::GetShowTabStackIndicator() const {
  return show_tab_stack_indicator_;
}

// =========================================================================
// Reading list decoration state
// =========================================================================

void AstraOmniboxDecorationModel::SetIsInReadingList(bool in_list) {
  if (is_in_reading_list_ == in_list) {
    return;
  }
  is_in_reading_list_ = in_list;
  SetDecorationActive(AstraOmniboxDecorationType::kReadingListBadge, in_list);
}

bool AstraOmniboxDecorationModel::IsInReadingList() const {
  return is_in_reading_list_;
}

void AstraOmniboxDecorationModel::SetShowReadingListButton(bool show) {
  if (show_reading_list_button_ == show) {
    return;
  }
  show_reading_list_button_ = show;
  SetDecorationVisible(AstraOmniboxDecorationType::kReadingListBadge, show);
}

bool AstraOmniboxDecorationModel::GetShowReadingListButton() const {
  return show_reading_list_button_;
}

// =========================================================================
// Note decoration state
// =========================================================================

void AstraOmniboxDecorationModel::SetHasNote(bool has_note) {
  if (has_note_ == has_note) {
    return;
  }
  has_note_ = has_note;
  SetDecorationActive(AstraOmniboxDecorationType::kNoteBadge, has_note);
}

bool AstraOmniboxDecorationModel::HasNote() const {
  return has_note_;
}

void AstraOmniboxDecorationModel::SetNotePreview(
    const std::u16string& preview) {
  note_preview_ = preview;
}

const std::u16string& AstraOmniboxDecorationModel::GetNotePreview() const {
  return note_preview_;
}

void AstraOmniboxDecorationModel::SetShowNoteButton(bool show) {
  if (show_note_button_ == show) {
    return;
  }
  show_note_button_ = show;
  SetDecorationVisible(AstraOmniboxDecorationType::kNoteBadge, show);
}

bool AstraOmniboxDecorationModel::GetShowNoteButton() const {
  return show_note_button_;
}

// =========================================================================
// Favorite decoration state
// =========================================================================

void AstraOmniboxDecorationModel::SetIsFavorited(bool favorited) {
  if (is_favorited_ == favorited) {
    return;
  }
  is_favorited_ = favorited;
  SetDecorationActive(AstraOmniboxDecorationType::kFavoriteStar, favorited);
}

bool AstraOmniboxDecorationModel::IsFavorited() const {
  return is_favorited_;
}

void AstraOmniboxDecorationModel::SetShowFavoriteStar(bool show) {
  if (show_favorite_star_ == show) {
    return;
  }
  show_favorite_star_ = show;
  SetDecorationVisible(AstraOmniboxDecorationType::kFavoriteStar, show);
}

bool AstraOmniboxDecorationModel::GetShowFavoriteStar() const {
  return show_favorite_star_;
}

// =========================================================================
// Sidebar decoration state
// =========================================================================

void AstraOmniboxDecorationModel::SetSidebarOpen(bool open) {
  if (sidebar_open_ == open) {
    return;
  }
  sidebar_open_ = open;
  SetDecorationActive(AstraOmniboxDecorationType::kSidebarToggle, open);
}

bool AstraOmniboxDecorationModel::IsSidebarOpen() const {
  return sidebar_open_;
}

void AstraOmniboxDecorationModel::SetShowSidebarToggle(bool show) {
  if (show_sidebar_toggle_ == show) {
    return;
  }
  show_sidebar_toggle_ = show;
  SetDecorationVisible(AstraOmniboxDecorationType::kSidebarToggle, show);
}

bool AstraOmniboxDecorationModel::GetShowSidebarToggle() const {
  return show_sidebar_toggle_;
}

// =========================================================================
// Split view decoration state
// =========================================================================

void AstraOmniboxDecorationModel::SetSplitViewActive(bool active) {
  if (split_view_active_ == active) {
    return;
  }
  split_view_active_ = active;
  SetDecorationActive(AstraOmniboxDecorationType::kSplitViewToggle, active);
}

bool AstraOmniboxDecorationModel::IsSplitViewActive() const {
  return split_view_active_;
}

void AstraOmniboxDecorationModel::SetShowSplitViewButton(bool show) {
  if (show_split_view_button_ == show) {
    return;
  }
  show_split_view_button_ = show;
  SetDecorationVisible(AstraOmniboxDecorationType::kSplitViewToggle, show);
}

bool AstraOmniboxDecorationModel::GetShowSplitViewButton() const {
  return show_split_view_button_;
}

// =========================================================================
// Presentation settings
// =========================================================================

void AstraOmniboxDecorationModel::SetShowBadgesOnHoverOnly(bool show) {
  show_badges_on_hover_only_ = show;
}

bool AstraOmniboxDecorationModel::GetShowBadgesOnHoverOnly() const {
  return show_badges_on_hover_only_;
}

void AstraOmniboxDecorationModel::SetCompactMode(bool compact) {
  compact_mode_ = compact;
}

bool AstraOmniboxDecorationModel::GetCompactMode() const {
  return compact_mode_;
}

void AstraOmniboxDecorationModel::SetAnimationEnabled(bool enabled) {
  animation_enabled_ = enabled;
}

bool AstraOmniboxDecorationModel::GetAnimationEnabled() const {
  return animation_enabled_;
}

void AstraOmniboxDecorationModel::SetDecorationIconSize(int size_px) {
  int clamped = ClampIconSize(size_px);
  decoration_icon_size_ = clamped;
}

int AstraOmniboxDecorationModel::GetDecorationIconSize() const {
  return decoration_icon_size_;
}

int AstraOmniboxDecorationModel::ClampIconSize(int size) {
  return std::clamp(size, kMinIconSize, kMaxIconSize);
}

// =========================================================================
// Observers
// =========================================================================

void AstraOmniboxDecorationModel::AddObserver(
    AstraOmniboxDecorationObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraOmniboxDecorationModel::RemoveObserver(
    AstraOmniboxDecorationObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Observer notification helpers
// =========================================================================

void AstraOmniboxDecorationModel::NotifyVisibilityChanged(
    AstraOmniboxDecorationType type,
    bool visible) {
  for (auto& observer : observers_) {
    observer.OnDecorationVisibilityChanged(this, type, visible);
  }
}

void AstraOmniboxDecorationModel::NotifyActiveChanged(
    AstraOmniboxDecorationType type,
    bool active) {
  for (auto& observer : observers_) {
    observer.OnDecorationActiveChanged(this, type, active);
  }
}

void AstraOmniboxDecorationModel::NotifyBadgeChanged(
    AstraOmniboxDecorationType type) {
  for (auto& observer : observers_) {
    observer.OnDecorationBadgeChanged(this, type);
  }
}

void AstraOmniboxDecorationModel::NotifyReordered() {
  for (auto& observer : observers_) {
    observer.OnDecorationsReordered(this);
  }
}

void AstraOmniboxDecorationModel::NotifyExecuted(
    AstraOmniboxDecorationType type) {
  for (auto& observer : observers_) {
    observer.OnDecorationExecuted(this, type);
  }
}

void AstraOmniboxDecorationModel::NotifyBubbleShown(
    AstraOmniboxDecorationType type) {
  for (auto& observer : observers_) {
    observer.OnBubbleShown(this, type);
  }
}

void AstraOmniboxDecorationModel::NotifyBubbleHidden(
    AstraOmniboxDecorationType type) {
  for (auto& observer : observers_) {
    observer.OnBubbleHidden(this, type);
  }
}

void AstraOmniboxDecorationModel::NotifyWorkspaceChanged(
    const std::u16string& name) {
  for (auto& observer : observers_) {
    observer.OnWorkspaceChanged(this, name);
  }
}

void AstraOmniboxDecorationModel::NotifyFocusModeChanged(bool active) {
  for (auto& observer : observers_) {
    observer.OnFocusModeChanged(this, active);
  }
}

void AstraOmniboxDecorationModel::NotifyShutdown() {
  for (auto& observer : observers_) {
    observer.OnOmniboxDecorationModelShutdown(this);
  }
}

// =========================================================================
// Persistence
// =========================================================================

void AstraOmniboxDecorationModel::LoadFromPrefs(PrefService* prefs) {
  if (!prefs) {
    return;
  }

  // TODO(astra): Load individual settings from PrefService once prefs
  // are registered in AstraPrefs.
  // For now, settings use their default values.
}

void AstraOmniboxDecorationModel::SaveToPrefs(PrefService* prefs) const {
  if (!prefs) {
    return;
  }

  // TODO(astra): Save individual settings to PrefService once prefs
  // are registered in AstraPrefs.
}

}  // namespace astra
