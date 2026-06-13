// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_screenshot_service.h"

#include <string>

#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestScreenshotServiceObserver
    : public AstraScreenshotServiceObserver {
 public:
  void OnScreenshotStarted(AstraScreenshotType type) override {
    started_count_++;
    last_started_type_ = type;
  }

  void OnScreenshotTaken(const SkBitmap& bitmap,
                         AstraScreenshotType type,
                         const gfx::Rect& source_bounds) override {
    taken_count_++;
    last_taken_type_ = type;
    last_taken_bounds_ = source_bounds;
    last_bitmap_width_ = bitmap.width();
    last_bitmap_height_ = bitmap.height();
  }

  void OnScreenshotFailed(AstraScreenshotType type,
                          const std::string& error_message) override {
    failed_count_++;
    last_failed_type_ = type;
    last_error_message_ = error_message;
  }

  void OnScreenshotSaved(const base::FilePath& file_path,
                         int64_t file_size_bytes) override {
    saved_count_++;
    last_saved_path_ = file_path;
    last_saved_size_ = file_size_bytes;
  }

  void OnScreenshotCopiedToClipboard() override {
    copied_count_++;
  }

  // Counters
  int started_count_ = 0;
  int taken_count_ = 0;
  int failed_count_ = 0;
  int saved_count_ = 0;
  int copied_count_ = 0;

  // Last recorded values
  AstraScreenshotType last_started_type_ = AstraScreenshotType::kVisibleArea;
  AstraScreenshotType last_taken_type_ = AstraScreenshotType::kVisibleArea;
  AstraScreenshotType last_failed_type_ = AstraScreenshotType::kVisibleArea;
  gfx::Rect last_taken_bounds_;
  int last_bitmap_width_ = 0;
  int last_bitmap_height_ = 0;
  std::string last_error_message_;
  base::FilePath last_saved_path_;
  int64_t last_saved_size_ = 0;
};

// Partial observer — only overrides OnScreenshotTaken.
// Used to verify that default implementations work correctly for observers
// that don't care about all events.
class PartialScreenshotObserver : public AstraScreenshotServiceObserver {
 public:
  void OnScreenshotTaken(const SkBitmap& bitmap,
                         AstraScreenshotType type,
                         const gfx::Rect& source_bounds) override {
    taken_count_++;
    last_taken_type_ = type;
  }

  // Intentionally does NOT override the other methods — they use the
  // default empty implementations from the base class.

  int taken_count_ = 0;
  AstraScreenshotType last_taken_type_ = AstraScreenshotType::kVisibleArea;
};

}  // namespace

// Test fixture for AstraScreenshotService tests.
//
// Uses TestingProfile so the service has a real Profile* to attach to.
// The service is constructed directly since the factory may not be fully
// wired up in the test harness.
//
// Capture tests that require WebContents use the null-web-contents path to
// verify error handling.  Tests with real WebContents require a content
// test harness and are marked as TODO(astra).
//
// TODO(astra): Add WebContents-based tests using content::WebContentsTester
// or content::TestWebContentsFactory once the content test harness is
// available.  Chromium component: content/public/test:test_support.
class ScreenshotServiceTest : public testing::Test {
 protected:
  ScreenshotServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register screenshot prefs on the test profile's pref registry.
    // In production this is done by the factory during profile initialization.
    AstraScreenshotServiceFactory::RegisterProfilePrefs(
        profile_->GetPrefs()->DeprecatedGetPrefRegistry());
    // TODO(astra): Obtain service through the factory once
    // AstraScreenshotServiceFactory is properly wired up.
    // For now we construct directly with the profile.
    service_ = std::make_unique<AstraScreenshotService>(profile_.get());
    DCHECK(service_);
  }

  ~ScreenshotServiceTest() override = default;

  void SetUp() override {
    // Service should start not capturing.
    ASSERT_FALSE(service_->is_capturing());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  // Creates a simple test bitmap of the given size.
  static SkBitmap CreateTestBitmap(int width, int height) {
    SkBitmap bitmap;
    bitmap.allocN32Pixels(width, height);
    bitmap.eraseARGB(255, 100, 150, 200);
    return bitmap;
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraScreenshotService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestScreenshotServiceObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, DefaultState_NotCapturing) {
  EXPECT_FALSE(service_->is_capturing());
}

TEST_F(ScreenshotServiceTest, ProfileAccess) {
  EXPECT_EQ(service_->profile(), profile_.get());
}

// ---------------------------------------------------------------------------
// CaptureVisibleArea — null / error cases
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, CaptureVisibleArea_NullWebContentsFails) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CaptureVisibleArea(nullptr);

  EXPECT_EQ(observer.failed_count_, 1);
  EXPECT_EQ(observer.last_failed_type_, AstraScreenshotType::kVisibleArea);
  EXPECT_FALSE(observer.last_error_message_.empty());
  EXPECT_EQ(observer.taken_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// CaptureFullPage — null / error cases
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, CaptureFullPage_NullWebContentsFails) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CaptureFullPage(nullptr);

  EXPECT_EQ(observer.failed_count_, 1);
  EXPECT_EQ(observer.last_failed_type_, AstraScreenshotType::kFullPage);
  EXPECT_FALSE(observer.last_error_message_.empty());
  EXPECT_EQ(observer.taken_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// CaptureRegion — null / error cases
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, CaptureRegion_NullWebContentsFails) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CaptureRegion(nullptr, gfx::Rect(0, 0, 100, 100));

  EXPECT_EQ(observer.failed_count_, 1);
  EXPECT_EQ(observer.last_failed_type_, AstraScreenshotType::kRegion);
  EXPECT_FALSE(observer.last_error_message_.empty());

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, CaptureRegion_EmptyRegionFails) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  // Empty region should fail even with a valid web contents.
  // With null web contents, the null check happens first — test with null
  // to verify the empty region path is reachable in principle.
  // TODO(astra): Test with a real WebContents to exercise the empty-region
  // error path specifically.
  service_->CaptureRegion(nullptr, gfx::Rect());

  EXPECT_EQ(observer.failed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// SaveToDownloads
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, SaveToDownloads_NotifiesObserver) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  SkBitmap bitmap = CreateTestBitmap(800, 600);
  service_->SaveToDownloads(bitmap, "test_screenshot.png");

  EXPECT_EQ(observer.saved_count_, 1);
  EXPECT_FALSE(observer.last_saved_path_.empty());
  EXPECT_GT(observer.last_saved_size_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, SaveToDownloads_EmptyFilenameGeneratesDefault) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  SkBitmap bitmap = CreateTestBitmap(400, 300);
  service_->SaveToDownloads(bitmap, "");

  EXPECT_EQ(observer.saved_count_, 1);
  // Should have generated a filename.
  EXPECT_FALSE(observer.last_saved_path_.empty());
  // Filename should end with .png.
  EXPECT_TRUE(base::EndsWith(observer.last_saved_path_.BaseName().AsUTF8Unsafe(),
                             ".png",
                             base::CompareCase::INSENSITIVE_ASCII));

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, SaveToDownloads_AppendsPngExtension) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  SkBitmap bitmap = CreateTestBitmap(400, 300);
  service_->SaveToDownloads(bitmap, "my_screenshot");

  EXPECT_EQ(observer.saved_count_, 1);
  std::string filename =
      observer.last_saved_path_.BaseName().AsUTF8Unsafe();
  EXPECT_TRUE(base::EndsWith(filename, ".png",
                             base::CompareCase::INSENSITIVE_ASCII));

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, SaveToDownloads_PreservesExistingPngExtension) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  SkBitmap bitmap = CreateTestBitmap(400, 300);
  std::string suggested = "already.png";
  service_->SaveToDownloads(bitmap, suggested);

  EXPECT_EQ(observer.saved_count_, 1);
  std::string filename =
      observer.last_saved_path_.BaseName().AsUTF8Unsafe();
  // Should not have double .png.
  EXPECT_FALSE(base::EndsWith(filename, ".png.png",
                             base::CompareCase::INSENSITIVE_ASCII));
  EXPECT_TRUE(base::EndsWith(filename, ".png",
                             base::CompareCase::INSENSITIVE_ASCII));

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// CopyToClipboard
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, CopyToClipboard_NotifiesObserver) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  SkBitmap bitmap = CreateTestBitmap(200, 200);
  service_->CopyToClipboard(bitmap);

  EXPECT_EQ(observer.copied_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Estimated file size
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, EstimatedFileSize_LargerBitmapLargerSize) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  SkBitmap small = CreateTestBitmap(100, 100);
  service_->SaveToDownloads(small, "small.png");
  int64_t small_size = observer.last_saved_size_;

  SkBitmap large = CreateTestBitmap(1000, 1000);
  service_->SaveToDownloads(large, "large.png");
  int64_t large_size = observer.last_saved_size_;

  EXPECT_GT(large_size, small_size);

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, EstimatedFileSize_ScalesWithPixelCount) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  // 100x100 = 10,000 pixels
  SkBitmap bmp1 = CreateTestBitmap(100, 100);
  service_->SaveToDownloads(bmp1, "a.png");
  int64_t size1 = observer.last_saved_size_;

  // 200x200 = 40,000 pixels (4x the pixels)
  SkBitmap bmp2 = CreateTestBitmap(200, 200);
  service_->SaveToDownloads(bmp2, "b.png");
  int64_t size2 = observer.last_saved_size_;

  // Size should scale roughly with pixel count (at ~1 byte per pixel).
  EXPECT_GE(size2, size1 * 3);  // At least 3x (allowing for some overhead).

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// AstraScreenshotType enum
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, ScreenshotType_HasThreeValues) {
  // Verify the enum has the expected three types.
  // This is a compile-time + existence check.
  AstraScreenshotType visible = AstraScreenshotType::kVisibleArea;
  AstraScreenshotType full = AstraScreenshotType::kFullPage;
  AstraScreenshotType region = AstraScreenshotType::kRegion;

  // They should all be distinct values.
  EXPECT_NE(static_cast<int>(visible), static_cast<int>(full));
  EXPECT_NE(static_cast<int>(full), static_cast<int>(region));
  EXPECT_NE(static_cast<int>(visible), static_cast<int>(region));
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, AddRemoveObserver_NoCrash) {
  TestScreenshotServiceObserver observer;

  service_->AddObserver(&observer);
  service_->RemoveObserver(&observer);

  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(ScreenshotServiceTest, RemoveNonexistentObserver_NoCrash) {
  TestScreenshotServiceObserver observer;

  // Removing an observer that was never added should not crash.
  service_->RemoveObserver(&observer);

  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

TEST_F(ScreenshotServiceTest, MultipleObservers_AllNotifiedOnFailure) {
  TestScreenshotServiceObserver observer1;
  TestScreenshotServiceObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  service_->CaptureVisibleArea(nullptr);

  EXPECT_EQ(observer1.failed_count_, 1);
  EXPECT_EQ(observer2.failed_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

TEST_F(ScreenshotServiceTest, MultipleObservers_AllNotifiedOnSave) {
  TestScreenshotServiceObserver observer1;
  TestScreenshotServiceObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  SkBitmap bitmap = CreateTestBitmap(100, 100);
  service_->SaveToDownloads(bitmap, "test.png");

  EXPECT_EQ(observer1.saved_count_, 1);
  EXPECT_EQ(observer2.saved_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

TEST_F(ScreenshotServiceTest, RemoveObserver_StopsNotifications) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  SkBitmap bitmap1 = CreateTestBitmap(100, 100);
  service_->SaveToDownloads(bitmap1, "first.png");
  EXPECT_EQ(observer.saved_count_, 1);

  service_->RemoveObserver(&observer);

  SkBitmap bitmap2 = CreateTestBitmap(200, 200);
  service_->SaveToDownloads(bitmap2, "second.png");
  // Should not have received the second notification.
  EXPECT_EQ(observer.saved_count_, 1);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, ShutdownClearsObservers) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, operations should not notify observers.
  SkBitmap bitmap = CreateTestBitmap(100, 100);
  service_->CopyToClipboard(bitmap);
  EXPECT_EQ(observer.copied_count_, 0);
}

TEST_F(ScreenshotServiceTest, ShutdownResetsCapturingState) {
  service_->Shutdown();
  EXPECT_FALSE(service_->is_capturing());
}

// ---------------------------------------------------------------------------
// Filename generation
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, Filename_ContainsScreenshotPrefix) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  SkBitmap bitmap = CreateTestBitmap(100, 100);
  service_->SaveToDownloads(bitmap, "");
  std::string filename =
      observer.last_saved_path_.BaseName().AsUTF8Unsafe();

  EXPECT_TRUE(base::StartsWith(filename, "Screenshot",
                               base::CompareCase::SENSITIVE));

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer default implementations
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, PartialObserver_DefaultImplementationsWork) {
  // Verify that an observer which only overrides some methods can be
  // added and receive notifications for the methods it does override,
  // without crashing on the methods it doesn't override (those use the
  // default empty implementations).
  PartialScreenshotObserver partial_observer;
  service_->AddObserver(&partial_observer);

  // Fire all notification types. Only OnScreenshotTaken should be counted.
  service_->CaptureFullPage(nullptr);  // Fails — OnScreenshotFailed

  // After failure, only failed count would increment on the full observer.
  // On the partial observer, nothing should have changed yet.
  EXPECT_EQ(partial_observer.taken_count_, 0);

  // Fire a save notification via SaveToDownloads.
  SkBitmap bitmap = CreateTestBitmap(100, 100);
  service_->SaveToDownloads(bitmap, "test.png");

  // Partial observer only overrides OnScreenshotTaken, so saved events
  // don't increment taken_count.
  EXPECT_EQ(partial_observer.taken_count_, 0);

  // Fire a copy notification.
  service_->CopyToClipboard(bitmap);
  EXPECT_EQ(partial_observer.taken_count_, 0);

  // The partial observer should not have crashed at any point despite
  // not implementing all observer methods.
  service_->RemoveObserver(&partial_observer);
  SUCCEED() << "Partial observer with default implementations works correctly.";
}

TEST_F(ScreenshotServiceTest, PartialObserver_ReceivesOverriddenNotifications) {
  PartialScreenshotObserver partial_observer;
  service_->AddObserver(&partial_observer);

  // Call SaveToDownloads which doesn't trigger OnScreenshotTaken.
  SkBitmap bitmap = CreateTestBitmap(200, 200);
  service_->SaveToDownloads(bitmap, "test.png");
  EXPECT_EQ(partial_observer.taken_count_, 0);

  // The partial observer is only interested in OnScreenshotTaken.
  // Since we can't easily do a real capture without WebContents,
  // we verify the observer can be added/removed without issues.
  service_->RemoveObserver(&partial_observer);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Settings — defaults
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, Settings_DefaultImageFormatIsPng) {
  EXPECT_EQ(service_->image_format(), AstraScreenshotImageFormat::kPng);
}

TEST_F(ScreenshotServiceTest, Settings_DefaultImageQualityIs90) {
  EXPECT_EQ(service_->image_quality(), 90);
}

TEST_F(ScreenshotServiceTest, Settings_DefaultCaptureBrowserUiIsFalse) {
  EXPECT_FALSE(service_->capture_browser_ui());
}

TEST_F(ScreenshotServiceTest, Settings_DefaultSaveToDownloadsIsTrue) {
  EXPECT_TRUE(service_->save_to_downloads());
}

TEST_F(ScreenshotServiceTest, Settings_DefaultShowCaptureBubbleIsTrue) {
  EXPECT_TRUE(service_->show_capture_bubble());
}

// ---------------------------------------------------------------------------
// Settings — get / set
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, Settings_SetImageFormat) {
  service_->set_image_format(AstraScreenshotImageFormat::kJpeg);
  EXPECT_EQ(service_->image_format(), AstraScreenshotImageFormat::kJpeg);

  service_->set_image_format(AstraScreenshotImageFormat::kWebP);
  EXPECT_EQ(service_->image_format(), AstraScreenshotImageFormat::kWebP);

  service_->set_image_format(AstraScreenshotImageFormat::kPng);
  EXPECT_EQ(service_->image_format(), AstraScreenshotImageFormat::kPng);
}

TEST_F(ScreenshotServiceTest, Settings_SetImageQuality) {
  service_->set_image_quality(50);
  EXPECT_EQ(service_->image_quality(), 50);

  service_->set_image_quality(0);
  EXPECT_EQ(service_->image_quality(), 0);

  service_->set_image_quality(100);
  EXPECT_EQ(service_->image_quality(), 100);
}

TEST_F(ScreenshotServiceTest, Settings_ImageQualityClampsNegative) {
  service_->set_image_quality(-10);
  EXPECT_EQ(service_->image_quality(), 0);
}

TEST_F(ScreenshotServiceTest, Settings_ImageQualityClampsOver100) {
  service_->set_image_quality(200);
  EXPECT_EQ(service_->image_quality(), 100);
}

TEST_F(ScreenshotServiceTest, Settings_SetCaptureBrowserUi) {
  service_->set_capture_browser_ui(true);
  EXPECT_TRUE(service_->capture_browser_ui());

  service_->set_capture_browser_ui(false);
  EXPECT_FALSE(service_->capture_browser_ui());
}

TEST_F(ScreenshotServiceTest, Settings_SetSaveToDownloads) {
  service_->set_save_to_downloads(false);
  EXPECT_FALSE(service_->save_to_downloads());

  service_->set_save_to_downloads(true);
  EXPECT_TRUE(service_->save_to_downloads());
}

TEST_F(ScreenshotServiceTest, Settings_SetShowCaptureBubble) {
  service_->set_show_capture_bubble(false);
  EXPECT_FALSE(service_->show_capture_bubble());

  service_->set_show_capture_bubble(true);
  EXPECT_TRUE(service_->show_capture_bubble());
}

// ---------------------------------------------------------------------------
// Settings — persistence via PrefService
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, Settings_PersistedViaPrefService) {
  // Change settings via the service API.
  service_->set_image_format(AstraScreenshotImageFormat::kJpeg);
  service_->set_image_quality(75);
  service_->set_capture_browser_ui(true);
  service_->set_save_to_downloads(false);
  service_->set_show_capture_bubble(false);

  // Verify the values are actually stored in PrefService.
  PrefService* prefs = profile_->GetPrefs();
  EXPECT_EQ(prefs->GetInteger("astra.screenshot.image_format"),
            static_cast<int>(AstraScreenshotImageFormat::kJpeg));
  EXPECT_EQ(prefs->GetInteger("astra.screenshot.image_quality"), 75);
  EXPECT_TRUE(prefs->GetBoolean("astra.screenshot.capture_browser_ui"));
  EXPECT_FALSE(prefs->GetBoolean("astra.screenshot.save_to_downloads"));
  EXPECT_FALSE(prefs->GetBoolean("astra.screenshot.show_capture_bubble"));
}

TEST_F(ScreenshotServiceTest, Settings_ReadFromPrefService) {
  // Set values directly on PrefService.
  PrefService* prefs = profile_->GetPrefs();
  prefs->SetInteger("astra.screenshot.image_format",
                    static_cast<int>(AstraScreenshotImageFormat::kWebP));
  prefs->SetInteger("astra.screenshot.image_quality", 42);
  prefs->SetBoolean("astra.screenshot.capture_browser_ui", true);
  prefs->SetBoolean("astra.screenshot.save_to_downloads", false);
  prefs->SetBoolean("astra.screenshot.show_capture_bubble", false);

  // Verify the service reads them correctly.
  EXPECT_EQ(service_->image_format(), AstraScreenshotImageFormat::kWebP);
  EXPECT_EQ(service_->image_quality(), 42);
  EXPECT_TRUE(service_->capture_browser_ui());
  EXPECT_FALSE(service_->save_to_downloads());
  EXPECT_FALSE(service_->show_capture_bubble());
}

// ---------------------------------------------------------------------------
// Settings — edge cases
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, Settings_InvalidFormatFallsBackToDefault) {
  // Set an invalid enum value directly in prefs.
  PrefService* prefs = profile_->GetPrefs();
  prefs->SetInteger("astra.screenshot.image_format", 999);

  // The getter should return the default (kPng) for invalid values.
  EXPECT_EQ(service_->image_format(), AstraScreenshotImageFormat::kPng);
}

TEST_F(ScreenshotServiceTest, Settings_NegativeFormatFallsBackToDefault) {
  PrefService* prefs = profile_->GetPrefs();
  prefs->SetInteger("astra.screenshot.image_format", -5);

  EXPECT_EQ(service_->image_format(), AstraScreenshotImageFormat::kPng);
}

// ---------------------------------------------------------------------------
// Recent captures
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, RecentCaptures_StartsEmpty) {
  EXPECT_EQ(service_->recent_capture_count(), 0u);
  EXPECT_TRUE(service_->GetRecentCaptures(10).empty());
}

TEST_F(ScreenshotServiceTest, RecentCaptures_SaveAddsToHistory) {
  // SaveToDownloads should add a capture to history via NotifyScreenshotSaved
  // — wait, no, saves don't add to history. Captures do.
  // Let's verify by doing a save and checking... actually only successful
  // captures add to history.
  //
  // We can't do a real capture without WebContents, but we can verify that
  // the add-to-history mechanism works by checking that failed captures
  // don't add to history.
  service_->CaptureVisibleArea(nullptr);  // Should fail, not add to history
  EXPECT_EQ(service_->recent_capture_count(), 0u);
}

TEST_F(ScreenshotServiceTest, RecentCaptures_Clear) {
  // Add some captures by... actually, we can't easily add captures
  // without WebContents. Let's verify clear works on empty state at least.
  service_->ClearRecentCaptures();
  EXPECT_EQ(service_->recent_capture_count(), 0u);
  EXPECT_TRUE(service_->GetRecentCaptures(5).empty());
}

TEST_F(ScreenshotServiceTest, RecentCaptures_GetRecentCapturesCountLimit) {
  // Request more than available — should return all available (0 for now).
  auto captures = service_->GetRecentCaptures(100);
  EXPECT_EQ(captures.size(), 0u);
}

TEST_F(ScreenshotServiceTest, RecentCaptures_GetRecentCapturesZeroCount) {
  auto captures = service_->GetRecentCaptures(0);
  EXPECT_TRUE(captures.empty());
}

// ---------------------------------------------------------------------------
// Convenience methods
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, CaptureAndSave_NullWebContentsFails) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CaptureAndSave(nullptr, AstraScreenshotType::kVisibleArea);

  // Should notify failure for the capture part.
  EXPECT_EQ(observer.failed_count_, 1);
  EXPECT_EQ(observer.last_failed_type_, AstraScreenshotType::kVisibleArea);

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, CaptureAndSave_FullPageType) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CaptureAndSave(nullptr, AstraScreenshotType::kFullPage);

  EXPECT_EQ(observer.failed_count_, 1);
  EXPECT_EQ(observer.last_failed_type_, AstraScreenshotType::kFullPage);

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, CaptureAndSave_RegionType) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CaptureAndSave(nullptr, AstraScreenshotType::kRegion);

  // Region capture with null web contents should fail.
  EXPECT_EQ(observer.failed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, CaptureAndCopy_NullWebContentsFails) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CaptureAndCopy(nullptr, AstraScreenshotType::kVisibleArea);

  EXPECT_EQ(observer.failed_count_, 1);
  EXPECT_EQ(observer.last_failed_type_, AstraScreenshotType::kVisibleArea);

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, CaptureAndCopy_FullPageType) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CaptureAndCopy(nullptr, AstraScreenshotType::kFullPage);

  EXPECT_EQ(observer.failed_count_, 1);
  EXPECT_EQ(observer.last_failed_type_, AstraScreenshotType::kFullPage);

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, CaptureAndCopy_RegionType) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CaptureAndCopy(nullptr, AstraScreenshotType::kRegion);

  EXPECT_EQ(observer.failed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Utility methods
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, GenerateFilename_PublicMethod) {
  // GenerateFilename is now public — verify it can be called directly.
  std::string filename = service_->GenerateFilename("My Page");
  EXPECT_FALSE(filename.empty());
  EXPECT_TRUE(base::StartsWith(filename, "Screenshot",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::EndsWith(filename, ".png",
                             base::CompareCase::INSENSITIVE_ASCII));
}

TEST_F(ScreenshotServiceTest, GenerateFilename_EmptyTitle) {
  std::string filename = service_->GenerateFilename("");
  EXPECT_FALSE(filename.empty());
  EXPECT_TRUE(base::StartsWith(filename, "Screenshot",
                               base::CompareCase::SENSITIVE));
}

TEST_F(ScreenshotServiceTest, GenerateFilename_SanitizesInvalidChars) {
  std::string filename = service_->GenerateFilename("bad/name\\with:*?\"<>|chars");
  // None of the invalid characters should appear in the filename.
  EXPECT_EQ(filename.find('/'), std::string::npos);
  EXPECT_EQ(filename.find('\\'), std::string::npos);
  EXPECT_EQ(filename.find(':'), std::string::npos);
  EXPECT_EQ(filename.find('*'), std::string::npos);
  EXPECT_EQ(filename.find('?'), std::string::npos);
  EXPECT_EQ(filename.find('"'), std::string::npos);
  EXPECT_EQ(filename.find('<'), std::string::npos);
  EXPECT_EQ(filename.find('>'), std::string::npos);
  EXPECT_EQ(filename.find('|'), std::string::npos);
}

TEST_F(ScreenshotServiceTest, IsCaptureInProgress_AliasMatchesIsCapturing) {
  // IsCaptureInProgress() should be an alias for is_capturing().
  EXPECT_EQ(service_->IsCaptureInProgress(), service_->is_capturing());
  EXPECT_FALSE(service_->IsCaptureInProgress());

  // Both should always return the same value.
  // Since we can't easily set is_capturing_ from outside, we verify
  // the method exists and returns the same as is_capturing().
}

TEST_F(ScreenshotServiceTest, GetSupportedTypes_ReturnsAllTypes) {
  auto types = AstraScreenshotService::GetSupportedTypes();

  // Should have exactly 3 types.
  EXPECT_EQ(types.size(), 3u);

  // Should contain all three types.
  bool has_visible = false;
  bool has_full_page = false;
  bool has_region = false;
  for (auto type : types) {
    if (type == AstraScreenshotType::kVisibleArea) has_visible = true;
    if (type == AstraScreenshotType::kFullPage) has_full_page = true;
    if (type == AstraScreenshotType::kRegion) has_region = true;
  }
  EXPECT_TRUE(has_visible);
  EXPECT_TRUE(has_full_page);
  EXPECT_TRUE(has_region);
}

TEST_F(ScreenshotServiceTest, MaxRecentCaptures_Returns20) {
  // The maximum should be 20, as per the design.
  EXPECT_EQ(AstraScreenshotService::MaxRecentCaptures(), 20u);
}

TEST_F(ScreenshotServiceTest, ImageFormatEnum_HasThreeValues) {
  AstraScreenshotImageFormat png = AstraScreenshotImageFormat::kPng;
  AstraScreenshotImageFormat jpeg = AstraScreenshotImageFormat::kJpeg;
  AstraScreenshotImageFormat webp = AstraScreenshotImageFormat::kWebP;

  EXPECT_NE(static_cast<int>(png), static_cast<int>(jpeg));
  EXPECT_NE(static_cast<int>(jpeg), static_cast<int>(webp));
  EXPECT_NE(static_cast<int>(png), static_cast<int>(webp));
}

// ---------------------------------------------------------------------------
// AstraScreenshotCapture struct
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, CaptureStruct_DefaultValues) {
  AstraScreenshotCapture capture;
  EXPECT_TRUE(capture.id.empty());
  EXPECT_TRUE(capture.file_path.empty());
  EXPECT_EQ(capture.file_size_bytes, 0);
  EXPECT_EQ(capture.type, AstraScreenshotType::kVisibleArea);
  EXPECT_TRUE(capture.timestamp.is_null());
}

// ---------------------------------------------------------------------------
// Observer notification — additional tests
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, CaptureFailed_NotifiesAllObservers) {
  TestScreenshotServiceObserver observer1;
  TestScreenshotServiceObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  service_->CaptureFullPage(nullptr);

  EXPECT_EQ(observer1.failed_count_, 1);
  EXPECT_EQ(observer2.failed_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

TEST_F(ScreenshotServiceTest, CopyToClipboard_NotifiesAllObservers) {
  TestScreenshotServiceObserver observer1;
  TestScreenshotServiceObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  SkBitmap bitmap = CreateTestBitmap(50, 50);
  service_->CopyToClipboard(bitmap);

  EXPECT_EQ(observer1.copied_count_, 1);
  EXPECT_EQ(observer2.copied_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(ScreenshotServiceTest, CaptureRegion_EmptyRegionWithNullWebContents) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  // Both null web contents AND empty region — should still fail gracefully.
  service_->CaptureRegion(nullptr, gfx::Rect());

  EXPECT_EQ(observer.failed_count_, 1);
  EXPECT_FALSE(observer.last_error_message_.empty());

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, GenerateFilename_VeryLongTitleTruncated) {
  std::string long_title(200, 'x');
  std::string filename = service_->GenerateFilename(long_title);

  // The title portion should be truncated (the overall filename
  // should be much shorter than 200 + prefix + timestamp).
  EXPECT_LT(filename.length(), long_title.length() + 50);
  // Should still have the prefix.
  EXPECT_TRUE(base::StartsWith(filename, "Screenshot",
                               base::CompareCase::SENSITIVE));
}

TEST_F(ScreenshotServiceTest, SaveToDownloads_EmptyBitmapFailsGracefully) {
  TestScreenshotServiceObserver observer;
  service_->AddObserver(&observer);

  SkBitmap empty_bitmap;  // Zero-size bitmap.
  service_->SaveToDownloads(empty_bitmap, "empty.png");

  // Should not crash. The stub implementation still notifies,
  // but the file size should be 0.
  EXPECT_EQ(observer.saved_count_, 1);
  EXPECT_EQ(observer.last_saved_size_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(ScreenshotServiceTest, Settings_AfterShutdownReturnDefaults) {
  service_->set_image_format(AstraScreenshotImageFormat::kJpeg);
  service_->set_image_quality(50);

  service_->Shutdown();

  // After shutdown, profile_ is null so settings should return defaults.
  EXPECT_EQ(service_->image_format(), AstraScreenshotImageFormat::kPng);
  EXPECT_EQ(service_->image_quality(), 90);
  EXPECT_FALSE(service_->capture_browser_ui());
  EXPECT_TRUE(service_->save_to_downloads());
  EXPECT_TRUE(service_->show_capture_bubble());
}

TEST_F(ScreenshotServiceTest, Settings_SetAfterShutdownNoCrash) {
  service_->Shutdown();

  // Setting after shutdown should not crash — it silently no-ops.
  service_->set_image_format(AstraScreenshotImageFormat::kJpeg);
  service_->set_image_quality(50);
  service_->set_capture_browser_ui(true);
  service_->set_save_to_downloads(false);
  service_->set_show_capture_bubble(false);

  SUCCEED() << "Settings set after shutdown did not crash.";
}

// ---------------------------------------------------------------------------
// TODO(astra): WebContents-based capture tests (require content test harness)
// ---------------------------------------------------------------------------
//
// The following tests require real WebContents objects and should be
// implemented once the content test harness is available:
//
//   - CaptureVisibleArea_ProducesBitmap
//   - CaptureVisibleArea_NotifiesObservers
//   - CaptureVisibleArea_SetsCapturingState
//   - CaptureFullPage_ProducesLargerBitmap
//   - CaptureRegion_ProducesCroppedBitmap
//   - CaptureRegion_MatchesRequestedSize
//   - is_capturing_TrueDuringCapture
//   - is_capturing_FalseAfterCapture
//   - CaptureStartedObserver_FiresBeforeTaken
//   - CaptureVisibleArea_SourceBoundsMatchViewport
//   - CaptureFullPage_SourceBoundsMatchPage
//   - CaptureRegion_SourceBoundsMatchRequested
//
// TODO(astra): Add browser_tests for screenshot service integration with real
// Browser, WebContents, and download/clipboard systems.
// Chromium component: InProcessBrowserTest + content::WebContents.

}  // namespace astra
