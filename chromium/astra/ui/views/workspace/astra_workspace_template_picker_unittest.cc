// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/workspace/astra_workspace_template_picker_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraWorkspaceTemplatePickerViewTest
// ===========================================================================

class AstraWorkspaceTemplatePickerViewTest : public testing::Test {
 protected:
  void SetUp() override {
    // Create an anchor view for the bubble.
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test that built-in templates can be retrieved.
TEST_F(AstraWorkspaceTemplatePickerViewTest, BuiltInTemplatesExist) {
  auto templates = GetBuiltInTemplates();
  EXPECT_GT(templates.size(), 5u);

  for (const auto& tmpl : templates) {
    EXPECT_FALSE(tmpl.id.empty());
    EXPECT_FALSE(tmpl.name.empty());
    EXPECT_FALSE(tmpl.description.empty());
    EXPECT_FALSE(tmpl.color.empty());
    EXPECT_EQ('#', tmpl.color[0]);
  }
}

// Test template categories.
TEST_F(AstraWorkspaceTemplatePickerViewTest, TemplateCategories) {
  auto categories = GetAllTemplateCategories();
  EXPECT_GT(categories.size(), 5u);

  for (auto cat : categories) {
    std::string name = GetTemplateCategoryDisplayName(cat);
    EXPECT_FALSE(name.empty());
  }
}

// Test filtering templates by category.
TEST_F(AstraWorkspaceTemplatePickerViewTest, FilterByCategory) {
  auto dev_templates = GetTemplatesByCategory(
      AstraWorkspaceTemplateCategory::kDevelopment);
  EXPECT_GT(dev_templates.size(), 0u);

  for (const auto& tmpl : dev_templates) {
    EXPECT_EQ(AstraWorkspaceTemplateCategory::kDevelopment, tmpl.category);
  }
}

// Test finding a template by ID.
TEST_F(AstraWorkspaceTemplatePickerViewTest, FindTemplate) {
  auto templates = GetBuiltInTemplates();
  ASSERT_GT(templates.size(), 0u);

  const auto* found = FindTemplate(templates[0].id);
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(templates[0].id, found->id);
  EXPECT_EQ(templates[0].name, found->name);
}

// Test finding non-existent template returns null.
TEST_F(AstraWorkspaceTemplatePickerViewTest, FindTemplateNonExistent) {
  const auto* found = FindTemplate("nonexistent_template_id");
  EXPECT_EQ(nullptr, found);
}

// Test template has default tabs.
TEST_F(AstraWorkspaceTemplatePickerViewTest, TemplateDefaultTabs) {
  auto templates = GetBuiltInTemplates();
  bool has_tabs = false;

  for (const auto& tmpl : templates) {
    if (!tmpl.default_tabs.empty()) {
      has_tabs = true;
      for (const auto& tab : tmpl.default_tabs) {
        EXPECT_TRUE(tab.url.is_valid());
        EXPECT_FALSE(tab.title.empty());
      }
    }
  }

  EXPECT_TRUE(has_tabs);
}

// Test template card view creation.
TEST_F(AstraWorkspaceTemplatePickerViewTest, TemplateCardCreation) {
  auto templates = GetBuiltInTemplates();
  ASSERT_GT(templates.size(), 0u);

  bool selected_called = false;
  std::string selected_id;

  auto card = std::make_unique<AstraWorkspaceTemplateCardView>(
      templates[0],
      base::BindRepeating(
          [](bool* called, std::string* id, const std::string& tid) {
            *called = true;
            *id = tid;
          },
          &selected_called, &selected_id));

  EXPECT_EQ(templates[0].id, card->template_id());
  EXPECT_FALSE(card->IsSelected());

  card->SetSelected(true);
  EXPECT_TRUE(card->IsSelected());
}

// Test category enum values are all distinct.
TEST_F(AstraWorkspaceTemplatePickerViewTest, CategoriesDistinct) {
  auto categories = GetAllTemplateCategories();
  std::set<std::string> names;

  for (auto cat : categories) {
    std::string name = GetTemplateCategoryDisplayName(cat);
    EXPECT_TRUE(names.find(name) == names.end())
        << "Duplicate category name: " << name;
    names.insert(name);
  }
}

}  // namespace astra
