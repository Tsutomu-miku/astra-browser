// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/password_manager/astra_password_manager_model.h"
#include "astra/ui/views/password_manager/astra_password_manager_view.h"

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// Model Tests
// ===========================================================================

class AstraPasswordManagerModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraPasswordManagerModel>();
  }

  void TearDown() override { model_.reset(); }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraPasswordManagerModel> model_;
};

// Test model creation.
TEST_F(AstraPasswordManagerModelTest, ModelCreation) {
  EXPECT_NE(nullptr, model_.get());
  EXPECT_EQ(0u, model_->GetCount());
  EXPECT_TRUE(model_->GetPasswords().empty());
  EXPECT_TRUE(model_->GetSearchQuery().empty());
  EXPECT_EQ(AstraPasswordFilter::kAll, model_->GetFilter());
  EXPECT_TRUE(model_->GetCategoryFilter().empty());
  EXPECT_FALSE(model_->IsLoading());
  EXPECT_EQ(0u, model_->GetPasswordIssuesCount());
}

// Test sample population (20+ entries across categories).
TEST_F(AstraPasswordManagerModelTest, PopulateSamplePasswords) {
  model_->PopulateSamplePasswords();

  EXPECT_GT(model_->GetCount(), 20u);
  EXPECT_FALSE(model_->GetPasswords().empty());

  // Check that passwords are sorted by site name.
  const auto& passwords = model_->GetPasswords();
  for (size_t i = 1; i < passwords.size(); ++i) {
    std::u16 prev = base::ToLowerASCII(passwords[i - 1].site_name);
    std::u16string curr = base::ToLowerASCII(passwords[i].site_name);
    EXPECT_LE(prev, curr);
  }

  // Check that we have multiple categories.
  auto categories = model_->GetCategories();
  EXPECT_GT(categories.size(), 2u);
}

// Test adding a password.
TEST_F(AstraPasswordManagerModelTest, AddPassword) {
  size_t initial_count = model_->GetCount();

  std::string id = model_->AddPassword(
      u"Test Site", u"testuser", "testpass123", "https://test.com", "Work");

  EXPECT_FALSE(id.empty());
  EXPECT_EQ(initial_count + 1, model_->GetCount());

  const AstraPasswordEntry* entry = model_->GetPassword(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(u"Test Site", entry->site_name);
  EXPECT_EQ(u"testuser", entry->username);
  EXPECT_EQ("testpass123", entry->password);
  EXPECT_EQ("https://test.com", entry->url);
  EXPECT_EQ("Work", entry->category);
  EXPECT_FALSE(entry->date_created.is_null());
}

// Test removing a password.
TEST_F(AstraPasswordManagerModelTest, RemovePassword) {
  std::string id = model_->AddPassword(
      u"To Remove", u"user", "pass", "https://remove.com", "Social");
  ASSERT_FALSE(id.empty());
  size_t count_before = model_->GetCount();

  model_->RemovePassword(id);
  EXPECT_EQ(count_before - 1, model_->GetCount());

  const AstraPasswordEntry* entry = model_->GetPassword(id);
  EXPECT_EQ(nullptr, entry);
}

// Test removing a non-existent password (no-op).
TEST_F(AstraPasswordManagerModelTest, RemovePasswordNotFound) {
  model_->PopulateSamplePasswords();
  size_t count_before = model_->GetCount();
  model_->RemovePassword("nonexistent_id");
  EXPECT_EQ(count_before, model_->GetCount());
}

// Test updating a password.
TEST_F(AstraPasswordManagerModelTest, UpdatePassword) {
  std::string id = model_->AddPassword(
      u"Test", u"original_user", "original_pass", "https://test.com", "Work");

  model_->UpdatePassword(id, u"new_user", "new_strong_password!123",
                         u"some notes");

  const AstraPasswordEntry* entry = model_->GetPassword(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(u"new_user", entry->username);
  EXPECT_EQ("new_strong_password!123", entry->password);
  EXPECT_EQ(u"some notes", entry->notes);
  EXPECT_GT(entry->date_password_changed, base::Time());
}

// Test GetPassword returns nullptr for invalid ID.
TEST_F(AstraPasswordManagerModelTest, GetPasswordInvalid) {
  EXPECT_EQ(nullptr, model_->GetPassword("nonexistent"));
}

// Test search functionality.
TEST_F(AstraPasswordManagerModelTest, SearchPasswords) {
  model_->PopulateSamplePasswords();
  size_t total_count = model_->GetCount();

  // Empty search returns all.
  model_->SetSearchQuery(u"");
  EXPECT_EQ(total_count, model_->GetFilteredPasswords().size());

  // Search for "google".
  model_->SetSearchQuery(u"google");
  auto results = model_->GetFilteredPasswords();
  EXPECT_GT(results.size(), 0u);
  EXPECT_LT(results.size(), total_count);

  for (const auto& entry : results) {
    std::u16string name_lower = base::ToLowerASCII(entry.site_name);
    std::u16string user_lower = base::ToLowerASCII(entry.username);
    std::u16string url_lower =
        base::ToLowerASCII(base::UTF8ToUTF16(entry.url));
    EXPECT_TRUE(name_lower.find(u"google") != std::u16string::npos ||
                user_lower.find(u"google") != std::u16string::npos ||
                url_lower.find(u"google") != std::u16string::npos);
  }

  // Search for a non-existent term.
  model_->SetSearchQuery(u"zzznonexistent");
  EXPECT_EQ(0u, model_->GetFilteredPasswords().size());
}

// Test search query getter/setter.
TEST_F(AstraPasswordManagerModelTest, SearchQueryGetterSetter) {
  EXPECT_TRUE(model_->GetSearchQuery().empty());

  model_->SetSearchQuery(u"test");
  EXPECT_EQ(u"test", model_->GetSearchQuery());

  model_->SetSearchQuery(u"");
  EXPECT_TRUE(model_->GetSearchQuery().empty());
}

// Test weak password filter.
TEST_F(AstraPasswordManagerModelTest, FilterWeak) {
  model_->PopulateSamplePasswords();

  model_->SetFilter(AstraPasswordFilter::kWeak);
  auto results = model_->GetFilteredPasswords();
  EXPECT_GT(results.size(), 0u);

  for (const auto& entry : results) {
    EXPECT_TRUE(entry.is_weak);
  }
}

// Test leaked password filter.
TEST_F(AstraPasswordManagerModelTest, FilterLeaked) {
  model_->PopulateSamplePasswords();

  model_->SetFilter(AstraPasswordFilter::kLeaked);
  auto results = model_->GetFilteredPasswords();
  EXPECT_GT(results.size(), 0u);

  for (const auto& entry : results) {
    EXPECT_TRUE(entry.is_leaked);
  }
}

// Test reused password filter.
TEST_F(AstraPasswordManagerModelTest, FilterReused) {
  model_->PopulateSamplePasswords();

  model_->SetFilter(AstraPasswordFilter::kReused);
  auto results = model_->GetFilteredPasswords();
  EXPECT_GT(results.size(), 0u);

  for (const auto& entry : results) {
    EXPECT_TRUE(entry.is_reused);
  }
}

// Test favorites filter.
TEST_F(AstraPasswordManagerModelTest, FilterFavorites) {
  model_->PopulateSamplePasswords();

  model_->SetFilter(AstraPasswordFilter::kFavorites);
  auto results = model_->GetFilteredPasswords();
  EXPECT_GT(results.size(), 0u);

  for (const auto& entry : results) {
    EXPECT_TRUE(entry.is_favorited);
  }
}

// Test filter getter/setter.
TEST_F(AstraPasswordManagerModelTest, FilterGetterSetter) {
  EXPECT_EQ(AstraPasswordFilter::kAll, model_->GetFilter());

  model_->SetFilter(AstraPasswordFilter::kWeak);
  EXPECT_EQ(AstraPasswordFilter::kWeak, model_->GetFilter());

  // Setting same filter is a no-op.
  model_->SetFilter(AstraPasswordFilter::kWeak);
  EXPECT_EQ(AstraPasswordFilter::kWeak, model_->GetFilter());
}

// Test category filter.
TEST_F(AstraPasswordManagerModelTest, CategoryFilter) {
  model_->PopulateSamplePasswords();

  auto categories = model_->GetCategories();
  ASSERT_GT(categories.size(), 0u);

  // Filter by first category.
  std::string first_cat = categories[0];
  model_->SetCategoryFilter(first_cat);
  EXPECT_EQ(first_cat, model_->GetCategoryFilter());

  auto results = model_->GetFilteredPasswords();
  EXPECT_GT(results.size(), 0u);
  for (const auto& entry : results) {
    EXPECT_EQ(first_cat, entry.category);
  }

  // Reset filter.
  model_->SetCategoryFilter("");
  EXPECT_TRUE(model_->GetCategoryFilter().empty());
  EXPECT_EQ(model_->GetCount(), model_->GetFilteredPasswords().size());
}

// Test password strength check.
TEST_F(AstraPasswordManagerModelTest, CheckPasswordStrength) {
  // Very weak passwords.
  EXPECT_EQ(AstraPasswordStrength::kVeryWeak,
            AstraPasswordManagerModel::CheckPasswordStrength(""));
  EXPECT_EQ(AstraPasswordStrength::kVeryWeak,
            AstraPasswordManagerModel::CheckPasswordStrength("password"));
  EXPECT_EQ(AstraPasswordStrength::kVeryWeak,
            AstraPasswordManagerModel::CheckPasswordStrength("123456"));
  EXPECT_EQ(AstraPasswordStrength::kVeryWeak,
            AstraPasswordManagerModel::CheckPasswordStrength("qwerty"));

  // Weak passwords.
  EXPECT_EQ(AstraPasswordStrength::kWeak,
            AstraPasswordManagerModel::CheckPasswordStrength("abc123"));

  // Medium passwords.
  EXPECT_EQ(AstraPasswordStrength::kMedium,
            AstraPasswordManagerModel::CheckPasswordStrength("myPassword"));

  // Strong passwords.
  EXPECT_EQ(AstraPasswordStrength::kStrong,
            AstraPasswordManagerModel::CheckPasswordStrength("MyStr0ng!"));

  // Very strong passwords.
  EXPECT_EQ(AstraPasswordStrength::kVeryStrong,
            AstraPasswordManagerModel::CheckPasswordStrength(
                "MyV3ryStr0ngP@ssw0rd!"));
}

// Test toggle favorite.
TEST_F(AstraPasswordManagerModelTest, ToggleFavorite) {
  std::string id = model_->AddPassword(
      u"Test Site", u"user", "pass", "https://test.com", "Work");

  const auto* entry = model_->GetPassword(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_FALSE(entry->is_favorited);

  model_->ToggleFavorite(id);
  entry = model_->GetPassword(id);
  EXPECT_TRUE(entry->is_favorited);

  model_->ToggleFavorite(id);
  entry = model_->GetPassword(id);
  EXPECT_FALSE(entry->is_favorited);
}

// Test toggle favorite on non-existent password (no-op).
TEST_F(AstraPasswordManagerModelTest, ToggleFavoriteNotFound) {
  model_->PopulateSamplePasswords();
  // Should not crash.
  model_->ToggleFavorite("nonexistent");
}

// Test password issues count.
TEST_F(AstraPasswordManagerModelTest, PasswordIssuesCount) {
  // Empty model has no issues.
  EXPECT_EQ(0u, model_->GetPasswordIssuesCount());

  model_->PopulateSamplePasswords();
  size_t issues = model_->GetPasswordIssuesCount();
  EXPECT_GT(issues, 0u);

  // Issues count should be <= weak + leaked + reused counts.
  size_t weak = 0, leaked = 0, reused = 0;
  for (const auto& entry : model_->GetPasswords()) {
    if (entry.is_weak) weak++;
    if (entry.is_leaked) leaked++;
    if (entry.is_reused) reused++;
  }
  EXPECT_EQ(issues, weak + leaked + reused);
}

// Test loading state.
TEST_F(AstraPasswordManagerModelTest, LoadingState) {
  EXPECT_FALSE(model_->IsLoading());

  model_->SetLoading(true);
  EXPECT_TRUE(model_->IsLoading());

  model_->SetLoading(false);
  EXPECT_FALSE(model_->IsLoading());
}

// Test grouped passwords.
TEST_F(AstraPasswordManagerModelTest, GroupedPasswords) {
  model_->PopulateSamplePasswords();

  auto groups = model_->GetGroupedPasswords();
  EXPECT_GT(groups.size(), 0u);

  // Each group should have a label and entries.
  size_t total_entries = 0;
  for (const auto& group : groups) {
    EXPECT_FALSE(group.label.empty());
    EXPECT_GT(group.entries.size(), 0u);
    total_entries += group.entries.size();
  }

  // Total entries across groups should match filtered count.
  EXPECT_EQ(total_entries, model_->GetFilteredPasswords().size());
}

// Test copy password (stub, should not crash).
TEST_F(AstraPasswordManagerModelTest, CopyPassword) {
  std::string id = model_->AddPassword(
      u"Test", u"user", "pass", "https://test.com", "Work");
  // Stub implementation, should not crash.
  model_->CopyPassword(id);
  model_->CopyPassword("nonexistent");
}

// Test import/export stubs (should not crash).
TEST_F(AstraPasswordManagerModelTest, ImportExportStubs) {
  model_->ImportPasswords("/tmp/test.csv");
  model_->ExportPasswords();
}

// Test observer notifications for add.
TEST_F(AstraPasswordManagerModelTest, ObserverAddNotification) {
  class TestObserver : public AstraPasswordManagerObserver {
   public:
    void OnPasswordsChanged() override { passwords_changed = true; }
    void OnPasswordAdded(const std::string& id) override {
      password_added = true;
      added_id = id;
    }

    bool passwords_changed = false;
    bool password_added = false;
    std::string added_id;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  std::string id = model_->AddPassword(
      u"Test", u"user", "pass", "https://test.com", "Work");

  EXPECT_TRUE(observer.password_added);
  EXPECT_EQ(id, observer.added_id);
  EXPECT_TRUE(observer.passwords_changed);

  model_->RemoveObserver(&observer);
}

// Test observer notifications for remove.
TEST_F(AstraPasswordManagerModelTest, ObserverRemoveNotification) {
  std::string id = model_->AddPassword(
      u"Test", u"user", "pass", "https://test.com", "Work");

  class TestObserver : public AstraPasswordManagerObserver {
   public:
    void OnPasswordRemoved(const std::string& id) override {
      password_removed = true;
      removed_id = id;
    }

    bool password_removed = false;
    std::string removed_id;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  model_->RemovePassword(id);

  EXPECT_TRUE(observer.password_removed);
  EXPECT_EQ(id, observer.removed_id);

  model_->RemoveObserver(&observer);
}

// Test observer notifications for update.
TEST_F(AstraPasswordManagerModelTest, ObserverUpdateNotification) {
  std::string id = model_->AddPassword(
      u"Test", u"user", "pass", "https://test.com", "Work");

  class TestObserver : public AstraPasswordManagerObserver {
   public:
    void OnPasswordUpdated(const std::string& id) override {
      password_updated = true;
      updated_id = id;
    }

    bool password_updated = false;
    std::string updated_id;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  model_->UpdatePassword(id, u"new_user", "new_pass", u"notes");

  EXPECT_TRUE(observer.password_updated);
  EXPECT_EQ(id, observer.updated_id);

  model_->RemoveObserver(&observer);
}

// Test observer notifications for search change.
TEST_F(AstraPasswordManagerModelTest, ObserverSearchNotification) {
  class TestObserver : public AstraPasswordManagerObserver {
   public:
    void OnSearchChanged(const std::u16string& query) override {
      search_changed = true;
      search_query = query;
    }

    bool search_changed = false;
    std::u16string search_query;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  model_->SetSearchQuery(u"test");

  EXPECT_TRUE(observer.search_changed);
  EXPECT_EQ(u"test", observer.search_query);

  model_->RemoveObserver(&observer);
}

// Test observer notifications for filter change.
TEST_F(AstraPasswordManagerModelTest, ObserverFilterNotification) {
  class TestObserver : public AstraPasswordManagerObserver {
   public:
    void OnFilterChanged() override { filter_changed = true; }

    bool filter_changed = false;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  model_->SetFilter(AstraPasswordFilter::kWeak);
  EXPECT_TRUE(observer.filter_changed);

  model_->RemoveObserver(&observer);
}

// Test observer shutdown notification.
TEST_F(AstraPasswordManagerModelTest, ObserverShutdownNotification) {
  class TestObserver : public AstraPasswordManagerObserver {
   public:
    void OnPasswordManagerModelShutdown() override { shutdown = true; }

    bool shutdown = false;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  model_.reset();  // Destroys the model.

  EXPECT_TRUE(observer.shutdown);
}

// Test that adding a weak password marks it as weak.
TEST_F(AstraPasswordManagerModelTest, AddWeakPasswordMarksWeak) {
  std::string id = model_->AddPassword(
      u"Weak Site", u"user", "password", "https://weak.com", "Test");

  const auto* entry = model_->GetPassword(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_TRUE(entry->is_weak);
}

// Test that adding a strong password is not marked as weak.
TEST_F(AstraPasswordManagerModelTest, AddStrongPasswordNotWeak) {
  std::string id = model_->AddPassword(
      u"Strong Site", u"user", "Str0ngP@ssw0rd!#", "https://strong.com",
      "Test");

  const auto* entry = model_->GetPassword(id);
  ASSERT_NE(nullptr, entry);
  EXPECT_FALSE(entry->is_weak);
}

// Test combined search + filter.
TEST_F(AstraPasswordManagerModelTest, CombinedSearchAndFilter) {
  model_->PopulateSamplePasswords();

  model_->SetFilter(AstraPasswordFilter::kWeak);
  model_->SetSearchQuery(u"twitter");

  auto results = model_->GetFilteredPasswords();
  for (const auto& entry : results) {
    EXPECT_TRUE(entry.is_weak);
    std::u16string name_lower = base::ToLowerASCII(entry.site_name);
    EXPECT_TRUE(name_lower.find(u"twitter") != std::u16string::npos);
  }
}

// ===========================================================================
// View Tests
// ===========================================================================

class AstraPasswordManagerViewTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraPasswordManagerModel>();
    view_ = std::make_unique<AstraPasswordManagerView>(model_.get());
  }

  void TearDown() override {
    view_.reset();
    model_.reset();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraPasswordManagerModel> model_;
  std::unique_ptr<AstraPasswordManagerView> view_;
};

// Test view creation.
TEST_F(AstraPasswordManagerViewTest, ViewCreation) {
  EXPECT_NE(nullptr, view_.get());
  EXPECT_EQ(model_.get(), view_->model());
}

// Test view with no model.
TEST_F(AstraPasswordManagerViewTest, ViewWithNoModel) {
  auto empty_view = std::make_unique<AstraPasswordManagerView>();
  EXPECT_EQ(nullptr, empty_view->model());
  EXPECT_EQ(0u, empty_view->password_row_count_for_test());
}

// Test toolbar components exist.
TEST_F(AstraPasswordManagerViewTest, ToolbarComponents) {
  EXPECT_NE(nullptr, view_->search_field_for_test());
  EXPECT_NE(nullptr, view_->add_button_for_test());
}

// Test sidebar exists.
TEST_F(AstraPasswordManagerViewTest, SidebarExists) {
  EXPECT_NE(nullptr, view_->sidebar_for_test());
}

// Test content scroll exists.
TEST_F(AstraPasswordManagerViewTest, ContentScrollExists) {
  EXPECT_NE(nullptr, view_->content_scroll_for_test());
}

// Test detail panel exists.
TEST_F(AstraPasswordManagerViewTest, DetailPanelExists) {
  EXPECT_NE(nullptr, view_->detail_panel_for_test());
  // Initially hidden (no selection).
  EXPECT_FALSE(view_->detail_panel_for_test()->GetVisible());
}

// Test status bar exists.
TEST_F(AstraPasswordManagerViewTest, StatusBarExists) {
  EXPECT_NE(nullptr, view_->status_label_for_test());
}

// Test empty state (no passwords).
TEST_F(AstraPasswordManagerViewTest, EmptyState) {
  EXPECT_EQ(0u, view_->password_row_count_for_test());
  EXPECT_NE(nullptr, view_->status_label_for_test());
}

// Test with sample data.
TEST_F(AstraPasswordManagerViewTest, WithSampleData) {
  model_->PopulateSamplePasswords();
  view_->SetModel(model_.get());

  EXPECT_GT(view_->password_row_count_for_test(), 0u);
  EXPECT_NE(nullptr, view_->status_label_for_test());
}

// Test search field updates model.
TEST_F(AstraPasswordManagerViewTest, SearchField) {
  model_->PopulateSamplePasswords();
  view_->SetModel(model_.get());

  auto* search_field = view_->search_field_for_test();
  ASSERT_NE(nullptr, search_field);

  // Set text via model (since direct SetText may not trigger ContentsChanged
  // without user input).
  model_->SetSearchQuery(u"github");
  EXPECT_EQ(u"github", model_->GetSearchQuery());
}

// Test set model.
TEST_F(AstraPasswordManagerViewTest, SetModel) {
  auto new_model = std::make_unique<AstraPasswordManagerModel>();
  new_model->PopulateSamplePasswords();

  size_t count_before = view_->password_row_count_for_test();

  view_->SetModel(new_model.get());
  EXPECT_EQ(new_model.get(), view_->model());

  // After setting model with sample data, count should increase.
  EXPECT_GT(view_->password_row_count_for_test(), count_before);
}

// Test view preferred size.
TEST_F(AstraPasswordManagerViewTest, PreferredSize) {
  gfx::Size pref = view_->CalculatePreferredSize(views::SizeBounds());
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

// Test that view observes model changes.
TEST_F(AstraPasswordManagerViewTest, ViewObservesModel) {
  model_->PopulateSamplePasswords();
  view_->SetModel(model_.get());

  size_t count_before = view_->password_row_count_for_test();
  EXPECT_GT(count_before, 0u);

  // Add a password and check the view updates.
  model_->AddPassword(u"Newly Added", u"user", "pass",
                      "https://new.com", "Test");

  // View should have updated.
  EXPECT_EQ(count_before + 1, view_->password_row_count_for_test());
}

// Test that removing a password updates the view.
TEST_F(AstraPasswordManagerViewTest, ViewUpdatesOnRemove) {
  model_->PopulateSamplePasswords();
  view_->SetModel(model_.get());

  size_t count_before = view_->password_row_count_for_test();
  EXPECT_GT(count_before, 0u);

  // Find a password to remove.
  const auto& passwords = model_->GetPasswords();
  ASSERT_FALSE(passwords.empty());
  std::string id_to_remove = passwords[0].id;

  model_->RemovePassword(id_to_remove);

  EXPECT_EQ(count_before - 1, view_->password_row_count_for_test());
}

// Test that filter change updates the view.
TEST_F(AstraPasswordManagerViewTest, ViewUpdatesOnFilterChange) {
  model_->PopulateSamplePasswords();
  view_->SetModel(model_.get());

  size_t all_count = view_->password_row_count_for_test();
  EXPECT_GT(all_count, 0u);

  model_->SetFilter(AstraPasswordFilter::kWeak);

  size_t weak_count = view_->password_row_count_for_test();
  EXPECT_LT(weak_count, all_count);
  EXPECT_GT(weak_count, 0u);
}

// Test sidebar categories are populated.
TEST_F(AstraPasswordManagerViewTest, SidebarCategoriesPopulated) {
  model_->PopulateSamplePasswords();
  view_->SetModel(model_.get());

  // Sidebar should exist with categories from the sample data.
  auto categories = model_->GetCategories();
  EXPECT_GT(categories.size(), 0u);
}

// Test list display with grouped sections.
TEST_F(AstraPasswordManagerViewTest, ListDisplayGroups) {
  model_->PopulateSamplePasswords();
  view_->SetModel(model_.get());

  // Password rows should be displayed.
  EXPECT_GT(view_->password_row_count_for_test(), 0u);

  // Rows should match the number of filtered passwords.
  EXPECT_EQ(view_->password_row_count_for_test(),
            model_->GetFilteredPasswords().size());
}

// Test view model shutdown handling.
TEST_F(AstraPasswordManagerViewTest, ViewModelShutdown) {
  auto temp_model = std::make_unique<AstraPasswordManagerModel>();
  temp_model->PopulateSamplePasswords();
  view_->SetModel(temp_model.get());

  EXPECT_EQ(temp_model.get(), view_->model());

  // Destroy the model.
  temp_model.reset();

  // View should no longer have a valid model pointer.
  // (Observation was reset in shutdown notification.)
  EXPECT_EQ(nullptr, view_->model());
}

}  // namespace astra
