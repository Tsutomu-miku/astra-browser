# Architecture

## Runtime Model

Astra is a direct Chromium product layer. The browser process, renderer
processes, profiles, WebContents lifecycle, downloads, permissions, history,
passwords, extensions, DevTools, WebUI, policy, and update infrastructure remain
Chromium-owned.

Astra code should be small, explicit, and product-specific:

```text
Chromium chrome/browser
  Browser, Profile, TabStripModel, BrowserView, command controller

Chromium content/components
  WebContents, NavigationController, extensions, passwords, history, downloads

//astra overlay
  Product metadata, Spaces, sidebar projection, Split/Glance metadata,
  Astra-only commands, Views integration hooks
```

## Ownership Boundaries

### Chromium-Owned

Do not reimplement these in Astra:

- Profile and request-context management.
- Tab ownership and WebContents lifecycle.
- Navigation, session history, loading state, favicon, zoom, mute, and crash
  state.
- Downloads, permissions, passwords, autofill, Safe Browsing, extensions,
  DevTools, WebUI, updater, and policy.

Use Chromium APIs or patch points. If an API is not exposed cleanly, write an ADR
before adding product code around it.

### Astra-Owned

Astra may own only the product semantics that Chromium does not provide:

- Spaces and Arc-style workspace projection.
- Sidebar folder classification: Favorites, Pinned presentation, groups, and
  split/glance affordances.
- Astra metadata attached to Chromium-owned `content::WebContents` through
  `WebContentsUserData`.
- Astra UI additions inside Chromium Views.
- Astra-only commands that delegate standard browser work back to Chrome command
  infrastructure.

## Active Layers

```text
chromium/astra/app
  Chromium startup patch helpers and BrowserMainExtraParts registration.

chromium/astra/browser
  Astra product services and WebContents metadata. These classes must depend on
  Chromium browser primitives, not replace them.

chromium/astra/ui/views
  Views-based BrowserView/sidebar/split/glance shell. No business state truth
  source may live here.

chromium/astra/patches
  Human-readable patch queue notes. Keep Chromium patches tiny and delegating.
```

## Dependency Direction

Allowed:

- `astra/app` may register `astra/browser` services.
- `astra/browser` may depend on Chromium browser/content primitives.
- `astra/ui/views` may read Chromium models and Astra metadata to render.
- Chromium patch points may call into `//astra`.

Forbidden:

- `astra/browser` depending on `astra/ui/views`.
- `astra/ui/views` owning product collections.
- New Electron, CEF, AppKit, or CMake browser runtime code.
- Parallel services named like Chromium-owned infrastructure, such as
  `DownloadManager`, `PermissionManager`, `HistoryService`, `ExtensionService`,
  `PasswordManager`, or `ProfileManager`.

## Legacy Code

`src/` is the Electron prototype. It is useful as behavior reference and test
fixture source, but it is not the new architecture. New direct Chromium work
must land in `chromium/astra/` or in a documented Chromium patch.
