// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_workspace_import_export.h"

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/browser/astra_workspace_service_factory.h"

namespace astra {

namespace {

// Test observer that records all calls for verification.
class TestImportExportObserver : public AstraWorkspaceImportExportObserver {
 public:
  void OnImportStarted(size_t total_workspaces) override {
    import_started_count_++;
    last_import_total_ = total_workspaces;
  }

  void OnImportProgress(size_t current, size_t total) override {
    import_progress_count_++;
    last_progress_current_ = current;
    last_progress_total_ = total;
  }

  void OnImportCompleted(const AstraWorkspaceImportResult& result) override {
    import_completed_count_++;
    last_import_result_ = result;
  }

  void OnImportFailed(const std::string& error_message) override {
    import_failed_count_++;
    last_import_error_ = error_message;
  }

  void OnExportStarted() override {
    export_started_count_++;
  }

  void OnExportCompleted(const AstraWorkspaceExportResult& result) override {
    export_completed_count_++;
    last_export_result_ = result;
  }

  // Counters
  int import_started_count_ = 0;
  int import_progress_count_ = 0;
  int import_completed_count_ = 0;
  int import_failed_count_ = 0;
  int export_started_count_ = 0;
  int export_completed_count_ = 0;

  // Last recorded values
  size_t last_import_total_ = 0;
  size_t last_progress_current_ = 0;
  size_t last_progress_total_ = 0;
  std::string last_import_error_;
  AstraWorkspaceImportResult last_import_result_;
  AstraWorkspaceExportResult last_export_result_;
};

// Observer that overrides nothing — tests default implementations.
class EmptyObserver : public AstraWorkspaceImportExportObserver {};

}  // namespace

// Test fixture for AstraWorkspaceImportExport tests.
class WorkspaceImportExportTest : public testing::Test {
 protected:
  WorkspaceImportExportTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register workspace service prefs.
    AstraWorkspaceServiceFactory::RegisterProfilePrefs(
        profile_->GetPrefs());
    // Register import/export prefs (via the prefs namespace).
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    import_export_ =
        std::make_unique<AstraWorkspaceImportExport>(profile_.get());
  }

  ~WorkspaceImportExportTest() override = default;

  void SetUp() override {
    ASSERT_NE(import_export_, nullptr);
    // Ensure default workspace exists.
    AstraWorkspaceService* service =
        AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
    if (service) {
      service->EnsureDefaultWorkspace();
    }
  }

  void TearDown() override {
    // Clean up observers.
    for (auto* observer : test_observers_) {
      import_export_->RemoveObserver(observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  // Builds a valid v1 workspace JSON string with the given workspace count.
  static std::string BuildValidJson(size_t num_workspaces = 1,
                                    size_t tabs_per_workspace = 2) {
    base::Value::Dict root;
    root.Set("version", 1);
    root.Set("exported_at", "2026-06-13T10:00:00Z");

    base::Value::List workspaces;
    for (size_t i = 0; i < num_workspaces; ++i) {
      base::Value::Dict ws;
      ws.Set("id", "workspace-" + base::NumberToString(i));
      ws.Set("name", "Workspace " + base::NumberToString(i));
      ws.Set("accent_color", "#5B8FF9");
      ws.Set("order_index", static_cast<int>(i));

      base::Value::List tabs;
      for (size_t j = 0; j < tabs_per_workspace; ++j) {
        base::Value::Dict tab;
        tab.Set("title", "Tab " + base::NumberToString(j));
        tab.Set("url", "https://example.com/" + base::NumberToString(j));
        tab.Set("pinned", false);
        tab.Set("favorite", false);
        tabs.Append(std::move(tab));
      }
      ws.Set("tabs", std::move(tabs));
      workspaces.Append(std::move(ws));
    }
    root.Set("workspaces", std::move(workspaces));

    return base::WriteJson(base::Value(std::move(root))).value_or("");
  }

  // Builds JSON with specific workspace names.
  static std::string BuildJsonWithNames(
      const std::vector<std::string>& names) {
    base::Value::Dict root;
    root.Set("version", 1);
    root.Set("exported_at", "2026-06-13T10:00:00Z");

    base::Value::List workspaces;
    for (size_t i = 0; i < names.size(); ++i) {
      base::Value::Dict ws;
      ws.Set("id", "ws-" + base::NumberToString(i));
      ws.Set("name", names[i]);
      ws.Set("accent_color", "#5B8FF9");
      ws.Set("order_index", static_cast<int>(i));
      ws.Set("tabs", base::Value::List());
      workspaces.Append(std::move(ws));
    }
    root.Set("workspaces", std::move(workspaces));

    return base::WriteJson(base::Value(std::move(root))).value_or("");
  }

  // Adds a test workspace via the service.
  std::string AddTestWorkspace(const std::string& name,
                                const std::string& color = "#5B8FF9") {
    AstraWorkspaceService* service =
        AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
    if (!service) return std::string();

    AstraWorkspace ws;
    ws.id = base::GenerateGUID();
    ws.name = name;
    ws.accent_color = color;
    ws.created_time = base::Time::Now();
    ws.is_default = false;
    ws.order_index = service->workspace_count();
    service->AddWorkspace(std::move(ws));
    return ws.id;
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraWorkspaceImportExport> import_export_;
  std::vector<TestImportExportObserver*> test_observers_;
};

// ===========================================================================
// Constructor / default state
// ===========================================================================

TEST_F(WorkspaceImportExportTest, DefaultState) {
  // Import/export instance should exist.
  EXPECT_NE(import_export_, nullptr);
}

TEST_F(WorkspaceImportExportTest, NullProfileHandled) {
  auto ie = std::make_unique<AstraWorkspaceImportExport>(nullptr);

  // Export with null profile should fail gracefully.
  AstraWorkspaceExportOptions options;
  auto result = ie->ExportWorkspaces(options);
  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());

  // Import with null profile should fail gracefully.
  AstraWorkspaceImportOptions import_options;
  auto import_result = ie->ImportWorkspaces(BuildValidJson(1), import_options);
  EXPECT_FALSE(import_result.success);
  EXPECT_FALSE(import_result.error_message.empty());
}

// ===========================================================================
// Validation: valid JSON
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ValidateValidSingleWorkspace) {
  std::string json = BuildValidJson(1, 3);
  ASSERT_FALSE(json.empty());

  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
  EXPECT_TRUE(result.error_message.empty());
  EXPECT_EQ(result.workspace_count, 1u);
  EXPECT_EQ(result.total_tab_count, 3u);
}

TEST_F(WorkspaceImportExportTest, ValidateValidMultipleWorkspaces) {
  std::string json = BuildValidJson(5, 10);
  ASSERT_FALSE(json.empty());

  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
  EXPECT_EQ(result.workspace_count, 5u);
  EXPECT_EQ(result.total_tab_count, 50u);
}

TEST_F(WorkspaceImportExportTest, ValidateValidEmptyTabs) {
  std::string json = BuildValidJson(1, 0);
  ASSERT_FALSE(json.empty());

  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
  EXPECT_EQ(result.total_tab_count, 0u);
}

TEST_F(WorkspaceImportExportTest, ValidateValidNoTabsField) {
  base::Value::Dict root;
  root.Set("version", 1);
  root.Set("exported_at", "2026-06-13T10:00:00Z");

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Workspace");
  ws.Set("accent_color", "#000000");
  ws.Set("order_index", 0);
  // No "tabs" field.
  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  ASSERT_FALSE(json.empty());

  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
}

TEST_F(WorkspaceImportExportTest, ValidateSimpleBoolReturnsSameResult) {
  std::string valid = BuildValidJson(2, 3);
  EXPECT_TRUE(AstraWorkspaceImportExport::ValidateWorkspaceJson(valid));
  EXPECT_EQ(AstraWorkspaceImportExport::ValidateWorkspaceJson(valid),
            AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(valid).is_valid);

  std::string invalid = "not json";
  EXPECT_FALSE(AstraWorkspaceImportExport::ValidateWorkspaceJson(invalid));
  EXPECT_EQ(AstraWorkspaceImportExport::ValidateWorkspaceJson(invalid),
            AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(invalid).is_valid);
}

// ===========================================================================
// Validation: invalid JSON structure
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ValidateInvalidEmptyString) {
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError("");
  EXPECT_FALSE(result.is_valid);
  EXPECT_FALSE(result.error_message.empty());
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidNotJson) {
  auto result1 = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError("not json");
  EXPECT_FALSE(result1.is_valid);
  EXPECT_FALSE(result1.error_message.empty());

  auto result2 = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError("{broken");
  EXPECT_FALSE(result2.is_valid);
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidJsonArrayNotDict) {
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError("[]");
  EXPECT_FALSE(result.is_valid);
  EXPECT_FALSE(result.error_message.empty());
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidMissingVersion) {
  base::Value::Dict root;
  root.Set("workspaces", base::Value::List());

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("version"), std::string::npos);
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidWrongVersion) {
  base::Value::Dict root;
  root.Set("version", 999);
  root.Set("workspaces", base::Value::List());

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("version"), std::string::npos);
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidVersionString) {
  base::Value::Dict root;
  root.Set("version", "1");
  root.Set("workspaces", base::Value::List());

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidMissingWorkspaces) {
  base::Value::Dict root;
  root.Set("version", 1);

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("workspaces"), std::string::npos);
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidWorkspacesNotList) {
  base::Value::Dict root;
  root.Set("version", 1);
  root.Set("workspaces", "not a list");

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
}

// ===========================================================================
// Validation: invalid workspace entries
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ValidateInvalidWorkspaceMissingId) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("name", "Workspace");
  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("id"), std::string::npos);
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidWorkspaceMissingName) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("name"), std::string::npos);
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidWorkspaceIdNotString) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", 123);
  ws.Set("name", "Name");
  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidWorkspaceTabsNotList) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Name");
  ws.Set("tabs", "not a list");
  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
}

// ===========================================================================
// Validation: invalid tab entries
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ValidateInvalidTabMissingUrl) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Name");

  base::Value::List tabs;
  base::Value::Dict tab;
  tab.Set("title", "Tab");
  tabs.Append(std::move(tab));
  ws.Set("tabs", std::move(tabs));

  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("url"), std::string::npos);
}

TEST_F(WorkspaceImportExportTest, ValidateInvalidTabUrlNotString) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Name");

  base::Value::List tabs;
  base::Value::Dict tab;
  tab.Set("url", 12345);
  tabs.Append(std::move(tab));
  ws.Set("tabs", std::move(tabs));

  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
}

// ===========================================================================
// Validation: URL safety
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ValidateValidHttpUrls) {
  std::string json = BuildValidJson(1, 1);
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
}

TEST_F(WorkspaceImportExportTest, ValidateRejectsJavascriptUrls) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Name");

  base::Value::List tabs;
  base::Value::Dict tab;
  tab.Set("url", "javascript:alert('xss')");
  tabs.Append(std::move(tab));
  ws.Set("tabs", std::move(tabs));

  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("unsafe"), std::string::npos);
}

TEST_F(WorkspaceImportExportTest, ValidateRejectsDataUrls) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Name");

  base::Value::List tabs;
  base::Value::Dict tab;
  tab.Set("url", "data:text/html,<h1>Hi</h1>");
  tabs.Append(std::move(tab));
  ws.Set("tabs", std::move(tabs));

  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
}

TEST_F(WorkspaceImportExportTest, ValidateAllowsChromeUrls) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Name");

  base::Value::List tabs;
  base::Value::Dict tab;
  tab.Set("url", "chrome://settings");
  tabs.Append(std::move(tab));
  ws.Set("tabs", std::move(tabs));

  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
}

TEST_F(WorkspaceImportExportTest, ValidateAllowsAboutBlank) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Name");

  base::Value::List tabs;
  base::Value::Dict tab;
  tab.Set("url", "about:blank");
  tabs.Append(std::move(tab));
  ws.Set("tabs", std::move(tabs));

  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
}

// ===========================================================================
// Validation: limits
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ValidateRespectsMaxWorkspaces) {
  std::string json = BuildValidJson(
      AstraWorkspaceImportExport::kMaxWorkspaces, 0);
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
  EXPECT_EQ(result.workspace_count, AstraWorkspaceImportExport::kMaxWorkspaces);
}

TEST_F(WorkspaceImportExportTest, ValidateRejectsTooManyWorkspaces) {
  std::string json = BuildValidJson(
      AstraWorkspaceImportExport::kMaxWorkspaces + 1, 0);
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("Too many"), std::string::npos);
}

TEST_F(WorkspaceImportExportTest, ValidateRespectsMaxTabsPerWorkspace) {
  std::string json = BuildValidJson(
      1, AstraWorkspaceImportExport::kMaxTabsPerWorkspace);
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
  EXPECT_EQ(result.total_tab_count, AstraWorkspaceImportExport::kMaxTabsPerWorkspace);
}

TEST_F(WorkspaceImportExportTest, ValidateRejectsTooManyTabs) {
  std::string json = BuildValidJson(
      1, AstraWorkspaceImportExport::kMaxTabsPerWorkspace + 1);
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("too many tabs"), std::string::npos);
}

// ===========================================================================
// Validation: extra fields are ignored
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ValidateIgnoresExtraRootFields) {
  std::string json = BuildValidJson(1, 1);

  absl::optional<base::Value> value = base::JSONReader::Read(json);
  ASSERT_TRUE(value.has_value());
  ASSERT_TRUE(value->is_dict());

  value->GetDict().Set("extra_field", "ignored");
  value->GetDict().Set("unknown_number", 42);

  std::string modified_json =
      base::WriteJson(std::move(*value)).value_or("");
  ASSERT_FALSE(modified_json.empty());

  auto result =
      AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(modified_json);
  EXPECT_TRUE(result.is_valid);
}

TEST_F(WorkspaceImportExportTest, ValidateIgnoresExtraWorkspaceFields) {
  std::string json = BuildValidJson(1, 0);

  absl::optional<base::Value> value = base::JSONReader::Read(json);
  ASSERT_TRUE(value.has_value());

  base::Value::List* workspaces = value->GetDict().FindList("workspaces");
  ASSERT_TRUE(workspaces);
  ASSERT_FALSE(workspaces->empty());

  (*workspaces)[0].GetDict().Set("custom_field", "value");
  (*workspaces)[0].GetDict().Set("icon", "star");

  std::string modified_json =
      base::WriteJson(std::move(*value)).value_or("");
  ASSERT_FALSE(modified_json.empty());

  auto result =
      AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(modified_json);
  EXPECT_TRUE(result.is_valid);
}

TEST_F(WorkspaceImportExportTest, ValidateIgnoresExtraTabFields) {
  std::string json = BuildValidJson(1, 1);

  absl::optional<base::Value> value = base::JSONReader::Read(json);
  ASSERT_TRUE(value.has_value());

  base::Value::List* workspaces = value->GetDict().FindList("workspaces");
  ASSERT_TRUE(workspaces);
  base::Value::List* tabs =
      (*workspaces)[0].GetDict().FindList("tabs");
  ASSERT_TRUE(tabs);
  ASSERT_FALSE(tabs->empty());

  (*tabs)[0].GetDict().Set("extra", "data");
  (*tabs)[0].GetDict().Set("favicon_url", "https://...");

  std::string modified_json =
      base::WriteJson(std::move(*value)).value_or("");
  ASSERT_FALSE(modified_json.empty());

  auto result =
      AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(modified_json);
  EXPECT_TRUE(result.is_valid);
}

// ===========================================================================
// Validation: error messages are descriptive
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ValidationErrorMessageContainsWorkspaceIndex) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  // Missing name.
  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("Workspace 0"), std::string::npos);
}

TEST_F(WorkspaceImportExportTest, ValidationErrorMessageContainsTabIndex) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Test");

  base::Value::List tabs;
  base::Value::Dict tab;
  tab.Set("title", "Bad Tab");
  // Missing URL.
  tabs.Append(std::move(tab));
  ws.Set("tabs", std::move(tabs));

  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_NE(result.error_message.find("Workspace 0"), std::string::npos);
  EXPECT_NE(result.error_message.find("tab 0"), std::string::npos);
}

// ===========================================================================
// Constants
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ConstantsHaveReasonableValues) {
  EXPECT_GT(AstraWorkspaceImportExport::kMaxWorkspaces, 0u);
  EXPECT_LE(AstraWorkspaceImportExport::kMaxWorkspaces, 1000u);

  EXPECT_GT(AstraWorkspaceImportExport::kMaxTabsPerWorkspace, 0u);
  EXPECT_LE(AstraWorkspaceImportExport::kMaxTabsPerWorkspace, 10000u);
}

// ===========================================================================
// Import: merge mode
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportMergeModeAddsWorkspaces) {
  // Start with default workspace only.
  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  ASSERT_NE(service, nullptr);
  size_t initial_count = service->workspace_count();

  std::string json = BuildJsonWithNames({"Imported A", "Imported B"});
  ASSERT_FALSE(json.empty());

  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kMerge;
  options.open_tabs = false;  // Don't open tabs (no browser in unit test).

  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.imported_count, 2u);
  EXPECT_EQ(result.skipped_count, 0u);
  EXPECT_EQ(result.failed_count, 0u);
  EXPECT_EQ(result.total(), 2u);

  EXPECT_EQ(service->workspace_count(), initial_count + 2);
}

TEST_F(WorkspaceImportExportTest, ImportMergeModePreservesExisting) {
  std::string existing_id = AddTestWorkspace("Existing WS");
  ASSERT_FALSE(existing_id.empty());

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  size_t initial_count = service->workspace_count();

  std::string json = BuildJsonWithNames({"New WS"});

  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kMerge;
  options.open_tabs = false;

  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.imported_count, 1u);

  // Existing workspace should still exist.
  EXPECT_NE(service->GetWorkspace(existing_id), nullptr);
  EXPECT_EQ(service->workspace_count(), initial_count + 1);
}

// ===========================================================================
// Import: replace mode
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportReplaceModeReplacesNonDefault) {
  std::string existing_id = AddTestWorkspace("To Be Replaced");
  ASSERT_FALSE(existing_id.empty());

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  ASSERT_NE(service, nullptr);

  std::string json = BuildJsonWithNames({"Replacement 1", "Replacement 2"});

  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kReplace;
  options.open_tabs = false;

  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.imported_count, 2u);

  // Old non-default workspace should be gone.
  EXPECT_EQ(service->GetWorkspace(existing_id), nullptr);

  // Should have default + 2 new = 3 total.
  EXPECT_EQ(service->workspace_count(), 3u);
}

TEST_F(WorkspaceImportExportTest, ImportReplaceModePreservesDefault) {
  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  ASSERT_NE(service, nullptr);
  std::string default_id = service->GetDefaultWorkspaceId();
  ASSERT_FALSE(default_id.empty());

  std::string json = BuildJsonWithNames({"New Only"});

  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kReplace;
  options.open_tabs = false;

  import_export_->ImportWorkspaces(json, options);

  // Default workspace should still exist.
  EXPECT_NE(service->GetWorkspace(default_id), nullptr);
}

// ===========================================================================
// Import: conflict resolution — rename
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportConflictResolutionRename) {
  AddTestWorkspace("My Workspace");

  std::string json = BuildJsonWithNames({"My Workspace"});

  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kMerge;
  options.conflict_resolution = AstraWorkspaceConflictResolution::kRename;
  options.open_tabs = false;

  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.imported_count, 1u);
  EXPECT_EQ(result.skipped_count, 0u);

  // Should have both the original and a renamed version.
  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  ASSERT_NE(service, nullptr);

  bool found_original = false;
  bool found_renamed = false;
  for (const auto& ws : service->workspaces()) {
    if (ws.name == "My Workspace") found_original = true;
    if (ws.name == "My Workspace (2)") found_renamed = true;
  }
  EXPECT_TRUE(found_original);
  EXPECT_TRUE(found_renamed);
}

TEST_F(WorkspaceImportExportTest, ImportConflictResolutionRenameMultiple) {
  AddTestWorkspace("Dup");
  AddTestWorkspace("Dup (2)");

  std::string json = BuildJsonWithNames({"Dup"});

  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kMerge;
  options.conflict_resolution = AstraWorkspaceConflictResolution::kRename;
  options.open_tabs = false;

  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.imported_count, 1u);

  // Should find "Dup (3)" since "Dup" and "Dup (2)" already exist.
  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  ASSERT_NE(service, nullptr);

  bool found_dup3 = false;
  for (const auto& ws : service->workspaces()) {
    if (ws.name == "Dup (3)") found_dup3 = true;
  }
  EXPECT_TRUE(found_dup3);
}

// ===========================================================================
// Import: conflict resolution — skip
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportConflictResolutionSkip) {
  AddTestWorkspace("Existing Name");

  size_t initial_count =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get())->workspace_count();

  std::string json = BuildJsonWithNames({"Existing Name", "New Name"});

  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kMerge;
  options.conflict_resolution = AstraWorkspaceConflictResolution::kSkip;
  options.open_tabs = false;

  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.imported_count, 1u);   // Only "New Name" imported.
  EXPECT_EQ(result.skipped_count, 1u);    // "Existing Name" skipped.

  EXPECT_EQ(AstraWorkspaceServiceFactory::GetForProfile(profile_.get())
                ->workspace_count(),
            initial_count + 1);
}

TEST_F(WorkspaceImportExportTest, ImportAllSkippedReturnsFailure) {
  AddTestWorkspace("Already There");

  std::string json = BuildJsonWithNames({"Already There"});

  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kMerge;
  options.conflict_resolution = AstraWorkspaceConflictResolution::kSkip;
  options.open_tabs = false;

  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.imported_count, 0u);
  EXPECT_EQ(result.skipped_count, 1u);
  EXPECT_FALSE(result.error_message.empty());
}

// ===========================================================================
// Import: error handling
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportInvalidJsonReturnsError) {
  AstraWorkspaceImportOptions options;
  auto result = import_export_->ImportWorkspaces("not valid json", options);
  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
  EXPECT_EQ(result.imported_count, 0u);
}

TEST_F(WorkspaceImportExportTest, ImportEmptyStringReturnsError) {
  AstraWorkspaceImportOptions options;
  auto result = import_export_->ImportWorkspaces("", options);
  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

TEST_F(WorkspaceImportExportTest, ImportWithValidationErrorReturnsError) {
  // JSON with missing version.
  std::string json = R"({"workspaces": []})";
  AstraWorkspaceImportOptions options;
  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

// ===========================================================================
// Import: empty workspaces list
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportEmptyWorkspacesList) {
  base::Value::Dict root;
  root.Set("version", 1);
  root.Set("workspaces", base::Value::List());

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");
  ASSERT_FALSE(json.empty());

  AstraWorkspaceImportOptions options;
  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_FALSE(result.success);  // No workspaces imported = not successful.
  EXPECT_EQ(result.imported_count, 0u);
}

// ===========================================================================
// Export
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ExportProducesValidJson) {
  AddTestWorkspace("Workspace A", "#FF0000");
  AddTestWorkspace("Workspace B", "#00FF00");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;  // No tabs without browser.
  auto result = import_export_->ExportWorkspaces(options);

  EXPECT_TRUE(result.success);
  EXPECT_FALSE(result.json.empty());
  EXPECT_GT(result.exported_count, 0u);

  // Parse to verify it's valid JSON.
  auto parsed = base::JSONReader::Read(result.json);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_TRUE(parsed->is_dict());

  // Check version.
  const base::Value::Dict& dict = parsed->GetDict();
  absl::optional<int> version = dict.FindInt("version");
  ASSERT_TRUE(version.has_value());
  EXPECT_EQ(*version, 1);

  // Check workspaces list.
  const base::Value::List* workspaces = dict.FindList("workspaces");
  ASSERT_TRUE(workspaces);
  EXPECT_EQ(workspaces->size(), result.exported_count);
}

TEST_F(WorkspaceImportExportTest, ExportIncludesWorkspaceFields) {
  AddTestWorkspace("My Workspace", "#5B8FF9");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  options.include_settings = true;
  auto result = import_export_->ExportWorkspaces(options);

  ASSERT_TRUE(result.success);
  auto parsed = base::JSONReader::Read(result.json);
  ASSERT_TRUE(parsed.has_value());

  const base::Value::List* workspaces = parsed->GetDict().FindList("workspaces");
  ASSERT_TRUE(workspaces);
  ASSERT_FALSE(workspaces->empty());

  const base::Value::Dict& ws = (*workspaces)[0].GetDict();
  EXPECT_TRUE(ws.FindString("id"));
  EXPECT_TRUE(ws.FindString("name"));
  EXPECT_TRUE(ws.FindString("accent_color"));
  EXPECT_TRUE(ws.FindInt("order_index"));
}

TEST_F(WorkspaceImportExportTest, ExportWithEmptyWorkspaces) {
  // Just the default workspace.
  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  auto result = import_export_->ExportWorkspaces(options);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.exported_count, 1u);  // At least default workspace.
  EXPECT_FALSE(result.json.empty());
}

TEST_F(WorkspaceImportExportTest, ExportMultipleWorkspaces) {
  AddTestWorkspace("Alpha");
  AddTestWorkspace("Beta");
  AddTestWorkspace("Gamma");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  auto result = import_export_->ExportWorkspaces(options);

  EXPECT_TRUE(result.success);
  // Default + 3 added = 4+.
  EXPECT_GE(result.exported_count, 4u);
}

// ===========================================================================
// Export: options
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ExportOptionExcludeSettings) {
  AddTestWorkspace("Test WS", "#AABBCC");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  options.include_settings = false;
  auto result = import_export_->ExportWorkspaces(options);

  ASSERT_TRUE(result.success);
  auto parsed = base::JSONReader::Read(result.json);
  ASSERT_TRUE(parsed.has_value());

  const base::Value::List* workspaces = parsed->GetDict().FindList("workspaces");
  ASSERT_TRUE(workspaces);

  // Find the test workspace (not default).
  bool found_test_ws = false;
  for (const auto& item : *workspaces) {
    const std::string* name = item.GetDict().FindString("name");
    if (name && *name == "Test WS") {
      found_test_ws = true;
      // When settings are excluded, accent_color should not be present.
      EXPECT_FALSE(item.GetDict().FindString("accent_color"));
      break;
    }
  }
  EXPECT_TRUE(found_test_ws);
}

TEST_F(WorkspaceImportExportTest, ExportOptionIncludeSettings) {
  AddTestWorkspace("Test WS", "#AABBCC");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  options.include_settings = true;
  auto result = import_export_->ExportWorkspaces(options);

  ASSERT_TRUE(result.success);
  auto parsed = base::JSONReader::Read(result.json);
  ASSERT_TRUE(parsed.has_value());

  const base::Value::List* workspaces = parsed->GetDict().FindList("workspaces");
  ASSERT_TRUE(workspaces);

  bool found_test_ws = false;
  for (const auto& item : *workspaces) {
    const std::string* name = item.GetDict().FindString("name");
    if (name && *name == "Test WS") {
      found_test_ws = true;
      EXPECT_TRUE(item.GetDict().FindString("accent_color"));
      EXPECT_TRUE(item.GetDict().FindInt("order_index"));
      break;
    }
  }
  EXPECT_TRUE(found_test_ws);
}

TEST_F(WorkspaceImportExportTest, ExportOptionExcludeTabs) {
  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  auto result = import_export_->ExportWorkspaces(options);

  ASSERT_TRUE(result.success);
  EXPECT_EQ(result.tabs_exported, 0u);
}

TEST_F(WorkspaceImportExportTest, ExportOptionIncludeMetadata) {
  AddTestWorkspace("Meta WS");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  options.include_metadata = true;
  auto result = import_export_->ExportWorkspaces(options);

  ASSERT_TRUE(result.success);
  auto parsed = base::JSONReader::Read(result.json);
  ASSERT_TRUE(parsed.has_value());

  const base::Value::List* workspaces = parsed->GetDict().FindList("workspaces");
  ASSERT_TRUE(workspaces);

  bool found = false;
  for (const auto& item : *workspaces) {
    const std::string* name = item.GetDict().FindString("name");
    if (name && *name == "Meta WS") {
      found = true;
      EXPECT_TRUE(item.GetDict().FindDouble("created_time"));
      EXPECT_TRUE(item.GetDict().FindDouble("last_used_time"));
      EXPECT_TRUE(item.GetDict().FindBool("is_hibernated"));
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(WorkspaceImportExportTest, ExportOptionExcludeMetadata) {
  AddTestWorkspace("NoMeta WS");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  options.include_metadata = false;
  auto result = import_export_->ExportWorkspaces(options);

  ASSERT_TRUE(result.success);
  auto parsed = base::JSONReader::Read(result.json);
  ASSERT_TRUE(parsed.has_value());

  const base::Value::List* workspaces = parsed->GetDict().FindList("workspaces");
  ASSERT_TRUE(workspaces);

  bool found = false;
  for (const auto& item : *workspaces) {
    const std::string* name = item.GetDict().FindString("name");
    if (name && *name == "NoMeta WS") {
      found = true;
      EXPECT_FALSE(item.GetDict().FindDouble("created_time"));
      EXPECT_FALSE(item.GetDict().FindDouble("last_used_time"));
      break;
    }
  }
  EXPECT_TRUE(found);
}

// ===========================================================================
// Export: by ID
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ExportByIdsSelectsSpecificWorkspaces) {
  std::string id1 = AddTestWorkspace("First");
  std::string id2 = AddTestWorkspace("Second");
  std::string id3 = AddTestWorkspace("Third");

  ASSERT_FALSE(id1.empty());
  ASSERT_FALSE(id2.empty());
  ASSERT_FALSE(id3.empty());

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  auto result = import_export_->ExportWorkspacesByIds({id1, id3}, options);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.exported_count, 2u);
}

TEST_F(WorkspaceImportExportTest, ExportByIdsSkipsNonexistent) {
  std::string real_id = AddTestWorkspace("Real");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  auto result = import_export_->ExportWorkspacesByIds(
      {real_id, "nonexistent-id-123"}, options);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.exported_count, 1u);
}

TEST_F(WorkspaceImportExportTest, ExportByIdsAllNonexistentFails) {
  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  auto result = import_export_->ExportWorkspacesByIds(
      {"fake-1", "fake-2"}, options);

  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.exported_count, 0u);
  EXPECT_FALSE(result.error_message.empty());
}

TEST_F(WorkspaceImportExportTest, ExportSingleWorkspace) {
  std::string id = AddTestWorkspace("Single");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  auto result = import_export_->ExportWorkspace(id, options);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.exported_count, 1u);
}

// ===========================================================================
// Stats verification
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportStatsAreAccurate) {
  AddTestWorkspace("Conflict");

  std::string json = BuildJsonWithNames({"New 1", "Conflict", "New 2"});

  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kMerge;
  options.conflict_resolution = AstraWorkspaceConflictResolution::kSkip;
  options.open_tabs = false;

  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_EQ(result.imported_count, 2u);
  EXPECT_EQ(result.skipped_count, 1u);
  EXPECT_EQ(result.failed_count, 0u);
  EXPECT_EQ(result.total(), 3u);
}

TEST_F(WorkspaceImportExportTest, ExportStatsAreAccurate) {
  AddTestWorkspace("A");
  AddTestWorkspace("B");
  AddTestWorkspace("C");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  auto result = import_export_->ExportWorkspaces(options);

  // Default + 3 = 4+.
  EXPECT_GE(result.exported_count, 4u);
  EXPECT_EQ(result.tabs_exported, 0u);
}

// ===========================================================================
// Observer notifications
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ObserverFiresOnImportStarted) {
  TestImportExportObserver observer;
  import_export_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  std::string json = BuildJsonWithNames({"WS1", "WS2", "WS3"});

  AstraWorkspaceImportOptions options;
  options.open_tabs = false;

  import_export_->ImportWorkspaces(json, options);

  EXPECT_EQ(observer.import_started_count_, 1);
  EXPECT_EQ(observer.last_import_total_, 3u);
}

TEST_F(WorkspaceImportExportTest, ObserverFiresOnImportProgress) {
  TestImportExportObserver observer;
  import_export_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  std::string json = BuildJsonWithNames({"WS1", "WS2", "WS3", "WS4", "WS5"});

  AstraWorkspaceImportOptions options;
  options.open_tabs = false;

  import_export_->ImportWorkspaces(json, options);

  // Progress should fire for each workspace.
  EXPECT_GE(observer.import_progress_count_, 5);
  // Last progress should be at the total.
  EXPECT_EQ(observer.last_progress_current_, 5u);
  EXPECT_EQ(observer.last_progress_total_, 5u);
}

TEST_F(WorkspaceImportExportTest, ObserverFiresOnImportCompleted) {
  TestImportExportObserver observer;
  import_export_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  std::string json = BuildJsonWithNames({"Success WS"});

  AstraWorkspaceImportOptions options;
  options.open_tabs = false;

  import_export_->ImportWorkspaces(json, options);

  EXPECT_EQ(observer.import_completed_count_, 1);
  EXPECT_TRUE(observer.last_import_result_.success);
  EXPECT_EQ(observer.last_import_result_.imported_count, 1u);
}

TEST_F(WorkspaceImportExportTest, ObserverFiresOnImportFailed) {
  TestImportExportObserver observer;
  import_export_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  AstraWorkspaceImportOptions options;
  import_export_->ImportWorkspaces("invalid json", options);

  EXPECT_EQ(observer.import_failed_count_, 1);
  EXPECT_FALSE(observer.last_import_error_.empty());
  // On failure, completed should NOT fire.
  EXPECT_EQ(observer.import_completed_count_, 0);
}

TEST_F(WorkspaceImportExportTest, ObserverFiresOnExportStarted) {
  TestImportExportObserver observer;
  import_export_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  import_export_->ExportWorkspaces(options);

  EXPECT_EQ(observer.export_started_count_, 1);
}

TEST_F(WorkspaceImportExportTest, ObserverFiresOnExportCompleted) {
  TestImportExportObserver observer;
  import_export_->AddObserver(&observer);
  test_observers_.push_back(&observer);

  AddTestWorkspace("Export Me");

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  import_export_->ExportWorkspaces(options);

  EXPECT_EQ(observer.export_completed_count_, 1);
  EXPECT_TRUE(observer.last_export_result_.success);
  EXPECT_GE(observer.last_export_result_.exported_count, 1u);
  EXPECT_FALSE(observer.last_export_result_.json.empty());
}

TEST_F(WorkspaceImportExportTest, MultipleObserversAllNotified) {
  TestImportExportObserver observer1;
  TestImportExportObserver observer2;
  import_export_->AddObserver(&observer1);
  import_export_->AddObserver(&observer2);
  test_observers_.push_back(&observer1);
  test_observers_.push_back(&observer2);

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  import_export_->ExportWorkspaces(options);

  EXPECT_EQ(observer1.export_started_count_, 1);
  EXPECT_EQ(observer2.export_started_count_, 1);
  EXPECT_EQ(observer1.export_completed_count_, 1);
  EXPECT_EQ(observer2.export_completed_count_, 1);
}

TEST_F(WorkspaceImportExportTest, RemoveObserverStopsNotifications) {
  TestImportExportObserver observer;
  import_export_->AddObserver(&observer);

  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  import_export_->ExportWorkspaces(options);
  EXPECT_EQ(observer.export_started_count_, 1);

  import_export_->RemoveObserver(&observer);

  import_export_->ExportWorkspaces(options);
  EXPECT_EQ(observer.export_started_count_, 1);  // Still 1, not incremented.
}

// ===========================================================================
// Observer default implementations
// ===========================================================================

TEST(AstraWorkspaceImportExportObserverTest, DefaultImplementationsAreNoOps) {
  // Observer with default empty implementations can be instantiated and all
  // methods called without crashing.
  EmptyObserver observer;

  observer.OnImportStarted(5);
  observer.OnImportProgress(3, 5);
  observer.OnImportCompleted(AstraWorkspaceImportResult());
  observer.OnImportFailed("error");
  observer.OnExportStarted();
  observer.OnExportCompleted(AstraWorkspaceExportResult());
  // No crash = success.
}

TEST(AstraWorkspaceImportExportObserverTest, PartialObserverWorks) {
  // Observer that only overrides one method should compile and work.
  class PartialObserver : public AstraWorkspaceImportExportObserver {
   public:
    void OnExportStarted() override { count_++; }
    int count_ = 0;
  };

  PartialObserver observer;
  EXPECT_EQ(observer.count_, 0);

  // Call un-overridden methods (should be no-ops).
  observer.OnImportStarted(1);
  observer.OnImportCompleted(AstraWorkspaceImportResult());

  // Call the overridden one.
  observer.OnExportStarted();
  EXPECT_EQ(observer.count_, 1);
}

// ===========================================================================
// Settings persistence (PrefService)
// ===========================================================================

TEST_F(WorkspaceImportExportTest, DefaultExportOptionsHaveExpectedDefaults) {
  auto options = import_export_->GetDefaultExportOptions();
  EXPECT_TRUE(options.include_tabs);
  EXPECT_TRUE(options.include_settings);
  EXPECT_TRUE(options.include_favorites);
  EXPECT_FALSE(options.include_metadata);
}

TEST_F(WorkspaceImportExportTest, DefaultImportOptionsHaveExpectedDefaults) {
  auto options = import_export_->GetDefaultImportOptions();
  EXPECT_EQ(options.mode, AstraWorkspaceImportMode::kMerge);
  EXPECT_EQ(options.conflict_resolution,
            AstraWorkspaceConflictResolution::kRename);
  EXPECT_TRUE(options.open_tabs);
  EXPECT_TRUE(options.apply_favorites);
}

TEST_F(WorkspaceImportExportTest, SetDefaultExportOptionsPersists) {
  AstraWorkspaceExportOptions options;
  options.include_tabs = false;
  options.include_settings = false;
  options.include_favorites = false;
  options.include_metadata = true;
  import_export_->SetDefaultExportOptions(options);

  auto loaded = import_export_->GetDefaultExportOptions();
  EXPECT_FALSE(loaded.include_tabs);
  EXPECT_FALSE(loaded.include_settings);
  EXPECT_FALSE(loaded.include_favorites);
  EXPECT_TRUE(loaded.include_metadata);
}

TEST_F(WorkspaceImportExportTest, SetDefaultImportOptionsPersists) {
  AstraWorkspaceImportOptions options;
  options.mode = AstraWorkspaceImportMode::kReplace;
  options.conflict_resolution = AstraWorkspaceConflictResolution::kSkip;
  options.open_tabs = false;
  options.apply_favorites = false;
  import_export_->SetDefaultImportOptions(options);

  auto loaded = import_export_->GetDefaultImportOptions();
  EXPECT_EQ(loaded.mode, AstraWorkspaceImportMode::kReplace);
  EXPECT_EQ(loaded.conflict_resolution,
            AstraWorkspaceConflictResolution::kSkip);
  EXPECT_FALSE(loaded.open_tabs);
  EXPECT_FALSE(loaded.apply_favorites);
}

TEST_F(WorkspaceImportExportTest, SettingsSurviveServiceRecreation) {
  // Change settings.
  AstraWorkspaceExportOptions export_opts;
  export_opts.include_metadata = true;
  export_opts.include_tabs = false;
  import_export_->SetDefaultExportOptions(export_opts);

  AstraWorkspaceImportOptions import_opts;
  import_opts.mode = AstraWorkspaceImportMode::kReplace;
  import_export_->SetDefaultImportOptions(import_opts);

  // Recreate the import/export instance.
  import_export_.reset();
  import_export_ =
      std::make_unique<AstraWorkspaceImportExport>(profile_.get());

  // Verify settings persisted.
  auto loaded_export = import_export_->GetDefaultExportOptions();
  EXPECT_TRUE(loaded_export.include_metadata);
  EXPECT_FALSE(loaded_export.include_tabs);

  auto loaded_import = import_export_->GetDefaultImportOptions();
  EXPECT_EQ(loaded_import.mode, AstraWorkspaceImportMode::kReplace);
}

// ===========================================================================
// File name helpers
// ===========================================================================

TEST_F(WorkspaceImportExportTest, GenerateExportFileNameHasCorrectFormat) {
  std::string name = AstraWorkspaceImportExport::GenerateExportFileName();
  EXPECT_FALSE(name.empty());
  EXPECT_NE(name.find("astra-workspaces-"), 0u);
  // Should end with .json
  EXPECT_NE(name.find(".json"), std::string::npos);
  EXPECT_EQ(name.substr(name.size() - 5), ".json");
}

TEST_F(WorkspaceImportExportTest, GetExportFileExtension) {
  EXPECT_EQ(AstraWorkspaceImportExport::GetExportFileExtension(), ".json");
}

TEST_F(WorkspaceImportExportTest, GenerateExportFileNameUnique) {
  // Two consecutive calls should produce different names (due to timestamp).
  std::string name1 = AstraWorkspaceImportExport::GenerateExportFileName();
  // Timestamps have second resolution, so sleep briefly.
  base::PlatformThread::Sleep(base::Milliseconds(1100));
  std::string name2 = AstraWorkspaceImportExport::GenerateExportFileName();
  EXPECT_NE(name1, name2);
}

// ===========================================================================
// Edge cases
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportWorkspaceWithDescription) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-1");
  ws.Set("name", "Described");
  ws.Set("description", "A workspace with a description");
  ws.Set("accent_color", "#FF0000");
  ws.Set("tabs", base::Value::List());
  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");

  AstraWorkspaceImportOptions options;
  options.open_tabs = false;
  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_TRUE(result.success);

  // Find the imported workspace and check description.
  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  bool found = false;
  for (const auto& ws : service->workspaces()) {
    if (ws.name == "Described") {
      found = true;
      EXPECT_EQ(ws.description, "A workspace with a description");
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(WorkspaceImportExportTest, ImportWorkspaceWithIcon) {
  base::Value::Dict root;
  root.Set("version", 1);

  base::Value::List workspaces;
  base::Value::Dict ws;
  ws.Set("id", "ws-icon");
  ws.Set("name", "Icon WS");
  ws.Set("icon", "star");
  ws.Set("tabs", base::Value::List());
  workspaces.Append(std::move(ws));
  root.Set("workspaces", std::move(workspaces));

  std::string json = base::WriteJson(base::Value(std::move(root))).value_or("");

  AstraWorkspaceImportOptions options;
  options.open_tabs = false;
  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_TRUE(result.success);

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  bool found = false;
  for (const auto& ws : service->workspaces()) {
    if (ws.name == "Icon WS") {
      found = true;
      ASSERT_TRUE(ws.icon.has_value());
      EXPECT_EQ(*ws.icon, "star");
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(WorkspaceImportExportTest, ImportEmptyNameIsAllowed) {
  std::string json = BuildJsonWithNames({""});

  AstraWorkspaceImportOptions options;
  options.open_tabs = false;
  auto result = import_export_->ImportWorkspaces(json, options);
  // Empty name is allowed (name field exists, just empty string).
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.imported_count, 1u);
}

TEST_F(WorkspaceImportExportTest, ImportWorkspacesGetNewIds) {
  std::string json = BuildJsonWithNames({"Imported"});

  AstraWorkspaceImportOptions options;
  options.open_tabs = false;
  auto result = import_export_->ImportWorkspaces(json, options);
  ASSERT_TRUE(result.success);

  AstraWorkspaceService* service =
      AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
  // Find the imported workspace.
  const AstraWorkspace* imported = nullptr;
  for (const auto& ws : service->workspaces()) {
    if (ws.name == "Imported") {
      imported = &ws;
      break;
    }
  }
  ASSERT_NE(imported, nullptr);
  // ID should not be "workspace-0" (the JSON's id) — it should be a GUID.
  EXPECT_NE(imported->id, "workspace-0");
  EXPECT_FALSE(imported->id.empty());
  // Should be a valid GUID format.
  EXPECT_EQ(imported->id.length(), 36u);
}

TEST_F(WorkspaceImportExportTest, ExportThenValidateProducesValidJson) {
  AddTestWorkspace("Roundtrip Test", "#112233");

  AstraWorkspaceExportOptions export_options;
  export_options.include_tabs = false;
  auto export_result = import_export_->ExportWorkspaces(export_options);
  ASSERT_TRUE(export_result.success);

  // The exported JSON should validate successfully.
  auto validation = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(
      export_result.json);
  EXPECT_TRUE(validation.is_valid) << "Validation error: " << validation.error_message;
  EXPECT_EQ(validation.workspace_count, export_result.exported_count);
}

TEST_F(WorkspaceImportExportTest, ImportResultTotalIsConsistent) {
  AddTestWorkspace("Skip Me");

  std::string json = BuildJsonWithNames({"Skip Me", "Import Me", "Also Import"});

  AstraWorkspaceImportOptions options;
  options.conflict_resolution = AstraWorkspaceConflictResolution::kSkip;
  options.open_tabs = false;

  auto result = import_export_->ImportWorkspaces(json, options);
  EXPECT_EQ(result.total(),
            result.imported_count + result.skipped_count + result.failed_count);
  EXPECT_EQ(result.total(), 3u);
}

// ===========================================================================
// Export options: favorites
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ExportOptionIncludeFavorites) {
  // Verify that when include_favorites is true, the tab favorite field exists.
  // We can't easily test with real tabs without a browser, but we can check
  // the export options are stored correctly via prefs.
  AstraWorkspaceExportOptions options;
  options.include_favorites = true;
  import_export_->SetDefaultExportOptions(options);

  auto loaded = import_export_->GetDefaultExportOptions();
  EXPECT_TRUE(loaded.include_favorites);
}

TEST_F(WorkspaceImportExportTest, ExportOptionExcludeFavorites) {
  AstraWorkspaceExportOptions options;
  options.include_favorites = false;
  import_export_->SetDefaultExportOptions(options);

  auto loaded = import_export_->GetDefaultExportOptions();
  EXPECT_FALSE(loaded.include_favorites);
}

// ===========================================================================
// Import options: favorites
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportOptionApplyFavorites) {
  AstraWorkspaceImportOptions options;
  options.apply_favorites = false;
  import_export_->SetDefaultImportOptions(options);

  auto loaded = import_export_->GetDefaultImportOptions();
  EXPECT_FALSE(loaded.apply_favorites);
}

// ===========================================================================
// Static validation helper
// ===========================================================================

TEST_F(WorkspaceImportExportTest, StaticValidateWorkspaceJsonMatchesInstance) {
  std::string valid = BuildValidJson(3, 5);
  std::string invalid = "broken json";

  // Static method and instance-facing validation should agree.
  EXPECT_EQ(AstraWorkspaceImportExport::ValidateWorkspaceJson(valid),
            AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(valid).is_valid);
  EXPECT_EQ(AstraWorkspaceImportExport::ValidateWorkspaceJson(invalid),
            AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(invalid).is_valid);
}

// ===========================================================================
// Bulk operations (import many workspaces)
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ImportBulkWorkspaces) {
  const size_t count = 10;
  std::vector<std::string> names;
  for (size_t i = 0; i < count; ++i) {
    names.push_back("Bulk WS " + base::NumberToString(i));
  }

  std::string json = BuildJsonWithNames(names);

  AstraWorkspaceImportOptions options;
  options.open_tabs = false;
  auto result = import_export_->ImportWorkspaces(json, options);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.imported_count, count);
  EXPECT_EQ(result.failed_count, 0u);
  EXPECT_EQ(result.skipped_count, 0u);
}

// ===========================================================================
// Validation result struct
// ===========================================================================

TEST_F(WorkspaceImportExportTest, ValidationResultCountsAreCorrect) {
  std::string json = BuildValidJson(3, 7);
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError(json);
  EXPECT_TRUE(result.is_valid);
  EXPECT_EQ(result.workspace_count, 3u);
  EXPECT_EQ(result.total_tab_count, 21u);
}

TEST_F(WorkspaceImportExportTest, ValidationResultErrorOnInvalid) {
  auto result = AstraWorkspaceImportExport::ValidateWorkspaceJsonWithError("not json");
  EXPECT_FALSE(result.is_valid);
  EXPECT_FALSE(result.error_message.empty());
  EXPECT_EQ(result.workspace_count, 0u);
}

// ===========================================================================
// TODO(astra): Browser integration tests
// ===========================================================================
//
// The following tests require a real Browser + TabStripModel and should
// be implemented as browser_tests using InProcessBrowserTest:
//
//   - ExportWorkspaces_IncludesTabs
//   - ExportWorkspaces_PreservesFavoriteState
//   - ImportWorkspaces_CreatesTabs
//   - ImportWorkspaces_GeneratesNewIds
//   - ImportWorkspaces_DefaultWorkspaceNotDeleted
//   - ImportWorkspaces_SetsFavoriteMetadata
//   - ImportWorkspaces_SetsWorkspaceMetadata
//   - RoundTrip_ExportThenImport
//
// TODO(astra): Add browser_tests target for import/export integration.
// Patch point: //chrome/test/BUILD.gn browser_tests test suites.

}  // namespace astra
