# Building Astra Chromium

Astra is a direct Chromium overlay project. The Astra code lives in
`chromium/astra/` and overlays onto a full Chromium source tree. Patches in
`chromium/astra/patches/` modify Chromium source files to integrate Astra.

This document covers everything you need to build Astra from source on
macOS, Linux, and Windows.

---

## Contents

- [CI/CD Status](#cicd-status)
- [Prerequisites](#prerequisites)
  - [All Platforms](#all-platforms)
  - [macOS](#macos)
  - [Linux](#linux)
  - [Windows](#windows)
- [Step-by-Step Build](#step-by-step-build)
  - [1. Clone the Astra repo](#1-clone-the-astra-repo)
  - [2. Bootstrap Chromium](#2-bootstrap-chromium)
  - [3. Sync the Astra overlay](#3-sync-the-astra-overlay)
  - [4. Apply patches](#4-apply-patches)
  - [5. Build](#5-build)
  - [6. Run the browser](#6-run-the-browser)
- [Build Options](#build-options)
- [Self-Hosted CI Runner Setup](#self-hosted-ci-runner-setup)
- [Troubleshooting](#troubleshooting)
  - [macOS](#macos-1)
  - [Linux](#linux-1)
  - [Windows](#windows-1)
- [Estimated Build Times](#estimated-build-times)
- [Windows-Specific Considerations](#windows-specific-considerations)

---

## CI/CD Status

| Workflow | Status | Description |
|----------|--------|-------------|
| Architecture Validation | [![build status](https://img.shields.io/badge/build-passing-brightgreen)] | Validates source code, headers, BUILD.gn, and patches on every PR |
| Self-Hosted Build | N/A (self-hosted) | Full Chromium build on self-hosted runner with `astra-chromium-builder` label |

The CI pipeline has three jobs:

1. **Architecture & Source Validation** (ubuntu-latest) — Fast checks that run on every push and PR to `chromium-native`. Verifies header guards, BUILD.gn completeness, patch format, and runs the architecture guard.
2. **Build Documentation** (ubuntu-latest) — Generates status badges and validates build documentation.
3. **Self-Hosted Build** (self-hosted, `astra-chromium-builder` label) — Full Chromium build with Astra overlay. Gracefully skips if no runner is available.

---

## Prerequisites

### All Platforms

- **Git** — for cloning and patch operations
- **Python 3.8+** — required by Chromium's build tools (included in depot_tools)
- **~60 GB free disk space** — Chromium source tree + build artifacts
- **~16 GB RAM** — minimum; 32 GB recommended for fast builds
- **Astra repo** — clone with `git clone https://github.com/Tsutomu-miku/astra-browser.git`

### macOS

- **macOS 13+** (Ventura or newer)
- **Xcode 15+** — with Command Line Tools
  ```bash
  xcode-select --install
  ```
- **The 10.15+ SDK** — included with Xcode
- **Homebrew** (optional, for fswatch and other tools)
  ```bash
  brew install fswatch ccache
  ```
- **depot_tools** — installed automatically by `scripts/chromium-bootstrap.sh`

### Linux

Supported distributions: Ubuntu 20.04+, Debian 11+, Fedora 37+

- **Build essentials**
  ```bash
  sudo apt-get install build-essential
  ```
- **Python 3** and dev packages
  ```bash
  sudo apt-get install python3 python3-dev
  ```
- **inotify-tools** (optional, for `--watch` mode)
  ```bash
  sudo apt-get install inotify-tools
  ```
- **ccache** (optional, for faster rebuilds)
  ```bash
  sudo apt-get install ccache
  ```
- **depot_tools** — installed automatically by `scripts/chromium-bootstrap.sh`
- **Additional dependencies** — Chromium's `build/install-build-deps.sh` script can install everything:
  ```bash
  cd chromium/src
  ./build/install-build-deps.sh
  ```

### Windows

Building Chromium on Windows requires native tooling. The bash scripts work
best-effort in Git Bash, but for a full build you should use `cmd.exe` or
PowerShell with depot_tools.

- **Windows 10/11 64-bit**
- **Visual Studio 2022** (version 17.0+) — with "Desktop development with C++" workload
- **Windows 11 SDK** (10.0.22621.0 or later)
- **Git for Windows** (Git Bash) — for running Astra scripts
  ```bash
  # Install from: https://git-scm.com/download/win
  ```
- **depot_tools** — for Windows, download from Google and add to PATH
  - Download: https://storage.googleapis.com/chrome-infra/depot_tools.zip
  - Extract and add to system PATH
  - Run `gclient` once from cmd.exe to install components
- **At least 100 GB free space** — Windows builds are larger

> **Note:** Windows support in Astra's bash scripts is best-effort. For
> production Windows builds, use depot_tools natively from cmd.exe and
> follow the official Chromium Windows build instructions.

---

## Step-by-Step Build

### 1. Clone the Astra repo

```bash
git clone https://github.com/Tsutomu-miku/astra-browser.git
cd astra-browser
git checkout chromium-native
```

### 2. Bootstrap Chromium

This step downloads depot_tools and fetches the full Chromium source tree
(~30 GB download, can take 30+ minutes depending on network speed).

```bash
./scripts/chromium-bootstrap.sh
```

Options:
- `--revision <tag>` — specify a Chromium version tag (default: `128.0.6613.119`)
- `--force` — force re-download even if checkout exists
- `--no-sync` — skip `gclient sync` after checkout
- `--help` — full usage

The script will:
1. Check for (or install) depot_tools in `third_party/depot_tools/`
2. Create `.gclient` config in `chromium/`
3. Fetch Chromium source into `chromium/src/`
4. Check out the specified revision tag
5. Run `gclient sync` to pull all dependencies

### 3. Sync the Astra overlay

The Astra source code lives in `chromium/astra/` of the Astra repo. It needs
to be copied into the Chromium source tree at `chromium/src/astra/`.

```bash
./scripts/sync-overlay.sh
```

Options:
- `--dry-run` — show what would be copied without modifying anything
- `--chromium-src <path>` — specify a different Chromium source path
- `--watch` — watch for changes and continuously sync (uses fswatch/inotifywait)
- `--check` — exit non-zero if overlay is out of sync

### 4. Apply patches

Astra uses small patches to hook into Chromium's entry points. Apply them
with:

```bash
./scripts/apply-patches.sh
```

Options:
- `--list` — list all patches with their current status
- `--check` — dry-run to verify patches apply cleanly
- `--reverse` — unapply all patches
- `--force` — apply even if already marked as applied
- `--patch-dir <path>` — use a different patch directory

Applied patches are tracked in `chromium/src/.astra_patches_applied`.

> **Tip:** Always apply patches after checking out a new Chromium revision.
> Patches are version-specific and may need updating for different Chromium
> versions.

### 5. Build

Build the Astra-branded Chromium browser:

```bash
./scripts/build-astra.sh           # Debug build (default)
./scripts/build-astra.sh --release # Release build
```

Options:
- `--release` — build release (default is debug)
- `--target <name>` — ninja target to build (default: `chrome`)
- `--jobs <N>` — number of parallel jobs (default: auto-detect)
- `--clean` — clean output directory before building
- `--chromium-src <path>` — Chromium source path
- `--out-dir <name>` — custom output directory name
- `--args <str>` — extra GN args
- `--no-patches` — skip patch verification

Output locations:
- Debug: `chromium/src/out/astra_Debug/`
- Release: `chromium/src/out/astra_Release/`

### 6. Run the browser

**macOS:**
```bash
open chromium/src/out/astra_Debug/Chromium.app
```

**Linux:**
```bash
./chromium/src/out/astra_Debug/chrome
```

**Windows (cmd.exe):**
```cmd
chromium\src\out\astra_Debug\chrome.exe
```

---

## Build Options

### Build Types

| Type | GN args | Size | Build Time | Use Case |
|------|---------|------|------------|----------|
| Debug | `is_debug=true`, `is_component_build=true` | ~15-20 GB | ~2-4 hrs | Development, debugging |
| Release | `is_debug=false`, `is_component_build=false` | ~2-4 GB | ~4-8 hrs | Testing, distribution |

### Useful GN Args

Add with `--args 'arg=value'`:

```
is_astra_branded=true       # Enable Astra branding (default on)
is_component_build=true     # Shared library build (faster links)
symbol_level=1              # Reduce debug symbols (faster build)
proprietary_codecs=true     # Enable proprietary codecs (requires patent licensing)
ffmpeg_branding="Chrome"    # Chrome FFmpeg branding (with proprietary codecs)
enable_nacl=false           # Disable Native Client (default)
cc_wrapper="ccache"         # Use ccache for faster rebuilds
```

### Common Targets

| Target | Description |
|--------|-------------|
| `chrome` | Full browser |
| `unit_tests` | All unit tests |
| `astra_unittests` | Astra-specific unit tests |
| `browser_tests` | Browser integration tests |
| `content_shell` | Minimal content shell (for testing) |

---

## Self-Hosted CI Runner Setup

The full Chromium build runs on self-hosted runners labeled `astra-chromium-builder`.
If no such runner is available, the build job gracefully skips.

### Requirements

- **Hardware**
  - 8+ CPU cores (16+ recommended)
  - 32 GB RAM (64 GB recommended)
  - 200+ GB SSD storage
  - Reliable internet (for fetching Chromium)

- **Software**
  - Linux (Ubuntu 22.04 recommended) or macOS 13+
  - GitHub Actions runner installed
  - depot_tools in PATH
  - All Chromium build dependencies installed

### Setup Steps (Linux)

1. **Create a dedicated user:**
   ```bash
   sudo useradd -m -s /bin/bash github-runner
   sudo usermod -aG sudo github-runner
   ```

2. **Install build dependencies:**
   ```bash
   # Clone Chromium first to get the deps script, or install manually.
   sudo apt-get install build-essential python3 git
   ```

3. **Install depot_tools:**
   ```bash
   git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git ~/depot_tools
   echo 'export PATH="$HOME/depot_tools:$PATH"' >> ~/.bashrc
   ```

4. **Install GitHub Actions runner:**
   - Go to your repo: Settings > Actions > Runners > New self-hosted runner
   - Follow the instructions to download and configure the runner
   - When prompted for labels, add `astra-chromium-builder`

5. **Install as a service:**
   ```bash
   sudo ./svc.sh install
   sudo ./svc.sh start
   ```

6. **Pre-fetch Chromium (optional but recommended):**
   ```bash
   mkdir -p ~/actions-runner/_work/astra-browser/astra-browser/chromium
   # The first build will fetch Chromium automatically, but pre-fetching
   # reduces first-build time.
   ```

### Cache Configuration

The workflow caches:
- `third_party/depot_tools` — depot_tools installation
- `chromium/src` — full Chromium checkout
- `chromium/src/out/astra_Debug` — build artifacts

> **Note:** Caching the full Chromium source (~30 GB) may exceed default
> cache limits. Consider using local storage or adjusting cache size limits
> in your GitHub organization settings.

---

## Troubleshooting

### macOS

**`xcode-select` error:**
```
xcode-select: error: tool 'xcodebuild' requires Xcode
```
Install Xcode from the App Store, then run:
```bash
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
sudo xcodebuild -license accept
```

**SDK not found:**
```
No matching SDK found
```
Make sure you have the latest Xcode and the macOS SDK installed. Astra
targets the macOS SDK version that corresponds to the Chromium version
you're building.

**Signing errors:**
If you get code signing errors, you can disable signing for development:
```bash
./scripts/build-astra.sh --args 'is_official_build=false'
```

### Linux

**`gclient` or `gn` not found:**
Make sure depot_tools is in your PATH:
```bash
export PATH="$PWD/third_party/depot_tools:$PATH"
```

**Missing build dependencies:**
Run Chromium's dependency installer:
```bash
cd chromium/src
./build/install-build-deps.sh
```

**Out of memory during link:**
Reduce parallel jobs or use component build (default in Debug):
```bash
./scripts/build-astra.sh --jobs 4
```

**Patch fails to apply:**
Patches are tied to specific Chromium revisions. Check that you're on the
expected revision:
```bash
./scripts/chromium-bootstrap.sh --revision 128.0.6613.119
```
Then re-apply patches with `--force`:
```bash
./scripts/apply-patches.sh --force
```

### Windows

**`python` not recognized:**
depot_tools includes its own Python. Make sure depot_tools is in your
system PATH, not just user PATH. Open cmd.exe as administrator and run:
```cmd
set PATH=C:\path\to\depot_tools;%PATH%
```

**Git Bash vs cmd.exe:**
Astra scripts are designed for bash and work in Git Bash, but Chromium's
build tools on Windows work best from cmd.exe. For full builds, use
cmd.exe for the `gn gen` and `ninja` steps.

**Visual Studio version mismatch:**
Chromium requires specific Visual Studio versions. For Chromium 128, use
Visual Studio 2022 (17.0+). Check `chromium/src/docs/windows_build_instructions.md`
for the exact requirements for your version.

**`gn gen` fails with "Visual Studio not found":**
Make sure you have the "Desktop development with C++" workload installed
in Visual Studio, including the Windows 11 SDK.

---

## Estimated Build Times

Build times vary widely based on hardware. These are rough estimates for
a full Debug build of the `chrome` target from scratch.

| Hardware | macOS (Apple Silicon) | Linux | Windows |
|----------|----------------------|-------|---------|
| M1 Max (10-core, 32GB) | ~1.5 hrs | — | — |
| M2 Ultra (24-core, 64GB) | ~45 min | — | — |
| Ryzen 9 7950X (16-core, 32GB) | — | ~2 hrs | ~2.5 hrs |
| Intel i7-12700K (12-core, 32GB) | — | ~3 hrs | ~3.5 hrs |
| 8-core VM / CI runner | — | ~4-6 hrs | — |

Incremental builds (changing a single source file) take:
- Debug, component build: **1-5 minutes**
- Release, non-component: **5-15 minutes**

Tips for faster builds:
- Use `ccache` for C++ compilation caching
- Build in Debug mode (component build = faster links)
- Use an SSD or NVMe drive
- Increase parallel jobs (`--jobs`) if you have enough RAM
- Build only the target you need (e.g., `astra_unittests` instead of `chrome`)

---

## Windows-Specific Considerations

Building Chromium on Windows has unique requirements. Here are important
details specific to Windows builds.

### Required Software

1. **Visual Studio 2022** (Community, Pro, or Enterprise)
   - Workload: "Desktop development with C++"
   - Must include: MSVC v143, Windows 11 SDK, C++ CMake tools

2. **Windows 11 SDK (10.0.22621.0+)**
   - Installed via Visual Studio Installer

3. **Debugging Tools for Windows**
   - Required for certain build steps
   - Install via Windows SDK

4. **depot_tools**
   - Download: https://storage.googleapis.com/chrome-infra/depot_tools.zip
   - Extract to a path without spaces (e.g., `C:\src\depot_tools`)
   - Add to system PATH (not user PATH)
   - Run `gclient` once from cmd.exe to bootstrap Python and other tools

### Build Steps (Windows Native)

While Astra provides bash scripts for convenience, official Chromium builds
on Windows use cmd.exe or PowerShell:

```cmd
:: 1. Set up environment
set PATH=C:\path\to\depot_tools;%PATH%
set DEPOT_TOOLS_WIN_TOOLCHAIN=0

:: 2. Fetch Chromium (run from chromium\ directory)
cd chromium
fetch --nohooks chromium

:: 3. Sync Astra overlay (from Astra repo root)
cd ..\..
bash scripts\sync-overlay.sh

:: 4. Apply patches
bash scripts\apply-patches.sh

:: 5. Generate build files
cd chromium\src
gn gen out\astra_Debug --args="is_debug=true is_component_build=true enable_nacl=false is_astra_branded=true"

:: 6. Build
autoninja -C out\astra_Debug chrome
```

### Known Issues

- **Path length limits:** Windows has a 260-character path limit by default.
  Enable long paths: run `gpedit.msc`, go to Computer Configuration >
  Administrative Templates > System > Filesystem > Enable Win32 long paths.

- **Antivirus interference:** Real-time antivirus scanning can slow builds
  significantly. Add the Chromium source directory to exclusions.

- **Git Bash limitations:** Some Chromium tools don't work properly in
  Git Bash due to path conversion issues. Use cmd.exe for `gn gen` and
  `autoninja`.

- **Case sensitivity:** Windows is case-insensitive by default. Chromium
  source expects case-sensitive file systems. Enable case sensitivity on
  the Chromium source directory:
  ```cmd
  fsutil.exe file setCaseSensitiveInfo C:\path\to\chromium\src enable
  ```

---

## See Also

- [AGENTS.md](../AGENTS.md) — Architecture rules and constraints
- [Architecture docs](ARCHITECTURE.md) — System architecture overview
- [Chromium build instructions](https://chromium.googlesource.com/chromium/src/+/main/docs/build.md) — Official Chromium build docs
- [Patch queue](../chromium/astra/patches/) — Astra patches and documentation
