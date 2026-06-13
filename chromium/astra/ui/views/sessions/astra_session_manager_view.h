// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_SESSIONS_ASTRA_SESSION_MANAGER_VIEW_H_
#define ASTRA_UI_VIEWS_SESSIONS_ASTRA_SESSION_MANAGER_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraSessionSnapshotItemView — single session snapshot card
// =========================================================================
//
// A card showing a saved session: name, tab count, creation date, and
// action buttons (restore, delete, rename).
//
// Layout:
//   +-------------------------------------------+
//   |  📸 Morning Research          [Restore]   |
//   |     8 tabs · saved 2 days ago             |
//   |     [Rename] [Delete]                     |
//   +-------------------------------------------+
// =========================================================================

class AstraSessionSnapshotItemView : public views::View {
 public:
  using RestoreCallback =
      base::RepeatingCallback<void(const std::string& session_id)>;
  using DeleteCallback =
      base::RepeatingCallback<void(const std::string& session_id)>;
  using RenameCallback =
      base::RepeatingCallback<void(const std::string& session_id,
                                   const std::u16string& new_name)>;

  struct SnapshotInfo {
    std::string session_id;
    std::u16string name;
    std::string description;
    int tab_count = 0;
    int window_count = 1;
    base::Time created_at;
    std::string workspace_id;  // Optional: associated workspace
    std::vector<std::string> sample_domains;
  };

  AstraSessionSnapshotItemView(const SnapshotInfo& info,
                               RestoreCallback restore_callback,
                               DeleteCallback delete_callback,
                               RenameCallback rename_callback);
  ~AstraSessionSnapshotItemView() override;

  AstraSessionSnapshotItemView(const AstraSessionSnapshotItemView&) = delete;
  AstraSessionSnapshotItemView& operator=(
      const AstraSessionSnapshotItemView&) = delete;

  const std::string& session_id() const { return session_id_; }
  const std::u16string& name() const { return name_; }

  void SetName(const std::u16string& name);

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnRestoreClicked();
  void OnDeleteClicked();
  void OnRenameClicked();

  std::string session_id_;
  std::u16string name_;
  std::string description_;
  int tab_count_ = 0;
  int window_count_ = 1;
  base::Time created_at_;
  std::string workspace_id_;

  RestoreCallback restore_callback_;
  DeleteCallback delete_callback_;
  RenameCallback rename_callback_;

  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> detail_label_ = nullptr;
  raw_ptr<views::MdTextButton> restore_button_ = nullptr;
  raw_ptr<views::MdTextButton> rename_button_ = nullptr;
  raw_ptr<views::MdTextButton> delete_button_ = nullptr;
};

// =========================================================================
// AstraSessionManagerView — session snapshot manager panel
// =========================================================================
//
// A bubble / side panel showing saved tab sessions (snapshots) with
// options to save current session, restore, rename, and delete.
//
// Layout:
//   +-------------------------------------------+
//   |  Session Manager              [Close]    |
//   +-------------------------------------------+
//   |  [ + Save current session ]                |
//   +-------------------------------------------+
//   |  Saved Sessions (5)                        |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ 📸 Morning Research    [Restore]    │  |
//   |  │    8 tabs · 2 days ago               │  |
//   |  │    [Rename] [Delete]                 │  |
//   |  └─────────────────────────────────────┘  |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ 📸 Work Backup        [Restore]    │  |
//   |  │    12 tabs · 1 week ago             │  |
//   |  │    [Rename] [Delete]                 │  |
//   |  └─────────────────────────────────────┘  |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Session data comes from Astra's
// session service, which persists snapshots of TabStripModel state.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - TabRestoreService / sessions (source of session data)
//   - TabStripModel (for saving current state)
// =========================================================================

class AstraSessionManagerView : public views::BubbleDialogDelegateView {
 public:
  using SaveSessionCallback = base::RepeatingClosure;
  using RestoreSessionCallback =
      base::RepeatingCallback<void(const std::string& session_id)>;
  using DeleteSessionCallback =
      base::RepeatingCallback<void(const std::string& session_id)>;
  using RenameSessionCallback =
      base::RepeatingCallback<void(const std::string& session_id,
                                   const std::u16string& new_name)>;

  explicit AstraSessionManagerView(views::View* anchor_view);
  ~AstraSessionManagerView() override;

  AstraSessionManagerView(const AstraSessionManagerView&) = delete;
  AstraSessionManagerView& operator=(
      const AstraSessionManagerView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetSnapshots(
      const std::vector<AstraSessionSnapshotItemView::SnapshotInfo>& snapshots);

  // -- Callbacks -----------------------------------------------------------

  void SetSaveSessionCallback(SaveSessionCallback callback);
  void SetRestoreSessionCallback(RestoreSessionCallback callback);
  void SetDeleteSessionCallback(DeleteSessionCallback callback);
  void SetRenameSessionCallback(RenameSessionCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildSaveButton();
  void BuildSnapshotsList();

  void RefreshSnapshots();

  void OnSaveClicked();
  void OnRestoreSession(const std::string& session_id);
  void OnDeleteSession(const std::string& session_id);
  void OnRenameSession(const std::string& session_id,
                       const std::u16string& new_name);

  std::vector<AstraSessionSnapshotItemView::SnapshotInfo> snapshots_;

  SaveSessionCallback save_callback_;
  RestoreSessionCallback restore_callback_;
  DeleteSessionCallback delete_callback_;
  RenameSessionCallback rename_callback_;

  raw_ptr<views::MdTextButton> save_button_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> snapshots_list_ = nullptr;

  std::vector<raw_ptr<AstraSessionSnapshotItemView>> snapshot_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SESSIONS_ASTRA_SESSION_MANAGER_VIEW_H_
