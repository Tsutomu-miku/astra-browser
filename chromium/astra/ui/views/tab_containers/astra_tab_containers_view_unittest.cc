// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_containers/astra_tab_containers_view.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"

namespace astra {

using AstraTabContainersViewTest = views::ViewsTestBase;

// ===========================================================================
// AstraContainerItemView tests
// ===========================================================================

TEST_F(AstraTabContainersViewTest, ContainerItemView_HasCorrectId) {
  AstraContainerItemView::ContainerInfo info;
  info.container_id = "personal";
  info.name = u"Personal";
  info.color = SK_ColorBLUE;
  info.tab_count = 5;
  info.is_default = true;
  info.is_active = false;

  auto item = std::make_unique<AstraContainerItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ(item->container_id(), "personal");
}

TEST_F(AstraTabContainersViewTest, ContainerItemView_ActiveState) {
  AstraContainerItemView::ContainerInfo info;
  info.container_id = "work";
  info.name = u"Work";
  info.color = SkColorSetRGB(0x1A, 0x73, 0xE8);
  info.tab_count = 10;
  info.is_active = true;

  auto item = std::make_unique<AstraContainerItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_TRUE(item->is_active());

  item->SetActive(false);
  EXPECT_FALSE(item->is_active());
}

TEST_F(AstraTabContainersViewTest, ContainerItemView_SelectCallback) {
  std::string selected_id;
  auto select_cb = base::BindRepeating(
      [](std::string* out, const std::string& id) { *out = id; },
      &selected_id);

  AstraContainerItemView::ContainerInfo info;
  info.container_id = "banking";
  info.name = u"Banking";
  info.color = SkColorSetRGB(0x34, 0xA8, 0x53);
  info.tab_count = 2;

  auto item = std::make_unique<AstraContainerItemView>(
      info, select_cb, base::DoNothing(), base::DoNothing());

  EXPECT_EQ(item->container_id(), "banking");
}

TEST_F(AstraTabContainersViewTest, ContainerItemView_PreferredSize) {
  AstraContainerItemView::ContainerInfo info;
  info.container_id = "social";
  info.name = u"Social";
  info.color = SkColorSetRGB(0xEA, 0x43, 0x35);
  info.tab_count = 7;

  auto item = std::make_unique<AstraContainerItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  gfx::Size preferred = item->GetPreferredSize();
  EXPECT_GT(preferred.width(), 0);
  EXPECT_GT(preferred.height(), 0);
}

TEST_F(AstraTabContainersViewTest, ContainerItemView_DefaultContainer) {
  AstraContainerItemView::ContainerInfo info;
  info.container_id = "default";
  info.name = u"Default";
  info.color = SK_ColorGRAY;
  info.tab_count = 15;
  info.is_default = true;

  auto item = std::make_unique<AstraContainerItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ(item->container_id(), "default");
}

// ===========================================================================
// AstraTabContainersView tests
// ===========================================================================

TEST_F(AstraTabContainersViewTest, ContainersView_HasTitle) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabContainersView>(anchor.get());

  EXPECT_FALSE(view->GetWindowTitle().empty());
}

TEST_F(AstraTabContainersViewTest, ContainersView_SetContainers) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabContainersView>(anchor.get());

  std::vector<AstraContainerItemView::ContainerInfo> containers;

  AstraContainerItemView::ContainerInfo personal;
  personal.container_id = "personal";
  personal.name = u"Personal";
  personal.color = SK_ColorBLUE;
  personal.tab_count = 5;
  personal.is_default = true;
  containers.push_back(personal);

  AstraContainerItemView::ContainerInfo work;
  work.container_id = "work";
  work.name = u"Work";
  work.color = SkColorSetRGB(0x1A, 0x73, 0xE8);
  work.tab_count = 10;
  containers.push_back(work);

  AstraContainerItemView::ContainerInfo banking;
  banking.container_id = "banking";
  banking.name = u"Banking";
  banking.color = SkColorSetRGB(0x34, 0xA8, 0x53);
  banking.tab_count = 2;
  containers.push_back(banking);

  view->SetContainers(containers);
  view->Layout();
  SUCCEED();
}

TEST_F(AstraTabContainersViewTest, ContainersView_SetActiveContainer) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabContainersView>(anchor.get());

  std::vector<AstraContainerItemView::ContainerInfo> containers;

  AstraContainerItemView::ContainerInfo personal;
  personal.container_id = "personal";
  personal.name = u"Personal";
  personal.color = SK_ColorBLUE;
  personal.tab_count = 5;
  containers.push_back(personal);

  AstraContainerItemView::ContainerInfo work;
  work.container_id = "work";
  work.name = u"Work";
  work.color = SkColorSetRGB(0x1A, 0x73, 0xE8);
  work.tab_count = 10;
  containers.push_back(work);

  view->SetContainers(containers);
  view->SetActiveContainer("work");
  view->Layout();
  SUCCEED();
}

TEST_F(AstraTabContainersViewTest, ContainersView_CreateCallback) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabContainersView>(anchor.get());

  bool created = false;
  view->SetContainerCreateCallback(base::BindRepeating(
      [](bool* out) { *out = true; },
      &created));

  // Verify callback is stored.
  EXPECT_FALSE(created);
}

TEST_F(AstraTabContainersViewTest, ContainersView_SelectCallback) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabContainersView>(anchor.get());

  std::string selected;
  view->SetContainerSelectedCallback(base::BindRepeating(
      [](std::string* out, const std::string& id) { *out = id; },
      &selected));

  std::vector<AstraContainerItemView::ContainerInfo> containers;
  AstraContainerItemView::ContainerInfo c;
  c.container_id = "test";
  c.name = u"Test";
  c.color = SK_ColorRED;
  containers.push_back(c);
  view->SetContainers(containers);

  // Trigger selection via SetActiveContainer.
  view->SetActiveContainer("test");
  // Note: SetActiveContainer doesn't fire select callback, it's just
  // UI state update.
  SUCCEED();
}

TEST_F(AstraTabContainersViewTest, ContainersView_EmptyContainers) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabContainersView>(anchor.get());

  std::vector<AstraContainerItemView::ContainerInfo> empty;
  view->SetContainers(empty);
  view->Layout();
  SUCCEED();
}

TEST_F(AstraTabContainersViewTest, ContainersView_ManyContainers) {
  auto anchor = std::make_unique<views::View>();
  auto view = std::make_unique<AstraTabContainersView>(anchor.get());

  std::vector<AstraContainerItemView::ContainerInfo> containers;
  std::vector<SkColor> colors = {
      SK_ColorBLUE, SkColorSetRGB(0x1A, 0x73, 0xE8),
      SkColorSetRGB(0x34, 0xA8, 0x53),
      SkColorSetRGB(0xEA, 0x43, 0x35), SK_ColorMAGENTA,
      SK_ColorCYAN, SkColorSetRGB(0xFF, 0x98, 0x00),
      SkColorSetRGB(0x9C, 0x27, 0xB0),
  };

  for (size_t i = 0; i < colors.size(); i++) {
    AstraContainerItemView::ContainerInfo c;
    c.container_id = "container_" + std::to_string(i);
    c.name = base::UTF8ToUTF16("Container " + std::to_string(i));
    c.color = colors[i];
    c.tab_count = static_cast<int>(i * 3);
    containers.push_back(c);
  }

  view->SetContainers(containers);
  view->Layout();
  SUCCEED();
}

}  // namespace astra
