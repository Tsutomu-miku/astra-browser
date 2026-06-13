// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Combined unit tests for Astra tab hover preview feature.
//
// Tests cover:
//   - Model state and observers (AstraTabHoverModel)
//   - Model settings and persistence
//   - Peek controller lifecycle and state
//   - View rendering and delegate callbacks
//   - Utility methods (formatting, clamping)
//   - Edge cases (null data, empty strings, rapid show/hide)
//   - Observer pattern verification
//   - Settings round-trip via PrefService
//
// Chromium test patterns:
//   - views::ViewsTestBase for view tests
//   - testing::Mock for observer verification
//   - PrefService via TestingPrefStore

#include "astra/ui/views/tab_hover/astra_tab_hover_model.h"
#include "astra/ui/views/tab_hover/astra_tab_hover_peek_controller.h"
#include "astra/ui/views/tab_hover/astra_tab_hover_preview_view.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Mock;

// =========================================================================
// Mock observer for model tests
// =========================================================================

class MockTabHoverModelObserver : public AstraTabHoverModelObserver {
 public:
  MOCK_METHOD(void, OnHoverShown, (), (override));
  MOCK_METHOD(void, OnHoverHidden, (), (override));
  MOCK_METHOD(void, OnPreviewImageChanged, (), (override));
  MOCK_METHOD(void, OnTabDataChanged, (), (override));
  MOCK_METHOD(void, OnPeekModeChanged, (), (override));
  MOCK_METHOD(void, OnHoverSettingsChanged, (), (override));
};

// =========================================================================
// Mock observer for controller tests
// =========================================================================

class MockPeekControllerObserver : public AstraTabHoverPeekController::Observer {
 public:
  MOCK_METHOD(void, OnPeekPreviewShown,
              (AstraTabHoverPeekController::Source, content::WebContents*),
              (override));
  MOCK_METHOD(void, OnPeekPreviewHidden, (), (override));
  MOCK_METHOD(void, OnPeekExpandedToGlance, (), (override));
  MOCK_METHOD(void, OnPeekCollapsedFromGlance, (), (override));
  MOCK_METHOD(void, OnPeekModeStarted, (), (override));
  MOCK_METHOD(void, OnPeekModeEnded, (), (override));
  MOCK_METHOD(void, OnPreviewImageLoading, (), (override));
  MOCK_METHOD(void, OnPreviewImageLoaded, (), (override));
  MOCK_METHOD(void, OnPeekCloseRequested, (), (override));
  MOCK_METHOD(void, OnPeekMuteToggled, (), (override));
  MOCK_METHOD(void, OnPeekControllerDestroyed, (), (override));
};

// Test delegate for view tests.
class TestPreviewDelegate : public AstraTabHoverPreviewView::Delegate {
 public:
  int peek_requested_count = 0;
  int close_requested_count = 0;
  int view_destroyed_count = 0;
  int mute_toggled_count = 0;

  void OnPeekRequested() override { peek_requested_count++; }
  void OnCloseRequested() override { close_requested_count++; }
  void OnPreviewViewDestroyed() override { view_destroyed_count++; }
  void OnMuteToggled() override { mute_toggled_count++; }
};

}  // namespace

// =========================================================================
// Model tests — AstraTabHoverModel
// =========================================================================

class AstraTabHoverModelTest : public testing::Test {
 public:
  AstraTabHoverModelTest() = default;
  ~AstraTabHoverModelTest() override = default;

  void SetUp() override {
    model_ = std::make_unique<AstraTabHoverModel>();
    model_->AddObserver(&observer_);
  }

  void TearDown() override {
    model_->RemoveObserver(&observer_);
    model_.reset();
  }

 protected:
  std::unique_ptr<AstraTabHoverModel> model_;
  MockTabHoverModelObserver observer_;
};

// -- Hover visibility tests ------------------------------------------------

TEST_F(AstraTabHoverModelTest, HoverStartsHidden) {
  EXPECT_FALSE(model_->is_hover_shown());
}

TEST_F(AstraTabHoverModelTest, ShowHoverNotifiesObserver) {
  EXPECT_CALL(observer_, OnHoverShown()).Times(1);
  model_->ShowHover();
  EXPECT_TRUE(model_->is_hover_shown());
}

TEST_F(AstraTabHoverModelTest, ShowHoverTwiceNoDuplicateNotification) {
  model_->ShowHover();
  Mock::VerifyAndClearExpectations(&observer_);

  // Showing again should be a no-op.
  EXPECT_CALL(observer_, OnHoverShown()).Times(0);
  model_->ShowHover();
  EXPECT_TRUE(model_->is_hover_shown());
}

TEST_F(AstraTabHoverModelTest, HideHoverNotifiesObserver) {
  model_->ShowHover();
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnHoverHidden()).Times(1);
  model_->HideHover();
  EXPECT_FALSE(model_->is_hover_shown());
}

TEST_F(AstraTabHoverModelTest, HideHoverWhenAlreadyHiddenNoOp) {
  EXPECT_CALL(observer_, OnHoverHidden()).Times(0);
  model_->HideHover();
  EXPECT_FALSE(model_->is_hover_shown());
}

TEST_F(AstraTabHoverModelTest, RapidShowHideCycles) {
  // Show then hide rapidly — both should notify.
  EXPECT_CALL(observer_, OnHoverShown()).Times(1);
  EXPECT_CALL(observer_, OnHoverHidden()).Times(1);
  model_->ShowHover();
  model_->HideHover();

  // Show again — should notify again.
  EXPECT_CALL(observer_, OnHoverShown()).Times(1);
  model_->ShowHover();
  EXPECT_TRUE(model_->is_hover_shown());
}

// -- Tab data tests --------------------------------------------------------

TEST_F(AstraTabHoverModelTest, DefaultTabDataIsEmpty) {
  const auto& data = model_->tab_data();
  EXPECT_TRUE(data.title.empty());
  EXPECT_FALSE(data.url.is_valid());
  EXPECT_TRUE(data.favicon.isNull());
  EXPECT_EQ(AstraTabHoverMediaState::kNone, data.media_state);
  EXPECT_FALSE(data.is_muted);
  EXPECT_EQ(-1, data.tab_index);
}

TEST_F(AstraTabHoverModelTest, SetTabDataNotifiesObserver) {
  EXPECT_CALL(observer_, OnTabDataChanged()).Times(1);
  AstraTabHoverTabData data;
  data.title = u"Test Tab";
  data.url = GURL("https://example.com");
  model_->SetTabData(data);
}

TEST_F(AstraTabHoverModelTest, SetTabTitleUpdatesData) {
  EXPECT_CALL(observer_, OnTabDataChanged()).Times(1);
  model_->SetTabTitle(u"My Title");
  EXPECT_EQ(u"My Title", model_->tab_data().title);
}

TEST_F(AstraTabHoverModelTest, SetTabTitleSameNoNotify) {
  model_->SetTabTitle(u"Same Title");
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnTabDataChanged()).Times(0);
  model_->SetTabTitle(u"Same Title");
}

TEST_F(AstraTabHoverModelTest, SetTabUrlUpdatesData) {
  EXPECT_CALL(observer_, OnTabDataChanged()).Times(1);
  model_->SetTabUrl(GURL("https://example.com"));
  EXPECT_EQ(GURL("https://example.com"), model_->tab_data().url);
}

TEST_F(AstraTabHoverModelTest, SetTabUrlSameNoNotify) {
  model_->SetTabUrl(GURL("https://example.com"));
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnTabDataChanged()).Times(0);
  model_->SetTabUrl(GURL("https://example.com"));
}

TEST_F(AstraTabHoverModelTest, SetMediaStatePlaying) {
  model_->SetMediaState(AstraTabHoverMediaState::kPlaying);
  EXPECT_EQ(AstraTabHoverMediaState::kPlaying, model_->tab_data().media_state);
  EXPECT_TRUE(model_->tab_data().has_media());
}

TEST_F(AstraTabHoverModelTest, SetMediaStatePaused) {
  model_->SetMediaState(AstraTabHoverMediaState::kPaused);
  EXPECT_EQ(AstraTabHoverMediaState::kPaused, model_->tab_data().media_state);
  EXPECT_TRUE(model_->tab_data().has_media());
}

TEST_F(AstraTabHoverModelTest, SetMediaStateMuted) {
  model_->SetMediaState(AstraTabHoverMediaState::kMuted);
  EXPECT_EQ(AstraTabHoverMediaState::kMuted, model_->tab_data().media_state);
  EXPECT_TRUE(model_->tab_data().has_media());
}

TEST_F(AstraTabHoverModelTest, SetMediaStateNone) {
  model_->SetMediaState(AstraTabHoverMediaState::kPlaying);
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnTabDataChanged()).Times(1);
  model_->SetMediaState(AstraTabHoverMediaState::kNone);
  EXPECT_EQ(AstraTabHoverMediaState::kNone, model_->tab_data().media_state);
  EXPECT_FALSE(model_->tab_data().has_media());
}

TEST_F(AstraTabHoverModelTest, SetMutedTrue) {
  EXPECT_CALL(observer_, OnTabDataChanged()).Times(1);
  model_->SetMuted(true);
  EXPECT_TRUE(model_->tab_data().is_muted);
}

TEST_F(AstraTabHoverModelTest, SetMutedFalse) {
  model_->SetMuted(true);
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnTabDataChanged()).Times(1);
  model_->SetMuted(false);
  EXPECT_FALSE(model_->tab_data().is_muted);
}

TEST_F(AstraTabHoverModelTest, SetMutedSameNoNotify) {
  model_->SetMuted(true);
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnTabDataChanged()).Times(0);
  model_->SetMuted(true);
}

TEST_F(AstraTabHoverModelTest, SetTabIndex) {
  model_->SetTabIndex(3);
  EXPECT_EQ(3, model_->tab_data().tab_index);
  EXPECT_TRUE(model_->tab_data().has_valid_index());
}

TEST_F(AstraTabHoverModelTest, SetTabIndexNegative) {
  model_->SetTabIndex(5);
  model_->SetTabIndex(-1);
  EXPECT_EQ(-1, model_->tab_data().tab_index);
  EXPECT_FALSE(model_->tab_data().has_valid_index());
}

TEST_F(AstraTabHoverModelTest, SetTabIndexZero) {
  model_->SetTabIndex(0);
  EXPECT_EQ(0, model_->tab_data().tab_index);
  EXPECT_TRUE(model_->tab_data().has_valid_index());
}

// -- Preview image tests ---------------------------------------------------

TEST_F(AstraTabHoverModelTest, DefaultPreviewImageState) {
  const auto& state = model_->preview_image_state();
  EXPECT_FALSE(state.has_image);
  EXPECT_TRUE(state.dimensions.IsEmpty());
  EXPECT_EQ(AstraTabHoverImageLoadingState::kNotLoaded, state.loading_state);
  EXPECT_FALSE(state.is_loading());
  EXPECT_FALSE(state.is_loaded());
  EXPECT_FALSE(state.has_failed());
}

TEST_F(AstraTabHoverModelTest, SetPreviewImageStateNotifiesObserver) {
  EXPECT_CALL(observer_, OnPreviewImageChanged()).Times(1);
  AstraTabHoverPreviewImageState state;
  state.has_image = true;
  state.dimensions = gfx::Size(300, 200);
  state.loading_state = AstraTabHoverImageLoadingState::kLoaded;
  model_->SetPreviewImageState(state);
  EXPECT_TRUE(model_->preview_image_state().has_image);
}

TEST_F(AstraTabHoverModelTest, SetPreviewImageLoading) {
  EXPECT_CALL(observer_, OnPreviewImageChanged()).Times(1);
  model_->SetPreviewImageLoadingState(AstraTabHoverImageLoadingState::kLoading);
  EXPECT_TRUE(model_->preview_image_state().is_loading());
  EXPECT_FALSE(model_->preview_image_state().is_loaded());
}

TEST_F(AstraTabHoverModelTest, SetPreviewImageLoaded) {
  model_->SetPreviewImageLoadingState(AstraTabHoverImageLoadingState::kLoading);
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnPreviewImageChanged()).Times(1);
  model_->SetPreviewImageLoadingState(AstraTabHoverImageLoadingState::kLoaded);
  EXPECT_TRUE(model_->preview_image_state().is_loaded());
}

TEST_F(AstraTabHoverModelTest, SetPreviewImageFailed) {
  EXPECT_CALL(observer_, OnPreviewImageChanged()).Times(1);
  model_->SetPreviewImageLoadingState(AstraTabHoverImageLoadingState::kFailed);
  EXPECT_TRUE(model_->preview_image_state().has_failed());
}

TEST_F(AstraTabHoverModelTest, ClearPreviewImage) {
  // Set up with an image first.
  gfx::ImageSkia image;
  model_->SetPreviewImage(image, gfx::Size(100, 100));
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnPreviewImageChanged()).Times(1);
  model_->ClearPreviewImage();
  EXPECT_FALSE(model_->preview_image_state().has_image);
  EXPECT_EQ(AstraTabHoverImageLoadingState::kNotLoaded,
            model_->preview_image_state().loading_state);
  EXPECT_TRUE(model_->preview_image_state().dimensions.IsEmpty());
}

TEST_F(AstraTabHoverModelTest, ClearPreviewImageWhenAlreadyClearedNoOp) {
  EXPECT_CALL(observer_, OnPreviewImageChanged()).Times(0);
  model_->ClearPreviewImage();
}

TEST_F(AstraTabHoverModelTest, LoadingStateSameNoNotify) {
  model_->SetPreviewImageLoadingState(AstraTabHoverImageLoadingState::kLoading);
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnPreviewImageChanged()).Times(0);
  model_->SetPreviewImageLoadingState(AstraTabHoverImageLoadingState::kLoading);
}

// -- Peek mode tests -------------------------------------------------------

TEST_F(AstraTabHoverModelTest, DefaultPeekState) {
  const auto& state = model_->peek_state();
  EXPECT_FALSE(state.is_peeking);
  EXPECT_EQ(AstraTabHoverPreviewSize::kMedium, state.peek_size);
  EXPECT_EQ(AstraTabHoverModel::kDefaultPeekQuality, state.peek_quality);
  EXPECT_FALSE(state.is_expanded());
}

TEST_F(AstraTabHoverModelTest, StartPeekNotifiesObserver) {
  EXPECT_CALL(observer_, OnPeekModeChanged()).Times(1);
  model_->StartPeek();
  EXPECT_TRUE(model_->peek_state().is_peeking);
}

TEST_F(AstraTabHoverModelTest, EndPeekNotifiesObserver) {
  model_->StartPeek();
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnPeekModeChanged()).Times(1);
  model_->EndPeek();
  EXPECT_FALSE(model_->peek_state().is_peeking);
}

TEST_F(AstraTabHoverModelTest, StartPeekTwiceNoDuplicate) {
  model_->StartPeek();
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnPeekModeChanged()).Times(0);
  model_->StartPeek();
}

TEST_F(AstraTabHoverModelTest, EndPeekWhenNotPeekingNoOp) {
  EXPECT_CALL(observer_, OnPeekModeChanged()).Times(0);
  model_->EndPeek();
}

TEST_F(AstraTabHoverModelTest, SetPeekSizeSmall) {
  model_->StartPeek();
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnPeekModeChanged()).Times(1);
  model_->SetPeekSize(AstraTabHoverPreviewSize::kSmall);
  EXPECT_EQ(AstraTabHoverPreviewSize::kSmall, model_->peek_state().peek_size);
}

TEST_F(AstraTabHoverModelTest, SetPeekSizeLarge) {
  EXPECT_CALL(observer_, OnPeekModeChanged()).Times(1);
  model_->SetPeekSize(AstraTabHoverPreviewSize::kLarge);
  EXPECT_EQ(AstraTabHoverPreviewSize::kLarge, model_->peek_state().peek_size);
}

TEST_F(AstraTabHoverModelTest, SetPeekSizeSameNoNotify) {
  model_->SetPeekSize(AstraTabHoverPreviewSize::kMedium);
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnPeekModeChanged()).Times(0);
  model_->SetPeekSize(AstraTabHoverPreviewSize::kMedium);
}

TEST_F(AstraTabHoverModelTest, PeekExpandedWithMediumOrLarger) {
  model_->SetPeekSize(AstraTabHoverPreviewSize::kSmall);
  model_->StartPeek();
  EXPECT_FALSE(model_->peek_state().is_expanded());

  model_->SetPeekSize(AstraTabHoverPreviewSize::kMedium);
  EXPECT_TRUE(model_->peek_state().is_expanded());

  model_->SetPeekSize(AstraTabHoverPreviewSize::kLarge);
  EXPECT_TRUE(model_->peek_state().is_expanded());
}

TEST_F(AstraTabHoverModelTest, SetPeekQuality) {
  model_->SetPeekQuality(85);
  EXPECT_EQ(85, model_->peek_state().peek_quality);
}

TEST_F(AstraTabHoverModelTest, SetPeekQualityClampedToMin) {
  model_->SetPeekQuality(0);
  EXPECT_EQ(AstraTabHoverModel::kMinPeekQuality,
            model_->peek_state().peek_quality);
}

TEST_F(AstraTabHoverModelTest, SetPeekQualityClampedToMax) {
  model_->SetPeekQuality(200);
  EXPECT_EQ(AstraTabHoverModel::kMaxPeekQuality,
            model_->peek_state().peek_quality);
}

TEST_F(AstraTabHoverModelTest, SetPeekQualitySameNoNotify) {
  model_->SetPeekQuality(70);
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnPeekModeChanged()).Times(0);
  model_->SetPeekQuality(70);
}

// -- Presentation settings tests -------------------------------------------

TEST_F(AstraTabHoverModelTest, DefaultSettingsValues) {
  const auto& settings = model_->settings();
  EXPECT_TRUE(settings.show_tab_hover_cards);
  EXPECT_EQ(base::Milliseconds(500), settings.hover_show_delay);
  EXPECT_EQ(base::Milliseconds(300), settings.hover_hide_delay);
  EXPECT_TRUE(settings.show_preview_image);
  EXPECT_TRUE(settings.show_tab_title);
  EXPECT_TRUE(settings.show_tab_url);
  EXPECT_TRUE(settings.show_favicon);
  EXPECT_TRUE(settings.show_close_button);
  EXPECT_EQ(AstraTabHoverPreviewSize::kMedium, settings.preview_image_size);
  EXPECT_EQ(AstraTabHoverCardPosition::kAuto, settings.card_position);
  EXPECT_TRUE(settings.enable_peek_mode);
  EXPECT_EQ(base::Milliseconds(1500), settings.peek_activation_delay);
  EXPECT_FALSE(settings.show_tab_index);
  EXPECT_TRUE(settings.show_media_indicator);
  EXPECT_TRUE(settings.show_mute_button);
  EXPECT_TRUE(settings.animation_enabled);
}

TEST_F(AstraTabHoverModelTest, SetShowTabHoverCards) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetShowTabHoverCards(false);
  EXPECT_FALSE(model_->settings().show_tab_hover_cards);
}

TEST_F(AstraTabHoverModelTest, SetShowTabHoverCardsSameNoNotify) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(0);
  model_->SetShowTabHoverCards(true);  // Already true by default.
}

TEST_F(AstraTabHoverModelTest, SetHoverShowDelay) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetHoverShowDelay(base::Milliseconds(200));
  EXPECT_EQ(base::Milliseconds(200), model_->settings().hover_show_delay);
}

TEST_F(AstraTabHoverModelTest, SetHoverHideDelay) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetHoverHideDelay(base::Milliseconds(500));
  EXPECT_EQ(base::Milliseconds(500), model_->settings().hover_hide_delay);
}

TEST_F(AstraTabHoverModelTest, SetShowPreviewImage) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetShowPreviewImage(false);
  EXPECT_FALSE(model_->settings().show_preview_image);
}

TEST_F(AstraTabHoverModelTest, SetShowTabTitle) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetShowTabTitle(false);
  EXPECT_FALSE(model_->settings().show_tab_title);
}

TEST_F(AstraTabHoverModelTest, SetShowTabUrl) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetShowTabUrl(false);
  EXPECT_FALSE(model_->settings().show_tab_url);
}

TEST_F(AstraTabHoverModelTest, SetShowFavicon) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetShowFavicon(false);
  EXPECT_FALSE(model_->settings().show_favicon);
}

TEST_F(AstraTabHoverModelTest, SetShowCloseButton) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetShowCloseButton(false);
  EXPECT_FALSE(model_->settings().show_close_button);
}

TEST_F(AstraTabHoverModelTest, SetPreviewImageSize) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetPreviewImageSize(AstraTabHoverPreviewSize::kLarge);
  EXPECT_EQ(AstraTabHoverPreviewSize::kLarge,
            model_->settings().preview_image_size);
}

TEST_F(AstraTabHoverModelTest, SetCardPosition) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetCardPosition(AstraTabHoverCardPosition::kBelow);
  EXPECT_EQ(AstraTabHoverCardPosition::kBelow,
            model_->settings().card_position);
}

TEST_F(AstraTabHoverModelTest, SetEnablePeekMode) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetEnablePeekMode(false);
  EXPECT_FALSE(model_->settings().enable_peek_mode);
}

TEST_F(AstraTabHoverModelTest, SetPeekActivationDelay) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetPeekActivationDelay(base::Milliseconds(2000));
  EXPECT_EQ(base::Milliseconds(2000),
            model_->settings().peek_activation_delay);
}

TEST_F(AstraTabHoverModelTest, SetShowTabIndex) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetShowTabIndex(true);
  EXPECT_TRUE(model_->settings().show_tab_index);
}

TEST_F(AstraTabHoverModelTest, SetShowMediaIndicator) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetShowMediaIndicator(false);
  EXPECT_FALSE(model_->settings().show_media_indicator);
}

TEST_F(AstraTabHoverModelTest, SetShowMuteButton) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetShowMuteButton(false);
  EXPECT_FALSE(model_->settings().show_mute_button);
}

TEST_F(AstraTabHoverModelTest, SetAnimationEnabled) {
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->SetAnimationEnabled(false);
  EXPECT_FALSE(model_->settings().animation_enabled);
}

TEST_F(AstraTabHoverModelTest, SetAllSettingsNotifiesOnce) {
  // Setting all settings via SetSettings should notify once.
  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  AstraTabHoverSettings settings;
  settings.show_tab_hover_cards = false;
  settings.hover_show_delay = base::Milliseconds(100);
  settings.show_preview_image = false;
  settings.enable_peek_mode = false;
  model_->SetSettings(settings);
}

TEST_F(AstraTabHoverModelTest, ResetSettingsToDefaults) {
  // Change a bunch of settings.
  model_->SetShowTabHoverCards(false);
  model_->SetShowPreviewImage(false);
  model_->SetEnablePeekMode(false);
  model_->SetAnimationEnabled(false);
  Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->ResetSettingsToDefaults();

  const auto& settings = model_->settings();
  EXPECT_TRUE(settings.show_tab_hover_cards);
  EXPECT_TRUE(settings.show_preview_image);
  EXPECT_TRUE(settings.enable_peek_mode);
  EXPECT_TRUE(settings.animation_enabled);
}

TEST_F(AstraTabHoverModelTest, ApplySettingsBulkUpdates) {
  AstraTabHoverModel::SettingsApplyMask mask;
  mask.apply_show_tab_hover_cards = true;
  mask.apply_hover_show_delay = true;
  mask.apply_enable_peek_mode = true;

  AstraTabHoverSettings settings;
  settings.show_tab_hover_cards = false;
  settings.hover_show_delay = base::Milliseconds(100);
  settings.enable_peek_mode = false;
  // Other fields in settings won't be applied since mask flags are false.

  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(1);
  model_->ApplySettings(settings, mask);

  EXPECT_FALSE(model_->settings().show_tab_hover_cards);
  EXPECT_EQ(base::Milliseconds(100), model_->settings().hover_show_delay);
  EXPECT_FALSE(model_->settings().enable_peek_mode);
  // show_tab_title should still be default (true) since mask doesn't apply it.
  EXPECT_TRUE(model_->settings().show_tab_title);
}

TEST_F(AstraTabHoverModelTest, ApplySettingsEmptyMaskNoChange) {
  AstraTabHoverModel::SettingsApplyMask mask;
  AstraTabHoverSettings settings;
  settings.show_tab_hover_cards = false;

  EXPECT_CALL(observer_, OnHoverSettingsChanged()).Times(0);
  model_->ApplySettings(settings, mask);
  EXPECT_TRUE(model_->settings().show_tab_hover_cards);
}

// -- Persistence / PrefService tests ---------------------------------------

class AstraTabHoverModelPrefsTest : public testing::Test {
 public:
  AstraTabHoverModelPrefsTest() = default;
  ~AstraTabHoverModelPrefsTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
    model_ = std::make_unique<AstraTabHoverModel>();
  }

  void TearDown() override {
    model_.reset();
  }

 protected:
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<AstraTabHoverModel> model_;
};

TEST_F(AstraTabHoverModelPrefsTest, LoadFromPrefsDefaultValues) {
  // With a fresh pref service, loading should match defaults.
  model_->LoadFromPrefs(&pref_service_);

  const auto& settings = model_->settings();
  EXPECT_TRUE(settings.show_tab_hover_cards);
  EXPECT_EQ(base::Milliseconds(500), settings.hover_show_delay);
  EXPECT_EQ(base::Milliseconds(300), settings.hover_hide_delay);
  EXPECT_TRUE(settings.show_preview_image);
  EXPECT_TRUE(settings.show_tab_title);
  EXPECT_TRUE(settings.show_tab_url);
  EXPECT_TRUE(settings.show_favicon);
  EXPECT_TRUE(settings.show_close_button);
  EXPECT_EQ(AstraTabHoverPreviewSize::kMedium, settings.preview_image_size);
  EXPECT_EQ(AstraTabHoverCardPosition::kAuto, settings.card_position);
  EXPECT_TRUE(settings.enable_peek_mode);
  EXPECT_EQ(base::Milliseconds(1500), settings.peek_activation_delay);
  EXPECT_FALSE(settings.show_tab_index);
  EXPECT_TRUE(settings.show_media_indicator);
  EXPECT_TRUE(settings.show_mute_button);
  EXPECT_TRUE(settings.animation_enabled);
}

TEST_F(AstraTabHoverModelPrefsTest, SaveAndLoadRoundTrip) {
  // Modify settings and save.
  model_->SetShowTabHoverCards(false);
  model_->SetHoverShowDelay(base::Milliseconds(200));
  model_->SetHoverHideDelay(base::Milliseconds(400));
  model_->SetShowPreviewImage(false);
  model_->SetShowTabTitle(false);
  model_->SetShowTabUrl(false);
  model_->SetShowFavicon(false);
  model_->SetShowCloseButton(false);
  model_->SetPreviewImageSize(AstraTabHoverPreviewSize::kLarge);
  model_->SetCardPosition(AstraTabHoverCardPosition::kAbove);
  model_->SetEnablePeekMode(false);
  model_->SetPeekActivationDelay(base::Milliseconds(2000));
  model_->SetShowTabIndex(true);
  model_->SetShowMediaIndicator(false);
  model_->SetShowMuteButton(false);
  model_->SetAnimationEnabled(false);

  model_->SaveToPrefs(&pref_service_);

  // Create a new model and load from prefs.
  auto model2 = std::make_unique<AstraTabHoverModel>();
  model2->LoadFromPrefs(&pref_service_);

  const auto& s = model2->settings();
  EXPECT_FALSE(s.show_tab_hover_cards);
  EXPECT_EQ(base::Milliseconds(200), s.hover_show_delay);
  EXPECT_EQ(base::Milliseconds(400), s.hover_hide_delay);
  EXPECT_FALSE(s.show_preview_image);
  EXPECT_FALSE(s.show_tab_title);
  EXPECT_FALSE(s.show_tab_url);
  EXPECT_FALSE(s.show_favicon);
  EXPECT_FALSE(s.show_close_button);
  EXPECT_EQ(AstraTabHoverPreviewSize::kLarge, s.preview_image_size);
  EXPECT_EQ(AstraTabHoverCardPosition::kAbove, s.card_position);
  EXPECT_FALSE(s.enable_peek_mode);
  EXPECT_EQ(base::Milliseconds(2000), s.peek_activation_delay);
  EXPECT_TRUE(s.show_tab_index);
  EXPECT_FALSE(s.show_media_indicator);
  EXPECT_FALSE(s.show_mute_button);
  EXPECT_FALSE(s.animation_enabled);
}

TEST_F(AstraTabHoverModelPrefsTest, LoadFromNullPrefsNoCrash) {
  // Should not crash with null PrefService.
  model_->LoadFromPrefs(nullptr);
  model_->SaveToPrefs(nullptr);
}

TEST_F(AstraTabHoverModelPrefsTest, PrefClampOnLoad) {
  // Set a delay value outside valid range in prefs.
  pref_service_.SetInteger(prefs::kPrefTabHoverShowDelayMs, 50000);  // Too big
  pref_service_.SetInteger(prefs::kPrefTabHoverPeekDelayMs, -100);  // Negative

  model_->LoadFromPrefs(&pref_service_);

  // Should be clamped to valid range.
  EXPECT_LE(model_->settings().hover_show_delay,
            AstraTabHoverModel::kMaxDelay);
  EXPECT_GE(model_->settings().peek_activation_delay,
            AstraTabHoverModel::kMinDelay);
}

// -- Utility method tests --------------------------------------------------

TEST(AstraTabHoverModelUtilTest, FormatTabTitleShort) {
  std::u16string title = u"Short Title";
  std::u16string result = AstraTabHoverModel::FormatTabTitle(title, 60);
  EXPECT_EQ(title, result);
}

TEST(AstraTabHoverModelUtilTest, FormatTabTitleLong) {
  std::u16string long_title(100, u'x');
  std::u16string result = AstraTabHoverModel::FormatTabTitle(long_title, 60);
  EXPECT_LT(result.length(), long_title.length());
  // Should end with ellipsis.
  EXPECT_EQ(u'\u2026', result.back());
}

TEST(AstraTabHoverModelUtilTest, FormatTabTitleExactLength) {
  std::u16string title(60, u'a');
  std::u16string result = AstraTabHoverModel::FormatTabTitle(title, 60);
  EXPECT_EQ(title, result);  // Exact length, no truncation.
}

TEST(AstraTabHoverModelUtilTest, FormatTabTitleEmpty) {
  std::u16string result = AstraTabHoverModel::FormatTabTitle(std::u16string(), 60);
  EXPECT_TRUE(result.empty());
}

TEST(AstraTabHoverModelUtilTest, FormatDomainFromUrlValid) {
  GURL url("https://www.example.com/path?query=1");
  std::u16string result = AstraTabHoverModel::FormatDomainFromUrl(url);
  EXPECT_EQ(u"www.example.com", result);
}

TEST(AstraTabHoverModelUtilTest, FormatDomainFromUrlInvalid) {
  GURL url("not a valid url");
  std::u16string result = AstraTabHoverModel::FormatDomainFromUrl(url);
  EXPECT_TRUE(result.empty());
}

TEST(AstraTabHoverModelUtilTest, FormatDomainFromEmptyUrl) {
  GURL url;
  std::u16string result = AstraTabHoverModel::FormatDomainFromUrl(url);
  EXPECT_TRUE(result.empty());
}

TEST(AstraTabHoverModelUtilTest, ClampDelayValid) {
  base::TimeDelta delay = base::Milliseconds(500);
  EXPECT_EQ(delay, AstraTabHoverModel::ClampDelay(delay));
}

TEST(AstraTabHoverModelUtilTest, ClampDelayMin) {
  base::TimeDelta delay = base::Milliseconds(-100);
  EXPECT_EQ(AstraTabHoverModel::kMinDelay,
            AstraTabHoverModel::ClampDelay(delay));
}

TEST(AstraTabHoverModelUtilTest, ClampDelayMax) {
  base::TimeDelta delay = base::Milliseconds(20000);  // Over 10s max.
  EXPECT_EQ(AstraTabHoverModel::kMaxDelay,
            AstraTabHoverModel::ClampDelay(delay));
}

TEST(AstraTabHoverModelUtilTest, ClampDelayZero) {
  EXPECT_EQ(base::TimeDelta(),
            AstraTabHoverModel::ClampDelay(base::TimeDelta()));
}

TEST(AstraTabHoverModelUtilTest, ClampPreviewSizeValid) {
  EXPECT_EQ(AstraTabHoverPreviewSize::kSmall,
            AstraTabHoverModel::ClampPreviewSize(0));
  EXPECT_EQ(AstraTabHoverPreviewSize::kMedium,
            AstraTabHoverModel::ClampPreviewSize(1));
  EXPECT_EQ(AstraTabHoverPreviewSize::kLarge,
            AstraTabHoverModel::ClampPreviewSize(2));
}

TEST(AstraTabHoverModelUtilTest, ClampPreviewSizeOutOfRangeNegative) {
  EXPECT_EQ(AstraTabHoverPreviewSize::kMedium,
            AstraTabHoverModel::ClampPreviewSize(-1));
}

TEST(AstraTabHoverModelUtilTest, ClampPreviewSizeOutOfRangeHigh) {
  EXPECT_EQ(AstraTabHoverPreviewSize::kMedium,
            AstraTabHoverModel::ClampPreviewSize(99));
}

TEST(AstraTabHoverModelUtilTest, ClampCardPositionValid) {
  EXPECT_EQ(AstraTabHoverCardPosition::kAbove,
            AstraTabHoverModel::ClampCardPosition(0));
  EXPECT_EQ(AstraTabHoverCardPosition::kBelow,
            AstraTabHoverModel::ClampCardPosition(1));
  EXPECT_EQ(AstraTabHoverCardPosition::kAuto,
            AstraTabHoverModel::ClampCardPosition(2));
}

TEST(AstraTabHoverModelUtilTest, ClampCardPositionOutOfRange) {
  EXPECT_EQ(AstraTabHoverCardPosition::kAuto,
            AstraTabHoverModel::ClampCardPosition(-1));
  EXPECT_EQ(AstraTabHoverCardPosition::kAuto,
            AstraTabHoverModel::ClampCardPosition(10));
}

TEST(AstraTabHoverModelUtilTest, ClampPeekQualityMin) {
  EXPECT_EQ(AstraTabHoverModel::kMinPeekQuality,
            AstraTabHoverModel::ClampPeekQuality(0));
  EXPECT_EQ(AstraTabHoverModel::kMinPeekQuality,
            AstraTabHoverModel::ClampPeekQuality(-10));
}

TEST(AstraTabHoverModelUtilTest, ClampPeekQualityMax) {
  EXPECT_EQ(AstraTabHoverModel::kMaxPeekQuality,
            AstraTabHoverModel::ClampPeekQuality(100));
  EXPECT_EQ(AstraTabHoverModel::kMaxPeekQuality,
            AstraTabHoverModel::ClampPeekQuality(200));
}

TEST(AstraTabHoverModelUtilTest, ClampPeekQualityValid) {
  EXPECT_EQ(50, AstraTabHoverModel::ClampPeekQuality(50));
  EXPECT_EQ(10, AstraTabHoverModel::ClampPeekQuality(10));
  EXPECT_EQ(100, AstraTabHoverModel::ClampPeekQuality(100));
}

TEST(AstraTabHoverModelUtilTest, GetPreviewSizePixels) {
  gfx::Size small = AstraTabHoverModel::GetPreviewSizePixels(
      AstraTabHoverPreviewSize::kSmall);
  gfx::Size medium = AstraTabHoverModel::GetPreviewSizePixels(
      AstraTabHoverPreviewSize::kMedium);
  gfx::Size large = AstraTabHoverModel::GetPreviewSizePixels(
      AstraTabHoverPreviewSize::kLarge);

  EXPECT_LT(small.width(), medium.width());
  EXPECT_LT(medium.width(), large.width());
  EXPECT_LT(small.height(), medium.height());
  EXPECT_LT(medium.height(), large.height());
}

TEST(AstraTabHoverModelUtilTest, DefaultPeekQualityIsValid) {
  int quality = AstraTabHoverModel::kDefaultPeekQuality;
  EXPECT_GE(quality, AstraTabHoverModel::kMinPeekQuality);
  EXPECT_LE(quality, AstraTabHoverModel::kMaxPeekQuality);
}

// -- Observer defaults test ------------------------------------------------

TEST(AstraTabHoverModelObserverDefaultsTest, AllMethodsHaveDefaults) {
  // Verify that all observer methods have empty default implementations
  // and calling them doesn't crash.
  class TestObserver : public AstraTabHoverModelObserver {
   public:
    // Deliberately override nothing — test that default implementations exist.
  };

  TestObserver observer;
  // Call all methods to verify default implementations don't crash.
  observer.OnHoverShown();
  observer.OnHoverHidden();
  observer.OnPreviewImageChanged();
  observer.OnTabDataChanged();
  observer.OnPeekModeChanged();
  observer.OnHoverSettingsChanged();
  // If we get here without crashing, defaults work.
}

TEST(AstraTabHoverPeekObserverDefaultsTest, AllMethodsHaveDefaults) {
  class TestObserver : public AstraTabHoverPeekController::Observer {
   public:
    // Override nothing — test defaults.
  };

  TestObserver observer;
  observer.OnPeekPreviewShown(AstraTabHoverPeekController::Source::kSidebarItem,
                              nullptr);
  observer.OnPeekPreviewHidden();
  observer.OnPeekExpandedToGlance();
  observer.OnPeekCollapsedFromGlance();
  observer.OnPeekModeStarted();
  observer.OnPeekModeEnded();
  observer.OnPreviewImageLoading();
  observer.OnPreviewImageLoaded();
  observer.OnPeekCloseRequested();
  observer.OnPeekMuteToggled();
  observer.OnPeekControllerDestroyed();
  // No crash = defaults work.
}

// =========================================================================
// View tests — AstraTabHoverPreviewView
// =========================================================================

class AstraTabHoverViewTest : public views::ViewsTestBase {
 public:
  AstraTabHoverViewTest() = default;
  ~AstraTabHoverViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();

    widget_ = CreateTestWidget();
    anchor_view_ = widget_->SetContentsView(std::make_unique<views::View>());
    anchor_view_->SetPreferredSize(gfx::Size(100, 100));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  AstraTabHoverPreviewView* CreatePreviewView(
      AstraTabHoverPreviewView::Delegate* delegate) {
    views::Widget* bubble_widget = AstraTabHoverPreviewView::ShowBubble(
        anchor_view_, gfx::Rect(), nullptr, delegate);
    return static_cast<AstraTabHoverPreviewView*>(
        bubble_widget->widget_delegate()->AsDialogDelegate());
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<views::View> anchor_view_ = nullptr;
};

TEST_F(AstraTabHoverViewTest, WidgetIsCreated) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  EXPECT_NE(nullptr, preview->GetWidget());
}

TEST_F(AstraTabHoverViewTest, DefaultThumbnailIsHidden) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  EXPECT_FALSE(preview->thumbnail_visible());
}

TEST_F(AstraTabHoverViewTest, SetThumbnailVisibleTrue) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetThumbnailVisible(true);
  EXPECT_TRUE(preview->thumbnail_visible());
}

TEST_F(AstraTabHoverViewTest, SetThumbnailVisibleFalse) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetThumbnailVisible(true);
  ASSERT_TRUE(preview->thumbnail_visible());
  preview->SetThumbnailVisible(false);
  EXPECT_FALSE(preview->thumbnail_visible());
}

TEST_F(AstraTabHoverViewTest, SetTitleText) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetTitleText(u"Test Tab Title");
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetDomainText) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetDomainText(u"example.com");
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetEmptyTitle) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetTitleText(std::u16string());
  // Should not crash with empty title.
}

TEST_F(AstraTabHoverViewTest, SetEmptyDomain) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetDomainText(std::u16string());
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetFaviconPlaceholder) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetFaviconPlaceholder();
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, ThumbnailPlaceholder) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetThumbnailPlaceholder();
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetThumbnailLoadingStateLoading) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetThumbnailLoadingState(
      AstraTabHoverImageLoadingState::kLoading);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetThumbnailLoadingStateLoaded) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetThumbnailLoadingState(
      AstraTabHoverImageLoadingState::kLoaded);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetThumbnailLoadingStateFailed) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetThumbnailLoadingState(
      AstraTabHoverImageLoadingState::kFailed);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetMediaStatePlaying) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetMediaState(AstraTabHoverMediaState::kPlaying);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetMediaStateNone) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetMediaState(AstraTabHoverMediaState::kNone);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetMediaIndicatorVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetMediaIndicatorVisible(true);
  preview->SetMediaIndicatorVisible(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetMuteButtonVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetMuteButtonVisible(true);
  preview->SetMuteButtonVisible(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetMuted) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetMuted(true);
  preview->SetMuted(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetTabIndex) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetTabIndex(0);
  preview->SetTabIndex(5);
  preview->SetTabIndex(-1);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetTabIndexVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetTabIndexVisible(true);
  preview->SetTabIndexVisible(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetCloseButtonVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetCloseButtonVisible(true);
  preview->SetCloseButtonVisible(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetPeekMode) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetPeekMode(true);
  EXPECT_TRUE(preview->thumbnail_visible());  // Peek mode shows thumbnail.
  preview->SetPeekMode(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetPeekButtonVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetPeekButtonVisible(true);
  preview->SetPeekButtonVisible(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetPeekButtonText) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetPeekButtonText(u"Peek");
  preview->SetPeekButtonText(u"Expand");
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetPreviewSize) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetPreviewSize(AstraTabHoverPreviewSize::kSmall);
  EXPECT_EQ(AstraTabHoverPreviewSize::kSmall, preview->preview_size());
  preview->SetPreviewSize(AstraTabHoverPreviewSize::kLarge);
  EXPECT_EQ(AstraTabHoverPreviewSize::kLarge, preview->preview_size());
  preview->SetPreviewSize(AstraTabHoverPreviewSize::kMedium);
  EXPECT_EQ(AstraTabHoverPreviewSize::kMedium, preview->preview_size());
}

TEST_F(AstraTabHoverViewTest, SetCardPosition) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetCardPosition(AstraTabHoverCardPosition::kAbove);
  preview->SetCardPosition(AstraTabHoverCardPosition::kBelow);
  preview->SetCardPosition(AstraTabHoverCardPosition::kAuto);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetWorkspaceIndicatorVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetWorkspaceIndicatorVisible(true);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetWorkspaceIndicatorColor) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetWorkspaceIndicatorColor(SK_ColorBLUE);
  preview->SetWorkspaceIndicatorColor(SK_ColorRED);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, WidgetDestroyNotifiesDelegate) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  EXPECT_EQ(0, delegate.view_destroyed_count);
  preview->GetWidget()->CloseNow();
  EXPECT_GE(delegate.view_destroyed_count, 1);
}

TEST_F(AstraTabHoverViewTest, OnThemeChangedDoesNotCrash) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->OnThemeChanged();
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, HasColorProvider) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  EXPECT_NE(nullptr, preview->GetColorProvider());
}

TEST_F(AstraTabHoverViewTest, StartPeekExpandAnimation) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->StartPeekExpandAnimation();
  // Stub implementation — should not crash.
}

TEST_F(AstraTabHoverViewTest, SetAccessibleName) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetAccessibleName(u"Test Tab Preview");
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetTitleVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetTitleVisible(true);
  preview->SetTitleVisible(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetFaviconVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetFaviconVisible(true);
  preview->SetFaviconVisible(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, SetDomainVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->SetDomainVisible(true);
  preview->SetDomainVisible(false);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, UpdateFromModel) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  AstraTabHoverModel model;
  AstraTabHoverTabData data;
  data.title = u"Test Tab";
  data.url = GURL("https://example.com");
  data.tab_index = 3;
  data.media_state = AstraTabHoverMediaState::kPlaying;
  model.SetTabData(data);

  preview->UpdateFromModel(model);
  // Should not crash.
}

TEST_F(AstraTabHoverViewTest, UpdateFromNullWebContents) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);
  preview->UpdateFromWebContents(nullptr);
  // Should not crash with null WebContents.
}

TEST_F(AstraTabHoverViewTest, CompactSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kCompactSize.width(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kCompactSize.height(), 0);
}

TEST_F(AstraTabHoverViewTest, ExpandedSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kExpandedSize.width(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kExpandedSize.height(), 0);
}

TEST_F(AstraTabHoverViewTest, ExpandedTallerThanCompact) {
  EXPECT_GT(AstraTabHoverPreviewView::kExpandedSize.height(),
            AstraTabHoverPreviewView::kCompactSize.height());
}

TEST_F(AstraTabHoverViewTest, SameWidthBothModes) {
  EXPECT_EQ(AstraTabHoverPreviewView::kCompactSize.width(),
            AstraTabHoverPreviewView::kExpandedSize.width());
}

TEST_F(AstraTabHoverViewTest, ThumbnailSizesArePositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kSmallThumbnailSize.width(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kSmallThumbnailSize.height(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kMediumThumbnailSize.width(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kMediumThumbnailSize.height(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kLargeThumbnailSize.width(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kLargeThumbnailSize.height(), 0);
}

TEST_F(AstraTabHoverViewTest, ThumbnailSizesIncrease) {
  EXPECT_LT(AstraTabHoverPreviewView::kSmallThumbnailSize.width(),
            AstraTabHoverPreviewView::kMediumThumbnailSize.width());
  EXPECT_LT(AstraTabHoverPreviewView::kMediumThumbnailSize.width(),
            AstraTabHoverPreviewView::kLargeThumbnailSize.width());
}

TEST_F(AstraTabHoverViewTest, FaviconSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kFaviconSize, 0);
}

TEST_F(AstraTabHoverViewTest, WorkspaceDotSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kWorkspaceDotSize, 0);
}

TEST_F(AstraTabHoverViewTest, CloseButtonSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kCloseButtonSize, 0);
}

TEST_F(AstraTabHoverViewTest, PeekButtonSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kPeekButtonHeight, 0);
  EXPECT_GT(AstraTabHoverPreviewView::kPeekButtonWidth, 0);
}

TEST_F(AstraTabHoverViewTest, MediaIndicatorSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kMediaIndicatorSize, 0);
}

TEST_F(AstraTabHoverViewTest, MuteButtonSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kMuteButtonSize, 0);
}

TEST_F(AstraTabHoverViewTest, TabIndexBadgeSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kTabIndexBadgeSize, 0);
}

TEST_F(AstraTabHoverViewTest, PaddingIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kHorizontalPadding, 0);
  EXPECT_GT(AstraTabHoverPreviewView::kVerticalPadding, 0);
}

TEST_F(AstraTabHoverViewTest, SpacingIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kHeaderToThumbnailSpacing, 0);
  EXPECT_GT(AstraTabHoverPreviewView::kThumbnailToFooterSpacing, 0);
  EXPECT_GT(AstraTabHoverPreviewView::kHeaderElementSpacing, 0);
}

// =========================================================================
// Peek controller tests — AstraTabHoverPeekController
// =========================================================================

class AstraTabHoverPeekControllerTest : public views::ViewsTestBase {
 public:
  AstraTabHoverPeekControllerTest() = default;
  ~AstraTabHoverPeekControllerTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();

    widget_ = CreateTestWidget();
    anchor_view_ = widget_->SetContentsView(std::make_unique<views::View>());
    anchor_view_->SetPreferredSize(gfx::Size(100, 100));
    widget_->Show();

    controller_ = std::make_unique<AstraTabHoverPeekController>();
  }

  void TearDown() override {
    controller_.reset();
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<views::View> anchor_view_ = nullptr;
  std::unique_ptr<AstraTabHoverPeekController> controller_;
};

TEST_F(AstraTabHoverPeekControllerTest, DefaultStateIsIdle) {
  EXPECT_EQ(AstraTabHoverPeekController::State::kIdle, controller_->state());
}

TEST_F(AstraTabHoverPeekControllerTest, HideAllWhenIdleNoCrash) {
  // Should be a no-op without crashing.
  controller_->HideAll();
  EXPECT_EQ(AstraTabHoverPeekController::State::kIdle, controller_->state());
}

TEST_F(AstraTabHoverPeekControllerTest, DismissPeekFromKeyboardWhenIdle) {
  EXPECT_FALSE(controller_->DismissPeekFromKeyboard());
}

TEST_F(AstraTabHoverPeekControllerTest, ExpandFromKeyboardWhenIdle) {
  EXPECT_FALSE(controller_->ExpandFromKeyboard());
}

TEST_F(AstraTabHoverPeekControllerTest, ToggleMuteFromKeyboardWhenIdle) {
  EXPECT_FALSE(controller_->ToggleMuteFromKeyboard());
}

TEST_F(AstraTabHoverPeekControllerTest, ModelAccessible) {
  // Model should be accessible and have default state.
  EXPECT_FALSE(controller_->model().is_hover_shown());
}

TEST_F(AstraTabHoverPeekControllerTest, OnHoverEndedWhenIdleNoCrash) {
  controller_->OnHoverEnded();
  // Should not crash.
}

TEST_F(AstraTabHoverPeekControllerTest, OnHoverMovedWhenIdleNoCrash) {
  controller_->OnHoverMoved(gfx::Point(10, 10));
  // Should not crash.
}

TEST_F(AstraTabHoverPeekControllerTest, ActivatePeekFromKeyboardNullArgs) {
  EXPECT_FALSE(controller_->ActivatePeekFromKeyboard(nullptr, gfx::Rect(),
                                                    nullptr));
}

TEST_F(AstraTabHoverPeekControllerTest, StartPreviewImageLoadingNoPreview) {
  // Should not crash when there's no preview shown.
  controller_->StartPreviewImageLoading();
}

TEST_F(AstraTabHoverPeekControllerTest, ClearPreviewImageNoPreview) {
  controller_->ClearPreviewImage();
  // Should not crash.
}

TEST_F(AstraTabHoverPeekControllerTest, PreviewImageLoadFailedNoPreview) {
  controller_->PreviewImageLoadFailed();
  // Should not crash.
}

TEST_F(AstraTabHoverPeekControllerTest, UpdateMediaStateNoPreview) {
  controller_->UpdateMediaState(AstraTabHoverMediaState::kPlaying);
  // Should not crash — updates model even without view.
  EXPECT_EQ(AstraTabHoverMediaState::kPlaying,
            controller_->model().tab_data().media_state);
}

TEST_F(AstraTabHoverPeekControllerTest, ToggleMuteNoPreview) {
  controller_->ToggleMute();
  EXPECT_TRUE(controller_->model().tab_data().is_muted);
  controller_->ToggleMute();
  EXPECT_FALSE(controller_->model().tab_data().is_muted);
}

TEST_F(AstraTabHoverPeekControllerTest, EnterPeekModeWhenIdleNoOp) {
  controller_->EnterPeekMode();
  EXPECT_EQ(AstraTabHoverPeekController::State::kIdle, controller_->state());
}

TEST_F(AstraTabHoverPeekControllerTest, ExitPeekModeWhenIdleNoOp) {
  controller_->ExitPeekMode();
  EXPECT_EQ(AstraTabHoverPeekController::State::kIdle, controller_->state());
}

TEST_F(AstraTabHoverPeekControllerTest, ExpandToGlanceWhenIdleNoOp) {
  controller_->ExpandToGlance();
  EXPECT_EQ(AstraTabHoverPeekController::State::kIdle, controller_->state());
}

TEST_F(AstraTabHoverPeekControllerTest, CollapseToPreviewWhenIdleNoOp) {
  controller_->CollapseToPreview();
  EXPECT_EQ(AstraTabHoverPeekController::State::kIdle, controller_->state());
}

TEST_F(AstraTabHoverPeekControllerTest, SourceDefaultIsSidebarItem) {
  EXPECT_EQ(AstraTabHoverPeekController::Source::kSidebarItem,
            controller_->source());
}

TEST_F(AstraTabHoverPeekControllerTest, HoverContentsNullByDefault) {
  EXPECT_EQ(nullptr, controller_->hover_contents());
}

TEST_F(AstraTabHoverPeekControllerTest, PreviewWidgetNullByDefault) {
  EXPECT_EQ(nullptr, controller_->preview_widget());
}

TEST_F(AstraTabHoverPeekControllerTest, GlanceControllerNullByDefault) {
  EXPECT_EQ(nullptr, controller_->glance_controller());
}

TEST_F(AstraTabHoverPeekControllerTest, OnHoverStartedWithNullAnchor) {
  controller_->OnHoverStarted(nullptr, gfx::Rect(), nullptr,
                              AstraTabHoverPeekController::Source::kSidebarItem);
  EXPECT_EQ(AstraTabHoverPeekController::State::kIdle, controller_->state());
}

TEST_F(AstraTabHoverPeekControllerTest, OnHoverStartedWithNullWebContents) {
  controller_->OnHoverStarted(anchor_view_, gfx::Rect(), nullptr,
                              AstraTabHoverPeekController::Source::kSidebarItem);
  EXPECT_EQ(AstraTabHoverPeekController::State::kIdle, controller_->state());
}

// -- Controller observer tests --------------------------------------------

TEST_F(AstraTabHoverPeekControllerTest, AddAndRemoveObserver) {
  MockPeekControllerObserver observer;
  controller_->AddObserver(&observer);
  controller_->RemoveObserver(&observer);
  // Should not crash.
}

TEST_F(AstraTabHoverPeekControllerTest, DestroyNotifiesObservers) {
  MockPeekControllerObserver observer;
  controller_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPeekControllerDestroyed()).Times(1);
  controller_.reset();  // Destroy the controller.

  // Verify expectations before controller is fully destroyed.
  Mock::VerifyAndClearExpectations(&observer);
}

// -- Keyboard activation test ---------------------------------------------

TEST_F(AstraTabHoverPeekControllerTest, ActivateFromKeyboardWithAnchor) {
  // Test that keyboard activation with valid args progresses state.
  bool activated = controller_->ActivatePeekFromKeyboard(
      anchor_view_, gfx::Rect(), nullptr);
  // Null WebContents should result in no activation.
  EXPECT_FALSE(activated);
  EXPECT_EQ(AstraTabHoverPeekController::State::kIdle, controller_->state());
}

// =========================================================================
// Pref registration tests
// =========================================================================

TEST(AstraTabHoverPrefsTest, AllPrefKeysRegistered) {
  TestingPrefServiceSimple pref_service;
  prefs::RegisterProfilePrefs(pref_service.registry());

  // Verify all tab hover pref keys are registered.
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowCards));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowDelayMs));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverHideDelayMs));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowPreviewImage));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowTitle));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowUrl));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowFavicon));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowCloseButton));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverPreviewImageSize));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverCardPosition));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverEnablePeekMode));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverPeekDelayMs));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowTabIndex));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowMediaIndicator));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverShowMuteButton));
  EXPECT_TRUE(pref_service.HasPrefPath(prefs::kPrefTabHoverAnimationEnabled));
}

TEST(AstraTabHoverPrefsTest, DefaultPrefValues) {
  TestingPrefServiceSimple pref_service;
  prefs::RegisterProfilePrefs(pref_service.registry());

  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverShowCards));
  EXPECT_EQ(500, pref_service.GetInteger(prefs::kPrefTabHoverShowDelayMs));
  EXPECT_EQ(300, pref_service.GetInteger(prefs::kPrefTabHoverHideDelayMs));
  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverShowPreviewImage));
  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverShowTitle));
  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverShowUrl));
  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverShowFavicon));
  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverShowCloseButton));
  EXPECT_EQ(1, pref_service.GetInteger(prefs::kPrefTabHoverPreviewImageSize));
  EXPECT_EQ(2, pref_service.GetInteger(prefs::kPrefTabHoverCardPosition));
  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverEnablePeekMode));
  EXPECT_EQ(1500, pref_service.GetInteger(prefs::kPrefTabHoverPeekDelayMs));
  EXPECT_FALSE(pref_service.GetBoolean(prefs::kPrefTabHoverShowTabIndex));
  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverShowMediaIndicator));
  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverShowMuteButton));
  EXPECT_TRUE(pref_service.GetBoolean(prefs::kPrefTabHoverAnimationEnabled));
}

TEST(AstraTabHoverPrefsTest, PrefKeysHaveValidPaths) {
  // All pref keys should start with "astra.tab_hover."
  EXPECT_EQ(0,
            std::string(prefs::kPrefTabHoverShowCards).find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverShowDelayMs)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverHideDelayMs)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverShowPreviewImage)
                   .find("astra.tab_hover."));
  EXPECT_EQ(
      0, std::string(prefs::kPrefTabHoverShowTitle).find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverShowUrl).find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverShowFavicon)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverShowCloseButton)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverPreviewImageSize)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverCardPosition)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverEnablePeekMode)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverPeekDelayMs)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverShowTabIndex)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverShowMediaIndicator)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverShowMuteButton)
                   .find("astra.tab_hover."));
  EXPECT_EQ(0, std::string(prefs::kPrefTabHoverAnimationEnabled)
                   .find("astra.tab_hover."));
}

}  // namespace astra
