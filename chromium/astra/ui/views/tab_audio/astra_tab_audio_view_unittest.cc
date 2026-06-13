// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_audio/astra_tab_audio_view.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabAudioViewTest
// ===========================================================================

class AstraTabAudioViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test audio tab item creation.
TEST_F(AstraTabAudioViewTest, AudioItemCreation) {
  AstraAudioTabItemView::TabInfo info;
  info.tab_id = "tab_001";
  info.title = u"YouTube — Music Mix";
  info.domain = "youtube.com";
  info.audio_state = AstraAudioTabItemView::AudioState::kPlaying;
  info.is_active = true;
  info.audio_started = base::Time::Now() - base::Minutes(15);

  auto item = std::make_unique<AstraAudioTabItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("tab_001", item->tab_id());
  EXPECT_FALSE(item->is_muted());
}

// Test muted audio tab.
TEST_F(AstraTabAudioViewTest, MutedAudioItem) {
  AstraAudioTabItemView::TabInfo info;
  info.tab_id = "tab_002";
  info.title = u"Spotify — Discover Weekly";
  info.domain = "spotify.com";
  info.audio_state = AstraAudioTabItemView::AudioState::kMuted;
  info.is_active = false;
  info.audio_started = base::Time::Now() - base::Hours(1);

  auto item = std::make_unique<AstraAudioTabItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_TRUE(item->is_muted());
}

// Test audible audio state.
TEST_F(AstraTabAudioViewTest, AudibleAudioItem) {
  AstraAudioTabItemView::TabInfo info;
  info.tab_id = "tab_003";
  info.title = u"Video Conference";
  info.domain = "meet.google.com";
  info.audio_state = AstraAudioTabItemView::AudioState::kAudible;
  info.is_media = true;
  info.audio_started = base::Time::Now() - base::Minutes(45);

  auto item = std::make_unique<AstraAudioTabItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("tab_003", item->tab_id());
  EXPECT_FALSE(item->is_muted());
}

// Test SetMuted.
TEST_F(AstraTabAudioViewTest, SetMuted) {
  AstraAudioTabItemView::TabInfo info;
  info.tab_id = "tab_004";
  info.title = u"Toggle Test";
  info.domain = "test.com";
  info.audio_state = AstraAudioTabItemView::AudioState::kPlaying;
  info.audio_started = base::Time::Now();

  auto item = std::make_unique<AstraAudioTabItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_FALSE(item->is_muted());
  item->SetMuted(true);
  EXPECT_TRUE(item->is_muted());
  item->SetMuted(false);
  EXPECT_FALSE(item->is_muted());
}

// Test media tab.
TEST_F(AstraTabAudioViewTest, MediaTab) {
  AstraAudioTabItemView::TabInfo info;
  info.tab_id = "media_tab";
  info.title = u"Netflix — Movie";
  info.domain = "netflix.com";
  info.audio_state = AstraAudioTabItemView::AudioState::kPlaying;
  info.is_media = true;
  info.audio_started = base::Time::Now() - base::Minutes(30);

  auto item = std::make_unique<AstraAudioTabItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("media_tab", item->tab_id());
}

// Test background audio tab.
TEST_F(AstraTabAudioViewTest, BackgroundAudioTab) {
  AstraAudioTabItemView::TabInfo info;
  info.tab_id = "bg_tab";
  info.title = u"Background Music";
  info.domain = "music.com";
  info.audio_state = AstraAudioTabItemView::AudioState::kPlaying;
  info.is_background = true;
  info.audio_started = base::Time::Now() - base::Hours(2);

  auto item = std::make_unique<AstraAudioTabItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("bg_tab", item->tab_id());
}

// Test audio view creation.
TEST_F(AstraTabAudioViewTest, ViewCreation) {
  auto* view = new AstraTabAudioView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test window title.
TEST_F(AstraTabAudioViewTest, WindowTitle) {
  auto* view = new AstraTabAudioView(anchor_view_.get());
  EXPECT_EQ(u"Audio Tabs", view->GetWindowTitle());
}

// Test setting audio tabs.
TEST_F(AstraTabAudioViewTest, SetAudioTabs) {
  auto* view = new AstraTabAudioView(anchor_view_.get());

  std::vector<AstraAudioTabItemView::TabInfo> tabs;

  AstraAudioTabItemView::TabInfo t1;
  t1.tab_id = "yt";
  t1.title = u"YouTube — Music";
  t1.domain = "youtube.com";
  t1.audio_state = AstraAudioTabItemView::AudioState::kPlaying;
  t1.is_active = true;
  t1.is_media = true;
  t1.audio_started = base::Time::Now();
  tabs.push_back(t1);

  AstraAudioTabItemView::TabInfo t2;
  t2.tab_id = "spot";
  t2.title = u"Spotify — Playlist";
  t2.domain = "spotify.com";
  t2.audio_state = AstraAudioTabItemView::AudioState::kMuted;
  t2.is_active = false;
  t2.audio_started = base::Time::Now() - base::Minutes(20);
  tabs.push_back(t2);

  AstraAudioTabItemView::TabInfo t3;
  t3.tab_id = "meet";
  t3.title = u"Google Meet";
  t3.domain = "meet.google.com";
  t3.audio_state = AstraAudioTabItemView::AudioState::kAudible;
  t3.is_active = false;
  t3.is_media = true;
  t3.audio_started = base::Time::Now() - base::Minutes(45);
  tabs.push_back(t3);

  view->SetAudioTabs(tabs);
  EXPECT_NE(nullptr, view);
}

// Test empty audio tabs.
TEST_F(AstraTabAudioViewTest, EmptyAudioTabs) {
  auto* view = new AstraTabAudioView(anchor_view_.get());
  view->SetAudioTabs({});
  EXPECT_NE(nullptr, view);
}

// Test toggle mute callback.
TEST_F(AstraTabAudioViewTest, ToggleMuteCallback) {
  std::string toggled_tab;
  bool toggled_value = false;

  auto* view = new AstraTabAudioView(anchor_view_.get());
  view->SetToggleMuteCallback(
      base::BindRepeating(
          [](std::string* out_id, bool* out_val,
             const std::string& id, bool val) {
            *out_id = id;
            *out_val = val;
          },
          &toggled_tab, &toggled_value));

  std::vector<AstraAudioTabItemView::TabInfo> tabs;
  AstraAudioTabItemView::TabInfo t;
  t.tab_id = "toggle_test";
  t.title = u"Test Audio";
  t.domain = "test.com";
  t.audio_state = AstraAudioTabItemView::AudioState::kPlaying;
  t.audio_started = base::Time::Now();
  tabs.push_back(t);
  view->SetAudioTabs(tabs);

  EXPECT_TRUE(toggled_tab.empty());
}

// Test close tab callback.
TEST_F(AstraTabAudioViewTest, CloseTabCallback) {
  std::string closed_tab;
  auto* view = new AstraTabAudioView(anchor_view_.get());
  view->SetCloseTabCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &closed_tab));

  std::vector<AstraAudioTabItemView::TabInfo> tabs;
  AstraAudioTabItemView::TabInfo t;
  t.tab_id = "close_me";
  t.title = u"Close Me";
  t.domain = "test.com";
  t.audio_state = AstraAudioTabItemView::AudioState::kPlaying;
  t.audio_started = base::Time::Now();
  tabs.push_back(t);
  view->SetAudioTabs(tabs);

  EXPECT_TRUE(closed_tab.empty());
}

// Test jump to tab callback.
TEST_F(AstraTabAudioViewTest, JumpToTabCallback) {
  std::string jumped_tab;
  auto* view = new AstraTabAudioView(anchor_view_.get());
  view->SetJumpToTabCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &jumped_tab));

  EXPECT_TRUE(jumped_tab.empty());
}

// Test mute all callback.
TEST_F(AstraTabAudioViewTest, MuteAllCallback) {
  bool mute_all_called = false;
  bool mute_value = false;
  auto* view = new AstraTabAudioView(anchor_view_.get());
  view->SetMuteAllCallback(
      base::BindRepeating(
          [](bool* called, bool* out_val, bool val) {
            *called = true;
            *out_val = val;
          },
          &mute_all_called, &mute_value));

  EXPECT_FALSE(mute_all_called);
}

// Test many audio tabs.
TEST_F(AstraTabAudioViewTest, ManyAudioTabs) {
  auto* view = new AstraTabAudioView(anchor_view_.get());

  std::vector<AstraAudioTabItemView::TabInfo> tabs;
  for (int i = 0; i < 10; i++) {
    AstraAudioTabItemView::TabInfo t;
    t.tab_id = "tab_" + std::to_string(i);
    t.title = base::UTF8ToUTF16("Audio Tab " + std::to_string(i));
    t.domain = "site" + std::to_string(i) + ".com";
    t.audio_state =
        (i % 3 == 0) ? AstraAudioTabItemView::AudioState::kMuted
                     : AstraAudioTabItemView::AudioState::kPlaying;
    t.is_active = (i == 0);
    t.audio_started = base::Time::Now() - base::Minutes(i * 5);
    tabs.push_back(t);
  }

  view->SetAudioTabs(tabs);
  SUCCEED();
}

// Test all audio states.
TEST_F(AstraTabAudioViewTest, AllAudioStates) {
  auto* view = new AstraTabAudioView(anchor_view_.get());

  std::vector<AstraAudioTabItemView::TabInfo> tabs;

  AstraAudioTabItemView::TabInfo t1;
  t1.tab_id = "playing";
  t1.title = u"Playing Tab";
  t1.domain = "playing.com";
  t1.audio_state = AstraAudioTabItemView::AudioState::kPlaying;
  t1.audio_started = base::Time::Now();
  tabs.push_back(t1);

  AstraAudioTabItemView::TabInfo t2;
  t2.tab_id = "muted";
  t2.title = u"Muted Tab";
  t2.domain = "muted.com";
  t2.audio_state = AstraAudioTabItemView::AudioState::kMuted;
  t2.audio_started = base::Time::Now();
  tabs.push_back(t2);

  AstraAudioTabItemView::TabInfo t3;
  t3.tab_id = "audible";
  t3.title = u"Audible Tab";
  t3.domain = "audible.com";
  t3.audio_state = AstraAudioTabItemView::AudioState::kAudible;
  t3.audio_started = base::Time::Now();
  tabs.push_back(t3);

  view->SetAudioTabs(tabs);
  SUCCEED();
}

// Test long title (elide behavior).
TEST_F(AstraTabAudioViewTest, LongTitle) {
  AstraAudioTabItemView::TabInfo info;
  info.tab_id = "long_title";
  info.title =
      u"This is a very long tab title that should be elided to fit "
      u"within the audio panel's available width without overflowing";
  info.domain = "very-long-domain-name.example.com";
  info.audio_state = AstraAudioTabItemView::AudioState::kPlaying;
  info.audio_started = base::Time::Now();

  auto item = std::make_unique<AstraAudioTabItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("long_title", item->tab_id());
}

}  // namespace astra
