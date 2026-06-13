#ifndef ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_SEARCH_BOX_H_
#define ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_SEARCH_BOX_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"
#include "ui/views/controls/textfield/textfield_controller.h"

namespace views {
class Label;
class MdTextButton;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraSettingsSearchBox — search text field for settings filtering
// =========================================================================
//
// AstraSettingsSearchBox is a search textfield with a search icon on the
// left and a clear button on the right.  It is used at the top of the
// settings bubble to filter settings sections by text search.
//
// When the user types, the |text_changed_callback| is called with the
// current query text.  The owning page view uses this to filter which
// sections are visible.
//
// Chromium subsystems reused:
//   - views::Textfield (text input)
//   - views::BoxLayout (layout)
//   - views::MdTextButton (clear button)
//
// Chromium pattern reference:
//   chrome/browser/ui/views/omnibox/omnibox_view.h — search text input
//   chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc — search field
//   ui/views/controls/textfield/textfield.h — base text field component
//
// TODO(astra): Consider using Chrome's SearchResultBaseView or
//   SearchEngineEditorDialog search field as a pattern reference.
//   For now, this is a simple Textfield-based search box.
// =========================================================================

class AstraSettingsSearchBox : public views::View,
                               public views::TextfieldController {
 public:
  using TextChangedCallback =
      base::RepeatingCallback<void(const std::u16string&)>;

  explicit AstraSettingsSearchBox(TextChangedCallback text_changed_callback);
  ~AstraSettingsSearchBox() override;

  AstraSettingsSearchBox(const AstraSettingsSearchBox&) = delete;
  AstraSettingsSearchBox& operator=(const AstraSettingsSearchBox&) = delete;

  // -- Query manipulation --------------------------------------------------

  // Set the search query programmatically.
  void SetQuery(const std::u16string& query);

  // Get the current search query.
  std::u16string GetQuery() const;

  // Alias for SetQuery (for backward compatibility).
  void SetText(const std::u16string& text) { SetQuery(text); }

  // Alias for GetQuery (for backward compatibility).
  std::u16string GetText() const { return GetQuery(); }

  // Clear the search query.
  void Clear();

  // -- Placeholder ---------------------------------------------------------

  // Set the placeholder text shown when the query is empty.
  void SetPlaceholder(const std::u16string& placeholder);

  // Get the current placeholder text.
  std::u16string GetPlaceholder() const;

  // -- Icon and button visibility ------------------------------------------

  // Set whether the search icon is visible.
  void SetSearchIconVisible(bool visible);

  // Set whether the clear button is visible.
  void SetClearButtonVisible(bool visible);

  // -- Accessors -----------------------------------------------------------

  // Returns the textfield view for accessibility / focus purposes.
  views::Textfield* textfield() { return textfield_; }

  // Returns the search icon view.
  views::View* search_icon() { return search_icon_; }

  // Returns the clear button view.
  views::MdTextButton* clear_button() { return clear_button_; }

  // -- views::TextfieldController ------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize() const override;

 private:
  // Handler for the clear button.
  void OnClearButtonPressed();

  // Updates the clear button visibility based on text content.
  void UpdateClearButtonVisibility();

  TextChangedCallback text_changed_callback_;

  raw_ptr<views::View> search_icon_ = nullptr;
  raw_ptr<views::Textfield> textfield_ = nullptr;
  raw_ptr<views::MdTextButton> clear_button_ = nullptr;

  // Whether the clear button was explicitly hidden by the caller.
  bool clear_button_explicitly_hidden_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_SEARCH_BOX_H_
