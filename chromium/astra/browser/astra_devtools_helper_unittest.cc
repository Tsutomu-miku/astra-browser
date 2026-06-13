// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_devtools_helper.h"

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestDevToolsObserver : public AstraDevToolsHelper::Observer {
 public:
  void OnDevToolsWindowOpened(content::WebContents* web_contents) override {
    window_opened_count_++;
    last_opened_web_contents_ = web_contents;
  }

  void OnDevToolsWindowClosed(content::WebContents* web_contents) override {
    window_closed_count_++;
    last_closed_web_contents_ = web_contents;
  }

  void OnDevToolsDockStateChanged(
      content::WebContents* web_contents,
      AstraDevToolsDockState dock_state) override {
    dock_state_changed_count_++;
    last_dock_state_web_contents_ = web_contents;
    last_dock_state_ = dock_state;
  }

  void OnAstraPanelVisibilityChanged(bool visible) override {
    astra_panel_visibility_changed_count_++;
    last_astra_panel_visible_ = visible;
  }

  void OnDevToolsSettingsChanged() override {
    settings_changed_count_++;
  }

  // Counters
  int window_opened_count_ = 0;
  int window_closed_count_ = 0;
  int dock_state_changed_count_ = 0;
  int astra_panel_visibility_changed_count_ = 0;
  int settings_changed_count_ = 0;

  // Last recorded values
  raw_ptr<content::WebContents> last_opened_web_contents_ = nullptr;
  raw_ptr<content::WebContents> last_closed_web_contents_ = nullptr;
  raw_ptr<content::WebContents> last_dock_state_web_contents_ = nullptr;
  AstraDevToolsDockState last_dock_state_ = AstraDevToolsDockState::kBottom;
  bool last_astra_panel_visible_ = false;
};

}  // namespace

// Test fixture for AstraDevToolsHelper tests.
class DevToolsHelperTest : public testing::Test {
 protected:
  DevToolsHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register Astra prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
  }

  ~DevToolsHelperTest() override = default;

  void SetUp() override {
    // Verify default state.
    ASSERT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
              AstraDevToolsDockState::kBottom);
    ASSERT_TRUE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));
    ASSERT_FALSE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      AstraDevToolsHelper::RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::vector<TestDevToolsObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Default state
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, DefaultState_DefaultDockStateIsBottom) {
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
            AstraDevToolsDockState::kBottom);
}

TEST_F(DevToolsHelperTest, DefaultState_DefaultPanelIsEmpty) {
  EXPECT_TRUE(
      AstraDevToolsHelper::GetDefaultPanel(profile_.get()).empty());
}

TEST_F(DevToolsHelperTest, DefaultState_AutoOpenDisabled) {
  EXPECT_FALSE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));
}

TEST_F(DevToolsHelperTest, DefaultState_AstraPanelVisible) {
  EXPECT_TRUE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));
}

TEST_F(DevToolsHelperTest, DefaultState_OpenCountIsZero) {
  // In the overlay skeleton, count is always 0 (placeholder).
  EXPECT_EQ(AstraDevToolsHelper::GetOpenDevToolsCount(), 0);
}

TEST_F(DevToolsHelperTest, DefaultState_NullWebContentsReturnsFalse) {
  EXPECT_FALSE(AstraDevToolsHelper::IsDevToolsOpenForTab(nullptr));
}

TEST_F(DevToolsHelperTest, DefaultState_NullWebContentsDefaultDock) {
  // Null web_contents should return default (kBottom).
  EXPECT_EQ(AstraDevToolsHelper::GetDevToolsDockState(nullptr),
            AstraDevToolsDockState::kBottom);
}

TEST_F(DevToolsHelperTest, DefaultState_RegisteredPanelsExist) {
  // Built-in Astra panels should be registered by default.
  auto panels = AstraDevToolsHelper::GetRegisteredPanels();
  EXPECT_GT(panels.size(), 0u);
  EXPECT_GE(AstraDevToolsHelper::GetRegisteredPanelCount(), 1u);
}

TEST_F(DevToolsHelperTest, DefaultState_WorkspacePanelRegistered) {
  auto panel = AstraDevToolsHelper::GetPanelById("workspace-inspector");
  EXPECT_FALSE(panel.id.empty());
  EXPECT_EQ(panel.id, "workspace-inspector");
  EXPECT_TRUE(panel.is_builtin);
}

TEST_F(DevToolsHelperTest, DefaultState_TabStackPanelRegistered) {
  auto panel = AstraDevToolsHelper::GetPanelById("tab-stack-viewer");
  EXPECT_FALSE(panel.id.empty());
  EXPECT_EQ(panel.id, "tab-stack-viewer");
  EXPECT_TRUE(panel.is_builtin);
}

TEST_F(DevToolsHelperTest, DefaultState_FavoritePanelRegistered) {
  auto panel = AstraDevToolsHelper::GetPanelById("favorite-manager");
  EXPECT_FALSE(panel.id.empty());
  EXPECT_EQ(panel.id, "favorite-manager");
  EXPECT_TRUE(panel.is_builtin);
}

TEST_F(DevToolsHelperTest, DefaultState_FocusPanelRegisteredButDisabled) {
  auto panel = AstraDevToolsHelper::GetPanelById("focus-mode");
  EXPECT_FALSE(panel.id.empty());
  EXPECT_EQ(panel.id, "focus-mode");
  EXPECT_TRUE(panel.is_builtin);
  EXPECT_FALSE(panel.enabled);
}

// ---------------------------------------------------------------------------
// Observer defaults — all methods have empty default implementations
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraDevToolsHelper::Observer {};

  DefaultObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  // Trigger all observer paths via settings changes and manual notifications.
  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kRight);
  AstraDevToolsHelper::SetDefaultPanel(profile_.get(), "test-panel");
  AstraDevToolsHelper::SetAutoOpenDevTools(profile_.get(), true);
  AstraDevToolsHelper::SetAstraPanelVisible(profile_.get(), false);

  // Manual notifications for lifecycle events.
  AstraDevToolsHelper::NotifyDevToolsWindowOpened(nullptr);
  AstraDevToolsHelper::NotifyDevToolsWindowClosed(nullptr);
  AstraDevToolsHelper::NotifyDevToolsDockStateChanged(
      nullptr, AstraDevToolsDockState::kUndocked);
  AstraDevToolsHelper::NotifyAstraPanelVisibilityChanged(true);
  AstraDevToolsHelper::NotifyDevToolsSettingsChanged();

  AstraDevToolsHelper::RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, AddRemoveObserver_NoCrash) {
  TestDevToolsObserver observer;

  AstraDevToolsHelper::AddObserver(&observer);
  AstraDevToolsHelper::RemoveObserver(&observer);

  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(DevToolsHelperTest, RemoveNonexistentObserver_NoCrash) {
  TestDevToolsObserver observer;

  AstraDevToolsHelper::RemoveObserver(&observer);

  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

TEST_F(DevToolsHelperTest, AddNullObserver_NoCrash) {
  AstraDevToolsHelper::AddObserver(nullptr);
  AstraDevToolsHelper::RemoveObserver(nullptr);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Settings — default dock state
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, SetDefaultDockState_ChangesValue) {
  ASSERT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
            AstraDevToolsDockState::kBottom);

  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kRight);
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
            AstraDevToolsDockState::kRight);

  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kLeft);
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
            AstraDevToolsDockState::kLeft);

  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kUndocked);
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
            AstraDevToolsDockState::kUndocked);

  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kMinimized);
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
            AstraDevToolsDockState::kMinimized);
}

TEST_F(DevToolsHelperTest, SetDefaultDockState_SameValueNoOp) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  ASSERT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
            AstraDevToolsDockState::kBottom);
  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kBottom);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, SetDefaultDockState_FiresSettingsObserver) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kRight);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, SetDefaultDockState_NullProfileNoCrash) {
  AstraDevToolsHelper::SetDefaultDockState(nullptr,
                                           AstraDevToolsDockState::kRight);
  SUCCEED();
}

TEST_F(DevToolsHelperTest, GetDefaultDockState_NullProfileReturnsDefault) {
  // Null profile should return the default value.
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultDockState(nullptr),
            AstraDevToolsHelper::DockStateFromString(
                prefs::kDefaultDevToolsDefaultDockState));
}

// ---------------------------------------------------------------------------
// Settings — default panel
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, SetDefaultPanel_ChangesValue) {
  ASSERT_TRUE(
      AstraDevToolsHelper::GetDefaultPanel(profile_.get()).empty());

  AstraDevToolsHelper::SetDefaultPanel(profile_.get(), "workspace-inspector");
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultPanel(profile_.get()),
            "workspace-inspector");

  AstraDevToolsHelper::SetDefaultPanel(profile_.get(), "tab-stack-viewer");
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultPanel(profile_.get()),
            "tab-stack-viewer");
}

TEST_F(DevToolsHelperTest, SetDefaultPanel_SameValueNoOp) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  ASSERT_TRUE(
      AstraDevToolsHelper::GetDefaultPanel(profile_.get()).empty());
  AstraDevToolsHelper::SetDefaultPanel(profile_.get(), std::string());

  EXPECT_EQ(observer.settings_changed_count_, 0);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, SetDefaultPanel_FiresSettingsObserver) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::SetDefaultPanel(profile_.get(), "test-panel");

  EXPECT_EQ(observer.settings_changed_count_, 1);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, SetDefaultPanel_NullProfileNoCrash) {
  AstraDevToolsHelper::SetDefaultPanel(nullptr, "test-panel");
  SUCCEED();
}

TEST_F(DevToolsHelperTest, GetDefaultPanel_NullProfileReturnsDefault) {
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultPanel(nullptr),
            prefs::kDefaultDevToolsDefaultPanel);
}

// ---------------------------------------------------------------------------
// Settings — auto-open DevTools
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, SetAutoOpenDevTools_ChangesValue) {
  ASSERT_FALSE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));

  AstraDevToolsHelper::SetAutoOpenDevTools(profile_.get(), true);
  EXPECT_TRUE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));

  AstraDevToolsHelper::SetAutoOpenDevTools(profile_.get(), false);
  EXPECT_FALSE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));
}

TEST_F(DevToolsHelperTest, SetAutoOpenDevTools_SameValueNoOp) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  ASSERT_FALSE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));
  AstraDevToolsHelper::SetAutoOpenDevTools(profile_.get(), false);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, SetAutoOpenDevTools_FiresSettingsObserver) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::SetAutoOpenDevTools(profile_.get(), true);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, ToggleAutoOpenDevTools_FlipsValue) {
  ASSERT_FALSE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));

  bool result = AstraDevToolsHelper::ToggleAutoOpenDevTools(profile_.get());
  EXPECT_TRUE(result);
  EXPECT_TRUE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));

  result = AstraDevToolsHelper::ToggleAutoOpenDevTools(profile_.get());
  EXPECT_FALSE(result);
  EXPECT_FALSE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));
}

TEST_F(DevToolsHelperTest, SetAutoOpenDevTools_NullProfileNoCrash) {
  AstraDevToolsHelper::SetAutoOpenDevTools(nullptr, true);
  SUCCEED();
}

TEST_F(DevToolsHelperTest, GetAutoOpenDevTools_NullProfileReturnsDefault) {
  EXPECT_EQ(AstraDevToolsHelper::GetAutoOpenDevTools(nullptr),
            prefs::kDefaultDevToolsAutoOpen);
}

// ---------------------------------------------------------------------------
// Settings — Astra panel visibility
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, SetAstraPanelVisible_ChangesValue) {
  ASSERT_TRUE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));

  AstraDevToolsHelper::SetAstraPanelVisible(profile_.get(), false);
  EXPECT_FALSE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));

  AstraDevToolsHelper::SetAstraPanelVisible(profile_.get(), true);
  EXPECT_TRUE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));
}

TEST_F(DevToolsHelperTest, SetAstraPanelVisible_SameValueNoOp) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  ASSERT_TRUE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));
  AstraDevToolsHelper::SetAstraPanelVisible(profile_.get(), true);

  // Should not fire either visibility or settings changed.
  EXPECT_EQ(observer.astra_panel_visibility_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, SetAstraPanelVisible_FiresBothObservers) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::SetAstraPanelVisible(profile_.get(), false);

  // Setting Astra panel visibility should fire both the specific
  // visibility notification and the general settings notification.
  EXPECT_EQ(observer.astra_panel_visibility_changed_count_, 1);
  EXPECT_FALSE(observer.last_astra_panel_visible_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, ToggleAstraPanelVisible_FlipsValue) {
  ASSERT_TRUE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));

  bool result = AstraDevToolsHelper::ToggleAstraPanelVisible(profile_.get());
  EXPECT_FALSE(result);
  EXPECT_FALSE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));

  result = AstraDevToolsHelper::ToggleAstraPanelVisible(profile_.get());
  EXPECT_TRUE(result);
  EXPECT_TRUE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));
}

TEST_F(DevToolsHelperTest, SetAstraPanelVisible_NullProfileNoCrash) {
  AstraDevToolsHelper::SetAstraPanelVisible(nullptr, false);
  SUCCEED();
}

TEST_F(DevToolsHelperTest, IsAstraPanelVisible_NullProfileReturnsDefault) {
  EXPECT_EQ(AstraDevToolsHelper::IsAstraPanelVisible(nullptr),
            prefs::kDefaultDevToolsAstraPanelVisible);
}

// ---------------------------------------------------------------------------
// Panel registration
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, RegisterAstraPanel_AddsPanel) {
  size_t initial_count = AstraDevToolsHelper::GetRegisteredPanelCount();

  AstraDevToolsPanelInfo panel;
  panel.id = "test-panel";
  panel.name = u"Test Panel";
  panel.icon = "test-icon";
  panel.enabled = true;
  panel.is_builtin = false;

  bool result = AstraDevToolsHelper::RegisterAstraPanel(panel);
  EXPECT_TRUE(result);
  EXPECT_EQ(AstraDevToolsHelper::GetRegisteredPanelCount(),
            initial_count + 1);

  // Verify panel was added.
  auto retrieved = AstraDevToolsHelper::GetPanelById("test-panel");
  EXPECT_EQ(retrieved.id, "test-panel");
  EXPECT_EQ(retrieved.name, u"Test Panel");
  EXPECT_EQ(retrieved.icon, "test-icon");
  EXPECT_TRUE(retrieved.enabled);
  EXPECT_FALSE(retrieved.is_builtin);

  // Clean up.
  AstraDevToolsHelper::UnregisterAstraPanel("test-panel");
}

TEST_F(DevToolsHelperTest, RegisterAstraPanel_DuplicateIdReturnsFalse) {
  size_t initial_count = AstraDevToolsHelper::GetRegisteredPanelCount();

  AstraDevToolsPanelInfo panel;
  panel.id = "duplicate-test";
  panel.name = u"Duplicate Test";

  bool result1 = AstraDevToolsHelper::RegisterAstraPanel(panel);
  EXPECT_TRUE(result1);

  bool result2 = AstraDevToolsHelper::RegisterAstraPanel(panel);
  EXPECT_FALSE(result2);

  EXPECT_EQ(AstraDevToolsHelper::GetRegisteredPanelCount(),
            initial_count + 1);

  // Clean up.
  AstraDevToolsHelper::UnregisterAstraPanel("duplicate-test");
}

TEST_F(DevToolsHelperTest, RegisterAstraPanel_EmptyIdReturnsFalse) {
  size_t initial_count = AstraDevToolsHelper::GetRegisteredPanelCount();

  AstraDevToolsPanelInfo panel;
  panel.id = "";
  panel.name = u"Empty ID Panel";

  bool result = AstraDevToolsHelper::RegisterAstraPanel(panel);
  EXPECT_FALSE(result);
  EXPECT_EQ(AstraDevToolsHelper::GetRegisteredPanelCount(), initial_count);
}

TEST_F(DevToolsHelperTest, UnregisterAstraPanel_RemovesPanel) {
  // First register a test panel.
  AstraDevToolsPanelInfo panel;
  panel.id = "unregister-test";
  panel.name = u"Unregister Test";
  AstraDevToolsHelper::RegisterAstraPanel(panel);
  size_t count_after_register = AstraDevToolsHelper::GetRegisteredPanelCount();

  bool result = AstraDevToolsHelper::UnregisterAstraPanel("unregister-test");
  EXPECT_TRUE(result);
  EXPECT_EQ(AstraDevToolsHelper::GetRegisteredPanelCount(),
            count_after_register - 1);

  // Should not find it anymore.
  auto retrieved = AstraDevToolsHelper::GetPanelById("unregister-test");
  EXPECT_TRUE(retrieved.id.empty());
}

TEST_F(DevToolsHelperTest, UnregisterAstraPanel_NonexistentReturnsFalse) {
  size_t initial_count = AstraDevToolsHelper::GetRegisteredPanelCount();

  bool result = AstraDevToolsHelper::UnregisterAstraPanel("nonexistent");
  EXPECT_FALSE(result);
  EXPECT_EQ(AstraDevToolsHelper::GetRegisteredPanelCount(), initial_count);
}

TEST_F(DevToolsHelperTest, UnregisterAstraPanel_EmptyIdReturnsFalse) {
  bool result = AstraDevToolsHelper::UnregisterAstraPanel("");
  EXPECT_FALSE(result);
}

TEST_F(DevToolsHelperTest, GetPanelById_EmptyIdReturnsEmpty) {
  auto panel = AstraDevToolsHelper::GetPanelById("");
  EXPECT_TRUE(panel.id.empty());
}

TEST_F(DevToolsHelperTest, GetPanelById_NonexistentReturnsEmpty) {
  auto panel = AstraDevToolsHelper::GetPanelById("nonexistent-panel");
  EXPECT_TRUE(panel.id.empty());
}

TEST_F(DevToolsHelperTest, GetRegisteredPanels_ReturnsAllPanels) {
  auto panels = AstraDevToolsHelper::GetRegisteredPanels();
  EXPECT_EQ(panels.size(),
            AstraDevToolsHelper::GetRegisteredPanelCount());

  // All panels should have non-empty IDs.
  for (const auto& panel : panels) {
    EXPECT_FALSE(panel.id.empty());
  }
}

// ---------------------------------------------------------------------------
// Dock state string conversion
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, DockStateToString_KnownValues) {
  EXPECT_EQ(AstraDevToolsHelper::DockStateToString(
                AstraDevToolsDockState::kBottom),
            "bottom");
  EXPECT_EQ(AstraDevToolsHelper::DockStateToString(
                AstraDevToolsDockState::kRight),
            "right");
  EXPECT_EQ(AstraDevToolsHelper::DockStateToString(
                AstraDevToolsDockState::kLeft),
            "left");
  EXPECT_EQ(AstraDevToolsHelper::DockStateToString(
                AstraDevToolsDockState::kUndocked),
            "undocked");
  EXPECT_EQ(AstraDevToolsHelper::DockStateToString(
                AstraDevToolsDockState::kMinimized),
            "minimized");
}

TEST_F(DevToolsHelperTest, DockStateFromString_KnownValues) {
  EXPECT_EQ(AstraDevToolsHelper::DockStateFromString("bottom"),
            AstraDevToolsDockState::kBottom);
  EXPECT_EQ(AstraDevToolsHelper::DockStateFromString("right"),
            AstraDevToolsDockState::kRight);
  EXPECT_EQ(AstraDevToolsHelper::DockStateFromString("left"),
            AstraDevToolsDockState::kLeft);
  EXPECT_EQ(AstraDevToolsHelper::DockStateFromString("undocked"),
            AstraDevToolsDockState::kUndocked);
  EXPECT_EQ(AstraDevToolsHelper::DockStateFromString("minimized"),
            AstraDevToolsDockState::kMinimized);
}

TEST_F(DevToolsHelperTest, DockStateFromString_UnknownReturnsBottom) {
  // Unknown strings should default to kBottom (matching Chromium default).
  EXPECT_EQ(AstraDevToolsHelper::DockStateFromString("invalid"),
            AstraDevToolsDockState::kBottom);
  EXPECT_EQ(AstraDevToolsHelper::DockStateFromString(""),
            AstraDevToolsDockState::kBottom);
  EXPECT_EQ(AstraDevToolsHelper::DockStateFromString("top"),
            AstraDevToolsDockState::kBottom);
}

TEST_F(DevToolsHelperTest, DockState_RoundTripConversion) {
  // Converting to string and back should yield the same value.
  auto states = {
      AstraDevToolsDockState::kBottom,
      AstraDevToolsDockState::kRight,
      AstraDevToolsDockState::kLeft,
      AstraDevToolsDockState::kUndocked,
      AstraDevToolsDockState::kMinimized,
  };

  for (auto state : states) {
    std::string str = AstraDevToolsHelper::DockStateToString(state);
    AstraDevToolsDockState result =
        AstraDevToolsHelper::DockStateFromString(str);
    EXPECT_EQ(result, state);
  }
}

// ---------------------------------------------------------------------------
// Shortcut helpers
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, GetDevToolsToggleShortcutText_ReturnsNonEmpty) {
  std::string shortcut = AstraDevToolsHelper::GetDevToolsToggleShortcutText();
  EXPECT_FALSE(shortcut.empty());
}

TEST_F(DevToolsHelperTest, GetDevToolsOpenShortcutText_AllStates) {
  EXPECT_FALSE(AstraDevToolsHelper::GetDevToolsOpenShortcutText(
                   AstraDevToolsDockState::kBottom)
                   .empty());
  EXPECT_FALSE(AstraDevToolsHelper::GetDevToolsOpenShortcutText(
                   AstraDevToolsDockState::kRight)
                   .empty());
  EXPECT_FALSE(AstraDevToolsHelper::GetDevToolsOpenShortcutText(
                   AstraDevToolsDockState::kLeft)
                   .empty());
  EXPECT_FALSE(AstraDevToolsHelper::GetDevToolsOpenShortcutText(
                   AstraDevToolsDockState::kUndocked)
                   .empty());
  EXPECT_FALSE(AstraDevToolsHelper::GetDevToolsOpenShortcutText(
                   AstraDevToolsDockState::kMinimized)
                   .empty());
}

// ---------------------------------------------------------------------------
// Manual observer notifications
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, NotifyDevToolsWindowOpened_FiresObserver) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::NotifyDevToolsWindowOpened(nullptr);
  EXPECT_EQ(observer.window_opened_count_, 1);
  EXPECT_EQ(observer.last_opened_web_contents_, nullptr);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, NotifyDevToolsWindowClosed_FiresObserver) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::NotifyDevToolsWindowClosed(nullptr);
  EXPECT_EQ(observer.window_closed_count_, 1);
  EXPECT_EQ(observer.last_closed_web_contents_, nullptr);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, NotifyDevToolsDockStateChanged_FiresObserver) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::NotifyDevToolsDockStateChanged(
      nullptr, AstraDevToolsDockState::kUndocked);
  EXPECT_EQ(observer.dock_state_changed_count_, 1);
  EXPECT_EQ(observer.last_dock_state_, AstraDevToolsDockState::kUndocked);
  EXPECT_EQ(observer.last_dock_state_web_contents_, nullptr);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, NotifyAstraPanelVisibilityChanged_FiresObserver) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::NotifyAstraPanelVisibilityChanged(true);
  EXPECT_EQ(observer.astra_panel_visibility_changed_count_, 1);
  EXPECT_TRUE(observer.last_astra_panel_visible_);

  AstraDevToolsHelper::NotifyAstraPanelVisibilityChanged(false);
  EXPECT_EQ(observer.astra_panel_visibility_changed_count_, 2);
  EXPECT_FALSE(observer.last_astra_panel_visible_);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

TEST_F(DevToolsHelperTest, NotifyDevToolsSettingsChanged_FiresObserver) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::NotifyDevToolsSettingsChanged();
  EXPECT_EQ(observer.settings_changed_count_, 1);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, MultipleObservers_AllNotified) {
  TestDevToolsObserver observer1;
  TestDevToolsObserver observer2;

  AstraDevToolsHelper::AddObserver(&observer1);
  AstraDevToolsHelper::AddObserver(&observer2);

  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kRight);

  EXPECT_EQ(observer1.settings_changed_count_, 1);
  EXPECT_EQ(observer2.settings_changed_count_, 1);

  AstraDevToolsHelper::RemoveObserver(&observer1);
  AstraDevToolsHelper::RemoveObserver(&observer2);
}

TEST_F(DevToolsHelperTest, RemoveObserver_StopsNotifications) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kRight);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  AstraDevToolsHelper::RemoveObserver(&observer);

  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kLeft);
  // Should not have been notified after removal.
  EXPECT_EQ(observer.settings_changed_count_, 1);
}

// ---------------------------------------------------------------------------
// Null web contents edge cases
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, ToggleDevTools_NullWebContentsNoCrash) {
  AstraDevToolsHelper::ToggleDevTools(nullptr);
  SUCCEED();
}

TEST_F(DevToolsHelperTest, OpenDevTools_NullWebContentsNoCrash) {
  AstraDevToolsHelper::OpenDevTools(nullptr,
                                    AstraDevToolsDockState::kBottom);
  SUCCEED();
}

TEST_F(DevToolsHelperTest, CloseDevTools_NullWebContentsNoCrash) {
  AstraDevToolsHelper::CloseDevTools(nullptr);
  SUCCEED();
}

TEST_F(DevToolsHelperTest, SetDevToolsDockState_NullWebContentsNoCrash) {
  AstraDevToolsHelper::SetDevToolsDockState(nullptr,
                                            AstraDevToolsDockState::kRight);
  SUCCEED();
}

TEST_F(DevToolsHelperTest, OpenAstraDevToolsPanel_NullWebContentsNoCrash) {
  AstraDevToolsHelper::OpenAstraDevToolsPanel(nullptr);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Null browser edge cases
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, ToggleDevToolsForBrowser_NullBrowserNoCrash) {
  AstraDevToolsHelper::ToggleDevToolsForBrowser(nullptr);
  SUCCEED();
}

TEST_F(DevToolsHelperTest, IsDevToolsOpenForActiveTab_NullBrowserReturnsFalse) {
  EXPECT_FALSE(AstraDevToolsHelper::IsDevToolsOpenForActiveTab(nullptr));
}

TEST_F(DevToolsHelperTest, OpenDevToolsForBrowser_NullBrowserNoCrash) {
  AstraDevToolsHelper::OpenDevToolsForBrowser(nullptr,
                                               AstraDevToolsDockState::kBottom);
  SUCCEED();
}

TEST_F(DevToolsHelperTest, CloseDevToolsForBrowser_NullBrowserNoCrash) {
  AstraDevToolsHelper::CloseDevToolsForBrowser(nullptr);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Bulk operations
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, CloseAllDevTools_NoCrash) {
  // In the overlay skeleton, this is a no-op placeholder.
  AstraDevToolsHelper::CloseAllDevTools();
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Persistence round-trip
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, PrefsPersist_DefaultDockState) {
  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kRight);

  // Read from the same profile's prefs — value should persist.
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
            AstraDevToolsDockState::kRight);
}

TEST_F(DevToolsHelperTest, PrefsPersist_DefaultPanel) {
  AstraDevToolsHelper::SetDefaultPanel(profile_.get(), "workspace-inspector");

  EXPECT_EQ(AstraDevToolsHelper::GetDefaultPanel(profile_.get()),
            "workspace-inspector");
}

TEST_F(DevToolsHelperTest, PrefsPersist_AutoOpen) {
  AstraDevToolsHelper::SetAutoOpenDevTools(profile_.get(), true);

  EXPECT_TRUE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));
}

TEST_F(DevToolsHelperTest, PrefsPersist_AstraPanelVisible) {
  AstraDevToolsHelper::SetAstraPanelVisible(profile_.get(), false);

  EXPECT_FALSE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));
}

TEST_F(DevToolsHelperTest, PrefsPersist_DefaultValues) {
  // All values should start at their defaults.
  EXPECT_EQ(AstraDevToolsHelper::GetDefaultDockState(profile_.get()),
            AstraDevToolsDockState::kBottom);
  EXPECT_TRUE(
      AstraDevToolsHelper::GetDefaultPanel(profile_.get()).empty());
  EXPECT_FALSE(AstraDevToolsHelper::GetAutoOpenDevTools(profile_.get()));
  EXPECT_TRUE(AstraDevToolsHelper::IsAstraPanelVisible(profile_.get()));
}

// ---------------------------------------------------------------------------
// Combined settings changes
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, MultipleSettingsChanges_AllNotify) {
  TestDevToolsObserver observer;
  AstraDevToolsHelper::AddObserver(&observer);

  // Change each setting — each should fire a settings notification.
  AstraDevToolsHelper::SetDefaultDockState(profile_.get(),
                                           AstraDevToolsDockState::kRight);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  AstraDevToolsHelper::SetDefaultPanel(profile_.get(), "test-panel");
  EXPECT_EQ(observer.settings_changed_count_, 2);

  AstraDevToolsHelper::SetAutoOpenDevTools(profile_.get(), true);
  EXPECT_EQ(observer.settings_changed_count_, 3);

  AstraDevToolsHelper::SetAstraPanelVisible(profile_.get(), false);
  // Setting Astra panel visibility also fires settings changed.
  EXPECT_EQ(observer.settings_changed_count_, 4);

  AstraDevToolsHelper::RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// AstraDevToolsPanelInfo struct
// ---------------------------------------------------------------------------

TEST_F(DevToolsHelperTest, DevToolsPanelInfo_DefaultConstructed) {
  AstraDevToolsPanelInfo info;
  EXPECT_TRUE(info.id.empty());
  EXPECT_TRUE(info.name.empty());
  EXPECT_TRUE(info.icon.empty());
  EXPECT_TRUE(info.enabled);  // Default is true.
  EXPECT_FALSE(info.is_builtin);
}

}  // namespace astra
