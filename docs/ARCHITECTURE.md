# Architecture

Astra is a direct Chromium product layer. The browser process, renderer processes,
profiles, WebContents lifecycle, downloads, permissions, history, passwords,
extensions, DevTools, WebUI, updater, and policy infrastructure remain
Chromium-owned.

Astra code is a productivity layer on top: product metadata, Spaces, sidebar
projection, Split/Glance metadata, Astra-only commands, and Views integration hooks.

---

## Component Diagram

```
  Chrome Browser Layer (chrome/browser)
  ================================================================
  |   Browser  <-->  BrowserView  <-->  BrowserCommandController    |
  |      \           |                      |                    |
  |       v          v                      v                      |
  |   TabStripModel  <-----------  chrome commands (IDC_*)       |
  |        |                                                |
  |        | observes / owns                                  |
  |        v                                                 |
  |   (many) WebContents  --> NavigationController          |
  |                                                          |
  ================^==========================================
                   | reads / owns
                   v
  Content Layer (content/public/browser)
  ================================================================
  |   WebContents   NavigationController   RenderProcessHost     |
  ================^==========================================
                   | composed of
  Components Layer (components/*)
  ================================================================
  |   extensions   passwords   history   downloads             |
  |   permissions   autofill   safe_browsing   policy           |
  |   devtools   webui   updater                               |
  ================^==========================================
                   | depends on
  Astra Overlay Layer (//astra)
  ================================================================
  |  Profile-scoped services (astra/browser)                            |
  |   AstraWorkspaceService  (ProfileKeyedService)                  |
  |   AstraTabFeatures  (WebContentsUserData)                       |
  |   AstraCommandDelegate                                          |
  |                                                                  |
  |  UI Views (astra/ui/views)                                     |
  |   AstraBrowserView  (controller, augments BrowserView)        |
  |   AstraSidebarView  (reads TabStripModel + AstraTabFeatures)        |
  |   Split View   Glance/Peek   Command Palette                       |
  |                                                                  |
  |  UI Color (astra/ui/color)                                     |
  |   AstraColorMixer  — extends Chromium ColorProvider          |
  |                                                                  |
  |  Common Layer (astra/common)                                   |
  |   Shared types, enums, constants                               |
  |                                                                  |
  |  Startup hooks (astra/app)                                      |
  |   AstraBrowserMainExtraParts / ContentBrowserClient / MainDelegate |
  ================^================================================
                   | patches call into
  Chromium Patch Points (tiny changes in chrome/*)
  ================================================================
  |   chrome_browser_main.cc   ->  register Astra parts        |
  |   browser_view.cc          ->  install AstraBrowserView      |
  |   browser_command_controller.cc -> forward Astra commands   |
  |   BUILD.gn                 ->  include //astra                   |
  ================================================================
```

Dependency direction arrows point upward: lower layers are depended on by upper layers.
Patch points are tiny delegation hooks.

---

## Key Principles

- **Chromium owns state.** `Browser`, `TabStripModel`, `WebContents`,
  `Profile`, extensions, passwords, history, etc. are all Chromium-owned.
- **Astra projects state.** Astra reads Chromium models and adds metadata.
  It does not own browser state.
- **UI dispatches, never owns.** Views code reads models and dispatches commands.
  It is never the source of truth.
- **Patches delegate.** Chromium patch points call into `//astra` and do
  nothing else.

---

## Ownership Boundaries

### Chromium-Owned

Do not reimplement these in Astra:

- Profile and request-context management.
- Tab ownership and WebContents lifecycle.
- Navigation, session history, loading state, favicon, zoom, mute, and crash state.
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

---

## Active Layers

```text
chromium/astra/app
  Startup hooks and Chromium patch helpers.
  (BrowserMainExtraParts, ContentBrowserClient, MainDelegate.

chromium/astra/browser
  Product services and WebContents metadata.
  ProfileKeyedService for profile-scoped state.
  WebContentsUserData for per-tab metadata.

chromium/astra/common
  Shared types, enums, constants. Bottom of the dependency graph.
  Depends only on //base, //skia, //ui/gfx.

chromium/astra/ui/views
  Views UI additions. Reads models, dispatches commands.
  Never the source of truth for product state.

chromium/astra/ui/color
  Astra ColorProvider mixer and color IDs.
  Extends Chromium's color system.

chromium/astra/patches
  Human-readable patch queue notes.
  Keep Chromium patches tiny and delegating.
```

---

## Dependency Direction

Full dependency graph (arrows point from dependent to dependency: X → Y means X depends on Y):

```
                     Chromium subsystems
  (//base, //skia, //ui/gfx, //ui/color, etc.)
            ▲                ▲
            │                │
       astra/common   astra/ui/color
            ▲    ▲          ▲
            │    │          │
       astra/browser │          │
            ▲    │          │
            │    └─────── astra/ui/views
            │                 ▲
       astra/app           astra/ui/views/devtools

  Chromium patch points ── call into any //astra layer
```

Key dependency rules:

- `astra/common` is the bottom layer. It depends only on fundamental
  Chromium types and nothing else in Astra.
- `astra/browser` depends on `astra/common` and Chromium browser/content.
  It does NOT depend on `astra/ui/views` or any UI layer.
- `astra/ui/views` depends on `astra/browser` and `astra/common`.
  It reads models to render, but never owns state.
- `astra/ui/color` depends on `astra/common` and Chromium's color
  `ui/color`. It does NOT depend on `astra/browser`.
- `astra/app` depends on `astra/browser` (for service registration) and
  `astra/common`.
- Chromium patch points may call into any `//astra` layer.

Forbidden:

- `astra/browser` depending on `astra/ui/views`.
- `astra/ui/views` owning product collections.
- `astra/common` depending on any other Astra layer.
- New Electron, CEF, AppKit, or CMake browser runtime code.
- Parallel services named like Chromium-owned infrastructure, such as
  `DownloadManager`, `PermissionManager`, `HistoryService`, `ExtensionService`,
  `PasswordManager`, or `ProfileManager`.

---

## Data Flow: Sidebar Projection

```
TabStripModel (Chrome)
    |
    |  TabStripModelObserver
    v
AstraSidebarView (Astra UI)
    |
    |  reads workspace_id from
    v
AstraTabFeatures (WebContentsUserData)
    |
    |  reads active workspace from
    v
AstraWorkspaceService (ProfileKeyedService)
```

The sidebar projects tabs by combining Chromium tab data plus Astra metadata. It never
modifies tab state directly; it dispatches commands that flow back through
Chrome's command infrastructure.

---

## Data Flow: Command Dispatch

```
User action (menu, keybinding, palette)
    |
    v
BrowserCommandController (Chrome)
    |
    +-- standard IDC_*  -->  Chrome handles normally
    |
    +-- Astra ID (60000+)  -->  patch forward
                              |
                              v
                        AstraCommandDelegate
                              |
                              +-- mutates Astra services
                              +-- calls Chrome commands for standard work
```

---

## Legacy Code

`src/` is the Electron prototype. It is useful as behavior reference and test
fixture source, but it is not the new architecture. New direct Chromium work
must land in `chromium/astra/` or in a documented Chromium patch.
