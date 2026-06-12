# Astra Browser — Native UI + Chromium CEF

A desktop browser built with **native macOS AppKit UI** + **Chromium Embedded Framework (CEF)**
for web content rendering. Inspired by Arc Browser's vertical spatial organization.

## Architecture

```
┌─────────────────────────────────────────┐
│  Native UI (AppKit / Objective-C++)      │  Sidebar, Topbar, Address Bar, Tabs
│  native/macos/                           │
├─────────────────────────────────────────┤
│  Browser Core (C++ / CEF)                │  Tab management, Navigation, State
│  native/src/app/                         │
├─────────────────────────────────────────┤
│  Chromium CEF (vendor/)                  │  Web content rendering, V8, etc.
└─────────────────────────────────────────┘
```

- **Native UI**: 100% AppKit for browser chrome (sidebar, address bar, tabs) — native performance
- **Web Content**: CEF (Chromium) for rendering web pages — full Chrome compatibility
- **State Model**: Native C++ data model as single source of truth
- **Communication**: Direct C++ ↔ Objective-C++ bridging (no IPC overhead within browser process)

## Directory Structure

```
native/
├── src/
│   ├── app/          # Core browser logic (CEF App, Client, Tab management)
│   └── common/       # Shared constants
├── macos/
│   ├── AppDelegate*   # NSApplication delegate
│   ├── MainWindowController*  # Main window with sidebar + browser area
│   ├── SidebarViewController*  # Tab list, workspace selector
│   ├── BrowserContainerView*   # CEF browser host view
│   ├── AddressBar*    # URL / search field
│   ├── helper/        # CEF helper subprocess
│   └── Resources/     # Info.plist, etc.
└── README.md
```

## Building

### Prerequisites

- macOS 14+ (Sonoma)
- Xcode 15+
- CMake 3.25+
- [CEF](https://cef-builds.spotifycdn.com/) 144+ (macOS arm64)

### Quick Start

```bash
# 1. Download CEF
./scripts/setup-cef.sh

# 2. Build
./scripts/build-native.sh Release

# 3. Run
open build/native/Release/Astra.app
```

### Manual Build

```bash
cmake -S . -B build/native -G Xcode -DCEF_ROOT=$PWD/third_party/cef
cmake --build build/native --config Release
```

## Features (Milestone 0)

- [x] Native macOS AppKit UI
- [x] Multi-tab browser with CEF
- [x] Sidebar tab list (native NSTableView)
- [x] Address bar (URL + search)
- [x] New tab / close tab / switch tab
- [x] Tab title / URL updates from CEF
- [x] Visual effect materials (sidebar, top bar)
- [ ] Tab favicons
- [ ] Loading indicators
- [ ] Back / forward navigation
- [ ] Workspace switching
- [ ] Tab groups
- [ ] Split view
- [ ] Settings

## Design Decisions

### Why Native UI?

1. **Performance**: Native AppKit controls are faster and more responsive than HTML/CSS chrome
2. **System integration**: Better integration with macOS features (HIG, services, accessibility)
3. **Memory**: No extra V8 instance + DOM for browser UI
4. **Arc-style feel**: Arc Browser uses native UI — following the same path

### Why CEF?

1. **Full Chromium**: Real Blink + V8, not a webview
2. **Extensions possible**: Can integrate Chrome extension system
3. **DevTools**: Full Chrome DevTools via remote debugging
4. **Mature**: Stable API, used by many production apps

### State Model

Browser state (tabs, workspaces, navigation state) lives in native C++ (`AstraApp`).
UI subscribes to state changes and updates reactively. This keeps the data model
clean and testable, independent of UI framework.

## Development

- UI code goes in `native/macos/` (Objective-C++)
- Core browser logic goes in `native/src/` (C++ + CEF)
- All new code must be ARC-compatible
