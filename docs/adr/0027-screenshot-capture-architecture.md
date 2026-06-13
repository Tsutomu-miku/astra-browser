# ADR-0027: Screenshot Capture Architecture

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

Astra includes a screenshot feature that lets users capture web page content. Screenshots can be of the visible area, a selected region, or the full page. Captured screenshots appear in a sidebar section (or a dedicated view) and can be saved, shared, or annotated.

Key architectural questions:
- How is screenshot capture implemented — using Chromium's existing capture APIs or a custom implementation?
- Where does screenshot metadata live (service layer, file system, Chromium downloads)?
- How does the screenshot UI relate to the content area (overlay, bubble, sidebar)?
- Does Astra implement its own screenshot storage, or reuse Chromium's downloads system?

## Decision

Screenshot capture builds on **Chromium's existing tab capture / screenshot APIs** with an Astra service layer (`AstraScreenshotService`) for metadata management and a Views-based capture UI. Screenshots are stored as files via Chromium's download system, with Astra maintaining metadata (source URL, timestamp, capture region).

**Chromium subsystems reused:**
- `content::WebContents::CaptureVisiblePage()` — visible area screenshot capture.
- `content::WebContents::GetCaptureHandle()` or full-page capture via render widget snapshot.
- `DownloadManager` / `download::DownloadItem` — saving screenshots to disk.
- `TabStripModel` / `WebContents` — identifying the active tab for capture.
- `SkBitmap` / `gfx::Image` — image processing and encoding.

**Astra service layer (AstraScreenshotService):**
- Profile-scoped service that manages screenshot capture lifecycle.
- Provides APIs: `CaptureVisibleArea()`, `CaptureRegion(rect)`, `CaptureFullPage()`.
- Triggers capture via Chromium's WebContents capture APIs.
- Stores screenshot metadata (source URL, timestamp, capture type, file path).
- Optionally maintains a recent screenshots list for the sidebar.
- Observer pattern for UI to react to new screenshots.
- Does not implement image capture itself — delegates to Chromium.

**Capture UI (Views layer):**
- Region selection: `AstraScreenshotRegionOverlay` — a full-window overlay that lets the user drag to select a region.
- Capture bubble: `AstraScreenshotCaptureBubble` — shows preview and actions after capture.
- The overlay is a transparent Views layer over the content area.
- UI never owns screenshot data — it reads from the service.

**Storage:**
- Screenshot image files are saved to disk via Chromium's download system (or a dedicated screenshots directory in the user's downloads folder).
- Screenshot metadata (URL, timestamp, capture info) is stored in `PrefService` (recent screenshots list) or via a lightweight storage mechanism.
- The actual image data is not stored in prefs — only file paths and metadata.
- This is consistent with how Chromium handles downloads and media capture.

**Sidebar projection:**
- A sidebar "Screenshots" section shows recent screenshots as thumbnail items.
- Clicking a screenshot opens it in a preview or the system file manager.
- The sidebar reads from `AstraScreenshotService` — it never stores screenshot data.

## Consequences

Positive:
- Reuses Chromium's mature capture infrastructure — no need to implement screenshot capture from scratch.
- Storage via download system means screenshots participate in Chrome's download management, notifications, and file handling.
- Service/UI separation is clean: service owns state and capture logic, UI handles presentation.
- Region selection overlay is a Views-level concern — works across platforms.
- Observer pattern allows multiple UI surfaces (sidebar, bubble, overlay) to stay in sync.

Negative:
- Full-page screenshot capture may require a custom implementation if Chromium's API only captures the visible area. Some platforms have limitations with offscreen content capture.
- Screenshot metadata in `PrefService` is limited to a recent list (scaling to many screenshots would need a different storage approach).
- Region selection overlay must coordinate with the content area's view hierarchy, which can be tricky with `BrowserView`'s existing layout.
- Image encoding and file I/O happen on the UI thread unless explicitly posted to background — need to be careful about performance.

Neutral:
- Screenshots are profile-scoped (metadata), but the actual files are in the user's downloads directory (system-level).
- The feature depends on Chromium's capture APIs, which may have platform-specific differences.

## Alternatives Considered

### Custom screenshot implementation (native OS APIs)
Implement screenshot capture using platform-native APIs (CoreGraphics on macOS, GDI on Windows, etc.).

- Rejected: Platform-specific code adds complexity and maintenance burden. Chromium's capture APIs already handle cross-platform capture, GPU acceleration, and various content types (WebGL, video, etc.). Reusing Chromium is simpler and more reliable.

### Extension-based screenshot
Implement screenshot capture as a Chrome extension using the `chrome.tabs.captureVisibleTab` API.

- Rejected: Extension IPC overhead, limited capabilities (only visible tab, no full-page or region selection), and integration complexity with the sidebar UI. A native service-layer implementation is more efficient and better integrated.

### Screenshots as downloads (no service)
Treat screenshots entirely as downloads, with no separate Astra screenshot service or metadata.

- Considered: The download system handles file storage and notifications. However, a screenshot service provides value: metadata tracking (source URL, capture time, region info), recent screenshots list for the sidebar, and integration with Astra-specific features (workspace association, notes). The service layer is thin but useful.

### WebUI screenshot viewer
Build the screenshot viewer and management UI as a WebUI page.

- Considered: A WebUI page would be easier to build with HTML/CSS and could support richer annotation tools. However, the capture overlay and bubble are Views-based for performance and integration with the browser chrome. The sidebar screenshot list is also Views-based for consistency with other sidebar sections. A hybrid approach is possible (WebUI viewer + Views capture UI) but adds complexity.

## References

- **Chromium subsystems reused:** `content::WebContents::CaptureVisiblePage()`, `DownloadManager`, `TabStripModel`, `SkBitmap`, `gfx::Image`, `Views::Widget`
- **Astra components:** `AstraScreenshotService`, `AstraScreenshotRegionOverlay`, `AstraScreenshotCaptureBubble`, `AstraSidebarView` (screenshots section)
- **Patch points:** None required for basic implementation (uses public Chromium APIs). Full-page capture may need a content-layer patch.
- **Related ADRs:** ADR-0011 (Sidebar Projection Model), ADR-0024 (Notes Feature Architecture)
