// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_OMNIBOX_POPUP_ASTRA_OMNIBOX_POPUP_VIEW_H_
#define ASTRA_UI_VIEWS_OMNIBOX_POPUP_ASTRA_OMNIBOX_POPUP_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

#include "astra/ui/views/omnibox_popup/astra_omnibox_popup_model.h"

namespace views {
class ImageView;
class Label;
class ScrollView;
}  // namespace views

namespace astra {

// Delegate for the omnibox popup.
class AstraOmniboxPopupDelegate {
 public:
  virtual ~AstraOmniboxPopupDelegate() = default;

  // Called when a suggestion is selected (e.g. by pressing Enter).
  virtual void OnOmniboxMatchSelected(const AstraOmniboxMatch& match) = 0;

  // Called when the secondary action on a match is triggered.
  virtual void OnOmniboxMatchAction(const AstraOmniboxMatch& match,
                                     const std::string& action_type) {}

  // Called when the popup is closed.
  virtual void OnOmniboxPopupClosed() {}

  // Called when the user presses Tab to move to next match (optional).
  virtual void OnOmniboxTabPressed() {}
};

// A single match row in the omnibox popup.
//
// Layout:
//   [icon] [contents text]      [right-side content]
//          [description text]   [action button]
//
// Chromium owner: OmniboxResultView
//   (chrome/browser/ui/views/omnibox/omnibox_result_view.h)
class AstraOmniboxPopupMatchView : public views::View {
 public:
  METADATA_HEADER(AstraOmniboxPopupMatchView);

  AstraOmniboxPopupMatchView(const AstraOmniboxMatch& match,
                             size_t index);
  AstraOmniboxPopupMatchView(const AstraOmniboxPopupMatchView&) = delete;
  AstraOmniboxPopupMatchView& operator=(const AstraOmniboxPopupMatchView&) =
      delete;
  ~AstraOmniboxPopupMatchView() override;

  // Update from a match.
  void UpdateFromMatch(const AstraOmniboxMatch& match);

  // -- Selection ------------------------------------------------------------

  void SetSelected(bool selected);
  bool IsSelected() const { return selected_; }

  // -- Accessors ------------------------------------------------------------

  size_t index() const { return index_; }
  const AstraOmniboxMatch& match() const { return match_; }

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
  void OnGestureEvent(ui::GestureEvent* event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Handle primary click.
  void HandlePrimaryClick();

  // Update visual appearance.
  void UpdateVisuals();

  AstraOmniboxMatch match_;
  size_t index_;
  bool selected_ = false;
  bool hovered_ = false;

  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> contents_label_ = nullptr;
  raw_ptr<views::Label> description_label_ = nullptr;
  raw_ptr<views::ImageView> bookmark_icon_ = nullptr;
  raw_ptr<views::View> answer_container_ = nullptr;
  raw_ptr<views::Label> answer_text_label_ = nullptr;

  static constexpr int kRowHeight = 44;
  static constexpr int kIconSize = 20;
  static constexpr int kIconLeftPadding = 16;
  static constexpr int kIconTextSpacing = 12;
  static constexpr int kTextRightPadding = 16;
  static constexpr int kAnswerHeight = 56;
};

// Section header for grouped omnibox results.
class AstraOmniboxPopupSectionHeader : public views::View {
 public:
  METADATA_HEADER(AstraOmniboxPopupSectionHeader);

  explicit AstraOmniboxPopupSectionHeader(const std::u16string& title);
  ~AstraOmniboxPopupSectionHeader() override;

  void SetTitle(const std::u16string& title);

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;

 private:
  raw_ptr<views::Label> title_label_ = nullptr;
  static constexpr int kHeaderHeight = 28;
  static constexpr int kHeaderLeftPadding = 16;
};

// The main omnibox popup view.
//
// A bubble-style popup that appears below the omnibox/textfield and
// shows autocomplete suggestions. Supports:
//   - Multiple suggestion types with icons
//   - Grouped results with section headers
//   - Answer cards (calculator, weather, etc.)
//   - Keyboard navigation (up/down arrows, Enter, Tab)
//   - Scrollable when many results
//   - "Switch to tab" actions
//   - Bookmark star indicator
//
// Chromium owner: OmniboxPopupViewViews / OmniboxPopupContentsView
//   (chrome/browser/ui/views/omnibox/omnibox_popup_view_views.h)
//
// TODO(astra): Integrate with Chromium's omnibox via a patch to
// chrome/browser/ui/views/omnibox/omnibox_view.cc to replace or
// decorate the standard omnibox popup with Astra's version.
// TODO(astra): Wire up keyboard handling from OmniboxView to this
// popup for arrow key navigation and Enter confirmation.
class AstraOmniboxPopupView : public views::BubbleDialogDelegateView,
                              public AstraOmniboxPopupObserver {
 public:
  METADATA_HEADER(AstraOmniboxPopupView);

  explicit AstraOmniboxPopupView(views::View* anchor_view);
  AstraOmniboxPopupView(const AstraOmniboxPopupView&) = delete;
  AstraOmniboxPopupView& operator=(const AstraOmniboxPopupView&) = delete;
  ~AstraOmniboxPopupView() override;

  // -- Model binding --------------------------------------------------------

  void SetModel(AstraOmniboxPopupModel* model);
  AstraOmniboxPopupModel* model() const { return model_; }

  // -- Delegate -------------------------------------------------------------

  void SetDelegate(AstraOmniboxPopupDelegate* delegate) {
    delegate_ = delegate;
  }
  AstraOmniboxPopupDelegate* delegate() const { return delegate_; }

  // -- Popup management -----------------------------------------------------

  // Show the popup.
  void ShowPopup();

  // Hide the popup.
  void HidePopup();

  // Whether the popup is currently showing.
  bool IsPopupShowing() const;

  // -- Selection ------------------------------------------------------------

  // Navigate to the next match.
  void SelectNext();

  // Navigate to the previous match.
  void SelectPrevious();

  // Accept the currently selected match.
  void AcceptSelectedMatch();

  // -- Keyboard -------------------------------------------------------------

  // Handle a key event. Returns true if handled.
  bool HandleKeyEvent(const ui::KeyEvent& event);

  // -- View access (for testing) -------------------------------------------

  views::ScrollView* scroll_view_for_test() { return scroll_view_; }
  size_t GetMatchViewCount() const { return match_views_.size(); }
  AstraOmniboxPopupMatchView* GetMatchViewAt(size_t index);

  // -- views::BubbleDialogDelegateView: -----------------------------------

  std::u16string GetWindowTitle() const override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

  // -- AstraOmniboxPopupObserver: ------------------------------------------

  void OnSuggestionsChanged(AstraOmniboxPopupModel* model) override;
  void OnSelectedMatchChanged(AstraOmniboxPopupModel* model) override;
  void OnPopupVisibilityChanged(AstraOmniboxPopupModel* model,
                                 bool visible) override;
  void OnOmniboxPopupModelShutdown(AstraOmniboxPopupModel* model) override;

 private:
  // Build the popup UI.
  void Build();

  // Rebuild all match views from the model.
  void RebuildMatches();

  // Update match views from model state.
  void UpdateMatches();

  // Update the selected highlight.
  void UpdateSelectionHighlight();

  // Ensure the selected match is visible (scroll into view).
  void EnsureSelectedVisible();

  // Handle a match being clicked.
  void OnMatchClicked(size_t index);

  raw_ptr<AstraOmniboxPopupModel> model_ = nullptr;
  raw_ptr<AstraOmniboxPopupDelegate> delegate_ = nullptr;

  base::ScopedObservation<AstraOmniboxPopupModel,
                          AstraOmniboxPopupObserver>
      model_observation_{this};

  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> content_view_ = nullptr;

  // All match views (owned by view hierarchy).
  std::vector<raw_ptr<AstraOmniboxPopupMatchView>> match_views_;

  // Section header views.
  std::vector<raw_ptr<AstraOmniboxPopupSectionHeader>> section_headers_;

  static constexpr int kPopupWidth = 600;
  static constexpr int kPopupMaxHeight = 400;
  static constexpr int kPopupMinHeight = 0;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_OMNIBOX_POPUP_ASTRA_OMNIBOX_POPUP_VIEW_H_
