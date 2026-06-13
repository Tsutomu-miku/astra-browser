// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_AUDIO_ASTRA_TAB_AUDIO_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_AUDIO_ASTRA_TAB_AUDIO_VIEW_H_

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
class Slider;
}  // namespace views

namespace astra {

// =========================================================================
// AstraAudioTabItemView — single audio-playing tab row
// =========================================================================
//
// A row showing a tab that's playing audio: title, domain, mute toggle,
// and volume slider.
//
// Layout:
//   +-------------------------------------------+
//   |  🔊 Tab Title                              |
//   |     domain.com         [Mute] [Close]   |
//   +-------------------------------------------+
// =========================================================================

class AstraAudioTabItemView : public views::View {
 public:
  using ToggleMuteCallback =
      base::RepeatingCallback<void(const std::string& tab_id, bool muted)>;
  using CloseTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using JumpToTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;

  enum class AudioState { kPlaying, kMuted, kAudible };

  struct TabInfo {
    std::string tab_id;
    std::u16string title;
    std::string domain;
    AudioState audio_state = AudioState::kPlaying;
    bool is_active = false;
    base::Time audio_started;
    bool is_media = false;  // e.g. video/audio tab
    bool is_background = false;
  };

  AstraAudioTabItemView(const TabInfo& info,
                        ToggleMuteCallback mute_callback,
                        CloseTabCallback close_callback,
                        JumpToTabCallback jump_callback);
  ~AstraAudioTabItemView() override;

  AstraAudioTabItemView(const AstraAudioTabItemView&) = delete;
  AstraAudioTabItemView& operator=(const AstraAudioTabItemView&) = delete;

  const std::string& tab_id() const { return tab_id_; }
  bool is_muted() const { return audio_state_ == AudioState::kMuted; }

  void SetMuted(bool muted);

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnMuteToggled();
  void OnCloseClicked();
  void OnTabClicked();

  static std::u16 AudioStateIcon(AudioState state);
  static std::u16 FormatAudioDuration(base::TimeDelta delta);

  std::string tab_id_;
  std::u16string title_;
  std::string domain_;
  AudioState audio_state_ = AudioState::kPlaying;
  bool is_active_ = false;
  base::Time audio_started_;
  bool is_media_ = false;
  bool is_background_ = false;

  ToggleMuteCallback mute_callback_;
  CloseTabCallback close_callback_;
  JumpToTabCallback jump_callback_;

  raw_ptr<views::Label> icon_label_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> domain_label_ = nullptr;
  raw_ptr<views::MdTextButton> mute_button_ = nullptr;
  raw_ptr<views::MdTextButton> close_button_ = nullptr;
};

// =========================================================================
// AstraTabAudioView — audio tab manager panel
// =========================================================================
//
// A bubble showing all tabs currently playing audio, with mute/unmute
// controls, volume, and quick close.
//
// Layout:
//   +-------------------------------------------+
//   |  Audio Tabs                   [Close]    |
//   +-------------------------------------------+
//   |  5 tabs playing audio · 2 muted           |
//   |  [ Mute all ] [ Unmute all ]              |
//   +-------------------------------------------+
//   |  🔊 YouTube — Music Mix                    |
//   |     youtube.com        [Mute] [Close]   |
//   |  🔇 Spotify — Discover Weekly             |
//   |     spotify.com       [Unmute] [Close]  |
//   |  🔊 Video Conference                       |
//   |     meet.google.com     [Mute] [Close]   |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Audio state comes from Chromium's
// TabStripModel and WebContents audio indicators.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - TabStripModel (source of audio state)
//   - content::WebContents (audio indicator)
// =========================================================================

class AstraTabAudioView : public views::BubbleDialogDelegateView {
 public:
  using ToggleMuteCallback =
      base::RepeatingCallback<void(const std::string& tab_id, bool muted)>;
  using CloseTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using JumpToTabCallback =
      base::RepeatingCallback<void(const std::string& tab_id)>;
  using MuteAllCallback = base::RepeatingCallback<void(bool mute)>;

  explicit AstraTabAudioView(views::View* anchor_view);
  ~AstraTabAudioView() override;

  AstraTabAudioView(const AstraTabAudioView&) = delete;
  AstraTabAudioView& operator=(const AstraTabAudioView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetAudioTabs(
      const std::vector<AstraAudioTabItemView::TabInfo>& tabs);

  // -- Callbacks -----------------------------------------------------------

  void SetToggleMuteCallback(ToggleMuteCallback callback);
  void SetCloseTabCallback(CloseTabCallback callback);
  void SetJumpToTabCallback(JumpToTabCallback callback);
  void SetMuteAllCallback(MuteAllCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildSummarySection();
  void BuildTabsList();

  void RefreshTabs();
  void RefreshSummary();

  void OnToggleMute(const std::string& tab_id, bool muted);
  void OnCloseTab(const std::string& tab_id);
  void OnJumpToTab(const std::string& tab_id);
  void OnMuteAll();
  void OnUnmuteAll();

  std::vector<AstraAudioTabItemView::TabInfo> audio_tabs_;
  int playing_count_ = 0;
  int muted_count_ = 0;

  ToggleMuteCallback mute_callback_;
  CloseTabCallback close_callback_;
  JumpToTabCallback jump_callback_;
  MuteAllCallback mute_all_callback_;

  raw_ptr<views::Label> summary_label_ = nullptr;
  raw_ptr<views::MdTextButton> mute_all_button_ = nullptr;
  raw_ptr<views::MdTextButton> unmute_all_button_ = nullptr;
  raw_ptr<views::View> tabs_list_ = nullptr;

  std::vector<raw_ptr<AstraAudioTabItemView>> tab_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_AUDIO_ASTRA_TAB_AUDIO_VIEW_H_
