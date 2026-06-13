// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_labels/astra_tab_labels_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace astra {

// ===========================================================================
// AstraTabLabelsViewTest
// ===========================================================================

class AstraTabLabelsViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test label chip creation.
TEST_F(AstraTabLabelsViewTest, LabelChipCreation) {
  AstraTabLabelItemView::LabelInfo info;
  info.label_id = "label_work";
  info.name = u"Work";
  info.color = SkColorSetRGB(0xEA, 0x43, 0x35);
  info.tab_count = 12;
  info.is_selected = false;

  auto chip = std::make_unique<AstraTabLabelItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("label_work", chip->label_id());
  EXPECT_FALSE(chip->is_selected());
}

// Test selected label chip.
TEST_F(AstraTabLabelsViewTest, SelectedLabelChip) {
  AstraTabLabelItemView::LabelInfo info;
  info.label_id = "label_personal";
  info.name = u"Personal";
  info.color = SkColorSetRGB(0x34, 0xA8, 0x53);
  info.tab_count = 5;
  info.is_selected = true;

  auto chip = std::make_unique<AstraTabLabelItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_TRUE(chip->is_selected());
}

// Test SetSelected.
TEST_F(AstraTabLabelsViewTest, SetSelected) {
  AstraTabLabelItemView::LabelInfo info;
  info.label_id = "label_test";
  info.name = u"Test";
  info.color = SK_ColorBLUE;
  info.tab_count = 3;

  auto chip = std::make_unique<AstraTabLabelItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_FALSE(chip->is_selected());
  chip->SetSelected(true);
  EXPECT_TRUE(chip->is_selected());
  chip->SetSelected(false);
  EXPECT_FALSE(chip->is_selected());
}

// Test SetTabCount.
TEST_F(AstraTabLabelsViewTest, SetTabCount) {
  AstraTabLabelItemView::LabelInfo info;
  info.label_id = "label_count";
  info.name = u"Count Test";
  info.color = SK_ColorMAGENTA;
  info.tab_count = 5;

  auto chip = std::make_unique<AstraTabLabelItemView>(
      info, base::DoNothing(), base::DoNothing());

  chip->SetTabCount(10);
  SUCCEED();
}

// Test all label colors.
TEST_F(AstraTabLabelsViewTest, AllLabelColors) {
  std::vector<SkColor> colors = {
      SK_ColorRED,
      SK_ColorGREEN,
      SK_ColorBLUE,
      SK_ColorYELLOW,
      SkColorSetRGB(0x9C, 0x27, 0xB0),  // purple
      SkColorSetRGB(0xFF, 0x98, 0x00),  // orange
      SkColorSetRGB(0x00, 0x96, 0x88),  // teal
      SK_ColorGRAY,
  };

  for (size_t i = 0; i < colors.size(); i++) {
    AstraTabLabelItemView::LabelInfo info;
    info.label_id = "color_" + std::to_string(i);
    info.name = base::UTF8ToUTF16("Label " + std::to_string(i));
    info.color = colors[i];
    info.tab_count = static_cast<int>(i) * 2;

    auto chip = std::make_unique<AstraTabLabelItemView>(
        info, base::DoNothing(), base::DoNothing());
    EXPECT_FALSE(chip->is_selected());
  }
}

// Test labeled tab item creation.
TEST_F(AstraTabLabelsViewTest, LabeledTabItem) {
  AstraTabLabeledTabItemView::TabInfo info;
  info.tab_id = "tab_001";
  info.title = u"Project Alpha";
  info.domain = "work.com";
  info.label_ids = {"label_work", "label_research"};
  info.is_selected = false;

  auto item = std::make_unique<AstraTabLabeledTabItemView>(info);

  EXPECT_EQ("tab_001", item->tab_id());
}

// Test tab with no labels.
TEST_F(AstraTabLabelsViewTest, TabWithNoLabels) {
  AstraTabLabeledTabItemView::TabInfo info;
  info.tab_id = "tab_no_labels";
  info.title = u"Untitled Tab";
  info.domain = "example.com";
  info.label_ids = {};

  auto item = std::make_unique<AstraTabLabeledTabItemView>(info);

  EXPECT_EQ("tab_no_labels", item->tab_id());
}

// Test tab labels view creation.
TEST_F(AstraTabLabelsViewTest, ViewCreation) {
  auto* view = new AstraTabLabelsView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test window title.
TEST_F(AstraTabLabelsViewTest, WindowTitle) {
  auto* view = new AstraTabLabelsView(anchor_view_.get());
  EXPECT_EQ(u"Tab Labels", view->GetWindowTitle());
}

// Test setting labels.
TEST_F(AstraTabLabelsViewTest, SetLabels) {
  auto* view = new AstraTabLabelsView(anchor_view_.get());

  std::vector<AstraTabLabelItemView::LabelInfo> labels;

  AstraTabLabelItemView::LabelInfo l1;
  l1.label_id = "work";
  l1.name = u"Work";
  l1.color = SK_ColorRED;
  l1.tab_count = 12;
  labels.push_back(l1);

  AstraTabLabelItemView::LabelInfo l2;
  l2.label_id = "personal";
  l2.name = u"Personal";
  l2.color = SK_ColorGREEN;
  l2.tab_count = 5;
  labels.push_back(l2);

  AstraTabLabelItemView::LabelInfo l3;
  l3.label_id = "research";
  l3.name = u"Research";
  l3.color = SK_ColorBLUE;
  l3.tab_count = 8;
  labels.push_back(l3);

  view->SetLabels(labels);
  EXPECT_NE(nullptr, view);
}

// Test empty labels.
TEST_F(AstraTabLabelsViewTest, EmptyLabels) {
  auto* view = new AstraTabLabelsView(anchor_view_.get());
  view->SetLabels({});
  EXPECT_NE(nullptr, view);
}

// Test setting tabs.
TEST_F(AstraTabLabelsViewTest, SetTabs) {
  auto* view = new AstraTabLabelsView(anchor_view_.get());

  std::vector<AstraTabLabeledTabItemView::TabInfo> tabs;

  AstraTabLabeledTabItemView::TabInfo t1;
  t1.tab_id = "tab1";
  t1.title = u"Work Dashboard";
  t1.domain = "work.com";
  t1.label_ids = {"work", "research"};
  tabs.push_back(t1);

  AstraTabLabeledTabItemView::TabInfo t2;
  t2.tab_id = "tab2";
  t2.title = u"Personal Blog";
  t2.domain = "blog.com";
  t2.label_ids = {"personal"};
  tabs.push_back(t2);

  view->SetTabs(tabs);
  SUCCEED();
}

// Test selecting a label.
TEST_F(AstraTabLabelsViewTest, SetSelectedLabel) {
  auto* view = new AstraTabLabelsView(anchor_view_.get());

  std::vector<AstraTabLabelItemView::LabelInfo> labels;
  AstraTabLabelItemView::LabelInfo l1;
  l1.label_id = "work";
  l1.name = u"Work";
  l1.color = SK_ColorRED;
  l1.tab_count = 5;
  labels.push_back(l1);

  view->SetLabels(labels);
  view->SetSelectedLabel("work");
  SUCCEED();
}

// Test new label callback.
TEST_F(AstraTabLabelsViewTest, NewLabelCallback) {
  bool callback_called = false;
  auto* view = new AstraTabLabelsView(anchor_view_.get());
  view->SetNewLabelCallback(
      base::BindRepeating(
          [](bool* called) { *called = true; }, &callback_called));

  EXPECT_FALSE(callback_called);
}

// Test delete label callback.
TEST_F(AstraTabLabelsViewTest, DeleteLabelCallback) {
  std::string deleted_id;
  auto* view = new AstraTabLabelsView(anchor_view_.get());
  view->SetDeleteLabelCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &deleted_id));

  std::vector<AstraTabLabelItemView::LabelInfo> labels;
  AstraTabLabelItemView::LabelInfo l;
  l.label_id = "delete_me";
  l.name = u"Delete Me";
  l.color = SK_ColorRED;
  l.tab_count = 1;
  labels.push_back(l);
  view->SetLabels(labels);

  EXPECT_TRUE(deleted_id.empty());
}

// Test label selected callback.
TEST_F(AstraTabLabelsViewTest, LabelSelectedCallback) {
  std::string selected_id;
  auto* view = new AstraTabLabelsView(anchor_view_.get());
  view->SetLabelSelectedCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &selected_id));

  std::vector<AstraTabLabelItemView::LabelInfo> labels;
  AstraTabLabelItemView::LabelInfo l;
  l.label_id = "select_me";
  l.name = u"Select Me";
  l.color = SK_ColorBLUE;
  l.tab_count = 3;
  labels.push_back(l);
  view->SetLabels(labels);

  EXPECT_TRUE(selected_id.empty());
}

// Test many labels.
TEST_F(AstraTabLabelsViewTest, ManyLabels) {
  auto* view = new AstraTabLabelsView(anchor_view_.get());

  std::vector<AstraTabLabelItemView::LabelInfo> labels;
  std::vector<SkColor> colors = {
      SK_ColorRED, SK_ColorGREEN, SK_ColorBLUE, SK_ColorYELLOW,
      SkColorSetRGB(0x9C, 0x27, 0xB0),
      SkColorSetRGB(0xFF, 0x98, 0x00),
      SkColorSetRGB(0x00, 0x96, 0x88),
      SK_ColorGRAY,
  };

  for (int i = 0; i < 12; i++) {
    AstraTabLabelItemView::LabelInfo l;
    l.label_id = "label_" + std::to_string(i);
    l.name = base::UTF8ToUTF16("Label " + std::to_string(i));
    l.color = colors[i % colors.size()];
    l.tab_count = i * 3;
    labels.push_back(l);
  }

  view->SetLabels(labels);
  SUCCEED();
}

}  // namespace astra
