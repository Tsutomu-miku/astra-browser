// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_FIND_BAR_ASTRA_FIND_BAR_VIEW_H_
#define ASTRA_UI_VIEWS_FIND_BAR_ASTRA_FIND_BAR_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
class Label;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraFindBarView — find-in-page bar
// =========================================================================
//
// A find bar that appears at the top of the page content area for
// find-in-page functionality.  Includes search field, navigation buttons,
// match count, and options.
//
// Layout (left to right):
//   1. Find icon
//   2. Search text field
//   3. Match count label (e.g. "3 of 12")
//   4. Previous match button
//   5. Next match button
//   6. Highlight all toggle
//   7. Case sensitivity toggle
//   8. Close button
//
// Chromium pattern: FindBar / FindBarView
//   (chrome/browser/ui/views/find_bar_view.h)
//
// TODO(astra): Integrate with BrowserView via a patch.
//   Chromium owner: FindBar (chrome/browser/ui/find_bar/find_bar.h)
//   Patch point: BrowserView — show/hide find bar on Ctrl+F
// =========================================================================

class AstraFindBarView : public views::View,
                         public views::TextfieldController {
 public:
  // Delegate for find bar actions.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the find text changes.
    virtual void OnFindTextChanged(const std::u16string& text) = 0;

    // Called to find the next match.
    virtual void OnFindNext() = 0;

    // Called to find the previous match.
    virtual void OnFindPrevious() = 0;

    // Called when the find bar is closed.
    virtual void OnFindBarClosed() = 0;

    // Called when highlight all toggle changes.
    virtual void OnHighlightAllChanged(bool highlight) = 0;

    // Called when case sensitivity changes.
    virtual void OnCaseSensitivityChanged(bool case_sensitive) = 0;
  };

  AstraFindBarView();
  ~AstraFindBarView() override;

  AstraFindBarView(const AstraFindBarView&) = delete;
  AstraFindBarView& operator=(const AstraFindBarView&) = delete;

  // -- Show / hide ----------------------------------------------------------

  // Show the find bar and focus the search field.
  void Show();

  // Hide the find bar.
  void Hide();

  // Whether the find bar is visible.
  bool IsBarVisible() const { return GetVisible(); }

  // -- Find state -----------------------------------------------------------

  // Set the search text programmatically.
  void SetFindText(const std::u16string& text);

  // Get the current search text.
  std::u16string GetFindText() const;

  // Set the match count and current match index.
  void SetMatchInfo(int current_match, int total_matches);

  // Set whether highlight all is enabled.
  void SetHighlightAll(bool highlight);
  bool highlight_all() const { return highlight_all_; }

  // Set whether case sensitivity is enabled.
  void SetCaseSensitive(bool case_sensitive);
  bool case_sensitive() const { return case_sensitive_; }

  // -- Delegate -------------------------------------------------------------

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // -- views::View ----------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // -- views::TextfieldController -------------------------------------------

  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

 private:
  // Build child views.
  void BuildLayout();

  // Update colors from the color provider.
  void UpdateColors();

  // Update the match count label.
  void UpdateMatchLabel();

  // Update button states.
  void UpdateButtonStates();

  // Button handlers.
  void OnPreviousPressed();
  void OnNextPressed();
  void OnClosePressed();
  void OnHighlightAllToggled();
  void OnCaseSensitiveToggled();

  // Delegate.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Find state.
  std::u16string find_text_;
  int current_match_ = 0;
  int total_matches_ = 0;
  bool highlight_all_ = true;
  bool case_sensitive_ = false;

  // Child views.
  raw_ptr<views::ImageView> find_icon_ = nullptr;
  raw_ptr<views::Textfield> textfield_ = nullptr;
  raw_ptr<views::Label> match_label_ = nullptr;
  raw_ptr<views::ImageButton> previous_button_ = nullptr;
  raw_ptr<views::ImageButton> next_button_ = nullptr;
  raw_ptr<views::ImageButton> highlight_button_ = nullptr;
  raw_ptr<views::ImageButton> case_button_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;

  // -- Constants ------------------------------------------------------------

  static constexpr int kFindBarHeight = 36;
  static constexpr int kHorizontalPadding = 8;
  static constexpr int kIconSize = 16;
  static constexpr int kIconTextSpacing = 6;
  static constexpr int kButtonSize = 24;
  static constexpr int kButtonSpacing = 2;
  static constexpr int kTextFieldWidth = 200;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_FIND_BAR_ASTRA_FIND_BAR_VIEW_H_
