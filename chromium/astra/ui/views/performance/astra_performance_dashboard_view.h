// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PERFORMANCE_ASTRA_PERFORMANCE_DASHBOARD_VIEW_H_
#define ASTRA_UI_VIEWS_PERFORMANCE_ASTRA_PERFORMANCE_DASHBOARD_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class ScrollView;
}  // namespace views

namespace astra {

// =========================================================================
// AstraProcessRowView — single process / tab memory row
// =========================================================================
//
// A row in the process list showing tab title, domain, memory usage,
// and a mini bar chart.
//
// Layout:
//   +-------------------------------------------+
//   |  📄 Tab Title              128 MB  ▬▬▬▬  |
//   |     example.com                          |
//   +-------------------------------------------+
// =========================================================================

class AstraProcessRowView : public views::View {
 public:
  struct ProcessInfo {
    std::string process_id;
    std::u16string title;
    std::string domain;
    std::string type;  // "tab", "extension", "gpu", "browser", "utility"
    int64_t memory_bytes = 0;
    double cpu_percent = 0.0;
    base::Time last_active;
  };

  explicit AstraProcessRowView(const ProcessInfo& info);
  ~AstraProcessRowView() override;

  AstraProcessRowView(const AstraProcessRowView&) = delete;
  AstraProcessRowView& operator=(const AstraProcessRowView&) = delete;

  const std::string& process_id() const { return process_id_; }

  void UpdateMemory(int64_t memory_bytes, double cpu_percent);

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  void BuildLayout();

  std::string process_id_;
  std::u16string title_;
  std::string domain_;
  std::string type_;
  int64_t memory_bytes_ = 0;
  double cpu_percent_ = 0.0;
  int64_t max_memory_bytes_ = 0;  // For bar scaling.

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> detail_label_ = nullptr;
  raw_ptr<views::Label> memory_label_ = nullptr;
  raw_ptr<views::Label> cpu_label_ = nullptr;
};

// =========================================================================
// AstraPerformanceDashboardView — performance overview panel
// =========================================================================
//
// A bubble / side panel showing memory and CPU usage across processes,
// with stats at the top and a sortable process list.
//
// Layout:
//   +-------------------------------------------+
//   |  Performance Dashboard          [Close]  |
//   +-------------------------------------------+
//   |  🧠 Total Memory: 2.4 GB     💻 CPU: 15% |
//   |  📑 Tabs: 12                🧩 Exts: 8   |
//   |  💤 Sleeping saves: 512 MB                |
//   +-------------------------------------------+
//   |  Top Processes                              |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ 📄 YouTube          320 MB  ███████ │  |
//   |  │    youtube.com       18%            │  |
//   |  └─────────────────────────────────────┘  |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ 📄 Gmail            180 MB  ████    │  |
//   |  │    mail.google.com   5%             │  |
//   |  └─────────────────────────────────────┘  |
//   |  ...                                      |
//   +-------------------------------------------+
//   |  [ Sort by memory ] [ Sort by CPU ]       |
//   |  [ Force sleep heaviest ]                 |
//   +-------------------------------------------+
//
// This is a presentation-only view.  Process data comes from Chromium's
// process metrics (TaskManager / resource_coordinator).
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - views::MdTextButton
//   - Custom Skia drawing for memory bars
// =========================================================================

class AstraPerformanceDashboardView : public views::BubbleDialogDelegateView {
 public:
  using ForceSleepCallback = base::RepeatingClosure;
  using SortChangedCallback =
      base::RepeatingCallback<void(const std::string& sort_by)>;

  enum class SortBy { kMemory, kCpu, kName };

  explicit AstraPerformanceDashboardView(views::View* anchor_view);
  ~AstraPerformanceDashboardView() override;

  AstraPerformanceDashboardView(const AstraPerformanceDashboardView&) = delete;
  AstraPerformanceDashboardView& operator=(
      const AstraPerformanceDashboardView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetProcesses(const std::vector<AstraProcessRowView::ProcessInfo>& processes);

  void SetTotalMemory(int64_t bytes);
  void SetTotalCpu(double percent);
  void SetTabCount(int count);
  void SetExtensionCount(int count);
  void SetMemorySavedBySleep(int64_t bytes);

  void SetSortBy(SortBy sort_by);

  // -- Callbacks -----------------------------------------------------------

  void SetForceSleepCallback(ForceSleepCallback callback);
  void SetSortChangedCallback(SortChangedCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildStatsSection();
  void BuildProcessList();
  void BuildActionButtons();

  void RefreshProcessList();
  void RefreshStats();

  void OnSortByMemory();
  void OnSortByCpu();
  void OnForceSleepHeaviest();

  // Process data.
  std::vector<AstraProcessRowView::ProcessInfo> processes_;
  SortBy sort_by_ = SortBy::kMemory;

  // Stats.
  int64_t total_memory_bytes_ = 0;
  double total_cpu_percent_ = 0.0;
  int tab_count_ = 0;
  int extension_count_ = 0;
  int64_t memory_saved_by_sleep_ = 0;

  // Callbacks.
  ForceSleepCallback force_sleep_callback_;
  SortChangedCallback sort_changed_callback_;

  // Child views.
  raw_ptr<views::Label> total_memory_label_ = nullptr;
  raw_ptr<views::Label> total_cpu_label_ = nullptr;
  raw_ptr<views::Label> tab_count_label_ = nullptr;
  raw_ptr<views::Label> ext_count_label_ = nullptr;
  raw_ptr<views::Label> memory_saved_label_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> process_list_ = nullptr;
  raw_ptr<views::MdTextButton> sort_memory_button_ = nullptr;
  raw_ptr<views::MdTextButton> sort_cpu_button_ = nullptr;
  raw_ptr<views::MdTextButton> force_sleep_button_ = nullptr;

  // Process rows (owned by process_list_).
  std::vector<raw_ptr<AstraProcessRowView>> process_rows_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PERFORMANCE_ASTRA_PERFORMANCE_DASHBOARD_VIEW_H_
