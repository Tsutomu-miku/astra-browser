// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_SNOOZE_ASTRA_TAB_SNOOZE_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_SNOOZE_ASTRA_TAB_SNOOZE_VIEW_H_

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
// AstraSnoozedTabItemView — single snoozed tab row
// =========================================================================
//
// A row showing a snoozed tab: title, domain, wake time, and action buttons.
//
// Layout:
//   +-------------------------------------------+
//   |  📄 Tab Title        ⏰ 2h 30m left       |
//   |     example.com · wakes at 3:45 PM        |
//   |                  [Wake now] [Edit] [Dismiss]
//   +-------------------------------------------+
// =========================================================================

class AstraSnoozedTabItemView : public views::View {
 public:
  using WakeCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using EditCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using DismissCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;

  struct SnoozedTab {
    std::string tab_id;
    std::u16string title;
    std::string domain;
    std::string url;
    std::string favicon_url;
    base::Time snoozed_at;
    base::Time wake_at;
    std::string snooze_duration_name;  // e.g. "Later today", "Tomorrow"
  };

  AstraSnoozedTabItemView(const SnoozedTab& tab,
                          WakeCallback wake_callback,
                          EditCallback edit_callback,
                          DismissCallback dismiss_callback);
  ~AstraSnoozedTabItemView() override;

  AstraSnoozedTabItemView(const AstraSnoozedTabItemView&) = delete;
  AstraSnoozedTabItemView& operator=(
      const AstraSnoozedTabItemView&) = delete;

  const std::string& tab_id() const { return tab_id_; }

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnWakeClicked();
  void OnEditClicked();
  void OnDismissClicked();

  std::string tab_id_;
  std::u16string title_;
  std::string domain_;
  base::Time wake_at_;
  base::Time snoozed_at_;

  WakeCallback wake_callback_;
  EditCallback edit_callback_;
  DismissCallback dismiss_callback_;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> detail_label_ = nullptr;
  raw_ptr<views::Label> time_left_label_ = nullptr;
  raw_ptr<views::MdTextButton> wake_button_ = nullptr;
  raw_ptr<views::MdTextButton> edit_button_ = nullptr;
  raw_ptr<views::MdTextButton> dismiss_button_ = nullptr;
};

// =========================================================================
// AstraTabSnoozeView — tab snooze / reminder panel
// =========================================================================
//
// A bubble showing snoozed tabs and quick snooze duration options.
// Lets users "snooze" tabs — close them temporarily with a reminder to
// come back later.
//
// Layout for snooze panel (when snoozing a tab):
//   +-------------------------------------------+
//   |  Snooze Tab                   [Close]    |
//   +-------------------------------------------+
//   |  📄 Example Page                          |
//   |     example.com                           |
//   +-------------------------------------------+
//   |  When should it come back?                 |
//   |  [ ⏰ Later today ]   [ 🌅 Tomorrow ]     |
//   |  [ 📅 This weekend ] [ 🎯 Custom... ]     |
//   +-------------------------------------------+
//   |  [ Snooze ]                                |
//   +-------------------------------------------+
//
// Layout for snoozed tabs list:
//   +-------------------------------------------+
//   |  Snoozed Tabs                [Close]    |
//   +-------------------------------------------+
//   |  💤 3 tabs snoozed                        |
//   +-------------------------------------------+
//   |  📄 Example Page      ⏰ 2h left          |
//   |     example.com · wakes 3:45 PM          |
//   |                  [Wake] [Edit] [Dismiss]  |
//   |  ...                                      |
//   +-------------------------------------------+
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::ScrollView
//   - views::MdTextButton
//   - TabStripModel (for tab data)
// =========================================================================

class AstraTabSnoozeView : public views::BubbleDialogDelegateView {
 public:
  // Snooze duration preset.
  struct SnoozePreset {
    std::string preset_id;
    std::u16string label;
    std::string emoji;
    base::TimeDelta duration;
  };

  // Callback when a snooze duration is selected (for snooze panel).
  using SnoozeSelectedCallback =
      base::RepeatingCallback<void(const std::string& preset_id)>;

  // Callbacks for snoozed tabs list.
  using WakeTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using EditSnoozeCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using DismissSnoozeCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;

  // Mode for the view: snooze a tab, or list snoozed tabs.
  enum class Mode { kSnoozeTab, kSnoozedList };

  explicit AstraTabSnoozeView(views::View* anchor_view, Mode mode);
  ~AstraTabSnoozeView() override;

  AstraTabSnoozeView(const AstraTabSnoozeView&) = delete;
  AstraTabSnoozeView& operator=(const AstraTabSnoozeView&) = delete;

  // -- Snooze tab mode -----------------------------------------------------

  void SetSnoozeTabTitle(const std::u16string& title);
  void SetSnoozeTabDomain(const std::string& domain);
  void SetSnoozePresets(const std::vector<SnoozePreset>& presets);

  void SetSnoozeSelectedCallback(SnoozeSelectedCallback callback);

  // -- Snoozed list mode ---------------------------------------------------

  void SetSnoozedTabs(const std::vector<AstraSnoozedTabItemView::SnoozedTab>& tabs);
  void SetSnoozedCount(int count);

  void SetWakeTabCallback(WakeTabCallback callback);
  void SetEditSnoozeCallback(EditSnoozeCallback callback);
  void SetDismissSnoozeCallback(DismissSnoozeCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildSnoozeTabUI();
  void BuildSnoozedListUI();

  void BuildTabHeader();
  void BuildPresetGrid();
  void BuildSnoozeButton();

  void BuildSnoozedListHeader();
  void BuildSnoozedTabList();

  void OnPresetClicked(const std::string& preset_id);
  void OnSnoozeClicked();
  void OnWakeTab(const std::string& tab_id);
  void OnEditSnooze(const std::string& tab_id);
  void OnDismissSnooze(const std::string& tab_id);

  Mode mode_;

  // Snooze tab data.
  std::u16string snooze_tab_title_;
  std::string snooze_tab_domain_;
  std::vector<SnoozePreset> presets_;
  std::string selected_preset_;
  SnoozeSelectedCallback snooze_selected_callback_;

  // Snoozed list data.
  std::vector<AstraSnoozedTabItemView::SnoozedTab> snoozed_tabs_;
  int snoozed_count_ = 0;
  WakeTabCallback wake_tab_callback_;
  EditSnoozeCallback edit_snooze_callback_;
  DismissSnoozeCallback dismiss_snooze_callback_;

  // Common child views.
  raw_ptr<views::Label> tab_title_label_ = nullptr;
  raw_ptr<views::Label> tab_domain_label_ = nullptr;

  // Snooze tab child views.
  raw_ptr<views::View> preset_grid_ = nullptr;
  raw_ptr<views::MdTextButton> snooze_button_ = nullptr;
  std::vector<raw_ptr<views::MdTextButton>> preset_buttons_;

  // Snoozed list child views.
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> tab_list_ = nullptr;
  std::vector<raw_ptr<AstraSnoozedTabItemView>> tab_items_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_SNOOZE_ASTRA_TAB_SNOOZE_VIEW_H_
