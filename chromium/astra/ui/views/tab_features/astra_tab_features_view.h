// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_FEATURES_ASTRA_TAB_FEATURES_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_FEATURES_ASTRA_TAB_FEATURES_VIEW_H_

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
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraTabFeaturesView — tab metadata panel
// =========================================================================
//
// A side panel / bubble showing Astra-specific metadata for the current tab.
// Similar to Arc browser's tab info panel.
//
// Layout:
//   +-------------------------------------------+
//   |  Tab Features                    [Close] |
//   +-------------------------------------------+
//   |  📌 Pinned to workspace                    |
//   |     [ Workspace Name ]                    |
//   +-------------------------------------------+
//   |  📝 Note                                  |
//   |     [ Note text...                   ]    |
//   +-------------------------------------------+
//   |  ⭐ Favorite                               |
//   |  📚 Reading list                           |
//   |  🏷️  Tags: [tag1] [tag2] [+ add]        |
//   +-------------------------------------------+
//   |  Last visited: 3 hours ago                |
//   |  Added to workspace: 2 days ago           |
//   +-------------------------------------------+
//
// This is a presentation-only view.  Tab feature data comes from
// AstraTabFeatures (content::WebContentsUserData in browser layer).
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::Textfield
// =========================================================================

class AstraTabFeaturesView : public views::BubbleDialogDelegateView {
 public:
  // Callback when workspace is changed.
  using WorkspaceChangedCallback =
      base::RepeatingCallback<void(const std::string& workspace_id)>;

  // Callback when favorite status is toggled.
  using FavoriteToggledCallback =
      base::RepeatingCallback<void(bool is_favorite)>;

  // Callback when reading list status is toggled.
  using ReadingListToggledCallback =
      base::RepeatingCallback<void(bool in_reading_list)>;

  // Callback when note is updated.
  using NoteUpdatedCallback =
      base::RepeatingCallback<void(const std::u16string& note)>;

  explicit AstraTabFeaturesView(views::View* anchor_view);
  ~AstraTabFeaturesView() override;

  AstraTabFeaturesView(const AstraTabFeaturesView&) = delete;
  AstraTabFeaturesView& operator=(const AstraTabFeaturesView&) = delete;

  // -- Tab data setters ----------------------------------------------------

  void SetTabTitle(const std::u16string& title);
  void SetTabUrl(const std::string& url);

  // Workspace membership.
  void SetWorkspaceName(const std::u16string& name);
  void SetWorkspaceId(const std::string& id);

  // Favorite status.
  void SetIsFavorite(bool is_favorite);
  bool IsFavorite() const { return is_favorite_; }

  // Reading list status.
  void SetInReadingList(bool in_reading_list);
  bool InReadingList() const { return in_reading_list_; }

  // Note.
  void SetNote(const std::u16string& note);
  std::u16string GetNote() const;

  // Tags.
  void SetTags(const std::vector<std::string>& tags);

  // Timestamps.
  void SetLastVisited(base::Time time);
  void SetAddedTime(base::Time time);

  // Pinned status.
  void SetIsPinned(bool is_pinned);

  // -- Callbacks -----------------------------------------------------------

  void SetWorkspaceChangedCallback(WorkspaceChangedCallback callback);
  void SetFavoriteToggledCallback(FavoriteToggledCallback callback);
  void SetReadingListToggledCallback(ReadingListToggledCallback callback);
  void SetNoteUpdatedCallback(NoteUpdatedCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildTabInfoHeader();
  void BuildWorkspaceSection();
  void BuildFavoriteSection();
  void BuildReadingListSection();
  void BuildNoteSection();
  void BuildTagsSection();
  void BuildMetadataSection();

  void UpdateFavoriteButton();
  void UpdateReadingListButton();

  void OnFavoriteToggled();
  void OnReadingListToggled();
  void OnWorkspaceClicked();
  void OnNoteChanged();
  void OnAddTagClicked();

  // Callbacks.
  WorkspaceChangedCallback workspace_changed_callback_;
  FavoriteToggledCallback favorite_toggled_callback_;
  ReadingListToggledCallback reading_list_toggled_callback_;
  NoteUpdatedCallback note_updated_callback_;

  // Tab state.
  std::u16string tab_title_;
  std::string tab_url_;
  std::string workspace_id_;
  std::u16string workspace_name_;
  bool is_favorite_ = false;
  bool in_reading_list_ = false;
  bool is_pinned_ = false;
  base::Time last_visited_;
  base::Time added_time_;
  std::vector<std::string> tags_;

  // Child views.
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> url_label_ = nullptr;

  raw_ptr<views::MdTextButton> workspace_button_ = nullptr;
  raw_ptr<views::MdTextButton> favorite_button_ = nullptr;
  raw_ptr<views::MdTextButton> reading_list_button_ = nullptr;
  raw_ptr<views::Textfield> note_field_ = nullptr;
  raw_ptr<views::View> tags_container_ = nullptr;

  raw_ptr<views::Label> last_visited_label_ = nullptr;
  raw_ptr<views::Label> added_time_label_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_FEATURES_ASTRA_TAB_FEATURES_VIEW_H_
