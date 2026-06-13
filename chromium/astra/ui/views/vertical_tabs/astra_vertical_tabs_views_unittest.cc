// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/vertical_tabs/astra_vertical_tab_strip_view.h"
#include "astra/ui/views/vertical_tabs/astra_vertical_tab_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraVerticalTabStripViewTest
// ===========================================================================

class AstraVerticalTabStripViewTest : public testing::Test {
 protected:
  void SetUp() override {
    strip_ = std::make_unique<AstraVerticalTabStripView>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraVerticalTabStripView> strip_;
};

// Test creation.
TEST_F(AstraVerticalTabStripViewTest, Creation) {
  EXPECT_EQ(0u, strip_->GetTabCount());
  EXPECT_EQ(0u, strip_->GetPinnedTabCount());
  EXPECT_TRUE(strip_->GetActiveTabId().empty());
  EXPECT_FALSE(strip_->IsCollapsed());
  EXPECT_GT(strip_->GetStripWidth(), 0);
}

// Test populate sample tabs.
TEST_F(AstraVerticalTabStripViewTest, PopulateSampleTabs) {
  strip_->PopulateSampleTabs();
  EXPECT_GT(strip_->GetTabCount(), 5u);
  EXPECT_GT(strip_->GetPinnedTabCount(), 0u);
  EXPECT_FALSE(strip_->GetActiveTabId().empty());
}

// Test add tab.
TEST_F(AstraVerticalTabStripViewTest, AddTab) {
  AstraVerticalTabData tab;
  tab.id = "test_tab_1";
  tab.title = u"Test Tab";
  tab.url = u"https://example.com";
  tab.is_active = true;

  strip_->AddTab(tab);
  EXPECT_EQ(1u, strip_->GetTabCount());
  EXPECT_EQ("test_tab_1", strip_->GetActiveTabId());
}

// Test remove tab.
TEST_F(AstraVerticalTabStripViewTest, RemoveTab) {
  AstraVerticalTabData tab;
  tab.id = "tab_to_remove";
  tab.title = u"Remove Me";

  strip_->AddTab(tab);
  EXPECT_EQ(1u, strip_->GetTabCount());

  strip_->RemoveTab("tab_to_remove");
  EXPECT_EQ(0u, strip_->GetTabCount());

  // Remove non-existent should be no-op.
  strip_->RemoveTab("nonexistent");
  EXPECT_EQ(0u, strip_->GetTabCount());
}

// Test activate tab.
TEST_F(AstraVerticalTabStripViewTest, ActivateTab) {
  AstraVerticalTabData tab1;
  tab1.id = "tab1";
  tab1.title = u"Tab 1";
  tab1.is_active = true;
  strip_->AddTab(tab1);

  AstraVerticalTabData tab2;
  tab2.id = "tab2";
  tab2.title = u"Tab 2";
  tab2.is_active = false;
  strip_->AddTab(tab2);

  EXPECT_EQ("tab1", strip_->GetActiveTabId());

  strip_->ActivateTab("tab2");
  EXPECT_EQ("tab2", strip_->GetActiveTabId());

  // Verify tab1 is no longer active.
  const auto& tabs = strip_->GetTabs();
  for (const auto& t : tabs) {
    if (t.id == "tab1") {
      EXPECT_FALSE(t.is_active);
    }
  }
}

// Test pinned tabs.
TEST_F(AstraVerticalTabStripViewTest, PinnedTabs) {
  strip_->PopulateSampleTabs();
  size_t pinned_count = strip_->GetPinnedTabCount();
  EXPECT_GT(pinned_count, 0u);
  EXPECT_LT(pinned_count, strip_->GetTabCount());
}

// Test collapse/expand.
TEST_F(AstraVerticalTabStripViewTest, CollapseExpand) {
  EXPECT_FALSE(strip_->IsCollapsed());
  int expanded_width = strip_->GetStripWidth();

  strip_->SetCollapsed(true);
  EXPECT_TRUE(strip_->IsCollapsed());
  EXPECT_LT(strip_->GetStripWidth(), expanded_width);

  strip_->SetCollapsed(false);
  EXPECT_FALSE(strip_->IsCollapsed());
  EXPECT_EQ(expanded_width, strip_->GetStripWidth());

  // Test toggle.
  strip_->ToggleCollapsed();
  EXPECT_TRUE(strip_->IsCollapsed());
  strip_->ToggleCollapsed();
  EXPECT_FALSE(strip_->IsCollapsed());
}

// Test set strip width.
TEST_F(AstraVerticalTabStripViewTest, StripWidth) {
  int default_width = strip_->GetStripWidth();

  strip_->SetStripWidth(320);
  EXPECT_EQ(320, strip_->GetStripWidth());

  // Collapsed overrides width.
  strip_->SetCollapsed(true);
  EXPECT_LT(strip_->GetStripWidth(), 320);

  strip_->SetCollapsed(false);
  EXPECT_EQ(320, strip_->GetStripWidth());
}

// Test update tab.
TEST_F(AstraVerticalTabStripViewTest, UpdateTab) {
  AstraVerticalTabData tab;
  tab.id = "update_test";
  tab.title = u"Original Title";
  tab.is_pinned = false;
  strip_->AddTab(tab);

  AstraVerticalTabData updated = tab;
  updated.title = u"New Title";
  updated.is_audible = true;
  strip_->UpdateTab(updated);

  const auto* found = strip_->GetTabs().end();
  for (const auto& t : strip_->GetTabs()) {
    if (t.id == "update_test") {
      EXPECT_EQ(u"New Title", t.title);
      EXPECT_TRUE(t.is_audible);
      break;
    }
  }
}

// Test observer pattern.
TEST_F(AstraVerticalTabStripViewTest, Observer) {
  class TestObserver : public AstraVerticalTabStripObserver {
   public:
    void OnTabActivated(const std::string& tab_id) override {
      activated_count_++;
      last_activated_ = tab_id;
    }
    void OnTabClosed(const std::string& tab_id) override {
      closed_count_++;
      last_closed_ = tab_id;
    }
    void OnNewTabRequested() override {
      new_tab_count_++;
    }
    void OnTabStripCollapsed(bool collapsed) override {
      collapsed_count_++;
      last_collapsed_ = collapsed;
    }

    int activated_count_ = 0;
    int closed_count_ = 0;
    int new_tab_count_ = 0;
    int collapsed_count_ = 0;
    std::string last_activated_;
    std::string last_closed_;
    bool last_collapsed_ = false;
  };

  TestObserver observer;
  strip_->AddObserver(&observer);

  // Add a tab.
  AstraVerticalTabData tab;
  tab.id = "observer_test";
  tab.title = u"Observer Test";
  strip_->AddTab(tab);
  EXPECT_EQ(0, observer.activated_count_);  // Add doesn't activate automatically

  // Activate.
  strip_->ActivateTab("observer_test");
  EXPECT_EQ(1, observer.activated_count_);
  EXPECT_EQ("observer_test", observer.last_activated_);

  // Collapse.
  strip_->SetCollapsed(true);
  EXPECT_EQ(1, observer.collapsed_count_);
  EXPECT_TRUE(observer.last_collapsed_);

  // Remove.
  strip_->RemoveTab("observer_test");
  EXPECT_EQ(1, observer.closed_count_);
  EXPECT_EQ("observer_test", observer.last_closed_);

  strip_->RemoveObserver(&observer);
}

// Test accessors for testing.
TEST_F(AstraVerticalTabStripViewTest, ViewAccessors) {
  strip_->PopulateSampleTabs();

  EXPECT_NE(nullptr, strip_->search_field());
  EXPECT_NE(nullptr, strip_->new_tab_button());
  EXPECT_NE(nullptr, strip_->collapse_button());
  EXPECT_NE(nullptr, strip_->pinned_tab_container());
  EXPECT_NE(nullptr, strip_->scroll_view());
  EXPECT_NE(nullptr, strip_->tab_list());
}

// Test tabs data is accessible.
TEST_F(AstraVerticalTabStripViewTest, GetTabs) {
  AstraVerticalTabData tab;
  tab.id = "data_test";
  tab.title = u"Data Test";
  tab.url = u"https://test.com";
  tab.is_audible = true;
  strip_->AddTab(tab);

  const auto& tabs = strip_->GetTabs();
  ASSERT_EQ(1u, tabs.size());
  EXPECT_EQ("data_test", tabs[0].id);
  EXPECT_EQ(u"Data Test", tabs[0].title);
  EXPECT_TRUE(tabs[0].is_audible);
}

}  // namespace astra
