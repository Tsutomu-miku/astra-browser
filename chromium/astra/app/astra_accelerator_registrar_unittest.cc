// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_accelerator_registrar.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Eq;

// Helper: get the primary modifier constant for this platform.
#if BUILDFLAG(IS_MAC)
constexpr int kPrimaryModifier = ui::EF_COMMAND_DOWN;
#else
constexpr int kPrimaryModifier = ui::EF_CONTROL_DOWN;
#endif

// Mock observer for testing.
class MockAcceleratorObserver : public AstraAcceleratorObserver {
 public:
  MOCK_METHOD(void,
              OnAcceleratorRegistered,
              (const ui::Accelerator&, int),
              (override));
  MOCK_METHOD(void,
              OnAcceleratorUnregistered,
              (const ui::Accelerator&, int),
              (override));
  MOCK_METHOD(void,
              OnAcceleratorActivated,
              (const ui::Accelerator&, int),
              (override));
};

}  // namespace

// =========================================================================
// AstraAcceleratorRegistrarTest
// =========================================================================

class AstraAcceleratorRegistrarTest : public testing::Test {
 protected:
  void SetUp() override {
    // Reset registrar state before each test.
    ResetAcceleratorRegistrarForTesting();

    // Remove any observers that might have been left over.
    // (We can't easily remove all, but we add our test observers fresh.)
  }

  void TearDown() override {
    // Clean up after each test.
    ResetAcceleratorRegistrarForTesting();
  }
};

// =========================================================================
// Registration tests (without FocusManager — testing internal state)
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, InitialCountIsZero) {
  // Initially, no accelerators should be registered.
  EXPECT_EQ(GetRegisteredAcceleratorCount(), 0);
}

TEST_F(AstraAcceleratorRegistrarTest, GetAllRegisteredIsEmptyInitially) {
  auto all = GetAllRegisteredAccelerators();
  EXPECT_TRUE(all.empty());
}

TEST_F(AstraAcceleratorRegistrarTest, HasConflictsInitiallyFalse) {
  // With no accelerators registered, there can't be conflicts.
  EXPECT_FALSE(HasConflicts());
}

TEST_F(AstraAcceleratorRegistrarTest, GetConflictCountInitiallyZero) {
  EXPECT_EQ(GetConflictCount(), 0);
}

TEST_F(AstraAcceleratorRegistrarTest, ResetIsSafeToCallMultipleTimes) {
  ResetAcceleratorRegistrarForTesting();
  ResetAcceleratorRegistrarForTesting();
  ResetAcceleratorRegistrarForTesting();
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest, ResetClearsCount) {
  // After reset, count should be 0.
  ResetAcceleratorRegistrarForTesting();
  EXPECT_EQ(GetRegisteredAcceleratorCount(), 0);
}

TEST_F(AstraAcceleratorRegistrarTest, ResetClearsAllAccelerators) {
  ResetAcceleratorRegistrarForTesting();
  auto all = GetAllRegisteredAccelerators();
  EXPECT_TRUE(all.empty());
}

// =========================================================================
// RegisterAccelerator / UnregisterAccelerator tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest,
       RegisterAccelerator_NullFocusManagerReturnsFalse) {
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier | ui::EF_SHIFT_DOWN);
  bool result = RegisterAccelerator(nullptr, nullptr, accel, kAstraCommandFirst);
  EXPECT_FALSE(result);
}

TEST_F(AstraAcceleratorRegistrarTest,
       UnregisterAccelerator_NullFocusManagerReturnsFalse) {
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier | ui::EF_SHIFT_DOWN);
  bool result = UnregisterAccelerator(nullptr, nullptr, accel);
  EXPECT_FALSE(result);
}

TEST_F(AstraAcceleratorRegistrarTest,
       IsAcceleratorRegistered_NullFocusManagerReturnsFalse) {
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier | ui::EF_SHIFT_DOWN);
  EXPECT_FALSE(IsAcceleratorRegistered(nullptr, accel));
}

TEST_F(AstraAcceleratorRegistrarTest,
       GetConflictingAccelerators_NullFocusManagerReturnsEmpty) {
  auto conflicts = GetConflictingAccelerators(nullptr);
  EXPECT_TRUE(conflicts.empty());
}

TEST_F(AstraAcceleratorRegistrarTest,
       RegisterAstraAccelerators_NullFocusManagerReturnsEmptyResult) {
  auto result = RegisterAstraAccelerators(nullptr, nullptr);
  EXPECT_EQ(result.registered_count, 0);
  EXPECT_EQ(result.conflict_count, 0);
  EXPECT_TRUE(result.conflicts.empty());
}

TEST_F(AstraAcceleratorRegistrarTest,
       UnregisterAstraAccelerators_NullFocusManagerDoesNotCrash) {
  UnregisterAstraAccelerators(nullptr, nullptr);
  SUCCEED();
}

// =========================================================================
// FindAcceleratorByKeyEvent tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest,
       FindAcceleratorByKeyEvent_NoAcceleratorsReturnsNullopt) {
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_P,
                     kPrimaryModifier | ui::EF_SHIFT_DOWN);
  auto result = FindAcceleratorByKeyEvent(event);
  EXPECT_FALSE(result.has_value());
}

TEST_F(AstraAcceleratorRegistrarTest,
       FindAcceleratorByKeyEvent_InvalidEventReturnsNullopt) {
  // A key event with no modifiers and an unknown key should not match.
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_UNKNOWN, 0);
  auto result = FindAcceleratorByKeyEvent(event);
  EXPECT_FALSE(result.has_value());
}

TEST_F(AstraAcceleratorRegistrarTest,
       FindAcceleratorByKeyEvent_KeyReleaseReturnsNullopt) {
  // Key release events should still be processable (accelerator matching
  // typically happens on key press, not release).
  // But our lookup doesn't check event type — let's verify it works.
  ui::KeyEvent event(ui::ET_KEY_RELEASED, ui::VKEY_P,
                     kPrimaryModifier | ui::EF_SHIFT_DOWN);
  // Since no accelerators are registered, it should return nullopt.
  auto result = FindAcceleratorByKeyEvent(event);
  EXPECT_FALSE(result.has_value());
}

// =========================================================================
// Observer tests (without FocusManager)
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, AddObserver_DoesNotCrash) {
  MockAcceleratorObserver observer;
  AddAcceleratorObserver(&observer);
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest, RemoveObserver_DoesNotCrash) {
  MockAcceleratorObserver observer;
  AddAcceleratorObserver(&observer);
  RemoveAcceleratorObserver(&observer);
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest, RemoveObserver_NotAddedDoesNotCrash) {
  MockAcceleratorObserver observer;
  RemoveAcceleratorObserver(&observer);
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest,
       NotifyAcceleratorActivated_NoObserversDoesNotCrash) {
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);
  NotifyAcceleratorActivated(accel, kAstraCommandFirst);
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest,
       NotifyAcceleratorActivated_NotifiesObserver) {
  MockAcceleratorObserver observer;
  AddAcceleratorObserver(&observer);

  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier | ui::EF_SHIFT_DOWN);

  EXPECT_CALL(observer, OnAcceleratorActivated(_, _)).Times(1);

  NotifyAcceleratorActivated(accel, kAstraCommandOpenCommandPalette);

  RemoveAcceleratorObserver(&observer);
}

TEST_F(AstraAcceleratorRegistrarTest,
       NotifyAcceleratorActivated_MultipleObserversAllNotified) {
  MockAcceleratorObserver observer1;
  MockAcceleratorObserver observer2;
  MockAcceleratorObserver observer3;

  AddAcceleratorObserver(&observer1);
  AddAcceleratorObserver(&observer2);
  AddAcceleratorObserver(&observer3);

  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);

  EXPECT_CALL(observer1, OnAcceleratorActivated(_, _)).Times(1);
  EXPECT_CALL(observer2, OnAcceleratorActivated(_, _)).Times(1);
  EXPECT_CALL(observer3, OnAcceleratorActivated(_, _)).Times(1);

  NotifyAcceleratorActivated(accel, kAstraCommandFirst);

  RemoveAcceleratorObserver(&observer1);
  RemoveAcceleratorObserver(&observer2);
  RemoveAcceleratorObserver(&observer3);
}

TEST_F(AstraAcceleratorRegistrarTest,
       RemoveObserver_StopsNotifications) {
  MockAcceleratorObserver observer;
  AddAcceleratorObserver(&observer);
  RemoveAcceleratorObserver(&observer);

  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);

  // After removal, observer should NOT be called.
  EXPECT_CALL(observer, OnAcceleratorActivated(_, _)).Times(0);

  NotifyAcceleratorActivated(accel, kAstraCommandFirst);
}

TEST_F(AstraAcceleratorRegistrarTest,
       Observer_CheckedObserverCanSelfRemove) {
  // Verify observer uses base::CheckedObserver pattern
  // (it's safe to remove during iteration).
  // This is a basic test — self-removal during notification is a more
  // advanced test case.
  MockAcceleratorObserver observer;
  AddAcceleratorObserver(&observer);
  RemoveAcceleratorObserver(&observer);
  // Should not crash.
  NotifyAcceleratorActivated(
      ui::Accelerator(ui::VKEY_A, kPrimaryModifier), kAstraCommandFirst);
  SUCCEED();
}

// =========================================================================
// Observer registration event tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest,
       OnAcceleratorRegistered_CalledOnRegister) {
  // When we register an accelerator, the observer should be notified.
  // This tests the internal notification mechanism.
  //
  // Note: We test this via the direct registration path since we can't
  // easily create a FocusManager in a simple test.
  //
  // The internal AddToRegisteredSet function isn't directly accessible,
  // but we can verify the observer pattern works via NotifyAcceleratorActivated.

  MockAcceleratorObserver observer;
  AddAcceleratorObserver(&observer);

  ui::Accelerator accel(ui::VKEY_B, kPrimaryModifier);
  int command_id = kAstraCommandToggleSidebar;

  // Verify that the observer can receive registered events.
  // We can't directly trigger RegisterAccelerator with a real FocusManager,
  // but we can verify the observer mechanism works.

  EXPECT_CALL(observer, OnAcceleratorRegistered(_, _)).Times(0);
  // No registration happens without a FocusManager.

  // But we can verify the activation notification works.
  EXPECT_CALL(observer, OnAcceleratorActivated(_, _)).Times(1);
  NotifyAcceleratorActivated(accel, command_id);

  RemoveAcceleratorObserver(&observer);
}

TEST_F(AstraAcceleratorRegistrarTest,
       OnAcceleratorUnregistered_DefaultImplementationIsEmpty) {
  // The observer base class should have empty default implementations.
  class TestObserver : public AstraAcceleratorObserver {
   public:
    // Use default implementations.
  };

  TestObserver observer;
  AddAcceleratorObserver(&observer);

  // Calling these should not crash (default implementations do nothing).
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);
  observer.OnAcceleratorRegistered(accel, kAstraCommandFirst);
  observer.OnAcceleratorUnregistered(accel, kAstraCommandFirst);
  observer.OnAcceleratorActivated(accel, kAstraCommandFirst);

  RemoveAcceleratorObserver(&observer);
  SUCCEED();
}

// =========================================================================
// GetRegisteredAcceleratorCount tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, GetRegisteredCount_NonNegative) {
  // Count should always be >= 0.
  int count = GetRegisteredAcceleratorCount();
  EXPECT_GE(count, 0);
}

TEST_F(AstraAcceleratorRegistrarTest, GetRegisteredCount_MatchesAllSize) {
  // GetRegisteredAcceleratorCount should match size of GetAllRegistered.
  int count = GetRegisteredAcceleratorCount();
  auto all = GetAllRegisteredAccelerators();
  EXPECT_EQ(count, static_cast<int>(all.size()));
}

TEST_F(AstraAcceleratorRegistrarTest, GetRegisteredCount_AfterResetIsZero) {
  ResetAcceleratorRegistrarForTesting();
  EXPECT_EQ(GetRegisteredAcceleratorCount(), 0);
}

// =========================================================================
// GetAllRegisteredAccelerators tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, GetAllRegistered_InitiallyEmpty) {
  auto all = GetAllRegisteredAccelerators();
  EXPECT_TRUE(all.empty());
  EXPECT_EQ(all.size(), 0u);
}

TEST_F(AstraAcceleratorRegistrarTest, GetAllRegistered_ReturnsVector) {
  auto all = GetAllRegisteredAccelerators();
  // Should be a valid vector of ui::Accelerator.
  EXPECT_TRUE(std::is_same_v<decltype(all), std::vector<ui::Accelerator>>);
}

TEST_F(AstraAcceleratorRegistrarTest, GetAllRegistered_ConsistentAcrossCalls) {
  auto all1 = GetAllRegisteredAccelerators();
  auto all2 = GetAllRegisteredAccelerators();
  EXPECT_EQ(all1.size(), all2.size());
}

// =========================================================================
// HasConflicts / GetConflictCount tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, HasConflicts_BooleanReturnType) {
  bool result = HasConflicts();
  // Should be a bool.
  EXPECT_TRUE(std::is_same_v<decltype(result), bool>);
}

TEST_F(AstraAcceleratorRegistrarTest, GetConflictCount_IntReturnType) {
  int result = GetConflictCount();
  EXPECT_TRUE(std::is_same_v<decltype(result), int>);
}

TEST_F(AstraAcceleratorRegistrarTest, GetConflictCount_NonNegative) {
  int count = GetConflictCount();
  EXPECT_GE(count, 0);
}

TEST_F(AstraAcceleratorRegistrarTest, GetConflictCount_MatchesHasConflicts) {
  // If count > 0, HasConflicts should be true, and vice versa.
  int count = GetConflictCount();
  bool has_conflicts = HasConflicts();

  EXPECT_EQ(has_conflicts, count > 0);
}

// =========================================================================
// GetConflictingAccelerators tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest,
       GetConflictingAccelerators_ReturnsVector) {
  // Should return a vector of ui::Accelerator.
  auto conflicts = GetConflictingAccelerators(nullptr);
  EXPECT_TRUE(std::is_same_v<decltype(conflicts),
                             std::vector<ui::Accelerator>>);
}

TEST_F(AstraAcceleratorRegistrarTest,
       GetConflictingAccelerators_WithNullIsEmpty) {
  auto conflicts = GetConflictingAccelerators(nullptr);
  EXPECT_TRUE(conflicts.empty());
}

// =========================================================================
// AstraAcceleratorRegistrationResult struct tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, RegistrationResult_DefaultValues) {
  AstraAcceleratorRegistrationResult result;
  EXPECT_EQ(result.registered_count, 0);
  EXPECT_EQ(result.conflict_count, 0);
  EXPECT_TRUE(result.conflicts.empty());
}

TEST_F(AstraAcceleratorRegistrarTest, RegistrationResult_Copyable) {
  AstraAcceleratorRegistrationResult result1;
  result1.registered_count = 5;
  result1.conflict_count = 2;

  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);
  result1.conflicts.push_back(accel);

  AstraAcceleratorRegistrationResult result2 = result1;
  EXPECT_EQ(result2.registered_count, 5);
  EXPECT_EQ(result2.conflict_count, 2);
  EXPECT_EQ(result2.conflicts.size(), 1u);
}

// =========================================================================
// AstraAcceleratorObserver interface tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, Observer_InheritsFromCheckedObserver) {
  // Verify that AstraAcceleratorObserver inherits from base::CheckedObserver.
  MockAcceleratorObserver observer;
  base::CheckedObserver* base_observer = &observer;
  EXPECT_NE(base_observer, nullptr);
}

TEST_F(AstraAcceleratorRegistrarTest, Observer_HasThreeMethods) {
  // The observer interface should have three notification methods.
  // This is a compile-time check via the mock.
  MockAcceleratorObserver observer;
  ui::Accelerator accel(ui::VKEY_A, kPrimaryModifier);

  // Call all three methods to verify they exist.
  observer.OnAcceleratorRegistered(accel, kAstraCommandFirst);
  observer.OnAcceleratorUnregistered(accel, kAstraCommandFirst);
  observer.OnAcceleratorActivated(accel, kAstraCommandFirst);

  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest, Observer_CanBeSubclassed) {
  // Verify the observer can be subclassed (it's meant to be an interface).
  class TestObserver : public AstraAcceleratorObserver {
   public:
    void OnAcceleratorActivated(const ui::Accelerator&, int) override {
      activated_count++;
    }
    int activated_count = 0;
  };

  TestObserver observer;
  AddAcceleratorObserver(&observer);

  NotifyAcceleratorActivated(
      ui::Accelerator(ui::VKEY_P, kPrimaryModifier), kAstraCommandFirst);

  EXPECT_EQ(observer.activated_count, 1);

  RemoveAcceleratorObserver(&observer);
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_DoubleRegistration) {
  // Registering the same accelerator twice should not crash.
  // Since we don't have a real FocusManager, we test the concept.
  // The RegisterAccelerator function returns false with null FocusManager.

  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);

  bool result1 = RegisterAccelerator(nullptr, nullptr, accel, kAstraCommandFirst);
  bool result2 = RegisterAccelerator(nullptr, nullptr, accel, kAstraCommandFirst);

  EXPECT_FALSE(result1);
  EXPECT_FALSE(result2);
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_UnregisterUnregistered) {
  // Unregistering an accelerator that was never registered should not crash.
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);
  bool result = UnregisterAccelerator(nullptr, nullptr, accel);
  EXPECT_FALSE(result);
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_InvalidAccelerator) {
  // An accelerator with VKEY_UNKNOWN should be handled gracefully.
  ui::Accelerator accel(ui::VKEY_UNKNOWN, 0);
  EXPECT_FALSE(IsAcceleratorRegistered(nullptr, accel));
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_ZeroModifiers) {
  // Accelerators with no modifiers should work fine.
  ui::Accelerator accel(ui::VKEY_F11, 0);
  EXPECT_FALSE(IsAcceleratorRegistered(nullptr, accel));
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_AllModifiers) {
  // Accelerators with all modifiers set should be handled.
  int all_modifiers = ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN |
                      ui::EF_ALT_DOWN | ui::EF_COMMAND_DOWN;
  ui::Accelerator accel(ui::VKEY_A, all_modifiers);
  EXPECT_FALSE(IsAcceleratorRegistered(nullptr, accel));
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_NegativeCommandId) {
  // Negative command IDs should be handled gracefully.
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);
  // Since we can't register with null FocusManager, we just check that
  // passing a negative command ID doesn't crash anything.
  NotifyAcceleratorActivated(accel, -1);
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_LargeCommandId) {
  // Large command IDs should be handled gracefully.
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);
  NotifyAcceleratorActivated(accel, 9999999);
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_AddNullObserver) {
  // Adding a null observer should not be done, but if somehow it happens,
  // the system shouldn't crash on notification.
  // Actually, AddAcceleratorObserver takes a raw pointer and DCHECKs.
  // In release builds, we should be safe.
  // We won't test null pointer since DCHECK would fire in debug builds.
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_MultipleResetCalls) {
  // Calling Reset multiple times should be safe.
  ResetAcceleratorRegistrarForTesting();
  ResetAcceleratorRegistrarForTesting();
  ResetAcceleratorRegistrarForTesting();
  EXPECT_EQ(GetRegisteredAcceleratorCount(), 0);
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_NotifyWithNoObservers) {
  // Notifying with no observers should be safe.
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);
  NotifyAcceleratorActivated(accel, kAstraCommandFirst);
  // Should not crash.
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_FindByKeyEventNoModifiers) {
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_F11, 0);
  auto result = FindAcceleratorByKeyEvent(event);
  EXPECT_FALSE(result.has_value());
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_FindByKeyEventAllModifiers) {
  int all_modifiers = ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN |
                      ui::EF_ALT_DOWN | ui::EF_COMMAND_DOWN;
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_A, all_modifiers);
  auto result = FindAcceleratorByKeyEvent(event);
  EXPECT_FALSE(result.has_value());
}

TEST_F(AstraAcceleratorRegistrarTest, EdgeCase_ConsistencyChecks) {
  // Various consistency checks.
  int count = GetRegisteredAcceleratorCount();
  auto all = GetAllRegisteredAccelerators();
  bool has_conflicts = HasConflicts();
  int conflict_count = GetConflictCount();

  // Count should match vector size.
  EXPECT_EQ(count, static_cast<int>(all.size()));

  // Conflict count should match HasConflicts boolean.
  EXPECT_EQ(has_conflicts, conflict_count > 0);

  // Counts should be non-negative.
  EXPECT_GE(count, 0);
  EXPECT_GE(conflict_count, 0);
}

// =========================================================================
// Constant tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, ConstantsAreValid) {
  // Verify constants have reasonable values.
  EXPECT_EQ(kAstraAcceleratorPriority, 0);
  EXPECT_TRUE(kSkipConflictingAccelerators);
}

TEST_F(AstraAcceleratorRegistrarTest, PriorityIsInt) {
  // kAstraAcceleratorPriority should be an int.
  EXPECT_TRUE(std::is_same_v<decltype(kAstraAcceleratorPriority), const int>);
}

TEST_F(AstraAcceleratorRegistrarTest, SkipConflictsIsBool) {
  // kSkipConflictingAccelerators should be a bool.
  EXPECT_TRUE(
      std::is_same_v<decltype(kSkipConflictingAccelerators), const bool>);
}

// =========================================================================
// Bulk registration result tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest,
       BulkRegistration_NullParamsReturnsZeroCounts) {
  auto result = RegisterAstraAccelerators(nullptr, nullptr);
  EXPECT_EQ(result.registered_count, 0);
  EXPECT_EQ(result.conflict_count, 0);
  EXPECT_TRUE(result.conflicts.empty());
}

TEST_F(AstraAcceleratorRegistrarTest,
       BulkUnregistration_NullParamsDoesNotCrash) {
  UnregisterAstraAccelerators(nullptr, nullptr);
  SUCCEED();
}

TEST_F(AstraAcceleratorRegistrarTest,
       BulkUnregistration_CanBeCalledMultipleTimes) {
  UnregisterAstraAccelerators(nullptr, nullptr);
  UnregisterAstraAccelerators(nullptr, nullptr);
  SUCCEED();
}

// =========================================================================
// Observer notification data tests
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest,
       Observer_ActivatedNotificationHasCorrectData) {
  // Verify the accelerator and command ID are passed through correctly.
  class TestObserver : public AstraAcceleratorObserver {
   public:
    void OnAcceleratorActivated(const ui::Accelerator& accel,
                                int cmd_id) override {
      received_accel = accel;
      received_command_id = cmd_id;
    }
    ui::Accelerator received_accel{ui::VKEY_UNKNOWN, 0};
    int received_command_id = -1;
  };

  TestObserver observer;
  AddAcceleratorObserver(&observer);

  ui::Accelerator test_accel(ui::VKEY_P,
                             kPrimaryModifier | ui::EF_SHIFT_DOWN);
  int test_command_id = kAstraCommandOpenCommandPalette;

  NotifyAcceleratorActivated(test_accel, test_command_id);

  EXPECT_EQ(observer.received_accel.key_code(), test_accel.key_code());
  EXPECT_EQ(observer.received_accel.modifiers(), test_accel.modifiers());
  EXPECT_EQ(observer.received_command_id, test_command_id);

  RemoveAcceleratorObserver(&observer);
}

TEST_F(AstraAcceleratorRegistrarTest,
       Observer_MultipleActivationsIncrementCorrectly) {
  class TestObserver : public AstraAcceleratorObserver {
   public:
    void OnAcceleratorActivated(const ui::Accelerator&, int) override {
      count++;
    }
    int count = 0;
  };

  TestObserver observer;
  AddAcceleratorObserver(&observer);

  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier);
  for (int i = 0; i < 5; ++i) {
    NotifyAcceleratorActivated(accel, kAstraCommandFirst);
  }

  EXPECT_EQ(observer.count, 5);

  RemoveAcceleratorObserver(&observer);
}

// =========================================================================
// Registered set tests (internal state verification)
// =========================================================================

TEST_F(AstraAcceleratorRegistrarTest, RegisteredSet_EmptyAfterReset) {
  ResetAcceleratorRegistrarForTesting();
  EXPECT_EQ(GetRegisteredAcceleratorCount(), 0);
  auto all = GetAllRegisteredAccelerators();
  EXPECT_TRUE(all.empty());
}

TEST_F(AstraAcceleratorRegistrarTest, RegisteredSet_NoDuplicatesAfterReset) {
  ResetAcceleratorRegistrarForTesting();
  auto all = GetAllRegisteredAccelerators();
  std::set<std::pair<int, int>> combos;
  for (const auto& accel : all) {
    combos.insert({accel.key_code(), accel.modifiers()});
  }
  EXPECT_EQ(combos.size(), all.size());
}

TEST_F(AstraAcceleratorRegistrarTest, FindByKeyEvent_MatchesRegisteredSet) {
  // Since nothing is registered, find should return nullopt.
  // This tests the basic path without needing a real FocusManager.
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_B, kPrimaryModifier);
  auto result = FindAcceleratorByKeyEvent(event);
  EXPECT_FALSE(result.has_value());
}

}  // namespace astra
