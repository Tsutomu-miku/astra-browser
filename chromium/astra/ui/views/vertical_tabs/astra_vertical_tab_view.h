// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_VERTICAL_TABS_ASTRA_VERTICAL_TAB_VIEW_H_
#define ASTRA_UI_VIEWS_VERTICAL_TABS_ASTRA_VERTICAL_TAB_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
}  // namespace views

namespace astra {

// =========================================================================
// AstraVerticalTabView — a single tab in the vertical tab strip
// =========================================================================
//
// Represents one tab in the vertical tab strip.  Shows the tab favicon,
// title, close button, and optional audio/muted indicator.
//
// Layout:
//
//   +-------------------------------------------------+
//   |  [favicon]  Tab title that might be long...  [x] |
//   +-------------------------------------------------+
//
// States: active, inactive, hovered, pinned
//
// Chromium subsystems reused:
//   - TabStripModel (truth source for tab data)
//   - views::Button (base class / hover behavior)
//
// TODO(astra): Wire to TabStripModel via TabStripModelObserver.
//   Chromium owner: chrome/browser/ui/tabs/tab_strip_model.h
//   Patch point: chrome/browser/ui/views/tabs/tab_strip.h
// =========================================================================

class AstraVerticalTabView : public views::Button {
 public:
  // Delegate interface for tab actions.
  class Delegate {
   public:
    virtual void OnTabClicked(const std::string& tab_id) = 0;
    virtual void OnTabClosed(const std::string& tab_id) = 0;
    virtual void OnTabPinnedToggled(const std::string& tab_id) = 0;
   protected:
    ~Delegate() = default;
  };

  AstraVerticalTabView(const std::string& tab_id,
                       const std::u16string& title,
                       const gfx::ImageSkia& favicon,
                       bool is_active,
                       bool is_pinned,
                       bool is_audible,
                       Delegate* delegate);
  ~AstraVerticalTabView() override;

  AstraVerticalTabView(const AstraVerticalTabView&) = delete;
  AstraVerticalTabView& operator=(const AstraVerticalTabView&) = delete;

  // Update tab state.
  void SetTitle(const std::u16string& title);
  void SetFavicon(const gfx::ImageSkia& favicon);
  void SetActive(bool active);
  void SetPinned(bool pinned);
  void SetAudible(bool audible);
  void SetLoading(bool loading);

  // Accessors.
  const std::string& tab_id() const { return tab_id_; }
  bool is_active() const { return is_active_; }
  bool is_pinned() const { return is_pinned_; }
  bool is_audible() const { return is_audible_; }

  // views::View:
  void OnThemeChanged() override;
  gfx::Size CalculatePreferredSize() const override;

  // Testing accessors.
  views::Label* title_label() { return title_label_; }
  views::ImageView* favicon_view() { return favicon_view_; }
  views::ImageButton* close_button() { return close_button_; }

 private:
  void BuildUI();
  void UpdateColors();
  void UpdateVisibility();
  void DrawFaviconFallback(gfx::Canvas* canvas, const gfx::Rect& bounds);

  std::string tab_id_;
  std::u16string title_;
  gfx::ImageSkia favicon_;
  bool is_active_ = false;
  bool is_pinned_ = false;
  bool is_audible_ = false;
  bool is_loading_ = false;

  raw_ptr<Delegate> delegate_ = nullptr;

  raw_ptr<views::ImageView> favicon_view_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_VERTICAL_TABS_ASTRA_VERTICAL_TAB_VIEW_H_
