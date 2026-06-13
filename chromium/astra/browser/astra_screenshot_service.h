#ifndef ASTRA_BROWSER_ASTRA_SCREENSHOT_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_SCREENSHOT_SERVICE_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace astra {

// =========================================================================
// Screenshot capture type
// =========================================================================
//
// Identifies which kind of screenshot was taken. Used by observers to
// determine the appropriate UI reaction (e.g., show capture bubble,
// show region overlay, etc.).
//
// Chromium subsystems reused:
//   - content::WebContents::GetContentBitmap (visible area capture)
//   - chrome/browser/screenshot/ (full page capture, if available)
//   - ui/base/clipboard/clipboard.h (copy to clipboard)
//   - chrome/browser/download/download_service.h (save to downloads)
//
// Chromium patch points:
//   - None required for basic visible-area capture (uses public WebContents API).
//   - Full page capture may require patching into
//     chrome/browser/screenshot/screenshot_manager.cc or using
//     content::ScreenshotManager if available in the content layer.
// =========================================================================

enum class AstraScreenshotType {
  kVisibleArea,   // Current viewport only.
  kFullPage,      // Entire scrollable page.
  kRegion,        // User-dragged rectangular region.
};

// Image format for screenshot encoding.
//
// Chromium subsystems reused:
//   - third_party/skia (PNG, JPEG, WebP encoding via SkEncoder)
//   - ui/gfx/codec (image encoding utilities)
enum class AstraScreenshotImageFormat {
  kPng,   // Lossless PNG — default.
  kJpeg,  // Lossy JPEG.
  kWebP,  // WebP (lossy or lossless).
};

// =========================================================================
// AstraScreenshotCapture
// =========================================================================
//
// Metadata about a single screenshot capture. Stored in the recent captures
// history (in-memory only — not persisted to disk).
//
// The service does not own the bitmap data long-term; this struct holds only
// metadata about captures that have occurred.
// =========================================================================

struct AstraScreenshotCapture {
  // Unique identifier for this capture.
  std::string id;

  // When the capture was taken.
  base::Time timestamp;

  // Which kind of capture this was.
  AstraScreenshotType type = AstraScreenshotType::kVisibleArea;

  // Path to the saved file, if saved. Empty if not yet saved.
  base::FilePath file_path;

  // Size of the capture file in bytes. May be an estimate if the file
  // has not been written yet.
  int64_t file_size_bytes = 0;
};

// =========================================================================
// AstraScreenshotServiceObserver
// =========================================================================
//
// Observer interface for UI layers (capture bubble, region overlay) to
// react to screenshot events. UI must never be the source of truth —
// AstraScreenshotService and Chromium capture APIs are.
//
// Screenshots are asynchronous operations (bitmap capture, file save,
// clipboard write). Observers receive notifications at each stage.
// =========================================================================

class AstraScreenshotServiceObserver : public base::CheckedObserver {
 public:
  // Called when a screenshot capture has started (before the bitmap is
  // ready). The UI can show a "capturing..." indicator.
  // |type| indicates which kind of capture is in progress.
  virtual void OnScreenshotStarted(AstraScreenshotType type) {}

  // Called when a screenshot has been successfully captured.
  // |bitmap| contains the captured image data.
  // |type| indicates which kind of capture was performed.
  // |source_bounds| is the region of the page that was captured, in
  //   page coordinates (for kFullPage this is the full document size;
  //   for kVisibleArea it is the viewport; for kRegion it is the
  //   user-selected rectangle).
  virtual void OnScreenshotTaken(const SkBitmap& bitmap,
                                 AstraScreenshotType type,
                                 const gfx::Rect& source_bounds) {}

  // Called when a screenshot capture has failed.
  // |type| indicates which kind of capture failed.
  // |error_message| is a human-readable description of the failure.
  virtual void OnScreenshotFailed(AstraScreenshotType type,
                                  const std::string& error_message) {}

  // Called when a screenshot has been saved to the downloads folder.
  // |file_path| is the absolute path to the saved file.
  // |file_size_bytes| is the size of the saved file in bytes.
  virtual void OnScreenshotSaved(const base::FilePath& file_path,
                                 int64_t file_size_bytes) {}

  // Called when a screenshot has been copied to the clipboard.
  virtual void OnScreenshotCopiedToClipboard() {}

 protected:
  ~AstraScreenshotServiceObserver() override = default;
};

// =========================================================================
// AstraScreenshotService
// =========================================================================
//
// Profile-scoped keyed service that handles screenshot capture operations.
//
// This service is a thin wrapper around Chromium's screenshot and capture
// APIs. It provides a unified Astra-level interface for:
//   - Visible area capture (viewport)
//   - Full page capture (entire scrollable document)
//   - Region capture (user-dragged rectangle)
//   - Save to downloads folder
//   - Copy to clipboard
//
// Truth source:
//   - Chromium's capture APIs produce the bitmap data (truth).
//   - This service orchestrates the capture flow and notifies observers.
//   - The screenshot bitmap is ephemeral — it is passed to observers and
//     then released. No screenshot state is persisted in this service.
//
// Not owned here:
//   - The actual capture engine (Chromium's ScreenshotManager / WebContents).
//   - Download service (Chromium owns downloads).
//   - Clipboard (Chromium ui/base/clipboard owns it).
//
// TODO(astra): Use proper Chromium screenshot API instead of the stub
//   implementation. Options include:
//     - content::WebContents::GetContentBitmap() for visible area.
//     - chrome/browser/screenshot/ScreenshotManager for full-page capture.
//     - content::ScreenshotManager if available in the content layer.
//   Chromium owner: ScreenshotManager / CaptureResult
//     (content/browser/screenshot/ or chrome/browser/screenshot/)
//   Patch point: May require a tiny patch to expose the capture API or
//     to add a hook in ScreenshotManager that delegates into Astra.
// =========================================================================

class AstraScreenshotService final : public KeyedService {
 public:
  explicit AstraScreenshotService(Profile* profile);
  AstraScreenshotService(const AstraScreenshotService&) = delete;
  AstraScreenshotService& operator=(const AstraScreenshotService&) = delete;
  ~AstraScreenshotService() override;

  // KeyedService:
  void Shutdown() override;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraScreenshotServiceObserver* observer);
  void RemoveObserver(AstraScreenshotServiceObserver* observer);

  // -- Capture operations ------------------------------------------------

  // Capture the visible area (viewport) of |web_contents|.
  // The capture is asynchronous; observers are notified when complete.
  //
  // TODO(astra): Implement using content::WebContents::GetContentBitmap()
  //   or the equivalent Chromium capture API. The current implementation
  //   is a stub that fires observer notifications with a placeholder bitmap.
  //   Chromium owner: content::WebContents / content::ScreenshotManager
  void CaptureVisibleArea(content::WebContents* web_contents);

  // Capture the full page (entire scrollable document) of |web_contents|.
  // The capture is asynchronous; observers are notified when complete.
  //
  // TODO(astra): Implement full-page capture using Chromium's full-page
  //   screenshot feature (e.g., ScreenshotManager::CaptureFullPage or
  //   the share/screenshot subsystem). Full-page capture requires
  //   stitching multiple viewport captures or using a compositor snapshot.
  //   Chromium owner: chrome/browser/screenshot/ or chrome/browser/share/
  //   Patch point: ScreenshotManager or ShareManager full-page capture path.
  void CaptureFullPage(content::WebContents* web_contents);

  // Capture a specific |region| (in viewport coordinates) of |web_contents|.
  // This captures the visible area and then crops to |region|.
  // The capture is asynchronous; observers are notified when complete.
  //
  // TODO(astra): Implement region capture by capturing the visible area
  //   and cropping the resulting bitmap to the requested region.
  //   For regions that extend beyond the viewport, combine with scrolling
  //   capture (full page + crop).
  void CaptureRegion(content::WebContents* web_contents,
                     const gfx::Rect& region);

  // -- Post-capture operations -------------------------------------------

  // Save the given |bitmap| to the downloads folder as a PNG file.
  // The filename is auto-generated based on the page title and timestamp.
  // The save is asynchronous; OnScreenshotSaved is called on completion.
  //
  // TODO(astra): Implement using Chromium's download system
  //   (DownloadService / DownloadManager) so the screenshot appears in
  //   the download shelf and download history.
  //   Chromium owner: chrome/browser/download/download_service.h
  //   Patch point: None needed — uses public DownloadService API.
  void SaveToDownloads(const SkBitmap& bitmap,
                       const std::string& suggested_filename);

  // Copy the given |bitmap| to the system clipboard as an image.
  // OnScreenshotCopiedToClipboard is called on completion.
  //
  // TODO(astra): Implement using ui::Clipboard::WriteImage().
  //   Chromium owner: ui/base/clipboard/clipboard.h
  //   Patch point: None needed — uses public Clipboard API.
  void CopyToClipboard(const SkBitmap& bitmap);

  // -- Convenience operations ----------------------------------------------

  // Capture a screenshot of |type| from |web_contents| and automatically
  // save it to the downloads folder. Combines capture + save in one call.
  // Equivalent to calling Capture*() followed by SaveToDownloads().
  //
  // The filename is auto-generated based on the page title.
  // Observers are notified for both capture and save events.
  //
  // TODO(astra): Wire up to use the real capture + save pipeline.
  //   Chromium owner: ScreenshotManager + DownloadService
  void CaptureAndSave(content::WebContents* web_contents,
                      AstraScreenshotType type);

  // Capture a screenshot of |type| from |web_contents| and automatically
  // copy it to the clipboard. Combines capture + copy in one call.
  //
  // Observers are notified for both capture and copy events.
  //
  // TODO(astra): Wire up to use the real capture + clipboard pipeline.
  //   Chromium owner: ScreenshotManager + ui::Clipboard
  void CaptureAndCopy(content::WebContents* web_contents,
                      AstraScreenshotType type);

  // -- Settings ------------------------------------------------------------

  // Returns the image format used for saving screenshots.
  AstraScreenshotImageFormat image_format() const;

  // Sets the image format used for saving screenshots.
  // The setting is persisted via PrefService.
  void set_image_format(AstraScreenshotImageFormat format);

  // Returns the image quality (0-100) for lossy formats (JPEG, WebP lossy).
  // Has no effect on PNG (lossless).
  int image_quality() const;

  // Sets the image quality for lossy formats.
  // Values outside 0-100 are clamped.
  // The setting is persisted via PrefService.
  void set_image_quality(int quality);

  // Returns whether browser UI (address bar, tab strip, etc.) should be
  // included in screenshots.
  bool capture_browser_ui() const;

  // Sets whether to include browser UI in screenshots.
  // The setting is persisted via PrefService.
  //
  // TODO(astra): Implement browser UI capture. Capturing browser chrome
  //   requires capturing the views::Widget or Browser window, not just
  //   the WebContents.
  //   Chromium owner: ui/views/widget/widget.h, aura::Window
  //   Patch point: May need a widget-level capture API.
  void set_capture_browser_ui(bool capture);

  // Returns whether screenshots are automatically saved to downloads.
  bool save_to_downloads() const;

  // Sets whether screenshots are automatically saved to downloads after
  // capture. The setting is persisted via PrefService.
  void set_save_to_downloads(bool save);

  // Returns whether the capture preview bubble is shown after capture.
  bool show_capture_bubble() const;

  // Sets whether to show the capture preview bubble after a successful
  // capture. The setting is persisted via PrefService.
  void set_show_capture_bubble(bool show);

  // -- Recent captures -----------------------------------------------------

  // Returns metadata for the most recent captures, up to |count| entries.
  // Entries are returned in reverse chronological order (newest first).
  std::vector<AstraScreenshotCapture> GetRecentCaptures(size_t count) const;

  // Returns the number of captures currently stored in the recent history.
  size_t recent_capture_count() const { return recent_captures_.size(); }

  // Clears all recent capture history.
  // Observers are not notified for this operation.
  void ClearRecentCaptures();

  // -- Query --------------------------------------------------------------

  // Returns whether a capture operation is currently in progress.
  bool is_capturing() const { return is_capturing_; }

  // Alias for is_capturing() for readability in calling code.
  bool IsCaptureInProgress() const { return is_capturing_; }

  // Returns the profile associated with this service.
  Profile* profile() { return profile_; }
  const Profile* profile() const { return profile_; }

  // -- Utility -------------------------------------------------------------

  // Generates a filename for a screenshot based on |base_title|.
  // Format: "Screenshot - <title> - YYYY-MM-DD at HH.MM.SS.png"
  //
  // The filename is sanitized to remove characters that are invalid in
  // filenames across common operating systems.
  //
  // Public version — usable by UI layers and other services that need
  // to generate screenshot filenames without performing a capture.
  std::string GenerateFilename(const std::string& base_title) const;

  // Returns all supported screenshot capture types.
  static std::vector<AstraScreenshotType> GetSupportedTypes();

  // Returns the maximum number of recent captures stored in memory.
  static size_t MaxRecentCaptures();

 private:
  // Helper: compute approximate file size for a PNG-encoded bitmap.
  // Used for the capture bubble display before the file is actually saved.
  // TODO(astra): Replace with actual encoded size once the save path
  //   is implemented through the download service.
  int64_t EstimateFileSize(const SkBitmap& bitmap) const;

  // Helper: add a capture entry to the recent captures history.
  // Trims the history to MaxRecentCaptures() if needed.
  void AddRecentCapture(AstraScreenshotType type,
                        const base::FilePath& file_path,
                        int64_t file_size_bytes);

  // Notify observers that capture has started.
  void NotifyCaptureStarted(AstraScreenshotType type);

  // Notify observers that a screenshot was successfully captured.
  void NotifyScreenshotTaken(const SkBitmap& bitmap,
                             AstraScreenshotType type,
                             const gfx::Rect& source_bounds);

  // Notify observers that capture failed.
  void NotifyCaptureFailed(AstraScreenshotType type,
                           const std::string& error_message);

  // Notify observers that a screenshot was saved.
  void NotifyScreenshotSaved(const base::FilePath& file_path,
                             int64_t file_size_bytes);

  // Notify observers that a screenshot was copied to clipboard.
  void NotifyCopiedToClipboard();

  raw_ptr<Profile> profile_;
  base::ObserverList<AstraScreenshotServiceObserver> observers_;

  // Whether a capture operation is currently in progress.
  // TODO(astra): Make this more granular to handle multiple concurrent
  //   capture types (e.g., per-WebContents capture state).
  bool is_capturing_ = false;

  // Recent capture history (in-memory, not persisted).
  // Stored in chronological order (oldest first); GetRecentCaptures
  // returns them in reverse order (newest first).
  std::vector<AstraScreenshotCapture> recent_captures_;
};

// =========================================================================
// AstraScreenshotServiceFactory
// =========================================================================
//
// Factory for AstraScreenshotService.
//
// Incognito behavior: the factory uses kOwnInstance for incognito profiles
// because screenshot operations are ephemeral and context-specific — an
// incognito window should have its own screenshot service instance to
// ensure that screenshot data (clipboard, saved files) does not leak
// between incognito and regular profiles' UI state.
//
// Note: the actual screenshots (bitmap data, saved files) are not stored
// in the service; this is about the service instance and its observers.
// =========================================================================

class AstraScreenshotServiceFactory final : public ProfileKeyedServiceFactory {
 public:
  static AstraScreenshotService* GetForProfile(Profile* profile);
  static AstraScreenshotServiceFactory* GetInstance();

  // Registers screenshot-related prefs on the profile's PrefRegistry.
  // TODO(astra): Register prefs for screenshot settings (default format,
  //   save location preference, capture-with-browser-UI flag, etc.).
  //   Chromium patch point: chrome/browser/profiles/profile_keyed_service_factory*.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  AstraScreenshotServiceFactory();
  ~AstraScreenshotServiceFactory() override;

  // ProfileKeyedServiceFactory:
  KeyedService* BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_SCREENSHOT_SERVICE_H_
