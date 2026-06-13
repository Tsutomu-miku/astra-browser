// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_INFO_BAR_ASTRA_INFO_BAR_VIEW_H_
#define ASTRA_UI_VIEWS_INFO_BAR_ASTRA_INFO_BAR_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

namespace astra {

// Types of info bars.
enum class AstraInfoBarType {
  kInformation,   // General info (blue)
  kWarning,       // Warning (yellow/orange)
  kError,         // Error/dangerous (red)
  kSuccess,       // Success (green)
  kPermission,    // Permission request
  kExtension,     // Extension install/update
  kPassword,      // Password save/update
  kAutofill,      // Autofill suggestion
};

// An individual action button on the info bar.
struct AstraInfoBarAction {
  int action_id;
  std::u16string label;
  bool is_primary = false;
  bool is_link = false;  // Render as text link instead of button
};

// Delegate for AstraInfoBarView.
class AstraInfoBarDelegate {
 public:
  virtual ~AstraInfoBarDelegate() = default;

  // Called when an action button is clicked.
  virtual void OnInfoBarAction(int action_id) = 0;

  // Called when the close button is clicked (dismiss the bar).
  virtual void OnInfoBarDismissed() = 0;

  // Called when the user clicks the main message link.
  virtual void OnInfoBarLinkClicked() {}
};

// A single info bar view.
//
// Info bars appear at the top of the content area to show notifications,
// permission requests, and other contextual information. They can be
// stacked vertically.
//
// Layout:
//   [icon] [message text] [action buttons] [close]
//
// Chromium owner: InfoBar / InfoBarView
//   (components/infobars/core/infobar.h)
//   (chrome/browser/ui/views/infobars/infobar_view.h)
//
// TODO(astra): Integrate with Chromium's InfoBarManager via a patch to
// chrome/browser/ui/views/infobars/infobar_container_view.cc to replace
// or augment the standard info bars with Astra-styled ones.
class AstraInfoBarView : public views::View {
 public:
  METADATA_HEADER(AstraInfoBarView);

  explicit AstraInfoBarView(AstraInfoBarType type = AstraInfoBarType::kInformation);
  AstraInfoBarView(const AstraInfoBarView&) = delete;
  AstraInfoBarView& operator=(const AstraInfoBarView&) = delete;
  ~AstraInfoBarView() override;

  // -- Type ---------------------------------------------------------------

  void SetType(AstraInfoBarType type);
  AstraInfoBarType GetType() const { return type_; }

  // -- Icon ---------------------------------------------------------------

  void SetIcon(const gfx::ImageSkia& icon);
  void SetIconVisible(bool visible);
  bool IsIconVisible() const;

  // -- Message -------------------------------------------------------------

  void SetMessage(const std::u16string& message);
  const std::u16string& GetMessage() const;

  // -- Link ----------------------------------------------------------------

  void SetLinkText(const std::u16string& link_text);
  void SetLinkVisible(bool visible);
  bool IsLinkVisible() const;

  // -- Actions -------------------------------------------------------------

  void SetActions(const std::vector<AstraInfoBarAction>& actions);
  void AddAction(const AstraInfoBarAction& action);
  void ClearActions();
  size_t GetActionCount() const;

  // -- Close button --------------------------------------------------------

  void SetCloseButtonVisible(bool visible);
  bool IsCloseButtonVisible() const;

  // -- Delegate ------------------------------------------------------------

  void SetDelegate(AstraInfoBarDelegate* delegate) { delegate_ = delegate; }
  AstraInfoBarDelegate* delegate() const { return delegate_; }

  // -- Animation -----------------------------------------------------------

  // Set whether the bar is expanded (shown) or collapsed.
  void SetExpanded(bool expanded);
  bool IsExpanded() const { return expanded_; }

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  // Handle action button click.
  void OnActionClicked(int action_id);

  // Handle close button click.
  void OnCloseClicked();

  // Handle link click.
  void OnLinkClicked();

  // Get the accent color based on type.
  SkColor GetAccentColor() const;

  // Update the bar's background and border based on type.
  void UpdateBarAppearance();

  // Update all child views' visibility.
  void UpdateVisibility();

  AstraInfoBarType type_;
  std::u16string message_;
  std::u16string link_text_;
  bool expanded_ = true;
  bool icon_visible_ = true;
  bool link_visible_ = false;
  bool close_visible_ = true;

  raw_ptr<AstraInfoBarDelegate> delegate_ = nullptr;

  // Child views.
  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> message_label_ = nullptr;
  raw_ptr<views::Link> link_view_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
  raw_ptr<views::View> actions_container_ = nullptr;
  std::vector<raw_ptr<views::LabelButton>> action_buttons_;

  static constexpr int kBarHeight = 48;
  static constexpr int kIconSize = 18;
  static constexpr int kCloseButtonSize = 20;
  static constexpr int kPaddingHorizontal = 16;
  static constexpr int kPaddingVertical = 8;
  static constexpr int kIconSpacing = 12;
  static constexpr int kActionSpacing = 8;
  static constexpr int kAccentBarWidth = 4;
};

// Container that stacks multiple info bars vertically.
//
// Manages a list of info bars and handles adding/removing them with
// smooth transitions.
//
// Chromium owner: InfoBarContainerView
//   (chrome/browser/ui/views/infobars/infobar_container_view.h)
class AstraInfoBarContainerView : public views::View {
 public:
  METADATA_HEADER(AstraInfoBarContainerView);

  AstraInfoBarContainerView();
  AstraInfoBarContainerView(const AstraInfoBarContainerView&) = delete;
  AstraInfoBarContainerView& operator=(const AstraInfoBarContainerView&) = delete;
  ~AstraInfoBarContainerView() override;

  // Add an info bar to the top of the stack.
  AstraInfoBarView* AddInfoBar(
      AstraInfoBarType type,
      const std::u16string& message,
      const std::vector<AstraInfoBarAction>& actions = {});

  // Add an existing info bar view.
  void AddInfoBar(std::unique_ptr<AstraInfoBarView> info_bar);

  // Remove a specific info bar.
  void RemoveInfoBar(AstraInfoBarView* info_bar);

  // Remove all info bars.
  void RemoveAllInfoBars();

  // Get the number of info bars.
  size_t GetInfoBarCount() const;

  // Get an info bar by index.
  AstraInfoBarView* GetInfoBarAt(size_t index) const;

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;

 private:
  std::vector<raw_ptr<AstraInfoBarView>> info_bars_;

  static constexpr int kBarSpacing = 1;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_INFO_BAR_ASTRA_INFO_BAR_VIEW_H_
