// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PERMISSION_PROMPT_ASTRA_PERMISSION_PROMPT_VIEW_H_
#define ASTRA_UI_VIEWS_PERMISSION_PROMPT_ASTRA_PERMISSION_PROMPT_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Button;
class Checkbox;
class Combobox;
class ImageButton;
class ImageView;
class Label;
class MdTextButton;
class ToggleButton;
}  // namespace views

namespace astra {

class AstraPermissionPromptModel;
class AstraPermissionRequest;
enum class AstraPermissionAction;

// =========================================================================
// AstraPermissionPromptView — permission request bubble
// =========================================================================
//
// A bubble-style dialog that appears when a website requests a permission
// (camera, microphone, location, notifications, etc.).
//
// Layout:
//
//   +-------------------------------------------+
//   |  [icon]  "example.com wants to use camera"  |
//   |          description text                   |
//   |          [device selector dropdown]        |
//   |          [ ] Remember this decision        |
//   |                                           |
//   |   [ Block ]  [ Allow ]  [ Allow once ]     |
//   +-------------------------------------------+
//
// Chromium subsystems reused:
//   - PermissionManager / PermissionRequestManager (truth source)
//   - views::BubbleDialogDelegateView (bubble pattern)
//
// TODO(astra): Wire to Chrome's permission prompt system.
//   Reference: chrome/browser/ui/views/permissions/permission_prompt_bubble_view.h
//   Patch point: chrome/browser/permissions/permission_request_manager.cc
// =========================================================================
class AstraPermissionPromptView
    : public views::BubbleDialogDelegateView,
      public AstraPermissionPromptObserver {
 public:
  // Construct a permission prompt anchored to a view.
  AstraPermissionPromptView(views::View* anchor_view,
                            AstraPermissionPromptModel* model);
  ~AstraPermissionPromptView() override;

  AstraPermissionPromptView(const AstraPermissionPromptView&) = delete;
  AstraPermissionPromptView& operator=(const AstraPermissionPromptView&) =
      delete;

  // Set the model.
  void SetModel(AstraPermissionPromptModel* model);
  AstraPermissionPromptModel* model() { return model_; }

  // Refresh the view from the model.
  void RefreshFromModel();

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

  // -- AstraPermissionPromptObserver ----------------------------------------

  void OnPermissionRequested(AstraPermissionPromptModel* model,
                             const std::string& request_id) override;
  void OnPermissionDecided(AstraPermissionPromptModel* model,
                           const std::string& request_id,
                           AstraPermissionAction action) override;
  void OnPermissionDismissed(AstraPermissionPromptModel* model,
                             const std::string& request_id) override;
  void OnActiveRequestChanged(AstraPermissionPromptModel* model,
                              const std::string& request_id) override;
  void OnPermissionPromptModelShutdown(
      AstraPermissionPromptModel* model) override;

  // Accessors for testing.
  views::ImageView* icon_view() { return icon_view_; }
  views::Label* title_label() { return title_label_; }
  views::Label* message_label() { return message_label_; }
  views::Label* origin_label() { return origin_label_; }
  views::MdTextButton* block_button() { return block_button_; }
  views::MdTextButton* allow_button() { return allow_button_; }
  views::MdTextButton* allow_once_button() { return allow_once_button_; }
  views::Checkbox* remember_checkbox() { return remember_checkbox_; }
  views::Combobox* device_combobox() { return device_combobox_; }
  views::ImageButton* close_button() { return close_button_; }
  views::Label* counter_label() { return counter_label_; }
  views::ImageButton* prev_button() { return prev_button_; }
  views::ImageButton* next_button() { return next_button_; }

 private:
  void BuildUI();
  void BuildHeader();
  void BuildContent();
  void BuildFooter();

  // Update content for the active request.
  void UpdateActiveRequest();

  // Button handlers.
  void OnBlockClicked();
  void OnAllowClicked();
  void OnAllowOnceClicked();
  void OnCloseClicked();
  void OnRememberToggled();
  void OnDeviceChanged();
  void OnPrevClicked();
  void OnNextClicked();

  // Draw permission icon.
  void DrawPermissionIcon(gfx::Canvas* canvas,
                          const gfx::Rect& bounds,
                          AstraPermissionType type);

  // Model (not owned).
  raw_ptr<AstraPermissionPromptModel> model_ = nullptr;

  // Scoped observation.
  base::ScopedObservation<AstraPermissionPromptModel,
                          AstraPermissionPromptObserver>
      scoped_observation_{this};

  // Header.
  raw_ptr<views::ImageView> icon_view_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> origin_label_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;

  // Content.
  raw_ptr<views::View> content_view_ = nullptr;
  raw_ptr<views::Label> message_label_ = nullptr;
  raw_ptr<views::Combobox> device_combobox_ = nullptr;
  raw_ptr<views::Checkbox> remember_checkbox_ = nullptr;

  // Footer / buttons.
  raw_ptr<views::View> button_row_ = nullptr;
  raw_ptr<views::MdTextButton> block_button_ = nullptr;
  raw_ptr<views::MdTextButton> allow_once_button_ = nullptr;
  raw_ptr<views::MdTextButton> allow_button_ = nullptr;

  // Multi-request navigation.
  raw_ptr<views::View> nav_row_ = nullptr;
  raw_ptr<views::ImageButton> prev_button_ = nullptr;
  raw_ptr<views::ImageButton> next_button_ = nullptr;
  raw_ptr<views::Label> counter_label_ = nullptr;

  // Whether to show the "Allow once" button.
  bool show_allow_once_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PERMISSION_PROMPT_ASTRA_PERMISSION_PROMPT_VIEW_H_
