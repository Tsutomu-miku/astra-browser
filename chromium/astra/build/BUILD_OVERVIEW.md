# Astra Build System Overview

This document describes how Astra's build system works, how buildflags flow
from GN to C++, and how branding and feature configuration is structured.

## High Level

Astra is built as a layer on top of Chromium's build system.  Astra code lives
in `//astra/` and hooks into Chromium via build targets and patch points.

```
Chromium build system (GN / Ninja)
  └── //astra/ targets (imported via BUILD.gn)
        ├── //astra/build/  — build flags and build config
        ├── //astra/app/    — startup hooks and branding
        ├── //astra/browser/ — browser-layer services
        └── //astra/ui/views/ — Views UI components
```

## Build Flags

Build flags are compile-time constants that control which code is compiled
into the binary.  They flow from GN build configuration into C++ preprocessor
macros.

### Flow: GN → C++

```
GN args (buildflags.gni)
       │
       ▼
BUILD.gn defines section
       │
       ▼
Compiler -D flags
       │
       ▼
BUILDFLAG() macros (astra_buildflags.h)
       │
       ▼
#if BUILDFLAG(ENABLE_ASTRA_*) in C++ code
```

1. **GN args** in `buildflags.gni` declare feature flags with default values.
   These can be overridden by the user via `gn gen --args="..."`.

2. **BUILD.gn** reads the GN args and generates compiler defines in the
   format `BUILDFLAG_INTERNAL_ENABLE_ASTRA_FOO()=(1)` or `=(0)`.

3. **astra_buildflags.h** uses Chromium's `build/buildflag.h` macro system
   to expose these via `BUILDFLAG(ENABLE_ASTRA_FOO)`.

4. **C++ code** checks flags with `#if BUILDFLAG(ENABLE_ASTRA_FOO)`.

### Flag List

| Flag | Default | Purpose |
|------|---------|---------|
| `is_astra_branded` | `false` | Enable Astra product branding (replaces "Chromium" strings) |
| `enable_astra_sidebar` | `true` | Vertical sidebar navigation surface |
| `enable_astra_split_view` | `true` | Side-by-side tab view (Glance mode) |
| `enable_astra_workspaces` | `true` | Workspace-based tab organization |
| `enable_astra_command_palette` | `true` | Command search palette (Cmd/Ctrl+K) |
| `enable_astra_focus_mode` | `true` | Distraction-free focus mode |
| `enable_astra_devtools_panel` | `true` | Astra DevTools inspection tab |
| `enable_astra_webui` | `false` | WebUI-based Astra surfaces (vs native Views) |

### How to Enable/Disable Features

From the command line:

```bash
# Disable sidebar for a minimal build
gn gen out/Minimal --args="enable_astra_sidebar=false"

# Enable WebUI surfaces for testing
gn gen out/WebUI --args="enable_astra_webui=true is_astra_branded=true"
```

Or in a GN build config file:

```gn
import("//astra/build/buildflags.gni")

enable_astra_sidebar = false
enable_astra_webui = true
```

### Build flags vs Feature flags

**Build flags** (this system, `BUILDFLAG()`) are compile-time.  Code that is
disabled by a build flag is not present in the binary at all.  Use build
flags for:
- Code that must be absent in certain build variants
- Size-optimized builds
- Platform-specific features

**Feature flags** (`base::FeatureList`, `astra_feature_list.h`) are runtime.
Code is always compiled in, but can be toggled on/off at runtime via command
line, field trials, or `about:flags`.  Use feature flags for:
- Features that should ship but be disabled by default
- A/B testing via field trials
- Developer toggles

## Feature List Configuration

`astra_feature_list.gni` declares all Astra feature flags as a structured
list.  This single source of truth is used by:

- **GN build** — for compile-time feature gating and validation
- **C++ feature system** — for runtime feature registration (generated)

Each feature entry has:
- `name` — C++ identifier and feature name string
- `display_name` — Human-readable name for `about:flags`
- `description` — Description text for `about:flags`
- `default_state` — `"enabled"` or `"disabled"`
- `build_flag` — `BUILDFLAG()` guard for compile-time gating

TODO(astra): Generate C++ feature definitions from this GNI file automatically.
Currently `astra_feature_list.cc` is manually maintained and should be kept in
sync with this file.

## Branding

Branding configuration defines Astra's product identity — names, identifiers,
version strings, and URLs.

### Brand Constants

Branding constants live in `astra/app/astra_brand.h` as `inline constexpr`
values:
- Product names (short, long, display)
- Company name
- App / bundle IDs
- Windows ProgIds and CLSIDs
- URLs (homepage, feedback, support, version)
- Update channels (stable, beta, dev, canary)

### Version

Astra has its own product version, independent of the Chromium engine
version.  Version information lives in `astra/app/astra_version.h`:

- `ASTRA_VERSION_MAJOR`, `ASTRA_VERSION_MINOR`, `ASTRA_VERSION_PATCH`,
  `ASTRA_VERSION_BUILD` — macro-style defines
- `ASTRA_VERSION` — full version string macro
- `kAstraVersionString` — C++ constexpr version string
- `kAstraMajorVersion`, `kAstraMinorVersion`, `kAstraPatchVersion` —
  C++ constexpr components

Version can be overridden at build time via compiler defines:

```bash
gn gen out/Release --args='cflags += ["-DASTRA_VERSION_MAJOR=1", "-DASTRA_VERSION_MINOR=0", "-DASTRA_VERSION_PATCH=0"]'
```

TODO(astra): Add proper GN args for version override and auto-generate
from git tags / CI build numbers.

### Brand Helper Functions

`astra/app/astra_brand.cc` provides helper functions:
- `GetAstraProductName()` — product name as `std::string`
- `GetAstraVersionString()` — version string as `std::string`
- `GetAstraFullVersionString()` — combined "Astra X.Y.Z (Chromium A.B.C.D)"
- `GetAstraUserAgentProduct()` — User-Agent token "Astra/X.Y.Z"
- `GetAstraUpdateChannel()` — current update channel string

## Relationship to Chromium's Build System

Astra reuses Chromium's build infrastructure rather than replacing it:

| Chromium component | Astra usage |
|-------------------|-------------|
| `//build/buildflag.h` | `BUILDFLAG()` macro system |
| `//build/buildflag_header.gni` | Template pattern (TODO(astra): use directly) |
| `//build/config/chrome_build.gni` | Model for Astra buildflags.gni |
| `//base/feature_list.h` | Runtime feature flag framework |
| `chrome/VERSION` | Engine version (Astra has its own product version) |
| `chrome/install_static/brand.h` | Branding pattern for channels |

### Patch Points

Astra modifies Chromium at these build-related patch points:

- `//build/config/chrome_build.gni` — Add Astra GN args
- `//chrome/common/chrome_constants.cc` — Brand string overrides
- `//chrome/browser/about_flags.cc` — about:flags branding
- `//content/public/common/user_agent.h` — User-Agent product token

## Build Targets

### `//astra/build:buildflags`

Source set that provides `astra_buildflags.h` and propagates build flag
defines to all dependents.  Every Astra target that uses `BUILDFLAG()`
must depend on this target.

```gn
deps += [ "//astra/build:buildflags" ]
```

### `//astra/app:app`

App layer target including branding, version info, and startup hooks.
Depends on `//astra/build:buildflags` for build flag access.

## Checks and Validation

Run these commands to verify the build system is consistent:

```bash
# Architecture check
pnpm check:architecture

# Check for whitespace and diff issues
git diff --check

# Check GN files (requires Chromium checkout)
gn check out/Default
```

## TODOs

- [ ] Generate `astra_buildflags.h` from GN using `buildflag_header.gni`
- [ ] Generate `astra_feature_list.cc` from `astra_feature_list.gni`
- [ ] Add version auto-generation from git tags / build numbers
- [ ] Wire update channel into brand configuration
- [ ] Add build flag validation test (all flags have matching GN arg + C++ macro)
- [ ] Create VERSION file in `astra/` (like `chrome/VERSION`)
