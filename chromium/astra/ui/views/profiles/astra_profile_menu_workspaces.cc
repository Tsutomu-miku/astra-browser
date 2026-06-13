// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/profiles/astra_profile_menu_workspaces.h"

#include <algorithm>
#include <utility>

#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/profiles/astra_workspace_menu_item_view.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants — sized to match Chromium profile menu item style.
// Chromium owner: ProfileMenuView (chrome/browser/ui/views/profiles/)
constexpr int kSectionHeaderHeight = 32;
constexpr int kSectionHeaderHorizontalPadding = 16;
constexpr int kActionRowHeight = 36;
constexpr int kActionHorizontalPadding = 16;
constexpr int kMenuMinWidth = 260;
constexpr int kMaxVisibleWorkspaces = 6;  // Before scrolling kicks in.

// Default max list height in px (6 items at 40px each = 240px).
constexpr int kDefaultMaxListHeight = 240;

// Helper to parse a hex color string (e.g., "#5B8FF9") to SkColor.
// TODO(astra): Use a proper color parsing utility.
SkColor HexToSkColor(const std::string& hex) {
  if (hex.empty() || hex[0] != '#') {
    return SK_ColorBLUE;
  }
  std::string cleaned = hex.substr(1);
  if (cleaned.size() != 6) {
    return SK_ColorBLUE;
  }
  unsigned int r, g, b;
  if (sscanf(cleaned.c_str(), "%02x%02x%02x", &r, &g, &b) != 3) {
    return SK_ColorBLUE;
  }
  return SkColorSetRGB(r, g, b);
}

// Convert our display mode enum to the item view's display mode.
AstraWorkspaceItemDisplayMode ToItemDisplayMode(AstraWorkspaceDisplayMode mode) {
  switch (mode) {
    case AstraWorkspaceDisplayMode::kIconsOnly:
      return AstraWorkspaceItemDisplayMode::kIconsOnly;
    case AstraWorkspaceDisplayMode::kNamesOnly:
      return AstraWorkspaceItemDisplayMode::kNamesOnly;
    case AstraWorkspaceDisplayMode::kIconsAndNames:
      return AstraWorkspaceItemDisplayMode::kIconsAndNames;
  }
  return AstraWorkspaceItemDisplayMode::kIconsAndNames;
}

}  // namespace

// ---------------------------------------------------------------------------
// AstraProfileMenuWorkspaces
// ---------------------------------------------------------------------------

AstraProfileMenuWorkspaces::AstraProfileMenuWorkspaces(
    AstraWorkspaceService* workspace_service,
    Delegate* delegate)
    : workspace_service_(workspace_service),
      delegate_(delegate),
      max_list_height_(kDefaultMaxListHeight) {
  DCHECK(delegate_);

  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  RebuildWorkspaceList();
}

AstraProfileMenuWorkspaces::~AstraProfileMenuWorkspaces() = default;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AstraProfileMenuWorkspaces::UpdateFromService() {
  RebuildWorkspaceList();
}

void AstraProfileMenuWorkspaces::SetMaxListHeight(int max_height) {
  max_list_height_ = max_height;
  if (scroll_view_) {
    scroll_view_->ClipHeightTo(0, max_list_height_);
    PreferredSizeChanged();
  }
}

// ---------------------------------------------------------------------------
// Display mode
// ---------------------------------------------------------------------------

void AstraProfileMenuWorkspaces::SetDisplayMode(AstraWorkspaceDisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;

  // Update all existing workspace items.
  for (auto* item : workspace_items_) {
    item->SetDisplayMode(ToItemDisplayMode(mode));
  }

  PreferredSizeChanged();
}

// ---------------------------------------------------------------------------
// Reorder handles
// ---------------------------------------------------------------------------

void AstraProfileMenuWorkspaces::SetReorderHandlesVisible(bool visible) {
  if (reorder_handles_visible_ == visible) {
    return;
  }
  reorder_handles_visible_ = visible;

  // Update all existing workspace items.
  for (auto* item : workspace_items_) {
    item->SetReorderHandleVisible(visible);
  }

  PreferredSizeChanged();
}

// ---------------------------------------------------------------------------
// Keyboard navigation
// ---------------------------------------------------------------------------

bool AstraProfileMenuWorkspaces::MoveFocusDown() {
  int count = GetItemCount();
  if (count == 0) {
    return false;
  }

  int current = GetFocusedItemIndex();
  int next = (current < 0 || current >= count - 1) ? 0 : current + 1;

  if (next >= 0 && next < count) {
    workspace_items_[next]->RequestFocus();
    return true;
  }
  return false;
}

bool AstraProfileMenuWorkspaces::MoveFocusUp() {
  int count = GetItemCount();
  if (count == 0) {
    return false;
  }

  int current = GetFocusedItemIndex();
  int prev = (current <= 0) ? count - 1 : current - 1;

  if (prev >= 0 && prev < count) {
    workspace_items_[prev]->RequestFocus();
    return true;
  }
  return false;
}

bool AstraProfileMenuWorkspaces::ActivateFocusedItem() {
  int index = GetFocusedItemIndex();
  if (index < 0 || index >= static_cast<int>(workspace_items_.size())) {
    return false;
  }
  workspace_items_[index]->OnKeyPressed(
      ui::KeyEvent(ui::ET_KEY_PRESSED, ui::VKEY_RETURN, ui::EF_NONE));
  return true;
}

bool AstraProfileMenuWorkspaces::ReorderFocusedItem(int direction) {
  int index = GetFocusedItemIndex();
  if (index < 0 || index >= static_cast<int>(workspace_items_.size())) {
    return false;
  }

  int target = index + direction;
  if (target < 0 || target >= static_cast<int>(workspace_items_.size())) {
    return false;
  }

  // Find the workspace ID for the focused item.
  // We need to map from the item to the workspace ID.
  // Since we don't store the ID directly on the item view, we rely on
  // the workspace_service_ for ordering, but for the delegate callback,
  // we can look up the service.
  // TODO(astra): Store workspace ID on the item view for more direct access.
  if (workspace_service_) {
    const auto& workspaces = workspace_service_->workspaces();
    if (index < static_cast<int>(workspaces.size())) {
      const std::string& workspace_id = workspaces[index].id;
      if (delegate_) {
        delegate_->OnWorkspaceReordered(workspace_id, direction);
      }
      return true;
    }
  }

  return false;
}

// ---------------------------------------------------------------------------
// views::View overrides
// ---------------------------------------------------------------------------

void AstraProfileMenuWorkspaces::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Section header text.
  if (section_header_) {
    section_header_->SetEnabledColor(
        color_provider->GetColor(ui::kColorLabelForegroundSecondary));
  }

  // Separators.
  if (header_separator_) {
    header_separator_->SetColorId(ui::kColorSeparator);
  }
  if (action_separator_) {
    action_separator_->SetColorId(ui::kColorSeparator);
  }

  // Action buttons.
  if (new_workspace_button_) {
    new_workspace_button_->SetTextColorId(
        views::Button::STATE_NORMAL, ui::kColorLabelForeground);
  }
  if (manage_workspaces_button_) {
    manage_workspaces_button_->SetTextColorId(
        views::Button::STATE_NORMAL, kColorAstraWorkspaceAccent);
  }
}

gfx::Size AstraProfileMenuWorkspaces::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = views::View::CalculatePreferredSize(available_size);
  size.set_width(std::max(size.width(), kMenuMinWidth));
  return size;
}

bool AstraProfileMenuWorkspaces::OnKeyPressed(const ui::KeyEvent& event) {
  // Handle arrow keys for navigating workspace items.
  switch (event.key_code()) {
    case ui::VKEY_DOWN:
      if (MoveFocusDown()) {
        return true;
      }
      break;
    case ui::VKEY_UP:
      if (MoveFocusUp()) {
        return true;
      }
      break;
    case ui::VKEY_HOME:
      if (GetItemCount() > 0) {
        workspace_items_[0]->RequestFocus();
        return true;
      }
      break;
    case ui::VKEY_END:
      if (GetItemCount() > 0) {
        workspace_items_.back()->RequestFocus();
        return true;
      }
      break;
    case ui::VKEY_RETURN:
    case ui::VKEY_SPACE:
      if (ActivateFocusedItem()) {
        return true;
      }
      break;
    default:
      break;
  }

  // Alt+Up / Alt+Down for reordering.
  if (event.IsAltDown() && reorder_handles_visible_) {
    if (event.key_code() == ui::VKEY_UP) {
      if (ReorderFocusedItem(-1)) {
        return true;
      }
    } else if (event.key_code() == ui::VKEY_DOWN) {
      if (ReorderFocusedItem(1)) {
        return true;
      }
    }
  }

  return views::View::OnKeyPressed(event);
}

void AstraProfileMenuWorkspaces::ViewHierarchyChanged(
    const views::ViewHierarchyChangedDetails& details) {
  views::View::ViewHierarchyChanged(details);

  // When this view is added to a widget, focus the active workspace item.
  if (details.is_add && details.child == this) {
    int active_index = GetFocusedItemIndex();
    if (active_index >= 0 &&
        active_index < static_cast<int>(workspace_items_.size())) {
      workspace_items_[active_index]->RequestFocus();
    } else if (!workspace_items_.empty()) {
      // Find and focus the active workspace item.
      for (auto* item : workspace_items_) {
        if (item->is_active()) {
          item->RequestFocus();
          break;
        }
      }
    }
  }
}

void AstraProfileMenuWorkspaces::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  View::GetAccessibleNodeData(node_data);

  node_data->role = ax::mojom::Role::kList;
  node_data->SetName(u"Workspaces");
  node_data->SetDescription(
      base::NumberToString16(static_cast<int>(workspace_items_.size())) +
      u" workspaces");
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void AstraProfileMenuWorkspaces::RebuildWorkspaceList() {
  RemoveAllChildViews();
  workspace_items_.clear();

  if (!workspace_service_) {
    return;
  }

  const auto& workspaces = workspace_service_->workspaces();
  const auto& active_id = workspace_service_->active_workspace_id();

  // --- Section header: "Workspaces" ---
  auto section_header = std::make_unique<views::Label>();
  section_header->SetText(u"Workspaces");
  section_header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  section_header->SetAutoColorReadabilityEnabled(false);
  section_header->SetFontList(
      section_header->font_list().Derive(
          -1, gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
  section_header->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kSectionHeaderHorizontalPadding)));
  section_header->SetMinimumSize(
      gfx::Size(0, kSectionHeaderHeight));
  section_header_ = AddChildView(std::move(section_header));

  // --- Header separator ---
  header_separator_ = AddChildView(std::make_unique<views::Separator>());
  header_separator_->SetColorId(ui::kColorSeparator);

  // --- Scrollable workspace list ---
  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetBackgroundColor(std::nullopt);
  scroll_view_->ClipHeightTo(0, max_list_height_);
  scroll_view_->SetDrawOverflowIndicator(true);

  auto list_container = std::make_unique<views::View>();
  auto* list_layout =
      list_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  list_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  list_container_ = list_container.get();

  for (const auto& ws : workspaces) {
    bool is_active = (ws.id == active_id);
    int tab_count = 0;
    if (workspace_service_) {
      tab_count = static_cast<int>(workspace_service_->GetTabCount(ws.id));
    }

    auto item = CreateWorkspaceItem(ws.id, ws.name, ws.accent_color,
                                    tab_count, is_active);
    workspace_items_.push_back(item.get());
    list_container->AddChildView(std::move(item));
  }

  scroll_view_->SetContents(std::move(list_container));

  // --- Action separator ---
  action_separator_ = AddChildView(std::make_unique<views::Separator>());
  action_separator_->SetColorId(ui::kColorSeparator);

  // --- "New workspace" button ---
  new_workspace_button_ = AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraProfileMenuWorkspaces::OnNewWorkspaceClicked,
              base::Unretained(this)),
          u"+ New workspace"));
  new_workspace_button_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  new_workspace_button_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kActionHorizontalPadding)));
  new_workspace_button_->SetMinSize(gfx::Size(0, kActionRowHeight));
  new_workspace_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  // --- "Manage workspaces" link ---
  manage_workspaces_button_ = AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraProfileMenuWorkspaces::OnManageWorkspacesClicked,
              base::Unretained(this)),
          u"Manage workspaces"));
  manage_workspaces_button_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  manage_workspaces_button_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kActionHorizontalPadding)));
  manage_workspaces_button_->SetMinSize(gfx::Size(0, kActionRowHeight));
  manage_workspaces_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  // Apply theme to all newly created children.
  if (GetColorProvider()) {
    OnThemeChanged();
  }
}

std::unique_ptr<AstraWorkspaceMenuItemView>
AstraProfileMenuWorkspaces::CreateWorkspaceItem(
    const std::string& workspace_id,
    const std::string& workspace_name,
    const std::string& accent_color,
    int tab_count,
    bool is_active) {
  std::u16string name_u16 = base::UTF8ToUTF16(workspace_name);
  SkColor accent_skcolor = HexToSkColor(accent_color);

  auto item = std::make_unique<AstraWorkspaceMenuItemView>(
      name_u16, accent_skcolor, tab_count, is_active,
      base::BindRepeating(&AstraProfileMenuWorkspaces::OnWorkspaceItemClicked,
                          weak_factory_.GetWeakPtr(), workspace_id));

  // Apply current display mode.
  item->SetDisplayMode(ToItemDisplayMode(display_mode_));

  // Apply reorder handle visibility.
  item->SetReorderHandleVisible(reorder_handles_visible_);

  // Set reorder callback.
  item->set_reorder_callback(base::BindRepeating(
      &AstraProfileMenuWorkspaces::OnWorkspaceItemReordered,
      weak_factory_.GetWeakPtr(), workspace_id));

  return item;
}

void AstraProfileMenuWorkspaces::OnWorkspaceItemClicked(
    const std::string& workspace_id) {
  if (delegate_) {
    delegate_->OnWorkspaceSelected(workspace_id);
  }
}

void AstraProfileMenuWorkspaces::OnWorkspaceItemReordered(
    const std::string& workspace_id,
    int direction) {
  if (delegate_) {
    delegate_->OnWorkspaceReordered(workspace_id, direction);
  }
}

void AstraProfileMenuWorkspaces::OnNewWorkspaceClicked() {
  if (delegate_) {
    delegate_->OnNewWorkspace();
  }
}

void AstraProfileMenuWorkspaces::OnManageWorkspacesClicked() {
  if (delegate_) {
    delegate_->OnManageWorkspaces();
  }
}

int AstraProfileMenuWorkspaces::GetFocusedItemIndex() const {
  for (size_t i = 0; i < workspace_items_.size(); ++i) {
    if (workspace_items_[i]->HasFocus()) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int AstraProfileMenuWorkspaces::GetItemCount() const {
  return static_cast<int>(workspace_items_.size());
}

}  // namespace astra
