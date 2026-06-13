// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_NOTES_PAGE_ASTRA_NOTES_PAGE_VIEW_H_
#define ASTRA_UI_VIEWS_NOTES_PAGE_ASTRA_NOTES_PAGE_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"
#include "astra/ui/views/notes_page/astra_notes_page_model.h"

namespace views {
class BoxLayout;
class FlexLayout;
class ImageButton;
class Label;
class ScrollView;
class Textfield;
}  // namespace views

namespace astra {

// Display mode for note items (grid or list).
enum class AstraNotesDisplayMode {
  kGrid,
  kList,
};

// A single note card / list item view.
class AstraNoteCardView : public views::View {
 public:
  METADATA_HEADER(AstraNoteCardView);

  explicit AstraNoteCardView(const AstraNoteEntry& entry);
  ~AstraNoteCardView() override;

  AstraNoteCardView(const AstraNoteCardView&) = delete;
  AstraNoteCardView& operator=(const AstraNoteCardView&) = delete;

  // Update the note data displayed.
  void Update(const AstraNoteEntry& entry);

  const std::string& note_id() const { return entry_.id; }
  const std::u16string& title() const { return entry_.title; }
  const AstraNoteEntry& entry() const { return entry_; }

  void SetDisplayMode(AstraNotesDisplayMode mode);
  void SetSelected(bool selected);
  bool selected() const { return selected_; }

  // Callback when the card is clicked.
  using ClickCallback = base::RepeatingCallback<void(const std::string&)>;
  void SetClickCallback(ClickCallback callback) {
    click_callback_ = std::move(callback);
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnThemeChanged() override;

 private:
  void Build();
  void DrawColorStripe(gfx::Canvas* canvas,
                       const gfx::Rect& bounds,
                       const std::string& color);
  void DrawPinIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color);
  SkColor GetColorForName(const std::string& color_name) const;

  AstraNoteEntry entry_;
  AstraNotesDisplayMode display_mode_ = AstraNotesDisplayMode::kGrid;
  bool selected_ = false;
  ClickCallback click_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> preview_label_ = nullptr;
  raw_ptr<views::Label> date_label_ = nullptr;
  raw_ptr<views::Label> tags_label_ = nullptr;
  raw_ptr<views::View> color_stripe_ = nullptr;
};

// A folder item view for the left sidebar.
class AstraNoteFolderItemView : public views::View {
 public:
  METADATA_HEADER(AstraNoteFolderItemView);

  // Special folder types.
  enum class SpecialFolder {
    kAllNotes,
    kPinned,
    kArchive,
  };

  // Constructor for special folders.
  explicit AstraNoteFolderItemView(SpecialFolder type, int count = 0);

  // Constructor for custom folders.
  AstraNoteFolderItemView(const AstraNoteFolder& folder, int depth);

  ~AstraNoteFolderItemView() override;

  AstraNoteFolderItemView(const AstraNoteFolderItemView&) = delete;
  AstraNoteFolderItemView& operator=(const AstraNoteFolderItemView&) = delete;

  void Update(const AstraNoteFolder& folder);
  void SetCount(int count);
  void SetSelected(bool selected);
  bool selected() const { return selected_; }

  SpecialFolder special_type() const { return special_type_; }
  const std::string& folder_id() const { return folder_id_; }
  const std::u16string& name() const { return name_; }
  bool is_special() const { return is_special_; }

  using SelectCallback = base::RepeatingCallback<void(AstraNoteFolderItemView*)>;
  void SetSelectCallback(SelectCallback callback) {
    select_callback_ = std::move(callback);
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnThemeChanged() override;

 private:
  void Build();
  void DrawFolderIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color);
  void DrawSpecialIcon(gfx::Canvas* canvas,
                       const gfx::Rect& bounds,
                       SkColor color);

  SpecialFolder special_type_;
  std::string folder_id_;
  std::u16string name_;
  std::string color_;
  int count_ = 0;
  int depth_ = 0;
  bool is_special_ = false;
  bool selected_ = false;

  SelectCallback select_callback_;

  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
};

// A tag chip view.
class AstraNoteTagChipView : public views::View {
 public:
  METADATA_HEADER(AstraNoteTagChipView);

  explicit AstraNoteTagChipView(const std::string& tag);
  ~AstraNoteTagChipView() override;

  AstraNoteTagChipView(const AstraNoteTagChipView&) = delete;
  AstraNoteTagChipView& operator=(const AstraNoteTagChipView&) = delete;

  const std::string& tag() const { return tag_; }
  void SetSelected(bool selected);
  bool selected() const { return selected_; }

  using ClickCallback = base::RepeatingCallback<void(const std::string&)>;
  void SetClickCallback(ClickCallback callback) {
    click_callback_ = std::move(callback);
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnThemeChanged() override;

 private:
  std::string tag_;
  bool selected_ = false;
  ClickCallback click_callback_;
};

// The full notes page view with three-pane layout.
//
// Layout:
//   +-------------------------------------------------------------+
//   |  New Note | Search ... | Sort | Grid | List |               |  <- top toolbar
//   +----------------+-----------------------+--------------------+
//   |                |                       |                    |
//   |  Sidebar       |   Note List           |   Note Editor      |
//   |  - Folders     |   (grid/list)         |   - Title field    |
//   |  - Tags        |                       |   - Content area   |
//   |                |                       |   - Toolbar        |
//   |                |                       |   - Color picker   |
//   |                |                       |   - Tags input     |
//   |                |                       |   - Delete button  |
//   +----------------+-----------------------+--------------------+
//
// Chromium owner: UserNotes WebUI (chrome/browser/ui/webui/user_notes/)
//   This is a Views-based alternative to the WebUI notes manager.
//
// TODO(astra): Wire up to Chromium's UserNotes service.  Patch point:
// chrome/browser/user_notes/user_notes_ui.cc.
class AstraNotesPageView : public views::View,
                           public AstraNotesPageObserver,
                           public views::TextfieldController {
 public:
  METADATA_HEADER(AstraNotesPageView);

  AstraNotesPageView();
  explicit AstraNotesPageView(AstraNotesPageModel* model);
  ~AstraNotesPageView() override;

  AstraNotesPageView(const AstraNotesPageView&) = delete;
  AstraNotesPageView& operator=(const AstraNotesPageView&) = delete;

  // Set the model to observe.
  void SetModel(AstraNotesPageModel* model);
  AstraNotesPageModel* model() const { return model_; }

  // -- Display mode ---------------------------------------------------------

  void SetDisplayMode(AstraNotesDisplayMode mode);
  AstraNotesDisplayMode display_mode() const { return display_mode_; }

  // -- AstraNotesPageObserver: ---------------------------------------------

  void OnNotesChanged() override;
  void OnNoteAdded(const std::string& id) override;
  void OnNoteRemoved(const std::string& id) override;
  void OnNoteUpdated(const std::string& id) override;
  void OnFolderAdded(const std::string& id) override;
  void OnFolderRemoved(const std::string& id) override;
  void OnActiveNoteChanged(const std::string& id) override;
  void OnSearchChanged(const std::u16string& query) override;
  void OnFilterChanged() override;
  void OnNotesPageModelShutdown() override;

  // -- views::View: --------------------------------------------------------

  void Layout() override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

  // -- TextfieldController: ------------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // Accessors for testing.
  views::Textfield* search_field_for_test() { return search_field_; }
  views::ImageButton* new_note_button_for_test() { return new_note_button_; }
  views::ImageButton* grid_view_button_for_test() { return grid_view_button_; }
  views::ImageButton* list_view_button_for_test() { return list_view_button_; }
  views::View* sidebar_for_test() { return sidebar_container_; }
  views::ScrollView* notes_scroll_for_test() { return notes_scroll_; }
  views::View* editor_for_test() { return editor_container_; }
  views::Textfield* editor_title_for_test() { return editor_title_; }
  views::Textfield* editor_content_for_test() { return editor_content_; }
  size_t note_card_count_for_test() const { return note_cards_.size(); }
  size_t folder_item_count_for_test() const { return folder_items_.size(); }
  size_t tag_chip_count_for_test() const { return tag_chips_.size(); }

 private:
  // Build the entire UI.
  void Build();

  // Build the top toolbar.
  void BuildToolbar();

  // Build the left sidebar.
  void BuildSidebar();

  // Build the middle note list pane.
  void BuildNotesList();

  // Build the right editor pane.
  void BuildEditor();

  // Rebuild folders list from model.
  void RebuildFolders();

  // Rebuild tags list from model.
  void RebuildTags();

  // Rebuild note cards from model.
  void RebuildNoteCards();

  // Update editor with active note content.
  void UpdateEditorFromActiveNote();

  // Show/hide empty state in notes list.
  void UpdateEmptyState();

  // Draw custom icon helpers.
  void DrawNoteIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);
  void DrawSearchIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color);
  void DrawAddIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color);
  void DrawGridIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);
  void DrawListIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);
  void DrawSortIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);
  void DrawFolderIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color);
  void DrawTagIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color);
  void DrawPinIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color);
  void DrawArchiveIcon(gfx::Canvas* canvas,
                       const gfx::Rect& bounds,
                       SkColor color);
  void DrawTrashIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color);
  void DrawColorPaletteIcon(gfx::Canvas* canvas,
                            const gfx::Rect& bounds,
                            SkColor color);
  void DrawEditIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);
  void DrawCheckIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color);
  void DrawMoreIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color);

  // Button handlers.
  void OnNewNoteClicked();
  void OnGridViewClicked();
  void OnListViewClicked();
  void OnSortClicked();
  void OnNoteCardClicked(const std::string& note_id);
  void OnFolderClicked(AstraNoteFolderItemView* item);
  void OnTagClicked(const std::string& tag);
  void OnDeleteNoteClicked();
  void OnColorClicked(const std::string& color);
  void OnPinNoteClicked();
  void OnArchiveNoteClicked();

  // Determine which special folder is selected.
  AstraNoteFolderItemView* GetSelectedFolderItem() const;

  // Model.
  raw_ptr<AstraNotesPageModel> model_ = nullptr;
  base::ScopedObservation<AstraNotesPageModel, AstraNotesPageObserver>
      model_observation_{this};

  // Display state.
  AstraNotesDisplayMode display_mode_ = AstraNotesDisplayMode::kGrid;

  // Child views - top toolbar.
  raw_ptr<views::View> toolbar_ = nullptr;
  raw_ptr<views::ImageButton> new_note_button_ = nullptr;
  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::ImageButton> sort_button_ = nullptr;
  raw_ptr<views::ImageButton> grid_view_button_ = nullptr;
  raw_ptr<views::ImageButton> list_view_button_ = nullptr;

  // Child views - left sidebar.
  raw_ptr<views::View> sidebar_container_ = nullptr;
  raw_ptr<views::ScrollView> sidebar_scroll_ = nullptr;
  raw_ptr<views::View> sidebar_content_ = nullptr;
  raw_ptr<views::View> folder_list_ = nullptr;
  raw_ptr<views::View> tags_section_ = nullptr;
  raw_ptr<views::View> tags_container_ = nullptr;

  // Child views - middle note list.
  raw_ptr<views::View> notes_container_ = nullptr;
  raw_ptr<views::ScrollView> notes_scroll_ = nullptr;
  raw_ptr<views::View> notes_grid_ = nullptr;
  raw_ptr<views::View> empty_state_view_ = nullptr;

  // Child views - right editor.
  raw_ptr<views::View> editor_container_ = nullptr;
  raw_ptr<views::Textfield> editor_title_ = nullptr;
  raw_ptr<views::Textfield> editor_content_ = nullptr;
  raw_ptr<views::View> editor_toolbar_ = nullptr;
  raw_ptr<views::ImageButton> pin_button_ = nullptr;
  raw_ptr<views::ImageButton> archive_button_ = nullptr;
  raw_ptr<views::ImageButton> delete_button_ = nullptr;
  raw_ptr<views::View> color_picker_ = nullptr;
  raw_ptr<views::Textfield> tags_input_ = nullptr;
  raw_ptr<views::View> empty_editor_view_ = nullptr;

  // Owned note card views.
  std::vector<raw_ptr<AstraNoteCardView, VectorExperimental>> note_cards_;

  // Owned folder item views.
  std::vector<raw_ptr<AstraNoteFolderItemView, VectorExperimental>> folder_items_;

  // Owned tag chip views.
  std::vector<raw_ptr<AstraNoteTagChipView, VectorExperimental>> tag_chips_;

  // Layout constants.
  static constexpr int kToolbarHeight = 52;
  static constexpr int kSidebarWidth = 240;
  static constexpr int kEditorWidth = 360;
  static constexpr int kSidePadding = 16;
  static constexpr int kToolbarSpacing = 8;
  static constexpr int kNoteCardWidth = 200;
  static constexpr int kNoteCardHeight = 160;
  static constexpr int kNoteCardSpacing = 12;
  static constexpr int kSearchFieldWidth = 280;
  static constexpr int kButtonSize = 32;
  static constexpr int kSectionSpacing = 16;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_NOTES_PAGE_ASTRA_NOTES_PAGE_VIEW_H_
