// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/permission_prompt/astra_permission_prompt_model.h"
#include "astra/ui/views/permission_prompt/astra_permission_prompt_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraPermissionPromptModelTest
// ===========================================================================

class AstraPermissionPromptModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraPermissionPromptModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraPermissionPromptModel> model_;
};

// Test model creation.
TEST_F(AstraPermissionPromptModelTest, Creation) {
  EXPECT_EQ(0u, model_->GetRequestCount());
  EXPECT_TRUE(model_->GetAllRequests().empty());
  EXPECT_EQ(nullptr, model_->GetActiveRequest());
  EXPECT_TRUE(model_->GetActiveRequestId().empty());
  EXPECT_FALSE(model_->IsPromptVisible());
  EXPECT_FALSE(model_->IsLoading());
  EXPECT_TRUE(model_->GetRememberDecision());
}

// Test adding a request.
TEST_F(AstraPermissionPromptModelTest, AddRequest) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kCamera;
  req.title = u"Use your camera";
  req.message = u"example.com wants to use your camera";
  req.origin = u"https://example.com";

  std::string id = model_->AddRequest(req);
  EXPECT_FALSE(id.empty());
  EXPECT_EQ(1u, model_->GetRequestCount());
  EXPECT_TRUE(model_->IsPromptVisible());

  const auto* active = model_->GetActiveRequest();
  ASSERT_NE(nullptr, active);
  EXPECT_EQ(id, active->id);
  EXPECT_EQ(AstraPermissionType::kCamera, active->type);
  EXPECT_EQ(u"Use your camera", active->title);
}

// Test adding multiple requests.
TEST_F(AstraPermissionPromptModelTest, AddMultipleRequests) {
  AstraPermissionRequest req1;
  req1.type = AstraPermissionType::kCamera;
  std::string id1 = model_->AddRequest(req1);

  AstraPermissionRequest req2;
  req2.type = AstraPermissionType::kMicrophone;
  std::string id2 = model_->AddRequest(req2);

  EXPECT_EQ(2u, model_->GetRequestCount());
  EXPECT_NE(id1, id2);

  // First request should be active initially.
  EXPECT_EQ(id1, model_->GetActiveRequestId());
}

// Test get request by ID.
TEST_F(AstraPermissionPromptModelTest, GetRequest) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kGeolocation;
  std::string id = model_->AddRequest(req);

  const auto* found = model_->GetRequest(id);
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(id, found->id);
  EXPECT_EQ(AstraPermissionType::kGeolocation, found->type);

  // Non-existent ID.
  EXPECT_EQ(nullptr, model_->GetRequest("nonexistent"));
}

// Test removing a request.
TEST_F(AstraPermissionPromptModelTest, RemoveRequest) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kNotifications;
  std::string id = model_->AddRequest(req);
  EXPECT_EQ(1u, model_->GetRequestCount());

  model_->RemoveRequest(id);
  EXPECT_EQ(0u, model_->GetRequestCount());
  EXPECT_EQ(nullptr, model_->GetRequest(id));
  EXPECT_FALSE(model_->IsPromptVisible());

  // Remove non-existent should do nothing.
  model_->RemoveRequest("nonexistent");
  EXPECT_EQ(0u, model_->GetRequestCount());
}

// Test active request navigation.
TEST_F(AstraPermissionPromptModelTest, ActiveRequestNavigation) {
  std::string id1 = model_->AddRequest(AstraPermissionRequest());
  std::string id2 = model_->AddRequest(AstraPermissionRequest());
  std::string id3 = model_->AddRequest(AstraPermissionRequest());

  EXPECT_EQ(id1, model_->GetActiveRequestId());

  // Next.
  model_->NextRequest();
  EXPECT_EQ(id2, model_->GetActiveRequestId());

  model_->NextRequest();
  EXPECT_EQ(id3, model_->GetActiveRequestId());

  // Wrap around.
  model_->NextRequest();
  EXPECT_EQ(id1, model_->GetActiveRequestId());

  // Previous.
  model_->PreviousRequest();
  EXPECT_EQ(id3, model_->GetActiveRequestId());

  model_->PreviousRequest();
  EXPECT_EQ(id2, model_->GetActiveRequestId());
}

// Test set active request.
TEST_F(AstraPermissionPromptModelTest, SetActiveRequest) {
  std::string id1 = model_->AddRequest(AstraPermissionRequest());
  std::string id2 = model_->AddRequest(AstraPermissionRequest());

  EXPECT_EQ(id1, model_->GetActiveRequestId());

  model_->SetActiveRequest(id2);
  EXPECT_EQ(id2, model_->GetActiveRequestId());

  // Setting same ID is no-op.
  model_->SetActiveRequest(id2);
  EXPECT_EQ(id2, model_->GetActiveRequestId());

  // Setting non-existent ID is no-op.
  model_->SetActiveRequest("nonexistent");
  EXPECT_EQ(id2, model_->GetActiveRequestId());
}

// Test Allow decision.
TEST_F(AstraPermissionPromptModelTest, Allow) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kCamera;
  std::string id = model_->AddRequest(req);

  model_->Allow();
  EXPECT_EQ(0u, model_->GetRequestCount());
  EXPECT_EQ(nullptr, model_->GetRequest(id));
}

// Test Block decision.
TEST_F(AstraPermissionPromptModelTest, Block) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kMicrophone;
  std::string id = model_->AddRequest(req);

  model_->Block();
  EXPECT_EQ(0u, model_->GetRequestCount());
}

// Test AllowOnce decision.
TEST_F(AstraPermissionPromptModelTest, AllowOnce) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kGeolocation;
  std::string id = model_->AddRequest(req);

  model_->AllowOnce();
  EXPECT_EQ(0u, model_->GetRequestCount());
}

// Test Dismiss.
TEST_F(AstraPermissionPromptModelTest, Dismiss) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kNotifications;
  std::string id = model_->AddRequest(req);

  model_->Dismiss();
  EXPECT_EQ(0u, model_->GetRequestCount());
}

// Test Decide with specific action.
TEST_F(AstraPermissionPromptModelTest, Decide) {
  std::string id = model_->AddRequest(AstraPermissionRequest());

  model_->Decide(id, AstraPermissionAction::kAllow);
  EXPECT_EQ(0u, model_->GetRequestCount());

  // Decide on non-existent ID is no-op.
  model_->Decide("nonexistent", AstraPermissionAction::kBlock);
  EXPECT_EQ(0u, model_->GetRequestCount());
}

// Test remember decision.
TEST_F(AstraPermissionPromptModelTest, RememberDecision) {
  EXPECT_TRUE(model_->GetRememberDecision());

  model_->SetRememberDecision(false);
  EXPECT_FALSE(model_->GetRememberDecision());

  model_->SetRememberDecision(true);
  EXPECT_TRUE(model_->GetRememberDecision());
}

// Test loading state.
TEST_F(AstraPermissionPromptModelTest, LoadingState) {
  EXPECT_FALSE(model_->IsLoading());

  model_->SetLoading(true);
  EXPECT_TRUE(model_->IsLoading());

  model_->SetLoading(false);
  EXPECT_FALSE(model_->IsLoading());
}

// Test clear all requests.
TEST_F(AstraPermissionPromptModelTest, ClearAllRequests) {
  model_->AddRequest(AstraPermissionRequest());
  model_->AddRequest(AstraPermissionRequest());
  model_->AddRequest(AstraPermissionRequest());
  EXPECT_EQ(3u, model_->GetRequestCount());

  model_->ClearAllRequests();
  EXPECT_EQ(0u, model_->GetRequestCount());
  EXPECT_EQ(nullptr, model_->GetActiveRequest());
}

// Test permission name lookup.
TEST_F(AstraPermissionPromptModelTest, PermissionName) {
  EXPECT_FALSE(AstraPermissionPromptModel::GetPermissionName(
      AstraPermissionType::kCamera).empty());
  EXPECT_FALSE(AstraPermissionPromptModel::GetPermissionName(
      AstraPermissionType::kMicrophone).empty());
  EXPECT_FALSE(AstraPermissionPromptModel::GetPermissionName(
      AstraPermissionType::kGeolocation).empty());
}

// Test permission description lookup.
TEST_F(AstraPermissionPromptModelTest, PermissionDescription) {
  EXPECT_FALSE(AstraPermissionPromptModel::GetPermissionDescription(
      AstraPermissionType::kCamera).empty());
  EXPECT_FALSE(AstraPermissionPromptModel::GetPermissionDescription(
      AstraPermissionType::kNotifications).empty());
}

// Test permission importance.
TEST_F(AstraPermissionPromptModelTest, PermissionImportance) {
  EXPECT_EQ(AstraPermissionImportance::kCritical,
      AstraPermissionPromptModel::GetPermissionImportance(
          AstraPermissionType::kCameraAndMicrophone));
  EXPECT_EQ(AstraPermissionImportance::kHigh,
      AstraPermissionPromptModel::GetPermissionImportance(
          AstraPermissionType::kCamera));
  EXPECT_EQ(AstraPermissionImportance::kMedium,
      AstraPermissionPromptModel::GetPermissionImportance(
          AstraPermissionType::kNotifications));
}

// Test one-time permission support.
TEST_F(AstraPermissionPromptModelTest, SupportsOneTime) {
  EXPECT_TRUE(AstraPermissionPromptModel::SupportsOneTimePermission(
      AstraPermissionType::kCamera));
  EXPECT_TRUE(AstraPermissionPromptModel::SupportsOneTimePermission(
      AstraPermissionType::kGeolocation));
  EXPECT_FALSE(AstraPermissionPromptModel::SupportsOneTimePermission(
      AstraPermissionType::kNotifications));
  EXPECT_FALSE(AstraPermissionPromptModel::SupportsOneTimePermission(
      AstraPermissionType::kJavaScript));
}

// Test permission icon name.
TEST_F(AstraPermissionPromptModelTest, PermissionIconName) {
  EXPECT_FALSE(AstraPermissionPromptModel::GetPermissionIconName(
      AstraPermissionType::kCamera).empty());
  EXPECT_FALSE(AstraPermissionPromptModel::GetPermissionIconName(
      AstraPermissionType::kMicrophone).empty());
}

// Test removing active request moves to next.
TEST_F(AstraPermissionPromptModelTest, RemoveActiveRequest) {
  std::string id1 = model_->AddRequest(AstraPermissionRequest());
  std::string id2 = model_->AddRequest(AstraPermissionRequest());
  std::string id3 = model_->AddRequest(AstraPermissionRequest());

  // id1 is active. Remove it.
  model_->RemoveRequest(id1);
  EXPECT_EQ(2u, model_->GetRequestCount());
  // Active should move to next (id2).
  EXPECT_EQ(id2, model_->GetActiveRequestId());
}

// Test that decisions remove the request and update active.
TEST_F(AstraPermissionPromptModelTest, DecisionUpdatesActive) {
  std::string id1 = model_->AddRequest(AstraPermissionRequest());
  std::string id2 = model_->AddRequest(AstraPermissionRequest());

  // id1 is active. Allow it.
  model_->Allow();
  EXPECT_EQ(1u, model_->GetRequestCount());
  EXPECT_EQ(id2, model_->GetActiveRequestId());
}

// ===========================================================================
// AstraPermissionPromptViewTest
// ===========================================================================

class AstraPermissionPromptViewTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraPermissionPromptModel>();

    // Add a sample request.
    AstraPermissionRequest req;
    req.type = AstraPermissionType::kCamera;
    req.title = u"Use your camera";
    req.message = u"example.com wants to use your camera";
    req.origin = u"https://example.com";
    req.has_remember_option = true;
    req.is_one_time_allowed = true;
    model_->AddRequest(req);
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraPermissionPromptModel> model_;
};

// Test model accessors.
TEST_F(AstraPermissionPromptViewTest, ModelAccessors) {
  EXPECT_TRUE(model_);
  EXPECT_EQ(1u, model_->GetRequestCount());

  const auto* active = model_->GetActiveRequest();
  ASSERT_NE(nullptr, active);
  EXPECT_EQ(AstraPermissionType::kCamera, active->type);
  EXPECT_TRUE(active->has_remember_option);
  EXPECT_TRUE(active->is_one_time_allowed);
}

// Test multi-request view state.
TEST_F(AstraPermissionPromptViewTest, MultiRequestState) {
  // Add another request.
  AstraPermissionRequest req2;
  req2.type = AstraPermissionType::kMicrophone;
  model_->AddRequest(req2);

  EXPECT_EQ(2u, model_->GetRequestCount());

  // Navigation should cycle.
  std::string first = model_->GetActiveRequestId();
  model_->NextRequest();
  std::string second = model_->GetActiveRequestId();
  EXPECT_NE(first, second);

  model_->NextRequest();
  EXPECT_EQ(first, model_->GetActiveRequestId());
}

// Test request with device options.
TEST_F(AstraPermissionPromptModelTest, DeviceOptions) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kCamera;
  req.device_options = {u"FaceTime HD Camera", u"External USB Camera"};
  req.selected_device_index = 0;
  std::string id = model_->AddRequest(req);

  const auto* found = model_->GetRequest(id);
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(2u, found->device_options.size());
  EXPECT_EQ(0, found->selected_device_index);
}

// Test request with auto-dismiss timeout.
TEST_F(AstraPermissionPromptModelTest, AutoDismissTimeout) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kNotifications;
  req.auto_dismiss = true;
  req.auto_dismiss_timeout = base::Seconds(10);
  std::string id = model_->AddRequest(req);

  const auto* found = model_->GetRequest(id);
  ASSERT_NE(nullptr, found);
  EXPECT_TRUE(found->auto_dismiss);
  EXPECT_EQ(base::Seconds(10), found->auto_dismiss_timeout);
}

// Test embedded permission request.
TEST_F(AstraPermissionPromptModelTest, EmbeddedRequest) {
  AstraPermissionRequest req;
  req.type = AstraPermissionType::kGeolocation;
  req.is_embedded = true;
  req.requesting_origin = "https://iframe.example.com";
  req.embedding_origin = "https://example.com";
  std::string id = model_->AddRequest(req);

  const auto* found = model_->GetRequest(id);
  ASSERT_NE(nullptr, found);
  EXPECT_TRUE(found->is_embedded);
  EXPECT_EQ("https://iframe.example.com", found->requesting_origin);
  EXPECT_EQ("https://example.com", found->embedding_origin);
}

}  // namespace astra
