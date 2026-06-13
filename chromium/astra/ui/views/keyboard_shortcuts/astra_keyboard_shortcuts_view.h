// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_KEYBOARD_SHORTCUTS_ASTRA_KEYBOARD_SHORTCUTS_VIEW_H_
#define ASTRA_UI_VIEWS_KEYBOARD_SHORTCUTS_ASTRA_KEYBOARD_SHORTCUTS_VIEW_H_

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
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraShortcutItemView — single keyboard shortcut row
// =========================================================================
//
// A row showing one keyboard shortcut: description and key combination.
//
// Layout:
//   +-------------------------------------------+
//   |  Open new tab           Ctrl+T             |
//   +-------------------------------------------+
// =========================================================================

class AstraShortcutItemView : public views::View {
 public:
  struct ShortcutInfo {
    std::string shortcut_id;
    std::u16string description;
    std::u16string shortcut;  // e.g. "Ctrl+T"
    std::u16string category;  // e.g. "Tabs", "Navigation"
    bool is_astra = false;    // Astra-specific shortcut
  };

  explicit AstraShortcutItemView(const ShortcutInfo& info);
  ~AstraShortcutItemView() override;

  AstraShortcutItemView(const AstraShortcutItemView&) = delete;
  AstraShortcutItemView& operator=(const AstraShortcutItemView&) = delete;

  const std::string& shortcut_id() const { return shortcut_id_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();

  std::string shortcut_id_;
  std::u16string description_;
  std::u16string shortcut_;
  std::u16string category_;
  bool is_astra_ = false;

  raw_ptr<views::Label> desc_label_ = nullptr;
  raw_ptr<views::Label> shortcut_label_ = nullptr;
};

// =========================================================================
// AstraKeyboardShortcutsView — keyboard shortcuts reference panel
// =========================================================================
//
// A bubble showing a searchable list of keyboard shortcuts, grouped by
// category (Tabs, Navigation, Page, Astra, etc.).
//
// Layout:
//   +-------------------------------------------+
//   |  Keyboard Shortcuts           [Close]    |
//   +-------------------------------------------+
//   |  🔍 Search shortcuts...                    |
//   +-------------------------------------------+
//   |  Tabs                                      |
//   |  Open new tab           Ctrl+T             |
//   |  Close tab               Ctrl+W            |
//   |  Next tab               Ctrl+Tab          |
//   |  ...                                      |
//   +-------------------------------------------+
//   |  Navigation                                |
//   |  Back                     Alt+Left         |
//   |  Forward                  Alt+Right        |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Shortcut definitions come from
// Chromium's accelerator table plus Astra-specific shortcuts.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - ui::Accelerator (source of shortcut definitions)
// =========================================================================

class AstraKeyboardShortcutsView : public views::BubbleDialogDelegateView {
 public:
  using SearchCallback =
      base::RepeatingCallback<void(const std::u16string& query)>;
  using ShortcutCallback =
      base::RepeatingCallback<void(const std::string& shortcut_id)>;

  explicit AstraKeyboardShortcutsView(views::View* anchor_view);
  ~AstraKeyboardShortcutsView() override;

  AstraKeyboardShortcutsView(const AstraKeyboardShortcutsView&) = delete;
  AstraKeyboardShortcutsView& operator=(
      const AstraKeyboardShortcutsView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetShortcuts(
      const std::vector<AstraShortcutItemView::ShortcutInfo>& shortcuts);

  // -- Callbacks -----------------------------------------------------------

  void SetSearchCallback(SearchCallback callback);
  void SetShortcutCallback(ShortcutCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildSearchBar();
  void BuildShortcutsList();

  void RefreshShortcuts();
  void OnSearchTextChanged();

  std::vector<AstraShortcutItemView::ShortcutInfo> shortcuts_;

  SearchCallback search_callback_;
  ShortcutCallback shortcut_callback_;

  raw_ptr<views::Textfield> search_field_ = nullptr;
  raw_ptr<views::View> shortcuts_list_ = nullptr;

  std::vector<raw_ptr<AstraShortcutItemView>> shortcut_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_KEYBOARD_SHORTCUTS_ASTRA_KEYBOARD_SHORTCUTS_VIEW_H_
