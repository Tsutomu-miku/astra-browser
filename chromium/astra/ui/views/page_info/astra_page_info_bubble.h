// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PAGE_INFO_ASTRA_PAGE_INFO_BUBBLE_H_
#define ASTRA_UI_VIEWS_PAGE_INFO_ASTRA_PAGE_INFO_BUBBLE_H_

#include <string>

#include "astra/ui/views/page_info/astra_page_info_model.h"

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Button;
class Combobox;
class ImageView;
class Label;
class MdTextButton;
class Separator;
}  // namespace views

namespace astra {

// =========================================================================
// AstraPageInfoBubble — page info bubble
// =========================================================================
//
// A bubble-style dialog that appears when clicking the lock/security icon
// in the address bar. Shows site security status, permissions, cookies,
// and connection/certificate details.
//
// Layout:
//
//   +-------------------------------------------+
//   |  [lock]  example.com                      |
//   |          Connection is secure             |
//   |                                           |
//   |  --- Site information -------------------  |
//   |  [cookie icon] Cookies                   |
//   |              12 in use                    |
//   |  [settings icon] Site settings            |
//   |                                           |
//   |  --- Permissions ------------------------  |
//   |  [camera icon] Camera      [Allow v]      |
//   |  [mic icon] Microphone     [Ask v]        |
//   |  ...                                       |
//   |                                           |
//   |  --- Security ---------------------------  |
//   |  [cert icon] Certificate                  |
//   |              Valid - Let's Encrypt        |
//   |  [shield icon] Connection                 |
//   |              TLS 1.3, AES_128_GCM         |
//   |                                           |
//   +-------------------------------------------+
//
// Chromium subsystems reused:
//   - PageInfo / PageInfoDelegate (truth source)
//   - views::BubbleDialogDelegateView (bubble pattern)
//
// TODO(astra): Wire to Chrome's page info bubble system.
//   Reference: chrome/browser/ui/views/page_info/page_info_bubble_view.h
//   Patch point: chrome/browser/page_info/page_info.cc
// =========================================================================
class AstraPageInfoBubble : public views::BubbleDialogDelegateView,
                            public AstraPageInfoModelObserver {
 public:
  // Construct a page info bubble anchored to a view.
  AstraPageInfoBubble(views::View* anchor_view,
                      AstraPageInfoModel* model);
  ~AstraPageInfoBubble() override;

  AstraPageInfoBubble(const AstraPageInfoBubble&) = delete;
  AstraPageInfoBubble& operator=(const AstraPageInfoBubble&) = delete;

  // Set the model.
  void SetModel(AstraPageInfoModel* model);
  AstraPageInfoModel* model() { return model_; }

  // Refresh the view from the model.
  void RefreshFromModel();

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

  // -- AstraPageInfoModelObserver -------------------------------------------

  void OnPermissionChanged(AstraPageInfoModel* model,
                           AstraPagePermissionType type) override;
  void OnCookiesChanged(AstraPageInfoModel* model) override;
  void OnSecurityStatusChanged(AstraPageInfoModel* model) override;
  void OnPageInfoModelShutdown(AstraPageInfoModel* model) override;

  // Accessors for testing.
  views::ImageView* security_icon() { return security_icon_; }
  views::Label* site_label() { return site_label_; }
  views::Label* connection_label() { return connection_label_; }
  views::View* site_info_section() { return site_info_section_; }
  views::View* permissions_section() { return permissions_section_; }
  views::View* security_section() { return security_section_; }
  views::MdTextButton* cookies_button() { return cookies_button_; }
  views::MdTextButton* site_settings_button() { return site_settings_button_; }
  views::View* permission_rows_container() { return permission_rows_container_; }
  views::Combobox* GetPermissionCombobox(AstraPagePermissionType type);

 private:
  void BuildUI();
  void BuildHeader();
  void BuildSiteInfoSection();
  void BuildPermissionsSection();
  void BuildSecuritySection();

  // Refresh a specific permission row.
  void RefreshPermissionRow(AstraPagePermissionType type);

  // Refresh the security icon and label.
  void RefreshSecurityDisplay();

  // Refresh the cookies section.
  void RefreshCookiesDisplay();

  // Draw a security icon (lock/warning/info).
  void DrawSecurityIcon(gfx::Canvas* canvas,
                        const gfx::Rect& bounds,
                        AstraSecurityStatus status,
                        SkColor color);

  // Draw a permission icon by type.
  void DrawPermissionIcon(gfx::Canvas* canvas,
                          const gfx::Rect& bounds,
                          AstraPagePermissionType type,
                          SkColor color);

  // Draw a generic icon by name.
  void DrawIconByName(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      const std::string& name,
                      SkColor color);

  // Permission combobox changed handler.
  void OnPermissionChanged(AstraPagePermissionType type);

  // Button handlers.
  void OnCookiesClicked();
  void OnSiteSettingsClicked();
  void OnCertificateClicked();
  void OnConnectionDetailsClicked();

  // Model (not owned).
  raw_ptr<AstraPageInfoModel> model_ = nullptr;

  // Scoped observation.
  base::ScopedObservation<AstraPageInfoModel, AstraPageInfoModelObserver>
      scoped_observation_{this};

  // Header.
  raw_ptr<views::ImageView> security_icon_ = nullptr;
  raw_ptr<views::Label> site_label_ = nullptr;
  raw_ptr<views::Label> connection_label_ = nullptr;

  // Site info section.
  raw_ptr<views::View> site_info_section_ = nullptr;
  raw_ptr<views::MdTextButton> cookies_button_ = nullptr;
  raw_ptr<views::MdTextButton> site_settings_button_ = nullptr;

  // Permissions section.
  raw_ptr<views::View> permissions_section_ = nullptr;
  raw_ptr<views::View> permission_rows_container_ = nullptr;

  // Security section.
  raw_ptr<views::View> security_section_ = nullptr;

  // Map of permission type to combobox index.
  std::vector<std::pair<AstraPagePermissionType, raw_ptr<views::Combobox>>>
      permission_comboboxes_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PAGE_INFO_ASTRA_PAGE_INFO_BUBBLE_H_
