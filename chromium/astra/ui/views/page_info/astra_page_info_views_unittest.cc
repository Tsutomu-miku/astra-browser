// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/page_info/astra_page_info_bubble.h"
#include "astra/ui/views/page_info/astra_page_info_model.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraPageInfoModelTest
// ===========================================================================

class AstraPageInfoModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraPageInfoModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraPageInfoModel> model_;
};

// Test model creation.
TEST_F(AstraPageInfoModelTest, Creation) {
  EXPECT_TRUE(model_);
  EXPECT_FALSE(model_->GetSiteInfo().display_name.empty());
  EXPECT_FALSE(model_->GetPermissions().empty());
}

// Test site info.
TEST_F(AstraPageInfoModelTest, SiteInfo) {
  const auto& site_info = model_->GetSiteInfo();
  EXPECT_FALSE(site_info.origin.empty());
  EXPECT_FALSE(site_info.display_name.empty());
  EXPECT_EQ(AstraSecurityStatus::kSecure, site_info.security_status);
  EXPECT_FALSE(site_info.security_summary.empty());
}

// Test security status color.
TEST_F(AstraPageInfoModelTest, SecurityStatusColor) {
  EXPECT_NE(SK_ColorTRANSPARENT,
            AstraPageInfoModel::GetSecurityStatusColor(
                AstraSecurityStatus::kSecure));
  EXPECT_NE(SK_ColorTRANSPARENT,
            AstraPageInfoModel::GetSecurityStatusColor(
                AstraSecurityStatus::kInsecure));
  EXPECT_NE(SK_ColorTRANSPARENT,
            AstraPageInfoModel::GetSecurityStatusColor(
                AstraSecurityStatus::kWarning));
  EXPECT_NE(SK_ColorTRANSPARENT,
            AstraPageInfoModel::GetSecurityStatusColor(
                AstraSecurityStatus::kNeutral));
  EXPECT_NE(SK_ColorTRANSPARENT,
            AstraPageInfoModel::GetSecurityStatusColor(
                AstraSecurityStatus::kUnknown));
}

// Test security status label.
TEST_F(AstraPageInfoModelTest, SecurityStatusLabel) {
  EXPECT_FALSE(AstraPageInfoModel::GetSecurityStatusLabel(
      AstraSecurityStatus::kSecure).empty());
  EXPECT_FALSE(AstraPageInfoModel::GetSecurityStatusLabel(
      AstraSecurityStatus::kInsecure).empty());
  EXPECT_FALSE(AstraPageInfoModel::GetSecurityStatusLabel(
      AstraSecurityStatus::kWarning).empty());
}

// Test security icon name.
TEST_F(AstraPageInfoModelTest, SecurityIconName) {
  EXPECT_EQ("lock", AstraPageInfoModel::GetSecurityIconName(
                        AstraSecurityStatus::kSecure));
  EXPECT_EQ("warning", AstraPageInfoModel::GetSecurityIconName(
                           AstraSecurityStatus::kInsecure));
  EXPECT_EQ("warning", AstraPageInfoModel::GetSecurityIconName(
                           AstraSecurityStatus::kWarning));
  EXPECT_EQ("info", AstraPageInfoModel::GetSecurityIconName(
                        AstraSecurityStatus::kNeutral));
}

// Test permissions list.
TEST_F(AstraPageInfoModelTest, PermissionsList) {
  const auto& permissions = model_->GetPermissions();
  EXPECT_GT(permissions.size(), 0u);

  // Check that we have the expected permission types.
  bool has_camera = false;
  bool has_microphone = false;
  bool has_geolocation = false;
  bool has_notifications = false;

  for (const auto& entry : permissions) {
    EXPECT_FALSE(entry.name.empty());
    switch (entry.type) {
      case AstraPagePermissionType::kCamera:
        has_camera = true;
        break;
      case AstraPagePermissionType::kMicrophone:
        has_microphone = true;
        break;
      case AstraPagePermissionType::kGeolocation:
        has_geolocation = true;
        break;
      case AstraPagePermissionType::kNotifications:
        has_notifications = true;
        break;
      default:
        break;
    }
  }

  EXPECT_TRUE(has_camera);
  EXPECT_TRUE(has_microphone);
  EXPECT_TRUE(has_geolocation);
  EXPECT_TRUE(has_notifications);
}

// Test permission names.
TEST_F(AstraPageInfoModelTest, PermissionNames) {
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionName(
      AstraPagePermissionType::kCamera).empty());
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionName(
      AstraPagePermissionType::kMicrophone).empty());
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionName(
      AstraPagePermissionType::kGeolocation).empty());
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionName(
      AstraPagePermissionType::kNotifications).empty());
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionName(
      AstraPagePermissionType::kSound).empty());
}

// Test permission icon names.
TEST_F(AstraPageInfoModelTest, PermissionIconNames) {
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionIconName(
      AstraPagePermissionType::kCamera).empty());
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionIconName(
      AstraPagePermissionType::kMicrophone).empty());
}

// Test permission setting labels.
TEST_F(AstraPageInfoModelTest, PermissionSettingLabels) {
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionSettingLabel(
      AstraPermissionSetting::kAllow).empty());
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionSettingLabel(
      AstraPermissionSetting::kBlock).empty());
  EXPECT_FALSE(AstraPageInfoModel::GetPermissionSettingLabel(
      AstraPermissionSetting::kAsk).empty());
}

// Test getting a specific permission.
TEST_F(AstraPageInfoModelTest, GetPermission) {
  const auto* camera =
      model_->GetPermission(AstraPagePermissionType::kCamera);
  ASSERT_NE(nullptr, camera);
  EXPECT_EQ(AstraPagePermissionType::kCamera, camera->type);

  const auto* mic =
      model_->GetPermission(AstraPagePermissionType::kMicrophone);
  ASSERT_NE(nullptr, mic);
  EXPECT_EQ(AstraPagePermissionType::kMicrophone, mic->type);
}

// Test setting a permission.
TEST_F(AstraPageInfoModelTest, SetPermission) {
  model_->SetPermission(AstraPagePermissionType::kCamera,
                        AstraPermissionSetting::kBlock);

  const auto* camera =
      model_->GetPermission(AstraPagePermissionType::kCamera);
  ASSERT_NE(nullptr, camera);
  EXPECT_EQ(AstraPermissionSetting::kBlock, camera->setting);
  EXPECT_FALSE(camera->is_default);
  EXPECT_EQ(AstraPermissionSource::kUser, camera->source);
}

// Test toggling a permission.
TEST_F(AstraPageInfoModelTest, TogglePermission) {
  // Start with Ask (default).
  const auto* camera =
      model_->GetPermission(AstraPagePermissionType::kMicrophone);
  ASSERT_NE(nullptr, camera);
  AstraPermissionSetting initial = camera->setting;

  // Toggle once.
  model_->TogglePermission(AstraPagePermissionType::kMicrophone);
  camera = model_->GetPermission(AstraPagePermissionType::kMicrophone);
  EXPECT_NE(initial, camera->setting);

  // Toggle again.
  AstraPermissionSetting after_first = camera->setting;
  model_->TogglePermission(AstraPagePermissionType::kMicrophone);
  camera = model_->GetPermission(AstraPagePermissionType::kMicrophone);
  EXPECT_NE(after_first, camera->setting);

  // Toggle a third time - should cycle back to original.
  model_->TogglePermission(AstraPagePermissionType::kMicrophone);
  camera = model_->GetPermission(AstraPagePermissionType::kMicrophone);
  EXPECT_EQ(initial, camera->setting);
}

// Test resetting a permission.
TEST_F(AstraPageInfoModelTest, ResetPermission) {
  // First, set a non-default value.
  model_->SetPermission(AstraPagePermissionType::kJavaScript,
                        AstraPermissionSetting::kBlock);

  auto* js = model_->GetPermission(AstraPagePermissionType::kJavaScript);
  ASSERT_NE(nullptr, js);
  EXPECT_FALSE(js->is_default);

  // Reset it.
  model_->ResetPermission(AstraPagePermissionType::kJavaScript);

  js = model_->GetPermission(AstraPagePermissionType::kJavaScript);
  ASSERT_NE(nullptr, js);
  EXPECT_TRUE(js->is_default);
  EXPECT_EQ(AstraPermissionSetting::kAsk, js->setting);
  EXPECT_EQ(AstraPermissionSource::kDefault, js->source);
}

// Test cookie info.
TEST_F(AstraPageInfoModelTest, CookieInfo) {
  const auto& cookies = model_->GetCookies();
  EXPECT_GE(cookies.cookies_in_use, 0);
  EXPECT_GE(cookies.site_data_count, 0);
}

// Test setting cookie info.
TEST_F(AstraPageInfoModelTest, SetCookies) {
  AstraCookieInfo info;
  info.cookies_in_use = 42;
  info.site_data_count = 7;
  info.blocked_cookies = 3;
  info.third_party_cookies_allowed = false;

  model_->SetCookies(info);

  const auto& cookies = model_->GetCookies();
  EXPECT_EQ(42, cookies.cookies_in_use);
  EXPECT_EQ(7, cookies.site_data_count);
  EXPECT_EQ(3, cookies.blocked_cookies);
  EXPECT_FALSE(cookies.third_party_cookies_allowed);
}

// Test connection info.
TEST_F(AstraPageInfoModelTest, ConnectionInfo) {
  const auto& conn = model_->GetConnectionInfo();
  EXPECT_FALSE(conn.protocol_version.empty());
  EXPECT_FALSE(conn.cipher_suite.empty());
  EXPECT_TRUE(conn.certificate_is_valid);
}

// Test setting site info.
TEST_F(AstraPageInfoModelTest, SetSiteInfo) {
  AstraSiteInfo info;
  info.origin = u"https://test.example.com";
  info.display_name = u"test.example.com";
  info.security_status = AstraSecurityStatus::kInsecure;

  model_->SetSiteInfo(info);

  const auto& site = model_->GetSiteInfo();
  EXPECT_EQ(u"test.example.com", site.display_name);
  EXPECT_EQ(u"https://test.example.com", site.origin);
  EXPECT_EQ(AstraSecurityStatus::kInsecure, site.security_status);
}

// Test permission count and types.
TEST_F(AstraPageInfoModelTest, PermissionCount) {
  const auto& perms = model_->GetPermissions();
  // Should have all 10 permission types.
  EXPECT_EQ(10u, perms.size());
}

// ===========================================================================
// AstraPageInfoBubbleTest
// ===========================================================================

class AstraPageInfoBubbleTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraPageInfoModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraPageInfoModel> model_;
};

// Test that the model has the expected number of permissions.
TEST_F(AstraPageInfoBubbleTest, ModelPermissions) {
  const auto& permissions = model_->GetPermissions();
  EXPECT_EQ(10u, permissions.size());
}

// Test security status after setting.
TEST_F(AstraPageInfoBubbleTest, SecurityStatusUpdate) {
  AstraSiteInfo info = model_->GetSiteInfo();
  info.security_status = AstraSecurityStatus::kWarning;
  model_->SetSiteInfo(info);

  EXPECT_EQ(AstraSecurityStatus::kWarning,
            model_->GetSiteInfo().security_status);
}

// Test permission toggle in model.
TEST_F(AstraPageInfoBubbleTest, PermissionToggle) {
  auto initial =
      model_->GetPermission(AstraPagePermissionType::kCamera)->setting;

  model_->TogglePermission(AstraPagePermissionType::kCamera);
  auto after =
      model_->GetPermission(AstraPagePermissionType::kCamera)->setting;

  EXPECT_NE(initial, after);
}

// Test that permissions start with sample data.
TEST_F(AstraPageInfoBubbleTest, SampleData) {
  // Camera should be Allow (non-default in sample data).
  const auto* camera =
      model_->GetPermission(AstraPagePermissionType::kCamera);
  ASSERT_NE(nullptr, camera);
  EXPECT_EQ(AstraPermissionSetting::kAllow, camera->setting);
  EXPECT_FALSE(camera->is_default);

  // Popups should be Block (non-default in sample data).
  const auto* popups =
      model_->GetPermission(AstraPagePermissionType::kPopups);
  ASSERT_NE(nullptr, popups);
  EXPECT_EQ(AstraPermissionSetting::kBlock, popups->setting);
  EXPECT_FALSE(popups->is_default);
}

// Test observer pattern - model notifies on permission change.
class TestPageInfoObserver : public AstraPageInfoModelObserver {
 public:
  void OnPermissionChanged(AstraPageInfoModel* model,
                           AstraPagePermissionType type) override {
    permission_changed_count_++;
    last_changed_type_ = type;
  }

  void OnCookiesChanged(AstraPageInfoModel* model) override {
    cookies_changed_count_++;
  }

  void OnSecurityStatusChanged(AstraPageInfoModel* model) override {
    security_changed_count_++;
  }

  void OnPageInfoModelShutdown(AstraPageInfoModel* model) override {
    model_shutdown_ = true;
  }

  int permission_changed_count_ = 0;
  AstraPagePermissionType last_changed_type_ =
      AstraPagePermissionType::kCamera;
  int cookies_changed_count_ = 0;
  int security_changed_count_ = 0;
  bool model_shutdown_ = false;
};

TEST_F(AstraPageInfoBubbleTest, ObserverNotifications) {
  TestPageInfoObserver observer;
  model_->AddObserver(&observer);

  // Permission change notification.
  EXPECT_EQ(0, observer.permission_changed_count_);
  model_->SetPermission(AstraPagePermissionType::kMicrophone,
                        AstraPermissionSetting::kBlock);
  EXPECT_EQ(1, observer.permission_changed_count_);
  EXPECT_EQ(AstraPagePermissionType::kMicrophone,
            observer.last_changed_type_);

  // Cookies change notification.
  EXPECT_EQ(0, observer.cookies_changed_count_);
  AstraCookieInfo cookies;
  cookies.cookies_in_use = 100;
  model_->SetCookies(cookies);
  EXPECT_EQ(1, observer.cookies_changed_count_);

  // Security status change notification.
  EXPECT_EQ(0, observer.security_changed_count_);
  AstraSiteInfo site = model_->GetSiteInfo();
  site.security_status = AstraSecurityStatus::kInsecure;
  model_->SetSiteInfo(site);
  EXPECT_EQ(1, observer.security_changed_count_);

  // Model shutdown notification.
  EXPECT_FALSE(observer.model_shutdown_);
  model_.reset();
  EXPECT_TRUE(observer.model_shutdown_);
}

// Test multiple observers.
TEST_F(AstraPageInfoBubbleTest, MultipleObservers) {
  TestPageInfoObserver observer1;
  TestPageInfoObserver observer2;
  model_->AddObserver(&observer1);
  model_->AddObserver(&observer2);

  model_->SetPermission(AstraPagePermissionType::kSound,
                        AstraPermissionSetting::kBlock);

  EXPECT_EQ(1, observer1.permission_changed_count_);
  EXPECT_EQ(1, observer2.permission_changed_count_);

  model_->RemoveObserver(&observer1);

  model_->SetPermission(AstraPagePermissionType::kFullscreen,
                        AstraPermissionSetting::kAllow);

  EXPECT_EQ(1, observer1.permission_changed_count_);  // Unchanged
  EXPECT_EQ(2, observer2.permission_changed_count_);  // Incremented
}

}  // namespace astra
