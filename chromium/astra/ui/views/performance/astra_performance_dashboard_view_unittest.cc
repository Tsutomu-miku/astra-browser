// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/performance/astra_performance_dashboard_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraPerformanceDashboardViewTest
// ===========================================================================

class AstraPerformanceDashboardViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test process row creation.
TEST_F(AstraPerformanceDashboardViewTest, ProcessRowCreation) {
  AstraProcessRowView::ProcessInfo info;
  info.process_id = "proc_123";
  info.title = u"YouTube";
  info.domain = "youtube.com";
  info.type = "tab";
  info.memory_bytes = 320LL * 1024 * 1024;
  info.cpu_percent = 18.5;
  info.last_active = base::Time::Now();

  auto row = std::make_unique<AstraProcessRowView>(info);

  EXPECT_EQ("proc_123", row->process_id());
}

// Test process row memory update.
TEST_F(AstraPerformanceDashboardViewTest, ProcessRowMemoryUpdate) {
  AstraProcessRowView::ProcessInfo info;
  info.process_id = "proc_update";
  info.title = u"Test Tab";
  info.domain = "test.com";
  info.type = "tab";
  info.memory_bytes = 100LL * 1024 * 1024;
  info.cpu_percent = 5.0;

  auto row = std::make_unique<AstraProcessRowView>(info);

  row->UpdateMemory(200LL * 1024 * 1024, 10.0);
  // Should not crash.
}

// Test various process types.
TEST_F(AstraPerformanceDashboardViewTest, VariousProcessTypes) {
  std::vector<std::string> types = {"tab", "extension", "gpu",
                                     "browser", "utility"};

  for (const auto& type : types) {
    AstraProcessRowView::ProcessInfo info;
    info.process_id = "proc_" + type;
    info.title = base::UTF8ToUTF16(type + " process");
    info.domain = type + ".internal";
    info.type = type;
    info.memory_bytes = 50LL * 1024 * 1024;
    info.cpu_percent = 2.0;

    auto row = std::make_unique<AstraProcessRowView>(info);
    EXPECT_EQ("proc_" + type, row->process_id());
  }
}

// Test dashboard creation.
TEST_F(AstraPerformanceDashboardViewTest, ViewCreation) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test setting processes.
TEST_F(AstraPerformanceDashboardViewTest, SetProcesses) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());

  std::vector<AstraProcessRowView::ProcessInfo> processes;

  AstraProcessRowView::ProcessInfo p1;
  p1.process_id = "tab1";
  p1.title = u"Gmail";
  p1.domain = "mail.google.com";
  p1.type = "tab";
  p1.memory_bytes = 180LL * 1024 * 1024;
  p1.cpu_percent = 5.2;
  processes.push_back(p1);

  AstraProcessRowView::ProcessInfo p2;
  p2.process_id = "tab2";
  p2.title = u"YouTube";
  p2.domain = "youtube.com";
  p2.type = "tab";
  p2.memory_bytes = 320LL * 1024 * 1024;
  p2.cpu_percent = 22.0;
  processes.push_back(p2);

  AstraProcessRowView::ProcessInfo p3;
  p3.process_id = "ext1";
  p3.title = u"Ad Blocker";
  p3.domain = "extension";
  p3.type = "extension";
  p3.memory_bytes = 45LL * 1024 * 1024;
  p3.cpu_percent = 1.5;
  processes.push_back(p3);

  view->SetProcesses(processes);
  // Should not crash.
}

// Test empty process list.
TEST_F(AstraPerformanceDashboardViewTest, EmptyProcessList) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());
  view->SetProcesses({});
  // Should not crash.
}

// Test stats updates.
TEST_F(AstraPerformanceDashboardViewTest, StatsUpdates) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());

  view->SetTotalMemory(2400LL * 1024 * 1024);
  view->SetTotalCpu(15.5);
  view->SetTabCount(12);
  view->SetExtensionCount(8);
  view->SetMemorySavedBySleep(512LL * 1024 * 1024);
  // Should not crash.
}

// Test sort by memory.
TEST_F(AstraPerformanceDashboardViewTest, SortByMemory) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());

  std::vector<AstraProcessRowView::ProcessInfo> processes;

  AstraProcessRowView::ProcessInfo low;
  low.process_id = "low";
  low.title = u"Low Memory";
  low.type = "tab";
  low.memory_bytes = 50LL * 1024 * 1024;
  low.cpu_percent = 5.0;
  processes.push_back(low);

  AstraProcessRowView::ProcessInfo high;
  high.process_id = "high";
  high.title = u"High Memory";
  high.type = "tab";
  high.memory_bytes = 500LL * 1024 * 1024;
  high.cpu_percent = 10.0;
  processes.push_back(high);

  view->SetProcesses(processes);
  view->SetSortBy(AstraPerformanceDashboardView::SortBy::kMemory);
  // Should sort by memory descending.
}

// Test sort by CPU.
TEST_F(AstraPerformanceDashboardViewTest, SortByCpu) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());

  std::vector<AstraProcessRowView::ProcessInfo> processes;

  AstraProcessRowView::ProcessInfo low_cpu;
  low_cpu.process_id = "low_cpu";
  low_cpu.title = u"Low CPU";
  low_cpu.type = "tab";
  low_cpu.memory_bytes = 100LL * 1024 * 1024;
  low_cpu.cpu_percent = 1.0;
  processes.push_back(low_cpu);

  AstraProcessRowView::ProcessInfo high_cpu;
  high_cpu.process_id = "high_cpu";
  high_cpu.title = u"High CPU";
  high_cpu.type = "tab";
  high_cpu.memory_bytes = 80LL * 1024 * 1024;
  high_cpu.cpu_percent = 50.0;
  processes.push_back(high_cpu);

  view->SetProcesses(processes);
  view->SetSortBy(AstraPerformanceDashboardView::SortBy::kCpu);
  // Should sort by CPU descending.
}

// Test force sleep callback.
TEST_F(AstraPerformanceDashboardViewTest, ForceSleepCallback) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());

  bool triggered = false;
  view->SetForceSleepCallback(
      base::BindRepeating(
          [](bool* t) { *t = true; },
          &triggered));
  // Callback can be set without crashing.
}

// Test sort changed callback.
TEST_F(AstraPerformanceDashboardViewTest, SortChangedCallback) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());

  bool triggered = false;
  std::string sort_by;

  view->SetSortChangedCallback(
      base::BindRepeating(
          [](bool* t, std::string* s, const std::string& sort) {
            *t = true;
            *s = sort;
          },
          &triggered, &sort_by));
}

// Test window title.
TEST_F(AstraPerformanceDashboardViewTest, WindowTitle) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());
  EXPECT_EQ(u"Performance", view->GetWindowTitle());
}

// Test theme change doesn't crash.
TEST_F(AstraPerformanceDashboardViewTest, ThemeChange) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());
  view->OnThemeChanged();
}

// Test theme change on process row.
TEST_F(AstraPerformanceDashboardViewTest, ProcessRowThemeChange) {
  AstraProcessRowView::ProcessInfo info;
  info.process_id = "proc_theme";
  info.title = u"Theme Test";
  info.domain = "theme.test";
  info.type = "tab";
  info.memory_bytes = 100LL * 1024 * 1024;

  auto row = std::make_unique<AstraProcessRowView>(info);
  row->OnThemeChanged();
}

// Test many processes.
TEST_F(AstraPerformanceDashboardViewTest, ManyProcesses) {
  auto* view = new AstraPerformanceDashboardView(anchor_view_.get());

  std::vector<AstraProcessRowView::ProcessInfo> processes;
  for (int i = 0; i < 20; ++i) {
    AstraProcessRowView::ProcessInfo p;
    p.process_id = "tab_" + std::to_string(i);
    p.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    p.domain = "site" + std::to_string(i) + ".com";
    p.type = "tab";
    p.memory_bytes = (50LL + i * 10) * 1024 * 1024;
    p.cpu_percent = i * 2.5;
    processes.push_back(p);
  }

  view->SetProcesses(processes);
  // Should handle many processes with scrolling.
}

}  // namespace astra
