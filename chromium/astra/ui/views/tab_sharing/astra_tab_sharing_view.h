// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_SHARING_ASTRA_TAB_SHARING_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_SHARING_ASTRA_TAB_SHARING_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Checkbox;
class Label;
class MdTextButton;
class Textfield;
}  // namespace views

namespace astra {

// =========================================================================
// AstraShareMethodItemView — single share method option
// =========================================================================
//
// A row showing one share method: icon, name, and description.
//
// Layout:
//   +-------------------------------------------+
//   |  📧 Email     Send tab via email            |
//   +-------------------------------------------+
// =========================================================================

class AstraShareMethodItemView : public views::View {
 public:
  using SelectCallback =
      base::RepeatingCallback<void(const std::string& method_id)>;

  enum class ShareMethod {
    kEmail,
    kSms,
    kCopyLink,
    kTwitter,
    kFacebook,
    kLinkedIn,
    kWhatsApp,
    kTelegram,
    kNotes,
    kWorkspace,
  };

  struct MethodInfo {
    std::string method_id;
    std::u16string name;
    std::u16string description;
    ShareMethod type = ShareMethod::kCopyLink;
  };

  AstraShareMethodItemView(const MethodInfo& info,
                           SelectCallback select_callback);
  ~AstraShareMethodItemView() override;

  AstraShareMethodItemView(const AstraShareMethodItemView&) = delete;
  AstraShareMethodItemView& operator=(
      const AstraShareMethodItemView&) = delete;

  const std::string& method_id() const { return method_id_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnClicked();

  static std::u16 MethodIcon(ShareMethod type);

  std::string method_id_;
  std::u16string name_;
  std::u16string description_;
  ShareMethod type_ = ShareMethod::kCopyLink;

  SelectCallback select_callback_;

  raw_ptr<views::Label> icon_label_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> desc_label_ = nullptr;
};

// =========================================================================
// AstraTabSharingView — tab sharing panel
// =========================================================================
//
// A bubble showing options to share the current tab: copy link,
// email, social media, messaging apps, save to workspace, etc.
//
// Layout:
//   +-------------------------------------------+
//   |  Share Tab                    [Close]    |
//   +-------------------------------------------+
//   |  📄 The Future of AI                        |
//   |     techreview.com                        |
//   +-------------------------------------------+
//   |  [ Copy Link ]                             |
//   +-------------------------------------------+
//   |  Share via                                 |
//   |  📧 Email      Send tab via email          |
//   |  💬 SMS        Send tab via text           |
//   |  🐦 Twitter    Post to Twitter             |
//   |  📘 Facebook   Share to Facebook           |
//   |  💼 LinkedIn   Share to LinkedIn           |
//   |  ...                                      |
//   +-------------------------------------------+
//   |  Save to                                   |
//   |  📝 Notes      Save as note                |
//   |  📁 Workspace  Add to workspace            |
//   +-------------------------------------------+
//
// This is a presentation-only view. Sharing integrations are handled by
// Astra's share service and system share sheet.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - content::WebContents (source of tab URL)
// =========================================================================

class AstraTabSharingView : public views::BubbleDialogDelegateView {
 public:
  using ShareMethodCallback =
      base::RepeatingCallback<void(const std::string& method_id)>;
  using CopyLinkCallback = base::RepeatingClosure;

  struct TabInfo {
    std::string tab_id;
    std::u16string title;
    std::string url;
    std::string domain;
  };

  explicit AstraTabSharingView(views::View* anchor_view);
  ~AstraTabSharingView() override;

  AstraTabSharingView(const AstraTabSharingView&) = delete;
  AstraTabSharingView& operator=(const AstraTabSharingView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetTabInfo(const TabInfo& info);
  void SetShareMethods(
      const std::vector<AstraShareMethodItemView::MethodInfo>& methods);

  // -- Callbacks -----------------------------------------------------------

  void SetShareMethodCallback(ShareMethodCallback callback);
  void SetCopyLinkCallback(CopyLinkCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildTabInfoSection();
  void BuildCopyLinkSection();
  void BuildShareMethodsSection();

  void RefreshTabInfo();
  void RefreshShareMethods();

  void OnCopyLink();
  void OnShareMethodSelected(const std::string& method_id);

  TabInfo tab_info_;
  std::vector<AstraShareMethodItemView::MethodInfo> share_methods_;

  ShareMethodCallback share_method_callback_;
  CopyLinkCallback copy_link_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> url_label_ = nullptr;
  raw_ptr<views::MdTextButton> copy_button_ = nullptr;
  raw_ptr<views::View> share_methods_list_ = nullptr;

  std::vector<raw_ptr<AstraShareMethodItemView>> method_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_SHARING_ASTRA_TAB_SHARING_VIEW_H_
