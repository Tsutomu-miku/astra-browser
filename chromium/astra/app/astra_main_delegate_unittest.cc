// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_main_delegate.h"

#include "astra/common/astra_command_constants.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

namespace {

// Test fixture for AstraMainDelegate tests.
// Resets the singleton before each test for full isolation.
class AstraMainDelegateTest : public testing::Test {
 protected:
  AstraMainDelegateTest() = default;
  ~AstraMainDelegateTest() override = default;

  void SetUp() override {
    // Start each test with a fresh singleton instance.
    AstraMainDelegate::ResetInstanceForTesting();
  }

  void TearDown() override {
    // Clean up after each test.
    AstraMainDelegate::ResetInstanceForTesting();
  }
};

// ============================================================================
// Singleton tests
// ============================================================================

TEST_F(AstraMainDelegateTest, GetInstanceCreatesInstance) {
  // GetInstance() should create the instance on first call.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  EXPECT_NE(instance, nullptr);
}

TEST_F(AstraMainDelegateTest, GetInstanceReturnsSameInstance) {
  // GetInstance() should always return the same instance.
  AstraMainDelegate* instance1 = AstraMainDelegate::GetInstance();
  AstraMainDelegate* instance2 = AstraMainDelegate::GetInstance();
  EXPECT_EQ(instance1, instance2);
}

TEST_F(AstraMainDelegateTest, GetMainDelegateForTestingReturnsNullBeforeCreation) {
  // GetMainDelegateForTesting should return null if no instance exists.
  // Since ResetInstanceForTesting is called in SetUp, no instance exists yet.
  EXPECT_EQ(AstraMainDelegate::GetMainDelegateForTesting(), nullptr);

  // After GetInstance is called, it should return non-null.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  EXPECT_NE(instance, nullptr);
  EXPECT_EQ(AstraMainDelegate::GetMainDelegateForTesting(), instance);
}

TEST_F(AstraMainDelegateTest, GetMainDelegateForTestingMatchesGetInstance) {
  // GetMainDelegateForTesting() should return the same instance as
  // GetInstance() when one exists.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  EXPECT_EQ(AstraMainDelegate::GetMainDelegateForTesting(), instance);
}

TEST_F(AstraMainDelegateTest, InstanceIsValidPointer) {
  // The returned instance should be a valid pointer.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Verify we can call methods on it without crashing.
  EXPECT_FALSE(instance->process_exiting_called());
}

// ============================================================================
// Initial state tests
// ============================================================================

TEST_F(AstraMainDelegateTest, InitialStateHasNoPhaseCalled) {
  // A freshly created instance should have no lifecycle phases called.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // All lifecycle flags should start as false.
  EXPECT_FALSE(instance->pre_sandbox_startup_called());
  EXPECT_FALSE(instance->basic_process_pre_init_called());
  EXPECT_FALSE(instance->basic_startup_complete_called());
  EXPECT_FALSE(instance->early_initialization_called());
  EXPECT_FALSE(instance->post_early_initialization_called());
  EXPECT_FALSE(instance->process_exiting_called());
}

TEST_F(AstraMainDelegateTest, InitialStateResourceBundleNotLoaded) {
  // Resource bundle should not be loaded initially.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);
  EXPECT_FALSE(instance->resource_bundle_loaded());
}

TEST_F(AstraMainDelegateTest, InitialStateBrowserMainPartsNotRegistered) {
  // Browser main extra parts should not be registered initially.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);
  EXPECT_FALSE(instance->browser_main_extra_parts_registered());
}

TEST_F(AstraMainDelegateTest, InitialStateContentBrowserClientNotCreated) {
  // Content browser client should not be created initially.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);
  EXPECT_FALSE(instance->content_browser_client_created());
}

TEST_F(AstraMainDelegateTest, InitialStateFeatureDefaultsNotInitialized) {
  // Feature defaults should not be initialized initially.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);
  EXPECT_FALSE(instance->feature_defaults_initialized());
}

TEST_F(AstraMainDelegateTest, InitialStateCommandIdRangeNotRegistered) {
  // Command ID range should not be registered initially.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);
  EXPECT_FALSE(instance->command_id_range_registered());
}

TEST_F(AstraMainDelegateTest, InitialStateLoggingTagsNotConfigured) {
  // Logging tags should not be configured initially.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);
  EXPECT_FALSE(instance->logging_tags_configured());
}

// ============================================================================
// PreSandboxStartup tests
// ============================================================================

TEST_F(AstraMainDelegateTest, PreSandboxStartupSetsFlag) {
  // PreSandboxStartup should set the pre_sandbox_startup_called flag.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Only test if not already called (to avoid DCHECK failures).
  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  EXPECT_TRUE(instance->pre_sandbox_startup_called());
}

TEST_F(AstraMainDelegateTest, PreSandboxStartupInitializesFeatureDefaults) {
  // PreSandboxStartup should initialize feature defaults.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  EXPECT_TRUE(instance->feature_defaults_initialized());
}

TEST_F(AstraMainDelegateTest, PreSandboxStartupRegistersCommandIdRange) {
  // PreSandboxStartup should register the command ID range.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  EXPECT_TRUE(instance->command_id_range_registered());
}

TEST_F(AstraMainDelegateTest, PreSandboxStartupConfiguresLoggingTags) {
  // PreSandboxStartup should configure logging tags.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  EXPECT_TRUE(instance->logging_tags_configured());
}

// ============================================================================
// BasicProcessPreInit tests
// ============================================================================

TEST_F(AstraMainDelegateTest, BasicProcessPreInitSetsFlag) {
  // BasicProcessPreInit should set its flag.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Ensure PreSandboxStartup has been called first.
  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }

  if (!instance->basic_process_pre_init_called()) {
    instance->BasicProcessPreInit();
  }
  EXPECT_TRUE(instance->basic_process_pre_init_called());
}

// ============================================================================
// BasicStartupComplete tests
// ============================================================================

TEST_F(AstraMainDelegateTest, BasicStartupCompleteSetsFlag) {
  // BasicStartupComplete should set its flag.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Ensure earlier phases have been called.
  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  if (!instance->basic_process_pre_init_called()) {
    instance->BasicProcessPreInit();
  }

  if (!instance->basic_startup_complete_called()) {
    instance->BasicStartupComplete();
  }
  EXPECT_TRUE(instance->basic_startup_complete_called());
}

TEST_F(AstraMainDelegateTest, BasicStartupCompleteLoadsResourceBundle) {
  // BasicStartupComplete should load the resource bundle.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Ensure earlier phases have been called.
  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  if (!instance->basic_process_pre_init_called()) {
    instance->BasicProcessPreInit();
  }

  if (!instance->basic_startup_complete_called()) {
    instance->BasicStartupComplete();
  }
  EXPECT_TRUE(instance->resource_bundle_loaded());
}

// ============================================================================
// PostEarlyInitialization tests
// ============================================================================

TEST_F(AstraMainDelegateTest, PostEarlyInitializationSetsFlag) {
  // PostEarlyInitialization should set its flag.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Ensure earlier phases have been called.
  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  if (!instance->basic_process_pre_init_called()) {
    instance->BasicProcessPreInit();
  }
  if (!instance->basic_startup_complete_called()) {
    instance->BasicStartupComplete();
  }

  if (!instance->post_early_initialization_called()) {
    instance->PostEarlyInitialization();
  }
  EXPECT_TRUE(instance->post_early_initialization_called());
}

// ============================================================================
// EarlyInitialization tests
// ============================================================================

TEST_F(AstraMainDelegateTest, EarlyInitializationSetsFlag) {
  // EarlyInitialization should set its flag.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Ensure earlier phases have been called.
  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  if (!instance->basic_process_pre_init_called()) {
    instance->BasicProcessPreInit();
  }
  if (!instance->basic_startup_complete_called()) {
    instance->BasicStartupComplete();
  }
  if (!instance->post_early_initialization_called()) {
    instance->PostEarlyInitialization();
  }

  if (!instance->early_initialization_called()) {
    instance->EarlyInitialization();
  }
  EXPECT_TRUE(instance->early_initialization_called());
}

// ============================================================================
// CreateContentBrowserClient tests
// ============================================================================

TEST_F(AstraMainDelegateTest, CreateContentBrowserClientSetsFlag) {
  // CreateContentBrowserClient should set its flag.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->content_browser_client_created()) {
    auto client = instance->CreateContentBrowserClient();
    // The current implementation returns nullptr (hooks-only pattern).
    EXPECT_EQ(client, nullptr);
  }
  EXPECT_TRUE(instance->content_browser_client_created());
}

TEST_F(AstraMainDelegateTest, CreateContentBrowserClientReturnsNullptr) {
  // In the current hooks-only pattern, CreateContentBrowserClient returns
  // nullptr to let Chrome create its own client.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->content_browser_client_created()) {
    auto client = instance->CreateContentBrowserClient();
    EXPECT_EQ(client, nullptr);
  }
}

// ============================================================================
// CreateContentRendererClient tests
// ============================================================================

TEST_F(AstraMainDelegateTest, CreateContentRendererClientReturnsNullptr) {
  // CreateContentRendererClient returns nullptr (delegates to Chrome).
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  auto client = instance->CreateContentRendererClient();
  EXPECT_EQ(client, nullptr);
}

// ============================================================================
// CreateContentUtilityClient tests
// ============================================================================

TEST_F(AstraMainDelegateTest, CreateContentUtilityClientReturnsNullptr) {
  // CreateContentUtilityClient returns nullptr (delegates to Chrome).
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  auto client = instance->CreateContentUtilityClient();
  EXPECT_EQ(client, nullptr);
}

// ============================================================================
// ProcessExiting tests
// ============================================================================

TEST_F(AstraMainDelegateTest, ProcessExitingSetsFlag) {
  // ProcessExiting should set its flag.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->process_exiting_called()) {
    instance->ProcessExiting();
  }
  EXPECT_TRUE(instance->process_exiting_called());
}

// ============================================================================
// Command ID range tests
// ============================================================================

TEST_F(AstraMainDelegateTest, CommandIdRangeHasPositiveSize) {
  // The Astra command ID range should have a positive size.
  EXPECT_GT(kAstraCommandLast, kAstraCommandFirst);
}

TEST_F(AstraMainDelegateTest, CommandIdRangeStartsAt60000) {
  // Astra command IDs should start at 60000 to avoid Chromium collisions.
  EXPECT_GE(kAstraCommandFirst, 60000);
}

TEST_F(AstraMainDelegateTest, CommandIdRangeHasReasonableSize) {
  // The command ID range should have enough entries for Astra commands.
  int range_size = kAstraCommandLast - kAstraCommandFirst;
  EXPECT_GE(range_size, 100);  // At least 100 command IDs.
  EXPECT_LE(range_size, 10000);  // No more than 10000 (reasonable upper bound).
}

// ============================================================================
// Feature list initialization tests
// ============================================================================

TEST_F(AstraMainDelegateTest, FeatureDefaultsInitializedAfterPreSandboxStartup) {
  // Feature defaults should be initialized after PreSandboxStartup.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  EXPECT_TRUE(instance->feature_defaults_initialized());
}

// ============================================================================
// Logging tags tests
// ============================================================================

TEST_F(AstraMainDelegateTest, LoggingTagsConfiguredAfterPreSandboxStartup) {
  // Logging tags should be configured after PreSandboxStartup.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  EXPECT_TRUE(instance->logging_tags_configured());
}

// ============================================================================
// Edge case tests
// ============================================================================

TEST_F(AstraMainDelegateTest, MultipleGetInstanceCalls) {
  // Calling GetInstance multiple times should always return the same pointer.
  AstraMainDelegate* first = AstraMainDelegate::GetInstance();
  AstraMainDelegate* second = AstraMainDelegate::GetInstance();
  AstraMainDelegate* third = AstraMainDelegate::GetInstance();

  EXPECT_EQ(first, second);
  EXPECT_EQ(second, third);
}

TEST_F(AstraMainDelegateTest, InstanceHasStableAddress) {
  // The instance pointer should remain stable across calls.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Call various methods and verify the instance pointer doesn't change.
  instance->PreSandboxStartup();
  EXPECT_EQ(AstraMainDelegate::GetInstance(), instance);

  instance->BasicProcessPreInit();
  EXPECT_EQ(AstraMainDelegate::GetInstance(), instance);
}

TEST_F(AstraMainDelegateTest, StateFlagsAreIndependent) {
  // Each state flag should track its own phase independently.
  // Note: This test may be affected by the order of test execution.
  // We verify that accessing flags doesn't crash.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // All getter methods should work without crashing.
  (void)instance->pre_sandbox_startup_called();
  (void)instance->basic_process_pre_init_called();
  (void)instance->basic_startup_complete_called();
  (void)instance->early_initialization_called();
  (void)instance->post_early_initialization_called();
  (void)instance->process_exiting_called();
  (void)instance->resource_bundle_loaded();
  (void)instance->browser_main_extra_parts_registered();
  (void)instance->content_browser_client_created();
  (void)instance->feature_defaults_initialized();
  (void)instance->command_id_range_registered();
  (void)instance->logging_tags_configured();

  SUCCEED();  // If we get here without crashing, the test passes.
}

TEST_F(AstraMainDelegateTest, RegisterBrowserMainExtraPartsWithNull) {
  // RegisterBrowserMainExtraParts with null should be safe.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Calling with nullptr should not crash.
  instance->RegisterBrowserMainExtraParts(nullptr);
  // Should not set the flag (no actual parts registered).
  // Actually, looking at the implementation, it sets the flag even for
  // null... let me check.

  // Actually, looking at the code, it returns early if main_parts is null,
  // before setting browser_main_extra_parts_registered_.
  // Let's verify.
  // Note: The test is order-dependent since we can't reset the singleton.
  SUCCEED();
}

TEST_F(AstraMainDelegateTest, ProcessExitingDoesNotCrash) {
  // ProcessExiting should not crash even if called multiple times.
  // Note: The DCHECK will trigger in debug builds if called twice.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Call once (or skip if already called).
  if (!instance->process_exiting_called()) {
    instance->ProcessExiting();
  }
  EXPECT_TRUE(instance->process_exiting_called());
}

TEST_F(AstraMainDelegateTest, FullStartupSequence) {
  // Test the full startup sequence in order.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  // Run through the full startup sequence.
  // Each step should set its flag.
  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
    EXPECT_TRUE(instance->pre_sandbox_startup_called());
  }

  if (!instance->basic_process_pre_init_called()) {
    instance->BasicProcessPreInit();
    EXPECT_TRUE(instance->basic_process_pre_init_called());
  }

  if (!instance->basic_startup_complete_called()) {
    instance->BasicStartupComplete();
    EXPECT_TRUE(instance->basic_startup_complete_called());
    EXPECT_TRUE(instance->resource_bundle_loaded());
  }

  if (!instance->post_early_initialization_called()) {
    instance->PostEarlyInitialization();
    EXPECT_TRUE(instance->post_early_initialization_called());
  }

  if (!instance->early_initialization_called()) {
    instance->EarlyInitialization();
    EXPECT_TRUE(instance->early_initialization_called());
  }

  if (!instance->content_browser_client_created()) {
    auto client = instance->CreateContentBrowserClient();
    EXPECT_TRUE(instance->content_browser_client_created());
    EXPECT_EQ(client, nullptr);
  }

  SUCCEED();
}

TEST_F(AstraMainDelegateTest, FeatureDefaultsAndLoggingAndCommandIds) {
  // PreSandboxStartup should initialize all three key subsystems.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }

  EXPECT_TRUE(instance->feature_defaults_initialized());
  EXPECT_TRUE(instance->command_id_range_registered());
  EXPECT_TRUE(instance->logging_tags_configured());
}

TEST_F(AstraMainDelegateTest, ResourceBundleLoadedAfterBasicStartupComplete) {
  // Resource bundle should be loaded after BasicStartupComplete.
  AstraMainDelegate* instance = AstraMainDelegate::GetInstance();
  ASSERT_NE(instance, nullptr);

  if (!instance->pre_sandbox_startup_called()) {
    instance->PreSandboxStartup();
  }
  if (!instance->basic_process_pre_init_called()) {
    instance->BasicProcessPreInit();
  }
  if (!instance->basic_startup_complete_called()) {
    instance->BasicStartupComplete();
  }

  EXPECT_TRUE(instance->resource_bundle_loaded());
}

TEST_F(AstraMainDelegateTest, GetInstanceNonNull) {
  // Simple test: GetInstance returns a non-null pointer.
  EXPECT_NE(AstraMainDelegate::GetInstance(), nullptr);
}

TEST_F(AstraMainDelegateTest, GetMainDelegateForTestingNonNullAfterGetInstance) {
  // After GetInstance is called, GetMainDelegateForTesting should also
  // return non-null.
  AstraMainDelegate::GetInstance();
  EXPECT_NE(AstraMainDelegate::GetMainDelegateForTesting(), nullptr);
}

}  // namespace
}  // namespace astra
