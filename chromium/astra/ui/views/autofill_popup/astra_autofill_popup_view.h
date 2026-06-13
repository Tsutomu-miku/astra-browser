// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_AUTOFILL_POPUP_ASTRA_AUTOFILL_POPUP_VIEW_H_
#define ASTRA_UI_VIEWS_AUTOFILL_POPUP_ASTRA_AUTOFILL_POPUP_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

namespace astra {

// Types of autofill suggestions.
enum class AstraAutofillSuggestionType {
  kAutofillProfile,    // Address/profile autofill
  kCreditCard,         // Credit card autofill
  kPassword,           // Password manager autofill
  kAddress,            // Address autocomplete
  kEmail,              // Email autocomplete
  kName,               // Name autocomplete
  kPhone,              // Phone number autocomplete
  kOrganization,       // Organization autocomplete
  kAutocomplete,       // General form autocomplete
  kIban,               // IBAN number
  kPlusAddress,        // Plus address / email alias
};

// A single autofill suggestion.
struct AstraAutofillSuggestion {
  AstraAutofillSuggestionType type =
      AstraAutofillSuggestionType::kAutocomplete;

  // Primary text (the value that will be filled).
  std::u16string main_text;

  // Secondary text (e.g. label, last used date, card last 4 digits).
  std::u16string secondary_text;

  // Minor text (e.g. full address, additional details).
  std::u16string minor_text;

  // Icon for the suggestion type.
  gfx::ImageSkia icon;
  std::string icon_name;

  // Whether this is a warning/info item (not fillable).
  bool is_instruction = false;
  bool is_warning = false;

  // Whether this is a separator.
  bool is_separator = false;

  // Whether this shows a delete button.
  bool deletable = false;

  // Unique identifier for this suggestion.
  int suggestion_id = 0;

  // For password suggestions.
  std::string password_value;  // Encrypted/obfuscated in real code.
  std::u16string username_label;

  // For credit card suggestions.
  std::u16string card_network_label;
  SkColor card_color = SK_ColorTRANSPARENT;
};

// Delegate for autofill popup interactions.
class AstraAutofillPopupDelegate {
 public:
  virtual ~AstraAutofillPopupDelegate() = default;

  // Called when a suggestion is accepted.
  virtual void OnAutofillSuggestionAccepted(int suggestion_id) = 0;

  // Called when a suggestion is deleted.
  virtual void OnAutofillSuggestionDeleted(int suggestion_id) {}

  // Called when the popup is closed.
  virtual void OnAutofillPopupClosed() {}

  // Called when the user presses "View more" / footer action.
  virtual void OnAutofillFooterAction() {}
};

// A single suggestion row in the autofill popup.
class AstraAutofillSuggestionView : public views::View {
 public:
  METADATA_HEADER(AstraAutofillSuggestionView);

  AstraAutofillSuggestionView(const AstraAutofillSuggestion& suggestion,
                              int index);
  AstraAutofillSuggestionView(const AstraAutofillSuggestionView&) = delete;
  AstraAutofillSuggestionView& operator=(
      const AstraAutofillSuggestionView&) = delete;
  ~AstraAutofillSuggestionView() override;

  void UpdateFromSuggestion(const AstraAutofillSuggestion& suggestion);

  void SetSelected(bool selected);
  bool IsSelected() const { return selected_; }

  int index() const { return index_; }
  int suggestion_id() const { return suggestion_.suggestion_id; }

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  void HandleClick();
  void UpdateVisuals();

  AstraAutofillSuggestion suggestion_;
  int index_;
  bool selected_ = false;
  bool hovered_ = false;

  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> main_label_ = nullptr;
  raw_ptr<views::Label> secondary_label_ = nullptr;
  raw_ptr<views::Label> minor_label_ = nullptr;
  raw_ptr<views::ImageButton> delete_button_ = nullptr;

  static constexpr int kRowHeight = 40;
  static constexpr int kTallRowHeight = 52;
  static constexpr int kIconSize = 20;
  static constexpr int kIconPadding = 12;
  static constexpr int kTextSpacing = 8;
  static constexpr int kRightPadding = 12;
};

// The main autofill popup view.
//
// A bubble-style popup that appears below form fields and shows
// autocomplete / autofill suggestions. Supports:
//   - Profile/address suggestions
//   - Credit card suggestions
//   - Password suggestions
//   - General form autocomplete
//   - Footer with "Manage" action
//   - Keyboard navigation
//
// Chromium owner: AutofillPopupViewViews / AutofillPopupController
//   (chrome/browser/ui/views/autofill/autofill_popup_view_views.h)
//
// TODO(astra): Integrate with Chromium's AutofillController via
// a patch to chrome/browser/ui/autofill/chrome_autofill_client.cc
// to use Astra's popup view.
class AstraAutofillPopupView : public views::BubbleDialogDelegateView {
 public:
  METADATA_HEADER(AstraAutofillPopupView);

  explicit AstraAutofillPopupView(views::View* anchor_view);
  AstraAutofillPopupView(const AstraAutofillPopupView&) = delete;
  AstraAutofillPopupView& operator=(const AstraAutofillPopupView&) = delete;
  ~AstraAutofillPopupView() override;

  // -- Suggestions ----------------------------------------------------------

  void SetSuggestions(const std::vector<AstraAutofillSuggestion>& suggestions);
  const std::vector<AstraAutofillSuggestion>& GetSuggestions() const;
  size_t GetSuggestionCount() const;

  // -- Selection ------------------------------------------------------------

  void SetSelectedIndex(int index);
  int GetSelectedIndex() const { return selected_index_; }
  void SelectNext();
  void SelectPrevious();

  // -- Popup management -----------------------------------------------------

  void ShowPopup();
  void HidePopup();
  bool IsPopupShowing() const;

  // -- Delegate -------------------------------------------------------------

  void SetDelegate(AstraAutofillPopupDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraAutofillPopupDelegate* delegate() const { return delegate_; }

  // -- Footer ---------------------------------------------------------------

  void SetFooterText(const std::u16string& text);
  void SetFooterVisible(bool visible);

  // -- Keyboard -------------------------------------------------------------

  bool HandleKeyEvent(const ui::KeyEvent& event);

  // -- View access (for testing) -------------------------------------------

  views::ScrollView* scroll_view_for_test() { return scroll_view_; }
  size_t GetSuggestionViewCount() const { return suggestion_views_.size(); }

  // -- views::BubbleDialogDelegateView: -----------------------------------

  std::u16string GetWindowTitle() const override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

 private:
  void Build();
  void RebuildSuggestions();
  void UpdateSelectionHighlight();
  void AcceptSelectedSuggestion();
  void OnSuggestionClicked(int index);

  raw_ptr<AstraAutofillPopupDelegate> delegate_ = nullptr;

  std::vector<AstraAutofillSuggestion> suggestions_;
  int selected_index_ = -1;

  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> content_view_ = nullptr;
  raw_ptr<views::View> footer_ = nullptr;
  raw_ptr<views::Label> footer_label_ = nullptr;

  std::vector<raw_ptr<AstraAutofillSuggestionView>> suggestion_views_;

  static constexpr int kPopupWidth = 320;
  static constexpr int kPopupMaxHeight = 400;
  static constexpr int kFooterHeight = 36;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_AUTOFILL_POPUP_ASTRA_AUTOFILL_POPUP_VIEW_H_
