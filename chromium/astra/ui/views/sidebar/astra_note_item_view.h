#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_NOTE_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_NOTE_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/view.h"

#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

namespace views {
class ImageButton;
class Label;
}  // namespace views

namespace astra {

// Delegate interface for AstraNoteItemView actions.
class AstraNoteItemDelegate {
 public:
  virtual ~AstraNoteItemDelegate() = default;

  // Called when the user clicks the note item (primary action: edit).
  virtual void OnNoteItemClicked(const std::string& note_id) = 0;

  // Called when the user clicks the delete button.
  virtual void OnNoteDeleteRequested(const std::string& note_id) = 0;
};

// A single note item row in the sidebar notes section.
// Shows a color indicator bar, title, content snippet, and last modified time.
// On hover, shows a delete button.
//
// This is a pure presentation view — it does not own note state.
// Data is projected from AstraNoteService by the parent
// AstraSidebarNotesView.
//
// TODO(astra): Replace placeholder color bar with proper styled element.
//   Chromium owner: views::View + ColorProvider for theming.
class AstraNoteItemView : public AstraSidebarItemView {
 public:
  AstraNoteItemView(const std::string& note_id,
                    const std::u16string& title,
                    const std::u16string& preview,
                    const std::u16string& modified_time,
                    SkColor color);
  AstraNoteItemView(const AstraNoteItemView&) = delete;
  AstraNoteItemView& operator=(const AstraNoteItemView&) = delete;
  ~AstraNoteItemView() override;

  // -- Note info ----------------------------------------------------------

  // Set all note info at once.
  void SetNoteInfo(const std::string& note_id,
                   const std::u16string& title,
                   const std::u16string& preview);

  // Get the note ID.
  const std::string& GetNoteId() const { return note_id_; }

  // -- Preview text -------------------------------------------------------

  // Set the preview/snippet text.
  void SetPreviewText(const std::u16string& preview);
  std::u16string GetPreviewText() const { return preview_text_; }

  // Show or hide the preview text.
  void ShowPreview(bool show);

  // -- Note color ---------------------------------------------------------

  // Set the accent color for the note.
  void SetNoteColor(SkColor color);
  SkColor GetNoteColor() const { return note_color_; }

  // -- Tag count ----------------------------------------------------------

  // Set the number of tags on this note.
  void SetTagCount(int count);
  int GetTagCount() const { return tag_count_; }

  // Show or hide the tag count badge.
  void ShowTagCount(bool show);

  // -- Pinned state -------------------------------------------------------

  // Set whether the note is pinned.
  void SetPinned(bool pinned);
  bool IsPinned() const { return is_pinned_; }

  // -- Modified time ------------------------------------------------------

  // Set the last modified time.
  void SetModifiedTime(base::Time time);
  base::Time GetModifiedTime() const { return modified_time_; }

  // -- Delegate -----------------------------------------------------------

  void set_delegate(AstraNoteItemDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;

 protected:
  // AstraSidebarItemView overrides.
  void BuildLayout() override;
  void OnItemClicked() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  // Update the color indicator bar.
  void UpdateColorIndicator();

  // Update the preview text label.
  void UpdatePreviewText();

  // Update action button visibility based on hover state.
  void UpdateActionButtonsVisibility();

  // Format modified time as a human-readable string.
  std::u16string FormatModifiedTime() const;

  // Update the time label.
  void UpdateTimeLabel();

  // Button action handlers.
  void OnDeleteButtonPressed();

  // The note ID this item represents.
  std::string note_id_;

  // Note state.
  std::u16string preview_text_;
  SkColor note_color_ = SK_ColorYELLOW;
  int tag_count_ = 0;
  bool is_pinned_ = false;
  base::Time modified_time_;
  bool show_preview_ = true;
  bool show_tag_count_ = false;

  // Action delegate. Not owned.
  raw_ptr<AstraNoteItemDelegate> delegate_ = nullptr;

  // Child views.
  raw_ptr<views::View> color_bar_ = nullptr;
  raw_ptr<views::Label> time_label_ = nullptr;
  raw_ptr<views::ImageButton> delete_button_ = nullptr;

  // Hover state for showing/hiding action buttons.
  bool is_hovered_internal_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_NOTE_ITEM_VIEW_H_
