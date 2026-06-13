// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_sharing/astra_tab_sharing_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabSharingViewTest
// ===========================================================================

class AstraTabSharingViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test share method item creation.
TEST_F(AstraTabSharingViewTest, ShareMethodItemCreation) {
  AstraShareMethodItemView::MethodInfo info;
  info.method_id = "email";
  info.name = u"Email";
  info.description = u"Send tab via email";
  info.type = AstraShareMethodItemView::ShareMethod::kEmail;

  auto item = std::make_unique<AstraShareMethodItemView>(
      info, base::DoNothing());

  EXPECT_EQ("email", item->method_id());
}

// Test copy link method.
TEST_F(AstraTabSharingViewTest, CopyLinkMethod) {
  AstraShareMethodItemView::MethodInfo info;
  info.method_id = "copy_link";
  info.name = u"Copy Link";
  info.description = u"Copy URL to clipboard";
  info.type = AstraShareMethodItemView::ShareMethod::kCopyLink;

  auto item = std::make_unique<AstraShareMethodItemView>(
      info, base::DoNothing());

  EXPECT_EQ("copy_link", item->method_id());
}

// Test all share method types.
TEST_F(AstraTabSharingViewTest, AllShareMethodTypes) {
  using SM = AstraShareMethodItemView::ShareMethod;
  std::vector<SM> types = {
      SM::kEmail,
      SM::kSms,
      SM::kCopyLink,
      SM::kTwitter,
      SM::kFacebook,
      SM::kLinkedIn,
      SM::kWhatsApp,
      SM::kTelegram,
      SM::kNotes,
      SM::kWorkspace,
  };

  for (size_t i = 0; i < types.size(); i++) {
    AstraShareMethodItemView::MethodInfo info;
    info.method_id = "method_" + std::to_string(i);
    info.name = base::UTF8ToUTF16("Method " + std::to_string(i));
    info.description = u"Description";
    info.type = types[i];

    auto item = std::make_unique<AstraShareMethodItemView>(
        info, base::DoNothing());
    EXPECT_EQ(info.method_id, item->method_id());
  }
}

// Test tab sharing view creation.
TEST_F(AstraTabSharingViewTest, ViewCreation) {
  auto* view = new AstraTabSharingView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test window title.
TEST_F(AstraTabSharingViewTest, WindowTitle) {
  auto* view = new AstraTabSharingView(anchor_view_.get());
  EXPECT_EQ(u"Share", view->GetWindowTitle());
}

// Test setting tab info.
TEST_F(AstraTabSharingViewTest, SetTabInfo) {
  auto* view = new AstraTabSharingView(anchor_view_.get());

  AstraTabSharingView::TabInfo info;
  info.tab_id = "tab_001";
  info.title = u"The Future of AI";
  info.url = "https://techreview.com/article";
  info.domain = "techreview.com";

  view->SetTabInfo(info);
  EXPECT_NE(nullptr, view);
}

// Test empty tab info.
TEST_F(AstraTabSharingViewTest, EmptyTabInfo) {
  auto* view = new AstraTabSharingView(anchor_view_.get());

  AstraTabSharingView::TabInfo info;
  info.tab_id = "";
  info.title = u"";
  info.url = "";
  info.domain = "";

  view->SetTabInfo(info);
  EXPECT_NE(nullptr, view);
}

// Test setting share methods.
TEST_F(AstraTabSharingViewTest, SetShareMethods) {
  auto* view = new AstraTabSharingView(anchor_view_.get());

  std::vector<AstraShareMethodItemView::MethodInfo> methods;

  AstraShareMethodItemView::MethodInfo m1;
  m1.method_id = "email";
  m1.name = u"Email";
  m1.description = u"Send via email";
  m1.type = AstraShareMethodItemView::ShareMethod::kEmail;
  methods.push_back(m1);

  AstraShareMethodItemView::MethodInfo m2;
  m2.method_id = "copy_link";
  m2.name = u"Copy Link";
  m2.description = u"Copy to clipboard";
  m2.type = AstraShareMethodItemView::ShareMethod::kCopyLink;
  methods.push_back(m2);

  AstraShareMethodItemView::MethodInfo m3;
  m3.method_id = "twitter";
  m3.name = u"Twitter";
  m3.description = u"Post to Twitter";
  m3.type = AstraShareMethodItemView::ShareMethod::kTwitter;
  methods.push_back(m3);

  AstraShareMethodItemView::MethodInfo m4;
  m4.method_id = "facebook";
  m4.name = u"Facebook";
  m4.description = u"Share to Facebook";
  m4.type = AstraShareMethodItemView::ShareMethod::kFacebook;
  methods.push_back(m4);

  AstraShareMethodItemView::MethodInfo m5;
  m5.method_id = "linkedin";
  m5.name = u"LinkedIn";
  m5.description = u"Share to LinkedIn";
  m5.type = AstraShareMethodItemView::ShareMethod::kLinkedIn;
  methods.push_back(m5);

  view->SetShareMethods(methods);
  SUCCEED();
}

// Test empty share methods.
TEST_F(AstraTabSharingViewTest, EmptyShareMethods) {
  auto* view = new AstraTabSharingView(anchor_view_.get());
  view->SetShareMethods({});
  EXPECT_NE(nullptr, view);
}

// Test share method callback.
TEST_F(AstraTabSharingViewTest, ShareMethodCallback) {
  std::string selected_method;
  auto* view = new AstraTabSharingView(anchor_view_.get());
  view->SetShareMethodCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &selected_method));

  std::vector<AstraShareMethodItemView::MethodInfo> methods;
  AstraShareMethodItemView::MethodInfo m;
  m.method_id = "click_me";
  m.name = u"Click Me";
  m.description = u"Click this method";
  m.type = AstraShareMethodItemView::ShareMethod::kCopyLink;
  methods.push_back(m);
  view->SetShareMethods(methods);

  EXPECT_TRUE(selected_method.empty());
}

// Test copy link callback.
TEST_F(AstraTabSharingViewTest, CopyLinkCallback) {
  bool callback_called = false;
  auto* view = new AstraTabSharingView(anchor_view_.get());
  view->SetCopyLinkCallback(
      base::BindRepeating(
          [](bool* called) { *called = true; }, &callback_called));

  AstraTabSharingView::TabInfo info;
  info.tab_id = "tab1";
  info.title = u"Test Tab";
  info.url = "https://test.com";
  info.domain = "test.com";
  view->SetTabInfo(info);

  EXPECT_FALSE(callback_called);
}

// Test full set of sharing methods.
TEST_F(AstraTabSharingViewTest, FullShareMethods) {
  auto* view = new AstraTabSharingView(anchor_view_.get());

  std::vector<AstraShareMethodItemView::MethodInfo> methods;

  // Social media.
  std::vector<std::pair<std::string, AstraShareMethodItemView::ShareMethod>> social = {
      {"twitter", AstraShareMethodItemView::ShareMethod::kTwitter},
      {"facebook", AstraShareMethodItemView::ShareMethod::kFacebook},
      {"linkedin", AstraShareMethodItemView::ShareMethod::kLinkedIn},
  };
  for (const auto& s : social) {
    AstraShareMethodItemView::MethodInfo m;
    m.method_id = s.first;
    m.name = base::UTF8ToUTF16(s.first);
    m.description = u"Share to " + base::UTF8ToUTF16(s.first);
    m.type = s.second;
    methods.push_back(m);
  }

  // Messaging.
  std::vector<std::pair<std::string, AstraShareMethodItemView::ShareMethod>> msg = {
      {"sms", AstraShareMethodItemView::ShareMethod::kSms},
      {"whatsapp", AstraShareMethodItemView::ShareMethod::kWhatsApp},
      {"telegram", AstraShareMethodItemView::ShareMethod::kTelegram},
  };
  for (const auto& m : msg) {
    AstraShareMethodItemView::MethodInfo info;
    info.method_id = m.first;
    info.name = base::UTF8ToUTF16(m.first);
    info.description = u"Send via " + base::UTF8ToUTF16(m.first);
    info.type = m.second;
    methods.push_back(info);
  }

  // Save options.
  AstraShareMethodItemView::MethodInfo notes;
  notes.method_id = "notes";
  notes.name = u"Notes";
  notes.description = u"Save as note";
  notes.type = AstraShareMethodItemView::ShareMethod::kNotes;
  methods.push_back(notes);

  AstraShareMethodItemView::MethodInfo workspace;
  workspace.method_id = "workspace";
  workspace.name = u"Workspace";
  workspace.description = u"Add to workspace";
  workspace.type = AstraShareMethodItemView::ShareMethod::kWorkspace;
  methods.push_back(workspace);

  view->SetShareMethods(methods);
  SUCCEED();
}

// Test tab with long title.
TEST_F(AstraTabSharingViewTest, LongTabTitle) {
  auto* view = new AstraTabSharingView(anchor_view_.get());

  AstraTabSharingView::TabInfo info;
  info.tab_id = "long_title";
  info.title =
      u"This is an extremely long article title that will definitely need "
      u"to be truncated to fit within the share panel constraints";
  info.url = "https://example.com/very/long/path/to/article";
  info.domain = "very-long-domain-name.example.com";

  view->SetTabInfo(info);
  SUCCEED();
}

// Test share method with long description.
TEST_F(AstraTabSharingViewTest, LongMethodDescription) {
  AstraShareMethodItemView::MethodInfo info;
  info.method_id = "long_desc";
  info.name = u"Method";
  info.description =
      u"This is a very long description for a share method that will "
      u"be ellipsized in the display";
  info.type = AstraShareMethodItemView::ShareMethod::kEmail;

  auto item = std::make_unique<AstraShareMethodItemView>(
      info, base::DoNothing());

  EXPECT_EQ("long_desc", item->method_id());
}

}  // namespace astra
