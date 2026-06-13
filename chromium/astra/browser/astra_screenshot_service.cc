#include "astra/browser/astra_screenshot_service.h"

#include <string>

#include "base/files/file_path.h"
#include "base/i18n/time_formatting.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace astra {

namespace {

// Default suggested filename prefix for screenshots.
constexpr char kScreenshotFilenamePrefix[] = "Screenshot";

// Maximum length of the page title portion of the filename (in characters).
constexpr size_t kMaxTitleLength = 50;

// Approximate bytes per pixel for a PNG-encoded screenshot.
// Used for estimating file size before actual encoding.
// Real PNG compression varies, but ~1 byte per pixel is a conservative
// estimate for typical web content screenshots.
constexpr float kEstimatedBytesPerPixel = 1.0f;

// Maximum number of recent captures kept in memory.
constexpr size_t kMaxRecentCaptures = 20;

// -- Pref keys ---------------------------------------------------------------
//
// Screenshot settings persisted via PrefService.
// Keys are defined here (not in astra_prefs.h) per the module boundary.
//
// TODO(astra): Consider moving these to a shared pref key header if other
//   services need to read screenshot settings directly from PrefService.
//   For now they are internal to the screenshot service.

constexpr char kPrefImageFormat[] = "astra.screenshot.image_format";
constexpr char kPrefImageQuality[] = "astra.screenshot.image_quality";
constexpr char kPrefCaptureBrowserUi[] = "astra.screenshot.capture_browser_ui";
constexpr char kPrefSaveToDownloads[] = "astra.screenshot.save_to_downloads";
constexpr char kPrefShowCaptureBubble[] = "astra.screenshot.show_capture_bubble";

// Default values
constexpr int kDefaultImageFormat =
    static_cast<int>(AstraScreenshotImageFormat::kPng);
constexpr int kDefaultImageQuality = 90;
constexpr bool kDefaultCaptureBrowserUi = false;
constexpr bool kDefaultSaveToDownloads = true;
constexpr bool kDefaultShowCaptureBubble = true;

}  // namespace

// ---------------------------------------------------------------------------
// AstraScreenshotService
// ---------------------------------------------------------------------------

AstraScreenshotService::AstraScreenshotService(Profile* profile)
    : profile_(profile) {}

AstraScreenshotService::~AstraScreenshotService() = default;

void AstraScreenshotService::Shutdown() {
  // Clear observer references and drop profile pointer before the profile
  // goes away.
  observers_.Clear();
  profile_ = nullptr;
  is_capturing_ = false;
}

// -- Observers ---------------------------------------------------------------

void AstraScreenshotService::AddObserver(
    AstraScreenshotServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraScreenshotService::RemoveObserver(
    AstraScreenshotServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Capture operations ------------------------------------------------------

void AstraScreenshotService::CaptureVisibleArea(content::WebContents* web_contents) {
  if (!web_contents) {
    NotifyCaptureFailed(AstraScreenshotType::kVisibleArea,
                        "No active web contents");
    return;
  }

  NotifyCaptureStarted(AstraScreenshotType::kVisibleArea);
  is_capturing_ = true;

  // TODO(astra): Use proper Chromium screenshot capture API.
  //
  // For visible area capture, the recommended approach is:
  //   - content::WebContents::GetContentBitmap() — captures the viewport.
  //   - Or use content::ScreenshotManager if available.
  //
  // Chromium owner: content::WebContents / content::ScreenshotManager
  //   (content/public/browser/web_contents.h,
  //    content/browser/screenshot/screenshot_manager.h)
  //
  // Patch point: None needed for GetContentBitmap — it's a public API.
  //   If ScreenshotManager is used, a tiny patch may be needed to expose
  //   the API or to add a hook that calls back into Astra.
  //
  // The capture is asynchronous — the result comes back via a callback.
  // Typical pattern:
  //   web_contents->GetContentBitmap(
  //       base::BindOnce(&AstraScreenshotService::OnCaptureComplete,
  //                      weak_ptr_factory_.GetWeakPtr(), type));

  // For now, create a placeholder bitmap to demonstrate the flow.
  // In the real implementation, this would come from the capture callback.
  gfx::Size viewport_size = web_contents->GetContainerBounds().size();
  if (viewport_size.IsEmpty()) {
    viewport_size = gfx::Size(800, 600);
  }

  SkBitmap placeholder;
  placeholder.allocN32Pixels(viewport_size.width(), viewport_size.height());
  placeholder.eraseARGB(255, 200, 200, 200);  // Gray placeholder.

  gfx::Rect source_bounds(viewport_size);

  // Simulate async completion by posting the notification.
  // In the real implementation, this is called from the capture callback.
  NotifyScreenshotTaken(placeholder, AstraScreenshotType::kVisibleArea,
                        source_bounds);
  is_capturing_ = false;
}

void AstraScreenshotService::CaptureFullPage(content::WebContents* web_contents) {
  if (!web_contents) {
    NotifyCaptureFailed(AstraScreenshotType::kFullPage,
                        "No active web contents");
    return;
  }

  NotifyCaptureStarted(AstraScreenshotType::kFullPage);
  is_capturing_ = true;

  // TODO(astra): Implement full-page capture using Chromium's full-page
  //   screenshot feature.
  //
  // Approaches:
  //   1. Use chrome/browser/screenshot/ScreenshotManager if it exists.
  //   2. Use chrome/browser/share/share_manager.h for share screenshots.
  //   3. Implement scrolling capture: capture viewport, scroll, repeat,
  //      then stitch the bitmaps together.
  //
  // Chromium owner: ScreenshotManager / ShareManager
  //   (chrome/browser/screenshot/, chrome/browser/share/)
  //
  // Patch point: May need to expose the full-page capture API or add
  //   an Astra hook in ScreenshotManager.
  //
  // Full page capture is more complex than visible area because it
  // requires scrolling or using the compositor's full-page snapshot.

  // For now, use the viewport size as a placeholder.
  // Real full-page capture would use the document scroll size.
  gfx::Size viewport_size = web_contents->GetContainerBounds().size();
  if (viewport_size.IsEmpty()) {
    viewport_size = gfx::Size(800, 600);
  }
  // Full page placeholder is 3x the viewport height (simulating a tall page).
  gfx::Size full_size(viewport_size.width(), viewport_size.height() * 3);

  SkBitmap placeholder;
  placeholder.allocN32Pixels(full_size.width(), full_size.height());
  placeholder.eraseARGB(255, 210, 210, 210);  // Light gray placeholder.

  gfx::Rect source_bounds(full_size);

  NotifyScreenshotTaken(placeholder, AstraScreenshotType::kFullPage,
                        source_bounds);
  is_capturing_ = false;
}

void AstraScreenshotService::CaptureRegion(content::WebContents* web_contents,
                                           const gfx::Rect& region) {
  if (!web_contents) {
    NotifyCaptureFailed(AstraScreenshotType::kRegion,
                        "No active web contents");
    return;
  }

  if (region.IsEmpty()) {
    NotifyCaptureFailed(AstraScreenshotType::kRegion,
                        "Empty capture region");
    return;
  }

  NotifyCaptureStarted(AstraScreenshotType::kRegion);
  is_capturing_ = true;

  // TODO(astra): Implement region capture.
  //
  // Approach:
  //   1. Capture the visible area (or full page if region extends beyond
  //      viewport).
  //   2. Crop the resulting bitmap to the requested region.
  //
  // For regions within the viewport: capture visible area + crop.
  // For regions spanning beyond the viewport: full page capture + crop.
  //
  // Chromium owner: SkBitmap / SkCanvas for cropping operations.
  //   (third_party/skia/include/core/SkBitmap.h)
  //
  // The cropping itself is straightforward SkBitmap operations — the
  // complexity is in determining whether to do a viewport or full-page
  // capture based on the region bounds.

  // Create a placeholder bitmap of the requested region size.
  SkBitmap placeholder;
  placeholder.allocN32Pixels(region.width(), region.height());
  placeholder.eraseARGB(255, 220, 220, 220);  // Lighter gray placeholder.

  NotifyScreenshotTaken(placeholder, AstraScreenshotType::kRegion, region);
  is_capturing_ = false;
}

// -- Post-capture operations -------------------------------------------------

void AstraScreenshotService::SaveToDownloads(
    const SkBitmap& bitmap,
    const std::string& suggested_filename) {
  // TODO(astra): Implement proper save-to-downloads using Chromium's
  //   download system.
  //
  // Approaches:
  //   1. Use DownloadService to create a download from the bitmap data.
  //   2. Use DownloadManager::DownloadUrl with a data: URI.
  //   3. Write the file directly to the downloads folder and register
  //      it with the download history.
  //
  // Chromium owner: DownloadService / DownloadManager
  //   (chrome/browser/download/download_service.h,
  //    content/public/browser/download_manager.h)
  //
  // Patch point: None needed — uses public download API.
  //
  // The save path should use the user's downloads directory, obtained from
  // Profile::GetDownloadPrefs() or the download service defaults.

  std::string filename = suggested_filename;
  if (filename.empty()) {
    filename = GenerateFilename("Untitled");
  }
  if (!base::EndsWith(filename, ".png",
                      base::CompareCase::INSENSITIVE_ASCII)) {
    filename += ".png";
  }

  // For now, estimate the file size and notify observers.
  // In the real implementation, the download system would provide actual
  // file size and path via callbacks.
  int64_t estimated_size = EstimateFileSize(bitmap);

  // Construct a placeholder file path for the notification.
  // In the real implementation, this comes from the download system.
  base::FilePath file_path = base::FilePath::FromUTF8Unsafe(filename);

  NotifyScreenshotSaved(file_path, estimated_size);
}

void AstraScreenshotService::CopyToClipboard(const SkBitmap& bitmap) {
  // TODO(astra): Implement clipboard copy using ui::Clipboard.
  //
  // The clipboard API is straightforward:
  //   ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  //   clipboard->WriteImage(bitmap);
  //
  // Chromium owner: ui::Clipboard (ui/base/clipboard/clipboard.h)
  // Patch point: None needed — uses public Clipboard API.

  // For now, just notify observers that the copy "succeeded".
  NotifyCopiedToClipboard();
}

// -- Convenience operations --------------------------------------------------

void AstraScreenshotService::CaptureAndSave(content::WebContents* web_contents,
                                            AstraScreenshotType type) {
  // TODO(astra): Implement full capture + save pipeline using real Chromium
  //   capture APIs and download service.
  //   Chromium owner: ScreenshotManager + DownloadService
  //   Patch point: None needed — uses public APIs.

  // For the stub implementation, dispatch to the appropriate capture method
  // and then save to downloads. The real implementation would chain these
  // as async operations: capture callback -> save -> save callback.
  switch (type) {
    case AstraScreenshotType::kVisibleArea:
      CaptureVisibleArea(web_contents);
      break;
    case AstraScreenshotType::kFullPage:
      CaptureFullPage(web_contents);
      break;
    case AstraScreenshotType::kRegion:
      // For region capture without a region parameter, use the full viewport
      // as a fallback. In the real implementation this would show a region
      // selection UI first.
      // TODO(astra): Region selection UI flow for CaptureAndSave with kRegion.
      //   Chromium owner: views/widget for region selection overlay.
      CaptureVisibleArea(web_contents);
      break;
  }

  // If capture failed (e.g., null WebContents), don't proceed to save.
  // We check is_capturing_ as a proxy — if it never went true, capture failed.
  // In the real implementation, this would be in the capture callback.
  //
  // For the stub, since capture is synchronous, we can't easily tell if it
  // succeeded or failed from here. In practice, the observer would handle
  // the save flow.
  //
  // TODO(astra): Properly chain capture -> save as async operations.
}

void AstraScreenshotService::CaptureAndCopy(content::WebContents* web_contents,
                                            AstraScreenshotType type) {
  // TODO(astra): Implement full capture + copy pipeline using real Chromium
  //   capture APIs and clipboard.
  //   Chromium owner: ScreenshotManager + ui::Clipboard
  //   Patch point: None needed — uses public APIs.

  // Same pattern as CaptureAndSave — stub dispatches to capture method.
  switch (type) {
    case AstraScreenshotType::kVisibleArea:
      CaptureVisibleArea(web_contents);
      break;
    case AstraScreenshotType::kFullPage:
      CaptureFullPage(web_contents);
      break;
    case AstraScreenshotType::kRegion:
      // TODO(astra): Region selection UI flow for CaptureAndCopy with kRegion.
      CaptureVisibleArea(web_contents);
      break;
  }

  // TODO(astra): Chain capture -> copy as async operations.
  // In the real implementation, OnScreenshotTaken callback would call
  // CopyToClipboard with the captured bitmap.
}

// -- Settings ----------------------------------------------------------------

AstraScreenshotImageFormat AstraScreenshotService::image_format() const {
  if (!profile_) {
    return static_cast<AstraScreenshotImageFormat>(kDefaultImageFormat);
  }
  int value = profile_->GetPrefs()->GetInteger(kPrefImageFormat);
  // Clamp to valid enum range.
  if (value < 0 || value > static_cast<int>(AstraScreenshotImageFormat::kWebP)) {
    return static_cast<AstraScreenshotImageFormat>(kDefaultImageFormat);
  }
  return static_cast<AstraScreenshotImageFormat>(value);
}

void AstraScreenshotService::set_image_format(AstraScreenshotImageFormat format) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetInteger(
      kPrefImageFormat, static_cast<int>(format));
}

int AstraScreenshotService::image_quality() const {
  if (!profile_) {
    return kDefaultImageQuality;
  }
  return profile_->GetPrefs()->GetInteger(kPrefImageQuality);
}

void AstraScreenshotService::set_image_quality(int quality) {
  if (!profile_) {
    return;
  }
  // Clamp to valid 0-100 range.
  if (quality < 0) {
    quality = 0;
  }
  if (quality > 100) {
    quality = 100;
  }
  profile_->GetPrefs()->SetInteger(kPrefImageQuality, quality);
}

bool AstraScreenshotService::capture_browser_ui() const {
  if (!profile_) {
    return kDefaultCaptureBrowserUi;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefCaptureBrowserUi);
}

void AstraScreenshotService::set_capture_browser_ui(bool capture) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefCaptureBrowserUi, capture);
}

bool AstraScreenshotService::save_to_downloads() const {
  if (!profile_) {
    return kDefaultSaveToDownloads;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefSaveToDownloads);
}

void AstraScreenshotService::set_save_to_downloads(bool save) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefSaveToDownloads, save);
}

bool AstraScreenshotService::show_capture_bubble() const {
  if (!profile_) {
    return kDefaultShowCaptureBubble;
  }
  return profile_->GetPrefs()->GetBoolean(kPrefShowCaptureBubble);
}

void AstraScreenshotService::set_show_capture_bubble(bool show) {
  if (!profile_) {
    return;
  }
  profile_->GetPrefs()->SetBoolean(kPrefShowCaptureBubble, show);
}

// -- Recent captures ---------------------------------------------------------

std::vector<AstraScreenshotCapture>
AstraScreenshotService::GetRecentCaptures(size_t count) const {
  std::vector<AstraScreenshotCapture> result;
  size_t actual_count = std::min(count, recent_captures_.size());
  // Return in reverse chronological order (newest first).
  for (size_t i = 0; i < actual_count; ++i) {
    result.push_back(recent_captures_[recent_captures_.size() - 1 - i]);
  }
  return result;
}

void AstraScreenshotService::ClearRecentCaptures() {
  recent_captures_.clear();
}

// -- Utility -----------------------------------------------------------------

// static
std::vector<AstraScreenshotType> AstraScreenshotService::GetSupportedTypes() {
  return {
      AstraScreenshotType::kVisibleArea,
      AstraScreenshotType::kFullPage,
      AstraScreenshotType::kRegion,
  };
}

// static
size_t AstraScreenshotService::MaxRecentCaptures() {
  return kMaxRecentCaptures;
}

// -- Private helpers ---------------------------------------------------------

std::string AstraScreenshotService::GenerateFilename(
    const std::string& page_title) const {
  // Build a filename like: "Screenshot - Page Title - 2024-01-15 at 14.30.45.png"
  // This follows the pattern used by macOS and Chrome's screenshot feature.

  // Truncate and sanitize the page title.
  std::string sanitized_title = page_title;
  if (sanitized_title.length() > kMaxTitleLength) {
    sanitized_title = sanitized_title.substr(0, kMaxTitleLength) + "...";
  }
  // Replace characters that are invalid in filenames.
  base::ReplaceChars(sanitized_title, "/\\:*?\"<>|", "_", &sanitized_title);

  // Format the timestamp.
  base::Time now = base::Time::Now();
  std::u16string time_str =
      base::TimeFormatShortDateAndTimeWithTimeZone(now);
  std::string time_str_utf8 = base::UTF16ToUTF8(time_str);

  // Build the full filename.
  std::string filename = kScreenshotFilenamePrefix;
  if (!sanitized_title.empty()) {
    filename += " - " + sanitized_title;
  }
  filename += " - " + time_str_utf8 + ".png";

  return filename;
}

int64_t AstraScreenshotService::EstimateFileSize(const SkBitmap& bitmap) const {
  // Rough estimate: width * height * bytes_per_pixel for PNG.
  // This is a very rough estimate — actual PNG size depends on content
  // complexity and compression. Used only for UI display before the
  // file is actually saved.
  int64_t pixel_count =
      static_cast<int64_t>(bitmap.width()) * bitmap.height();
  return static_cast<int64_t>(pixel_count * kEstimatedBytesPerPixel);
}

void AstraScreenshotService::AddRecentCapture(AstraScreenshotType type,
                                              const base::FilePath& file_path,
                                              int64_t file_size_bytes) {
  AstraScreenshotCapture capture;
  capture.id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  capture.timestamp = base::Time::Now();
  capture.type = type;
  capture.file_path = file_path;
  capture.file_size_bytes = file_size_bytes;

  recent_captures_.push_back(capture);

  // Trim to max size if needed.
  while (recent_captures_.size() > kMaxRecentCaptures) {
    recent_captures_.erase(recent_captures_.begin());
  }
}

// -- Observer notification helpers -------------------------------------------

void AstraScreenshotService::NotifyCaptureStarted(AstraScreenshotType type) {
  for (auto& observer : observers_) {
    observer.OnScreenshotStarted(type);
  }
}

void AstraScreenshotService::NotifyScreenshotTaken(const SkBitmap& bitmap,
                                                   AstraScreenshotType type,
                                                   const gfx::Rect& source_bounds) {
  // Add to recent captures history on successful capture.
  // File path is empty (not yet saved); size is estimated from the bitmap.
  AddRecentCapture(type, base::FilePath(), EstimateFileSize(bitmap));

  for (auto& observer : observers_) {
    observer.OnScreenshotTaken(bitmap, type, source_bounds);
  }
}

void AstraScreenshotService::NotifyCaptureFailed(AstraScreenshotType type,
                                                 const std::string& error_message) {
  for (auto& observer : observers_) {
    observer.OnScreenshotFailed(type, error_message);
  }
}

void AstraScreenshotService::NotifyScreenshotSaved(
    const base::FilePath& file_path,
    int64_t file_size_bytes) {
  for (auto& observer : observers_) {
    observer.OnScreenshotSaved(file_path, file_size_bytes);
  }
}

void AstraScreenshotService::NotifyCopiedToClipboard() {
  for (auto& observer : observers_) {
    observer.OnScreenshotCopiedToClipboard();
  }
}

// ---------------------------------------------------------------------------
// AstraScreenshotServiceFactory
// ---------------------------------------------------------------------------

// static
AstraScreenshotService* AstraScreenshotServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AstraScreenshotService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AstraScreenshotServiceFactory* AstraScreenshotServiceFactory::GetInstance() {
  static base::NoDestructor<AstraScreenshotServiceFactory> instance;
  return instance.get();
}

// static
void AstraScreenshotServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Register screenshot-related preferences.
  //
  // All settings are persisted via PrefService using the profile's pref
  // store. No custom file I/O is used.
  //
  // Chromium component: PrefService / PrefRegistry.
  // Patch point: profile keyed service registration.

  registry->RegisterIntegerPref(kPrefImageFormat, kDefaultImageFormat);
  registry->RegisterIntegerPref(kPrefImageQuality, kDefaultImageQuality);
  registry->RegisterBooleanPref(kPrefCaptureBrowserUi, kDefaultCaptureBrowserUi);
  registry->RegisterBooleanPref(kPrefSaveToDownloads, kDefaultSaveToDownloads);
  registry->RegisterBooleanPref(kPrefShowCaptureBubble, kDefaultShowCaptureBubble);
}

AstraScreenshotServiceFactory::AstraScreenshotServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraScreenshotService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // Screenshots are ephemeral and context-specific. Each
              // profile (regular and incognito) gets its own service
              // instance so that incognito screenshot operations don't
              // share observer state with the regular profile.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest sessions get their own ephemeral instance.
              .WithSystem(ProfileSelection::kNone)
              // System profile has no user-visible screenshots.
              .Build()) {}

AstraScreenshotServiceFactory::~AstraScreenshotServiceFactory() = default;

KeyedService*
AstraScreenshotServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return new AstraScreenshotService(Profile::FromBrowserContext(context));
}

}  // namespace astra
