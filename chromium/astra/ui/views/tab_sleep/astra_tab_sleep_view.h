// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_SLEEP_ASTRA_TAB_SLEEP_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_SLEEP_ASTRA_TAB_SLEEP_VIEW_H_

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
// AstraSleepingTabItemView — single sleeping tab row
// =========================================================================
//
// A row in the sleeping tabs list. Shows tab title, domain, sleep time,
// memory saved estimate, and a "Wake" button.
//
// Layout:
//   +-------------------------------------------+
//   |  📄 Tab Title              [ Wake up ]   |
//   |     example.com · slept 3h ago           |
//   |     💾 ~15 MB saved                       |
//   +-------------------------------------------+
// =========================================================================

class AstraSleepingTabItemView : public views::View {
 public:
  using WakeCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using CloseCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;

  struct TabInfo {
    std::string tab_id;
    std::u16string title;
    std::string domain;
    std::string favicon_url;
    base::Time sleep_time;
    int64_t memory_saved_bytes = 0;
  };

  AstraSleepingTabItemView(const TabInfo& tab_info,
                           WakeCallback wake_callback,
                           CloseCallback close_callback);
  ~AstraSleepingTabItemView() override;

  AstraSleepingTabItemView(const AstraSleepingTabItemView&) = delete;
  AstraSleepingTabItemView& operator=(
      const AstraSleepingTabItemView&) = delete;

  const std::string& tab_id() const { return tab_id_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();

  std::string tab_id_;
  std::u16string title_;
  std::string domain_;
  base::Time sleep_time_;
  int64_t memory_saved_bytes_ = 0;

  WakeCallback wake_callback_;
  CloseCallback close_callback_;

  raw_ptr<views::Label> favicon_label_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> detail_label_ = nullptr;
  raw_ptr<views::Label> memory_label_ = nullptr;
  raw_ptr<views::MdTextButton> wake_button_ = nullptr;
};

// =========================================================================
// AstraTabSleepView — sleeping tabs management panel
// =========================================================================
//
// A bubble / side panel showing sleeping tabs and sleep settings.
//
// Layout:
//   +-------------------------------------------+
//   |  Sleeping Tabs                 [Close]  |
//   +-------------------------------------------+
//   |  💤 Sleeping tabs free up memory by     |
//   |     unloading inactive tabs.              |
//   +-------------------------------------------+
//   |  Sleeping (5)                              |
//   |  ┌─────────────────────────────────┐     |
//   |  │ 📄 Example Page    [ Wake up ]  │     |
//   |  │    example.com · 3h · 15MB      │     |
//   |  └─────────────────────────────────┘     |
//   |  ┌─────────────────────────────────┐     |
//   |  │ 📄 Another Tab   [ Wake up ]    │     |
//   |  │    site.org · 1d · 22MB         │     |
//   |  └─────────────────────────────────┘     |
//   |  ...                                      |
//   +-------------------------------------------+
//   |  ⚙ Settings:                              |
//   |  [x] Auto-sleep after 1 hour               |
//   |  [x] Show sleep indicator in tab strip    |
//   |  [Wake all]  [Sleep all inactive]          |
//   +-------------------------------------------+
//
// This is a presentation-only view.  Tab sleep state comes from the
// memory saver service layer.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - views::MdTextButton
// =========================================================================

class AstraTabSleepView : public views::BubbleDialogDelegateView {
 public:
  // Callbacks.
  using WakeTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using CloseTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using WakeAllCallback = base::RepeatingClosure;
  using SleepAllInactiveCallback = base::RepeatingClosure;

  explicit AstraTabSleepView(views::View* anchor_view);
  ~AstraTabSleepView() override;

  AstraTabSleepView(const AstraTabSleepView&) = delete;
  AstraTabSleepView& operator=(const AstraTabSleepView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetSleepingTabs(
      const std::vector<AstraSleepingTabItemView::TabInfo>& tabs);

  void SetAutoSleepEnabled(bool enabled);
  void SetAutoSleepMinutes(int minutes);
  void SetShowSleepIndicator(bool show);

  // -- Stats ---------------------------------------------------------------

  void SetTotalMemorySaved(int64_t bytes);
  void SetSleepingTabCount(int count);

  // -- Callbacks -----------------------------------------------------------

  void SetWakeTabCallback(WakeTabCallback callback);
  void SetCloseTabCallback(CloseTabCallback callback);
  void SetWakeAllCallback(WakeAllCallback callback);
  void SetSleepAllInactiveCallback(SleepAllInactiveCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildInfoSection();
  void BuildStatsSection();
  void BuildTabList();
  void BuildSettingsSection();
  void BuildActionButtons();

  void RefreshTabList();
  void RefreshStats();

  void OnWakeTab(const std::string& tab_id);
  void OnCloseTab(const std::string& tab_id);
  void OnWakeAllClicked();
  void OnSleepAllClicked();

  // Sleeping tabs data.
  std::vector<AstraSleepingTabItemView::TabInfo> sleeping_tabs_;

  // Settings.
  bool auto_sleep_enabled_ = true;
  int auto_sleep_minutes_ = 60;
  bool show_sleep_indicator_ = true;

  // Stats.
  int64_t total_memory_saved_ = 0;
  int sleeping_tab_count_ = 0;

  // Callbacks.
  WakeTabCallback wake_tab_callback_;
  CloseTabCallback close_tab_callback_;
  WakeAllCallback wake_all_callback_;
  SleepAllInactiveCallback sleep_all_callback_;

  // Child views.
  raw_ptr<views::Label> info_label_ = nullptr;
  raw_ptr<views::Label> stats_label_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> tab_list_ = nullptr;
  raw_ptr<views::MdTextButton> wake_all_button_ = nullptr;
  raw_ptr<views::MdTextButton> sleep_all_button_ = nullptr;

  // Tab items (owned by tab_list_).
  std::vector<raw_ptr<AstraSleepingTabItemView>> tab_items_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_SLEEP_ASTRA_TAB_SLEEP_VIEW_H_
