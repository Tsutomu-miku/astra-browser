# ADR-0019: Focus Mode (Distraction-Free)

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Focus mode is a productivity feature that helps users avoid distractions while browsing. It combines UI simplification (hiding sidebar, reducing toolbar clutter) with optional site blocking (preventing access to distracting websites like social media, news, etc.) for a configurable duration.

The architectural question is how focus mode fits into Chromium's existing systems. Options include:
- Building everything as Astra-only UI + logic.
- Reusing Chromium's content settings for site blocking.
- Leveraging existing Chromium features like Reader Mode or the " Simplify view" experiment.

Key components:
- Focus session state (active, duration, remaining time).
- Distraction site blocklist.
- UI changes (sidebar visibility, toolbar density).
- Site blocking enforcement.

## Decision

Focus mode is split into two layers: **state/service** (`AstraFocusModeService`, a `ProfileKeyedService`) and **UI/controller** (`AstraFocusModeController`, Views layer). Site blocking builds on Chromium's `HostContentSettingsMap` and navigation throttling.

**Service layer (AstraFocusModeService):**
- Profile-scoped keyed service that owns focus mode state.
- Manages focus session lifecycle: `EnterFocusMode(duration)`, `ExitFocusMode()`, `ToggleFocusMode()`.
- Tracks remaining time via a `base::RepeatingTimer` (tick per second).
- Owns the distraction blocklist (URL patterns).
- Persists default duration and blocklist via `PrefService`.
- Active session state is in-memory only (does not survive browser restart).
- Observer pattern for UI to react to state changes.

**UI/Controller layer (AstraFocusModeController):**
- Per-browser-window Views controller.
- Observes `AstraFocusModeService` for state changes.
- Manages UI presentation: hides sidebar, reduces toolbar clutter, shows focus indicator.
- Does NOT own state -- purely a projection of service state onto the Views hierarchy.
- Shows a focus mode indicator (timer display) in the toolbar or window frame.

**Site blocking enforcement:**
- Distraction sites are blocked using Chromium's content settings framework.
- Blocklist patterns follow `ContentSettingsPattern` format.
- Enforcement is done via a `NavigationThrottle` or content settings rule that blocks navigation to matched sites during active focus sessions.
- When focus mode ends, blocking rules are removed -- normal browsing resumes.
- Builds on `HostContentSettingsMap` for pattern matching and storage.

**Incognito behavior:**
- Incognito profiles get their own `AstraFocusModeService` instance (`kOwnInstance`).
- An incognito focus session does not affect the main profile's session.
- Default duration and blocklist prefs are shared (via pref forwarding).

## Consequences

Positive:

- Clean separation: service owns state, controller owns UI. Standard Chromium pattern.
- Reuses `HostContentSettingsMap` / `ContentSettingsPattern` for site blocking instead of building a custom URL matcher.
- `ProfileKeyedService` pattern ensures correct lifecycle with profile creation/destruction.
- Persistence via `PrefService` means preferences participate in profile lifecycle and can be controlled by policy.
- Observer pattern allows multiple UI surfaces (sidebar, toolbar, indicator) to stay in sync.

Negative:

- Active session state is in-memory and does not survive browser restart. A crash during focus mode loses the session. This is acceptable because focus mode is a short-lived productivity session (25-90 minutes).
- Site blocking via navigation throttle requires a content/ layer patch or integration point. A simpler initial implementation could use `chrome.tabs` API or a `WebContentsObserver` approach.
- Blocking only applies to top-level navigations by default. Subresource blocking would require additional plumbing.
- Focus mode UI changes (hiding sidebar, etc.) must be coordinated with the normal sidebar toggle state to avoid confusion.

Neutral:

- Pomodoro-style timer (default 25 minutes) with extendable duration.
- Blocklist is user-managed; no default blocklist is provided.

## Alternatives Considered

### Chrome extensions for site blocking
Use the `chrome.declarativeNetRequest` API or `chrome.contentSettings` to implement site blocking via a built-in extension.

- Rejected: Adds extension IPC overhead and requires bundling a built-in extension. Using content settings directly from browser code is more efficient and integrated.

### Reader Mode / Distill page approach
Build focus mode on top of Chromium's Reader Mode feature.

- Rejected: Reader Mode changes the rendering of individual pages (simplified article view), while Astra's focus mode is a browser-level feature that hides UI distractions and optionally blocks entire sites. Different scope and purpose.

### OS-level focus mode integration
Integrate with macOS Focus / Windows Focus Assist for system-level distraction blocking.

- Considered as a future enhancement, but the core browser-level focus mode should work independently. OS integration can be added as an optional layer.

## References

- **Chromium subsystems reused:** `ProfileKeyedServiceFactory`, `PrefService`, `HostContentSettingsMap`, `ContentSettingsPattern`, `NavigationThrottle` (planned), `base::RepeatingTimer`
- **Astra components:** `AstraFocusModeService`, `AstraFocusModeController`, `AstraFocusModeIndicator`
- **Patch points:** Profile keyed service registration, navigation throttle (content/public/browser/navigation_throttle.h) or content settings integration
