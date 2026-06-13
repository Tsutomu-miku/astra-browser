// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_HIBERNATION_VIEW_H_
#define ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_HIBERNATION_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
}  // namespace views

namespace astra {

struct AstraWorkspace;

// =========================================================================
// AstraHibernatedWorkspaceItemView — single hibernated workspace row
// =========================================================================
//
// A row in the hibernated workspaces list. Shows workspace name, tab count,
// last used time, memory saved estimate, and a "Restore" button.
//
// Layout:
//   +-------------------------------------------+
//   |  ●  Workspace Name          [ Restore ]  |
//   |     12 tabs · last used 3 days ago       |
//   |     💾 ~256 MB saved                     |
//   +-------------------------------------------+
// =========================================================================

class AstraHibernatedWorkspaceItemView : public views::View {
 public:
  using RestoreCallback =
      base::RepeatingCallback<void(const std::string& workspace_id)>;
  using DeleteCallback =
      base::RepeatingCallback<void(const std::string& workspace_id)>;

  AstraHibernatedWorkspaceItemView(const AstraWorkspace& workspace,
                                   RestoreCallback restore_callback,
                                   DeleteCallback delete_callback);
  ~AstraHibernatedWorkspaceItemView() override;

  AstraHibernatedWorkspaceItemView(
      const AstraHibernatedWorkspaceItemView&) = delete;
  AstraHibernatedWorkspaceItemView& operator=(
      const AstraHibernatedWorkspaceItemView&) = delete;

  const std::string& workspace_id() const { return workspace_id_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void UpdateFromWorkspace();

  std::string workspace_id_;
  std::u16string workspace_name_;
  std::string accent_color_;
  int tab_count_ = 0;
  int window_count_ = 1;
  base::Time last_used_time_;
  int64_t memory_saved_bytes_ = 0;

  RestoreCallback restore_callback_;
  DeleteCallback delete_callback_;

  raw_ptr<views::View> accent_dot_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> detail_label_ = nullptr;
  raw_ptr<views::Label> memory_label_ = nullptr;
  raw_ptr<views::MdTextButton> restore_button_ = nullptr;
};

// =========================================================================
// AstraWorkspaceHibernationView — hibernated workspaces panel
// =========================================================================
//
// A bubble / side panel showing hibernated workspaces and hibernation
// settings.
//
// Layout:
//   +-------------------------------------------+
//   |  Hibernated Workspaces           [Close] |
//   +-------------------------------------------+
//   |  💤 Hibernation saves memory by unloading |
//   |     inactive workspaces from memory.      |
//   +-------------------------------------------+
//   |  Hibernated (3)                           |
//   |  ┌─────────────────────────────────┐     |
//   |  │ ●  Work Projects   [ Restore ]  │     |
//   │ │    12 tabs · 3d ago · 256MB     │     |
//   |  └─────────────────────────────────┘     |
//   |  ┌─────────────────────────────────┐     |
//   |  │ ●  Research        [ Restore ]  │     |
//   │ │    8 tabs · 1w ago · 180MB      │     |
//   |  └─────────────────────────────────┘     |
//   |  ...                                      |
//   +-------------------------------------------+
//   |  ⚙ Auto-hibernate after 24 hours          |
//   +-------------------------------------------+
//
// This is a presentation-only view.  Workspace data comes from the
// workspace service layer.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - views::MdTextButton
// =========================================================================

class AstraWorkspaceHibernationView
    : public views::BubbleDialogDelegateView {
 public:
  // Callback when a workspace restore is requested.
  using RestoreCallback =
      base::RepeatingCallback<void(const std::string& workspace_id)>;

  // Callback when a workspace delete is requested.
  using DeleteCallback =
      base::RepeatingCallback<void(const std::string& workspace_id)>;

  explicit AstraWorkspaceHibernationView(views::View* anchor_view);
  ~AstraWorkspaceHibernationView() override;

  AstraWorkspaceHibernationView(
      const AstraWorkspaceHibernationView&) = delete;
  AstraWorkspaceHibernationView& operator=(
      const AstraWorkspaceHibernationView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetHibernatedWorkspaces(
      const std::vector<AstraWorkspace>& workspaces);

  void SetAutoHibernateEnabled(bool enabled);
  void SetAutoHibernateHours(int hours);

  // -- Callbacks -----------------------------------------------------------

  void SetRestoreCallback(RestoreCallback callback);
  void SetDeleteCallback(DeleteCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildInfoSection();
  void BuildWorkspaceList();
  void BuildSettingsSection();

  void RefreshWorkspaceList();

  void OnRestoreWorkspace(const std::string& workspace_id);
  void OnDeleteWorkspace(const std::string& workspace_id);
  void OnAutoHibernateToggled();

  // Callbacks.
  RestoreCallback restore_callback_;
  DeleteCallback delete_callback_;

  // Workspace data.
  std::vector<AstraWorkspace> hibernated_workspaces_;

  // Settings.
  bool auto_hibernate_enabled_ = true;
  int auto_hibernate_hours_ = 24;

  // Child views.
  raw_ptr<views::Label> info_label_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> workspace_list_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;

  // Workspace items (owned by workspace_list_).
  std::vector<raw_ptr<AstraHibernatedWorkspaceItemView>> workspace_items_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_WORKSPACE_ASTRA_WORKSPACE_HIBERNATION_VIEW_H_
